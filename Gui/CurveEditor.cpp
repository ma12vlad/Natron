//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
*Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
*contact: immarespond at gmail dot com
*
*/

#include "CurveEditor.h"

#include <utility>

#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QSplitter>
#include <QHeaderView>
#include <QtGui/QUndoStack>
#include <QAction>

#include "Engine/Knob.h"
#include "Engine/Curve.h"
#include "Engine/Node.h"

#include "Gui/CurveWidget.h"
#include "Gui/NodeGui.h"
#include "Gui/KnobGui.h"


using std::make_pair;
using std::cout;
using std::endl;

CurveEditor::CurveEditor(QWidget *parent)
    : QWidget(parent)
    , _nodes()
    , _mainLayout(NULL)
    , _splitter(NULL)
    , _curveWidget(NULL)
    , _tree(NULL)
    , _undoStack(new QUndoStack())
{

    _undoAction = _undoStack->createUndoAction(this,tr("&Undo"));
    _undoAction->setShortcuts(QKeySequence::Undo);
    _redoAction = _undoStack->createRedoAction(this,tr("&Redo"));
    _redoAction->setShortcuts(QKeySequence::Redo);

    _mainLayout = new QHBoxLayout(this);
    setLayout(_mainLayout);
    _mainLayout->setContentsMargins(0,0,0,0);
    _mainLayout->setSpacing(0);

    _splitter = new QSplitter(Qt::Horizontal,this);

    _curveWidget = new CurveWidget(_splitter);

    _tree = new QTreeWidget(_splitter);
    _tree->setColumnCount(1);
    _tree->header()->close();

    _splitter->addWidget(_tree);
    _splitter->addWidget(_curveWidget);


    _mainLayout->addWidget(_splitter);
    
    QObject::connect(_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
                     this, SLOT(onCurrentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

}

CurveEditor::~CurveEditor(){

    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        delete (*it);
    }
    _nodes.clear();
}

std::pair<QAction*,QAction*> CurveEditor::getUndoRedoActions() const{
    return std::make_pair(_undoAction,_redoAction);
}


void CurveEditor::addNode(NodeGui* node){

    const std::vector<boost::shared_ptr<Knob> >& knobs = node->getNode()->getKnobs();
    if(knobs.empty()){
        return;
    }
    bool hasKnobsAnimating = false;
    for(U32 i = 0;i < knobs.size();++i){
        if(knobs[i]->canAnimate()){
            hasKnobsAnimating = true;
            break;
        }
    }
    if(!hasKnobsAnimating){
        return;
    }

    NodeCurveEditorContext* nodeContext = new NodeCurveEditorContext(_tree,_curveWidget,node);
    _nodes.push_back(nodeContext);

}

void CurveEditor::removeNode(NodeGui *node){
    for(std::list<NodeCurveEditorContext*>::iterator it = _nodes.begin();it!=_nodes.end();++it){
        if((*it)->getNode() == node){
            delete (*it);
            _nodes.erase(it);
            break;
        }
    }
}


NodeCurveEditorContext::NodeCurveEditorContext(QTreeWidget* tree,CurveWidget* curveWidget,NodeGui *node)
    : _node(node)
    , _nodeElements()
    , _nameItem()
{
    
    boost::shared_ptr<QTreeWidgetItem> nameItem(new QTreeWidgetItem(tree));
    nameItem->setText(0,_node->getNode()->getName().c_str());

    QObject::connect(node,SIGNAL(nameChanged(QString)),this,SLOT(onNameChanged(QString)));
    const std::map<Knob*,KnobGui*>& knobs = node->getKnobs();

    bool hasAtLeast1KnobWithACurve = false;
    bool hasAtLeast1KnobWithACurveShown = false;

    for(std::map<Knob*,KnobGui*>::const_iterator it = knobs.begin();it!=knobs.end();++it){
        
        Knob* k = it->first;
        KnobGui* kgui = it->second;
        if(!k->canAnimate()){
            continue;
        }
        
        hasAtLeast1KnobWithACurve = true;
        
        boost::shared_ptr<QTreeWidgetItem> knobItem(new QTreeWidgetItem(nameItem.get()));

        knobItem->setText(0,k->getName().c_str());
        CurveGui* knobCurve = NULL;
        bool hideKnob = true;
        if(k->getDimension() == 1){

            knobCurve = curveWidget->createCurve(k->getCurve(0),k->getDimensionName(0).c_str());
            if(!k->getCurve(0)->isAnimated()){
                knobItem->setHidden(true);
            }else{
                hasAtLeast1KnobWithACurveShown = true;
                hideKnob = false;
            }
        }else{
            for(int j = 0 ; j < k->getDimension();++j){
                
                boost::shared_ptr<QTreeWidgetItem> dimItem(new QTreeWidgetItem(knobItem.get()));
                dimItem->setText(0,k->getDimensionName(j).c_str());
                CurveGui* dimCurve = curveWidget->createCurve(k->getCurve(j),k->getDimensionName(j).c_str());
                NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,kgui,j,dimItem,dimCurve);
                QObject::connect(k,SIGNAL(restorationComplete()),elem,SLOT(checkVisibleState()));
                _nodeElements.push_back(elem);
                if(!dimCurve->getInternalCurve()->isAnimated()){
                    dimItem->setHidden(true);
                }else{
                    hasAtLeast1KnobWithACurveShown = true;
                    hideKnob = false;
                }
            }
        }
        if(hideKnob){
            knobItem->setHidden(true);
        }
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,kgui,0,knobItem,knobCurve);
        QObject::connect(k,SIGNAL(restorationComplete()),elem,SLOT(checkVisibleState()));
        _nodeElements.push_back(elem);
    }
    if(hasAtLeast1KnobWithACurve){
        NodeCurveEditorElement* elem = new NodeCurveEditorElement(tree,curveWidget,(KnobGui*)NULL,-1,
                                                                  nameItem,(CurveGui*)NULL);
        _nodeElements.push_back(elem);
        if(!hasAtLeast1KnobWithACurveShown){
            nameItem->setHidden(true);
        }
        _nameItem = nameItem;
    }


}

