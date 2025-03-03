/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "NodeGraph.h"
#include "NodeGraphPrivate.h"

#include <limits>
#include <sstream> // stringstream
#include <stdexcept>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
// clang-format off
GCC_DIAG_OFF(unused-parameter)
// /opt/local/include/boost/serialization/smart_cast.hpp:254:25: warning: unused parameter 'u' [-Wunused-parameter]
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
// /usr/local/include/boost/serialization/shared_ptr.hpp:112:5: warning: unused typedef 'boost_static_assert_typedef_112' [-Wunused-local-typedef]
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
GCC_DIAG_ON(unused-parameter)
// clang-format on
#endif

#include <QApplication>
#include <QClipboard>
#include <QtCore/QMimeData>

#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RotoLayer.h"
#include "Engine/Project.h"
#include "Engine/ViewerInstance.h"

#include "Gui/BackdropGui.h"
#include "Gui/CurveEditor.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/NodeClipBoard.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeGuiSerialization.h"

#include "Global/QtCompat.h"

NATRON_NAMESPACE_ENTER
void
NodeGraph::togglePreviewsForSelectedNodes()
{
    bool empty = false;
    {
        QMutexLocker l(&_imp->_nodesMutex);
        empty = _imp->_selection.empty();
        for (NodesGuiList::iterator it = _imp->_selection.begin();
             it != _imp->_selection.end();
             ++it) {
            (*it)->togglePreview();
        }
    }

    if (empty) {
        Dialogs::warningDialog( tr("Переключить предпросмотр").toStdString(), tr("Сначала выберите узел").toStdString() );
    }
}

void
NodeGraph::showSelectedNodeSettingsPanel()
{
    if (_imp->_selection.size() == 1) {
        showNodePanel(false, false, _imp->_selection.front().get());
    }
}

void
NodeGraph::switchInputs1and2ForSelectedNodes()
{
    QMutexLocker l(&_imp->_nodesMutex);

    for (NodesGuiList::iterator it = _imp->_selection.begin();
         it != _imp->_selection.end();
         ++it) {
        (*it)->onSwitchInputActionTriggered();
    }
}

void
NodeGraph::centerOnItem(QGraphicsItem* item)
{
    _imp->_refreshOverlays = true;
    centerOn(item);
}

void
NodeGraph::copyNodes(const NodesGuiList& nodes,
                     NodeClipBoard& clipboard)
{
    _imp->copyNodesInternal(nodes, clipboard);
}

void
NodeGraph::copySelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Копировать").toStdString(), tr("Сначала выбрите хотя бы один узел для копирования.").toStdString() );

        return;
    }

    NodeClipBoard& cb = appPTR->getNodeClipBoard();
    _imp->copyNodesInternal(_imp->_selection, cb);

    std::ostringstream ss;
    try {
        boost::archive::xml_oarchive oArchive(ss);
        oArchive << boost::serialization::make_nvp("Clipboard", cb);
    } catch (...) {
        qDebug() << "Failed to copy selection to system clipboard";
    }

    QMimeData* mimedata = new QMimeData;
    QByteArray data( ss.str().c_str() );
    mimedata->setData(QLatin1String("text/plain"), data);
    QClipboard* clipboard = QApplication::clipboard();

    //ownership is transferred to the clipboard
    clipboard->setMimeData(mimedata);
}

void
NodeGraph::cutSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Обрезать").toStdString(), tr("Сначала выбрите хотя бы один узел, который нужно обрезать.").toStdString() );

        return;
    }
    copySelectedNodes();
    deleteSelection();
}

void
NodeGraph::pasteCliboard(const NodeClipBoard& clipboard,
                         std::list<std::pair<std::string, NodeGuiPtr> >* newNodes)
{
    QPointF position = _imp->_root->mapFromScene( mapToScene( mapFromGlobal( QCursor::pos() ) ) );

    _imp->pasteNodesInternal(clipboard, position, false, newNodes);
}