NodeCurveEditorContext::~NodeCurveEditorContext() {
    for(U32 i = 0 ; i < _nodeElements.size();++i){
        delete _nodeElements[i];
    }
    _nodeElements.clear();
}

void NodeCurveEditorContext::onNameChanged(const QString& name){
    _nameItem->setText(0,name);
}

void NodeCurveEditorElement::checkVisibleState(){
    if(!_curve)
        return;
    int i = _curve->getInternalCurve()->keyFramesCount();
    if(i > 1){
        if(!_curveDisplayed){
            _curveDisplayed = true;
            _curve->setVisibleAndRefresh(true);
            _treeItem->setHidden(false);
            _treeItem->parent()->setHidden(false);
            _treeItem->parent()->setExpanded(true);
            if(_treeItem->parent()->parent()){
                _treeItem->parent()->parent()->setHidden(false);
                _treeItem->parent()->parent()->setExpanded(true);
            }
        }
        _treeWidget->setCurrentItem(_treeItem.get());
    }else{
        if(_curveDisplayed){
            _curveDisplayed = false;
            _treeItem->setHidden(true);
            _treeItem->parent()->setHidden(true);
            _treeItem->parent()->setExpanded(false);
            if(_treeItem->parent()->parent()){
                _treeItem->parent()->parent()->setHidden(true);
                _treeItem->parent()->parent()->setExpanded(false);
            }
            _curve->setVisibleAndRefresh(false);
        }
    }
}


NodeCurveEditorElement::NodeCurveEditorElement(QTreeWidget *tree, CurveWidget* curveWidget,
                                               KnobGui *knob, int dimension, boost::shared_ptr<QTreeWidgetItem> item, CurveGui* curve):
    _treeItem(item)
  ,_curve(curve)
  ,_curveDisplayed(false)
  ,_curveWidget(curveWidget)
  ,_treeWidget(tree)
  ,_knob(knob)
  ,_dimension(dimension)
{

    if(curve){
        if(curve->getInternalCurve()->keyFramesCount() > 1){
            _curveDisplayed = true;
        }
    }else{
        _dimension = -1; //set dimension to be meaningless
    }
}

NodeCurveEditorElement::~NodeCurveEditorElement(){
    _curveWidget->removeCurve(_curve);
}

void CurveEditor::centerOn(const std::vector<boost::shared_ptr<Curve> >& curves){
    
    // find the curve's gui
    std::vector<CurveGui*> curvesGuis;
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        const NodeCurveEditorContext::Elements& elems = (*it)->getElements();
        for (U32 i = 0; i < elems.size(); ++i) {
            CurveGui* curve = elems[i]->getCurve();
            if (curve) {
                std::vector<boost::shared_ptr<Curve> >::const_iterator found =
                        std::find(curves.begin(), curves.end(), curve->getInternalCurve());
                if(found != curves.end()){
                    curvesGuis.push_back(curve);
                    elems[i]->getTreeItem()->setSelected(true);
                }else{
                    elems[i]->getTreeItem()->setSelected(false);
                }
            }else{
                elems[i]->getTreeItem()->setSelected(false);
            }
        }
    }
    _curveWidget->centerOn(curvesGuis);
    _curveWidget->showCurvesAndHideOthers(curvesGuis);
    
}