bool
NodeGraph::pasteNodeClipBoards(const QPointF& pos)
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimedata = clipboard->mimeData();

    if ( !mimedata->hasFormat( QLatin1String("text/plain") ) ) {
        return false;
    }
    QByteArray data = mimedata->data( QLatin1String("text/plain") );
    std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
    NodeClipBoard& cb = appPTR->getNodeClipBoard();
    std::string s = QString::fromUtf8(data).toStdString();
    try {
        std::stringstream ss(s);
        boost::archive::xml_iarchive iArchive(ss);
        iArchive >> boost::serialization::make_nvp("Clipboard", cb);
    } catch (...) {
        return false;
    }

    _imp->pasteNodesInternal(cb, _imp->_root->mapFromScene(pos), true, &newNodes);

    return true;
}

bool
NodeGraph::pasteNodeClipBoards()
{
    QPointF position = mapToScene( mapFromGlobal( QCursor::pos() ) );

    return pasteNodeClipBoards(position);
}

void
NodeGraph::duplicateSelectedNodes(const QPointF& pos)
{
    if ( _imp->_selection.empty() && _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Дублировать").toStdString(), tr("Сначала выбрите хотя бы узел для дублирования.").toStdString() );

        return;
    }

    ///Don't use the member clipboard as the user might have something copied
    NodeClipBoard tmpClipboard;
    _imp->copyNodesInternal(_imp->_selection, tmpClipboard);
    std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
    _imp->pasteNodesInternal(tmpClipboard, _imp->_root->mapFromScene(pos), true, &newNodes);
}

void
NodeGraph::duplicateSelectedNodes()
{
    QPointF scenePos = mapToScene( mapFromGlobal( QCursor::pos() ) );

    duplicateSelectedNodes(scenePos);
}

void
NodeGraph::cloneSelectedNodes(const QPointF& scenePos)
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Клонировать").toStdString(), tr("Сначала выбрите хотя бы узел для клонирования.").toStdString() );

        return;
    }

    double xmax = std::numeric_limits<int>::min();
    double xmin = std::numeric_limits<int>::max();
    double ymin = std::numeric_limits<int>::max();
    double ymax = std::numeric_limits<int>::min();
    NodesGuiList nodesToCopy = _imp->_selection;
    for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        if ( (*it)->getNode()->getMasterNode() ) {
            Dialogs::errorDialog( tr("Клонировать").toStdString(), tr("Нельзя клонировать узел, который уже является клоном.").toStdString() );

            return;
        }
        QRectF bbox = (*it)->mapToScene( (*it)->boundingRect() ).boundingRect();
        if ( ( bbox.x() + bbox.width() ) > xmax ) {
            xmax = ( bbox.x() + bbox.width() );
        }
        if (bbox.x() < xmin) {
            xmin = bbox.x();
        }

        if ( ( bbox.y() + bbox.height() ) > ymax ) {
            ymax = ( bbox.y() + bbox.height() );
        }
        if (bbox.y() < ymin) {
            ymin = bbox.y();
        }

        ///Also copy all nodes within the backdrop
        BackdropGui* isBd = dynamic_cast<BackdropGui*>( it->get() );
        if (isBd) {
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(*it);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodesGuiList::iterator found = std::find(nodesToCopy.begin(), nodesToCopy.end(), *it2);
                if ( found == nodesToCopy.end() ) {
                    nodesToCopy.push_back(*it2);
                }
            }
        }
    }

    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        if ( (*it)->getNode()->getEffectInstance()->isSlave() ) {
            Dialogs::errorDialog( tr("Клонировать").toStdString(), tr("Нельзя клонировать узел, который уже является клоном.").toStdString() );

            return;
        }
        ViewerInstance* isViewer = (*it)->getNode()->isEffectViewer();
        if (isViewer) {
            Dialogs::errorDialog( tr("Клонировать").toStdString(), tr("Клонирование средства просмотра невозможно.").toStdString() );

            return;
        }
        if ( (*it)->getNode()->isMultiInstance() ) {
            QString err = QString::fromUtf8("%1 невозможно клонировать.").arg( QString::fromUtf8( (*it)->getNode()->getLabel().c_str() ) );
            Dialogs::errorDialog( tr("Клонировать").toStdString(),
                                  tr( err.toStdString().c_str() ).toStdString() );

            return;
        }
    }

    QPointF offset( scenePos.x() - ( (xmax + xmin) / 2. ), scenePos.y() -  ( (ymax + ymin) / 2. ) );
    std::list<std::pair<std::string, NodeGuiPtr> > newNodes;
    std::list<NodeSerializationPtr> serializations;
    std::list<NodeGuiPtr> newNodesList;
    std::map<std::string, std::string> oldNewScriptNamesMapping;
    for (NodesGuiList::iterator it = nodesToCopy.begin(); it != nodesToCopy.end(); ++it) {
        NodeSerializationPtr  internalSerialization( new NodeSerialization( (*it)->getNode() ) );
        NodeGuiSerializationPtr guiSerialization = std::make_shared<NodeGuiSerialization>();
        (*it)->serialize( guiSerialization.get() );
        NodeGuiPtr clone = _imp->pasteNode(internalSerialization, guiSerialization, offset,
                                           _imp->group.lock(), std::string(), true, &oldNewScriptNamesMapping);

        newNodes.push_back( std::make_pair(internalSerialization->getNodeScriptName(), clone) );
        newNodesList.push_back(clone);
        serializations.push_back(internalSerialization);

        oldNewScriptNamesMapping[internalSerialization->getNodeScriptName()] = clone->getNode()->getScriptName();
    }


    assert( serializations.size() == newNodes.size() );
    ///restore connections
    _imp->restoreConnections(serializations, newNodes, oldNewScriptNamesMapping);


    NodesList allNodes;
    getGui()->getApp()->getProject()->getActiveNodesExpandGroups(&allNodes);

    //Restore links once all children are created for alias knobs/expressions
    std::list<NodeSerializationPtr>::iterator itS = serializations.begin();
    for (std::list<NodeGuiPtr> ::iterator it = newNodesList.begin(); it != newNodesList.end(); ++it, ++itS) {
        (*it)->getNode()->storeKnobsLinks(**itS, oldNewScriptNamesMapping);
        (*it)->getNode()->restoreKnobsLinks(allNodes, oldNewScriptNamesMapping, true); // clone should never fail
    }


    pushUndoCommand( new AddMultipleNodesCommand(this, newNodesList) );
} // NodeGraph::cloneSelectedNodes

void
NodeGraph::cloneSelectedNodes()
{
    QPointF scenePos = mapToScene( mapFromGlobal( QCursor::pos() ) );

    cloneSelectedNodes(scenePos);
} // cloneSelectedNodes

void
NodeGraph::decloneSelectedNodes()
{
    if ( _imp->_selection.empty() ) {
        Dialogs::warningDialog( tr("Деклонировать").toStdString(), tr("Сначала нужно выбрать хотя бы узел деклонирования.").toStdString() );

        return;
    }
    NodesGuiList nodesToDeclone;


    for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
        BackdropGui* isBd = dynamic_cast<BackdropGui*>( it->get() );
        if (isBd) {
            ///Also copy all nodes within the backdrop
            NodesGuiList nodesWithinBD = getNodesWithinBackdrop(*it);
            for (NodesGuiList::iterator it2 = nodesWithinBD.begin(); it2 != nodesWithinBD.end(); ++it2) {
                NodesGuiList::iterator found = std::find(nodesToDeclone.begin(), nodesToDeclone.end(), *it2);
                if ( found == nodesToDeclone.end() ) {
                    nodesToDeclone.push_back(*it2);
                }
            }
        }
        if ( (*it)->getNode()->getEffectInstance()->isSlave() ) {
            nodesToDeclone.push_back(*it);
        }
    }

    pushUndoCommand( new DecloneMultipleNodesCommand(this, nodesToDeclone) );
}