void CurveEditor::recursiveSelect(QTreeWidgetItem* cur,std::vector<CurveGui*> *curves){
    if(!cur){
        return;
    }
    cur->setSelected(true);
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        NodeCurveEditorElement* elem = (*it)->findElement(cur);
        if(elem){
            CurveGui* curve = elem->getCurve();
            if (curve && curve->getInternalCurve()->isAnimated()) {
                curves->push_back(curve);
            }
            break;
        }

    }
    for (int j = 0; j < cur->childCount(); ++j) {
        recursiveSelect(cur->child(j),curves);
    }
}

static void recursiveDeselect(QTreeWidgetItem* current){
    current->setSelected(false);
    for (int j = 0; j < current->childCount(); ++j) {
        recursiveDeselect(current->child(j));
    }
}

void CurveEditor::onCurrentItemChanged(QTreeWidgetItem* current,QTreeWidgetItem* previous){
    std::vector<CurveGui*> curves;
    if(previous){
        recursiveDeselect(previous);
    }
    recursiveSelect(current,&curves);
    
    _curveWidget->centerOn(curves);
    _curveWidget->showCurvesAndHideOthers(curves);
    
}

NodeCurveEditorElement* NodeCurveEditorContext::findElement(CurveGui* curve){
    for(U32 i = 0; i < _nodeElements.size();++i){
        if(_nodeElements[i]->getCurve() == curve){
            return _nodeElements[i];
        }
    }
    return NULL;
}

NodeCurveEditorElement* NodeCurveEditorContext::findElement(KnobGui* knob,int dimension){
    for(U32 i = 0; i < _nodeElements.size();++i){
        if(_nodeElements[i]->getKnob() == knob && _nodeElements[i]->getDimension() == dimension){
            return _nodeElements[i];
        }
    }
    return NULL;
}

NodeCurveEditorElement* NodeCurveEditorContext::findElement(QTreeWidgetItem* item){
    for(U32 i = 0; i < _nodeElements.size();++i){
        if(_nodeElements[i]->getTreeItem().get() == item){
            return _nodeElements[i];
        }
    }
    return NULL;
}



class AddKeyCommand : public QUndoCommand{
public:

    AddKeyCommand(CurveWidget *editor,NodeCurveEditorElement *curveEditorElement,
                  KnobGui *knob, SequenceTime time, int dimension, QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    KnobGui* _knob;
    NodeCurveEditorElement* _element;
    int _dimension;
    SequenceTime _time;
    boost::shared_ptr<KeyFrame> _key;
    CurveWidget *_editor;
};

class RemoveKeyCommand : public QUndoCommand{
public:

    RemoveKeyCommand(CurveWidget* editor,NodeCurveEditorElement* curveEditorElement
                     ,boost::shared_ptr<KeyFrame> key,QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();

private:

    NodeCurveEditorElement* _element;
    boost::shared_ptr<KeyFrame> _key;
    CurveWidget* _curveWidget;
};

class MoveKeyCommand : public QUndoCommand{

public:

    MoveKeyCommand(CurveWidget* editor,NodeCurveEditorElement* curveEditorElement
                   ,boost::shared_ptr<KeyFrame> key,double oldx,const Variant& oldy,
                   double newx,const Variant& newy,
                   QUndoCommand *parent = 0);
    virtual void undo();
    virtual void redo();
    virtual int id() const { return kCurveEditorMoveKeyCommandCompressionID; }
    virtual bool mergeWith(const QUndoCommand * command);

private:

    double _newX,_oldX;
    Variant _newY,_oldY;
    NodeCurveEditorElement* _element;
    boost::shared_ptr<KeyFrame> _key;
    CurveWidget* _curveWidget;
};

void CurveEditor::addKeyFrame(KnobGui* knob,SequenceTime time,int dimension){
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        NodeCurveEditorElement* elem = (*it)->findElement(knob,dimension);
        if(elem){
            _undoStack->push(new AddKeyCommand(_curveWidget,elem,knob,time,dimension));
            return;
        }
    }
}

AddKeyCommand::AddKeyCommand(CurveWidget *editor,  NodeCurveEditorElement *curveEditorElement, KnobGui *knob, SequenceTime time,
                             int dimension, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _knob(knob)
    , _element(curveEditorElement)
    , _dimension(dimension)
    , _time(time)
    , _key()
    , _editor(editor)
{

}

void AddKeyCommand::undo(){
    CurveGui* curve = _element->getCurve();
    assert(curve);
    assert(_key);
    _editor->removeKeyFrame(curve,_key);
    _element->checkVisibleState();
    setText(QObject::tr("Add keyframe to %1.%2")
            .arg(_knob->getKnob()->getDescription().c_str()).arg(_knob->getKnob()->getDimensionName(_dimension).c_str()));
}
void AddKeyCommand::redo(){
    CurveGui* curve = _element->getCurve();

    if(!_key){
        assert(curve);
        _key = _editor->addKeyFrame(curve,_knob->getKnob()->getValue(_dimension),_time);
    }else{
        _editor->addKeyFrame(curve,_key);
    }
    _element->checkVisibleState();
    setText(QObject::tr("Add keyframe to %1.%2")
            .arg(_knob->getKnob()->getDescription().c_str()).arg(_knob->getKnob()->getDimensionName(_dimension).c_str()));



}

void CurveEditor::removeKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key){
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        NodeCurveEditorElement* elem = (*it)->findElement(curve);
        if(elem){
            _undoStack->push(new RemoveKeyCommand(_curveWidget,elem,key));
            return;
        }
    }
}

RemoveKeyCommand::RemoveKeyCommand(CurveWidget *editor, NodeCurveEditorElement *curveEditorElement, boost::shared_ptr<KeyFrame> key, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _element(curveEditorElement)
    , _key(key)
    , _curveWidget(editor)
{

}

void RemoveKeyCommand::undo(){
    assert(_key);
    _curveWidget->addKeyFrame(_element->getCurve(),_key);
    _element->checkVisibleState();
    setText(QObject::tr("Remove keyframe from %1.%2")
            .arg(_element->getKnob()->getKnob()->getDescription().c_str())
            .arg(_element->getKnob()->getKnob()->getDimensionName(_element->getDimension()).c_str()));
}
void RemoveKeyCommand::redo(){
    _curveWidget->removeKeyFrame(_element->getCurve(),_key);
    _element->checkVisibleState();
    setText(QObject::tr("Remove keyframe from %1.%2")
            .arg(_element->getKnob()->getKnob()->getDescription().c_str())
            .arg(_element->getKnob()->getKnob()->getDimensionName(_element->getDimension()).c_str()));



}

void CurveEditor::setKeyFrame(CurveGui* curve,boost::shared_ptr<KeyFrame> key,double x,const Variant& y){
    for(std::list<NodeCurveEditorContext*>::const_iterator it = _nodes.begin();
        it!=_nodes.end();++it){
        NodeCurveEditorElement* elem = (*it)->findElement(curve);
        if(elem){
            _undoStack->push(new MoveKeyCommand(_curveWidget,elem,key,key->getTime(),key->getValue(), x,y));
            return;
        }
    }
}

MoveKeyCommand::MoveKeyCommand(CurveWidget* editor, NodeCurveEditorElement* curveEditorElement
                               , boost::shared_ptr<KeyFrame> key, double oldx, const Variant &oldy, double newx, const Variant &newy, QUndoCommand *parent)
    : QUndoCommand(parent)
    , _newX(newx)
    , _oldX(oldx)
    , _newY(newy)
    , _oldY(oldy)
    , _element(curveEditorElement)
    , _key(key)
    , _curveWidget(editor)
{

}
void MoveKeyCommand::undo(){
    assert(_key);
    _curveWidget->setKeyPos(_key,_oldX,_oldY);
    setText(QObject::tr("Move keyframe from %1.%2"));
}
void MoveKeyCommand::redo(){
    assert(_key);
    _curveWidget->setKeyPos(_key,_newX,_newY);
    setText(QObject::tr("Remove keyframe from %1.%2"));
}
bool MoveKeyCommand::mergeWith(const QUndoCommand * command){
    const MoveKeyCommand* cmd = dynamic_cast<const MoveKeyCommand*>(command);
    if(cmd && cmd->id() == id()){
        _newX = cmd->_newX;
        _newY = cmd->_newY;
         return true;
    }else{
        return false;
    }
}