void
NodeGraph::setUndoRedoStackLimit(int limit)
{
    _imp->_undoStack->clear();
    _imp->_undoStack->setUndoLimit(limit);
}

void
NodeGraph::deleteNodePermanantly(const NodeGuiPtr& n)
{
    assert(n);
    NodesGuiList::iterator it = std::find(_imp->_nodesTrash.begin(), _imp->_nodesTrash.end(), n);

    if ( it != _imp->_nodesTrash.end() ) {
        _imp->_nodesTrash.erase(it);
    }

    {
        QMutexLocker l(&_imp->_nodesMutex);
        NodesGuiList::iterator it = std::find(_imp->_nodes.begin(), _imp->_nodes.end(), n);
        if ( it != _imp->_nodes.end() ) {
            _imp->_nodes.erase(it);
        }
    }

    NodesGuiList::iterator found = std::find(_imp->_selection.begin(), _imp->_selection.end(), n);
    if ( found != _imp->_selection.end() ) {
        n->setUserSelected(false);
        _imp->_selection.erase(found);
    }
} // deleteNodePermanantly

void
NodeGraph::invalidateAllNodesParenting()
{
    for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
    for (NodesGuiList::iterator it = _imp->_nodesTrash.begin(); it != _imp->_nodesTrash.end(); ++it) {
        (*it)->setParentItem(NULL);
        if ( (*it)->scene() ) {
            (*it)->scene()->removeItem( it->get() );
        }
    }
}

void
NodeGraph::centerOnAllNodes()
{
    assert( QThread::currentThread() == qApp->thread() );
    double xmin = std::numeric_limits<int>::max();
    double xmax = std::numeric_limits<int>::min();
    double ymin = std::numeric_limits<int>::max();
    double ymax = std::numeric_limits<int>::min();
    //_imp->_root->setPos(0,0);

    if ( _imp->_selection.empty() ) {
        QMutexLocker l(&_imp->_nodesMutex);

        for (NodesGuiList::iterator it = _imp->_nodes.begin(); it != _imp->_nodes.end(); ++it) {
            if ( /*(*it)->isActive() &&*/ (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene( (*it)->mapFromParent( (*it)->getPos_mt_safe() ) );
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax, pos.x() + size.width() );
                ymin = std::min( ymin, pos.y() );
                ymax = std::max( ymax, pos.y() + size.height() );
            }
        }
    } else {
        for (NodesGuiList::iterator it = _imp->_selection.begin(); it != _imp->_selection.end(); ++it) {
            if ( /*(*it)->isActive() && */ (*it)->isVisible() ) {
                QSize size = (*it)->getSize();
                QPointF pos = (*it)->mapToScene( (*it)->mapFromParent( (*it)->getPos_mt_safe() ) );
                xmin = std::min( xmin, pos.x() );
                xmax = std::max( xmax, pos.x() + size.width() );
                ymin = std::min( ymin, pos.y() );
                ymax = std::max( ymax, pos.y() + size.height() );
            }
        }
    }
    // Move the scene so that topleft of the viewing area is at 0,0, and avoid issues
    // when topleft has negative coords.
    moveRootInternal(-xmin, -ymin);
    QRectF bbox( 0, 0, (xmax - xmin), (ymax - ymin) );
    setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    fitInView(bbox, Qt::KeepAspectRatio);

    double currentZoomFactor = transform().mapRect( QRectF(0, 0, 1, 1) ).width();
    assert(currentZoomFactor != 0);
    //we want to fit at scale 1 at most
    if (currentZoomFactor > 1.) {
        double scaleFactor = 1. / currentZoomFactor;
        setTransformationAnchor(QGraphicsView::AnchorViewCenter);
        scale(scaleFactor, scaleFactor);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    }

    _imp->_refreshOverlays = true;
    update();
} // NodeGraph::centerOnAllNodes

NATRON_NAMESPACE_EXIT
