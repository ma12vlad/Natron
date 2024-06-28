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

#include "EditScriptDialog.h"

#include <cassert>
#include <cfloat>
#include <stdexcept>

#include <QtCore/QString>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFormLayout>
#include <QFileDialog>
#include <QTextEdit>
#include <QStyle> // in QtGui on Qt4, in QtWidgets on Qt5
#include <QtCore/QTimer>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QColorDialog>
#include <QGroupBox>
#include <QtGui/QVector4D>
#include <QStyleFactory>
#include <QCompleter>

#include "Global/GlobalDefines.h"

#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobSerialization.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/Variant.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Button.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveEditor.h"
#include "Gui/CurveGui.h"
#include "Gui/CustomParamInteract.h"
#include "Gui/DialogButtonBox.h"
#include "Gui/DockablePanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/KnobGuiGroup.h"
#include "Gui/Label.h"
#include "Gui/LineEdit.h"
#include "Gui/Menu.h"
#include "Gui/Menu.h"
#include "Gui/NodeCreationDialog.h"
#include "Gui/NodeGui.h"
#include "Gui/NodeSettingsPanel.h"
#include "Gui/ScriptTextEdit.h"
#include "Gui/SequenceFileDialog.h"
#include "Gui/SpinBox.h"
#include "Gui/TabWidget.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerTab.h"

NATRON_NAMESPACE_ENTER


struct EditScriptDialogPrivate
{
    Gui* gui;
    QVBoxLayout* mainLayout;
    Label* expressionLabel;
    InputScriptTextEdit* expressionEdit;
    QWidget* midButtonsContainer;
    QHBoxLayout* midButtonsLayout;
    Button* useRetButton;
    Button* helpButton;
    Label* resultLabel;
    OutputScriptTextEdit* resultEdit;
    DialogButtonBox* buttons;

    EditScriptDialogPrivate(Gui* gui)
        : gui(gui)
        , mainLayout(0)
        , expressionLabel(0)
        , expressionEdit(0)
        , midButtonsContainer(0)
        , midButtonsLayout(0)
        , useRetButton(0)
        , helpButton(0)
        , resultLabel(0)
        , resultEdit(0)
        , buttons(0)
    {
    }
};

EditScriptDialog::EditScriptDialog(Gui* gui,
                                   QWidget* parent)
    : QDialog(parent)
    , _imp( new EditScriptDialogPrivate(gui) )
{
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

void
EditScriptDialog::create(const QString& initialScript,
                         bool makeUseRetButton)
{
    setTitle();

    _imp->mainLayout = new QVBoxLayout(this);

    QStringList modules;
    getImportedModules(modules);
    std::list<std::pair<QString, QString> > variables;
    getDeclaredVariables(variables);
    QString labelHtml( tr("%1 скрипт:").arg( QString::fromUtf8("<b>Python</b>") ) + QString::fromUtf8("<br />") );
    if ( !modules.empty() ) {
        labelHtml.append( tr("Для удобства были импортированы следующие модули:") + QString::fromUtf8("<br />") );
        for (int i = 0; i < modules.size(); ++i) {
            QString toAppend = QString::fromUtf8("<i><font color=orange>from %1 import *</font></i><br />").arg(modules[i]);
            labelHtml.append(toAppend);
        }
        labelHtml.append( QString::fromUtf8("<br />") );
    }
    if ( !variables.empty() ) {
        labelHtml.append( tr("Также были объявлены следующие переменные:") + QString::fromUtf8("<br />") );
        for (std::list<std::pair<QString, QString> > ::iterator it = variables.begin(); it != variables.end(); ++it) {
            QString toAppend = QString::fromUtf8("<b>%1</b>: %2<br />").arg(it->first).arg(it->second);
            labelHtml.append(toAppend);
        }
        QKeySequence s(Qt::CTRL);
        labelHtml.append( QString::fromUtf8("<p>") + tr("Обратите внимание, что на параметры можно ссылаться, перетаскивая их, удерживая %1 на их виджете").arg( s.toString(QKeySequence::NativeText) ) + QString::fromUtf8("</p>") );
    }

    _imp->expressionLabel = new Label(labelHtml, this);
    _imp->mainLayout->addWidget(_imp->expressionLabel);

    _imp->expressionEdit = new InputScriptTextEdit(_imp->gui, this);
    _imp->expressionEdit->setAcceptDrops(true);
    _imp->expressionEdit->setMouseTracking(true);
    _imp->mainLayout->addWidget(_imp->expressionEdit);
    _imp->expressionEdit->setPlainText(initialScript);

    _imp->midButtonsContainer = new QWidget(this);
    _imp->midButtonsLayout = new QHBoxLayout(_imp->midButtonsContainer);


    if (makeUseRetButton) {
        bool retVariable = hasRetVariable();
        _imp->useRetButton = new Button(tr("Многолинейный"), _imp->midButtonsContainer);
        _imp->useRetButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Если этот флажок установлен, выражение Python будет интерпретироваться "
                                                                          "как серия заявлений. Возвращаемое значение затем должно быть присвоено "
                                                                          "\"ret\" переменная. Если флажок снят, выражение не должно содержать "
                                                                          "любой символ новой строки, и результат будет интерпретирован на основе "
                                                                          "интерпретации одной строки."), NATRON_NAMESPACE::WhiteSpaceNormal) );
        _imp->useRetButton->setCheckable(true);
        bool checked = !initialScript.isEmpty() && retVariable;
        _imp->useRetButton->setChecked(checked);
        _imp->useRetButton->setDown(checked);
        QObject::connect( _imp->useRetButton, SIGNAL(clicked(bool)), this, SLOT(onUseRetButtonClicked(bool)) );
        _imp->midButtonsLayout->addWidget(_imp->useRetButton);
    }


    _imp->helpButton = new Button(tr("Помощь"), _imp->midButtonsContainer);
    QObject::connect( _imp->helpButton, SIGNAL(clicked(bool)), this, SLOT(onHelpRequested()) );
    _imp->midButtonsLayout->addWidget(_imp->helpButton);
    _imp->midButtonsLayout->addStretch();

    _imp->mainLayout->addWidget(_imp->midButtonsContainer);

    _imp->resultLabel = new Label(tr("Результат:"), this);
    _imp->mainLayout->addWidget(_imp->resultLabel);

    _imp->resultEdit = new OutputScriptTextEdit(this);
    _imp->resultEdit->setFixedHeight( TO_DPIY(80) );
    _imp->resultEdit->setFocusPolicy(Qt::NoFocus);
    _imp->resultEdit->setReadOnly(true);
    _imp->mainLayout->addWidget(_imp->resultEdit);

    _imp->buttons = new DialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    _imp->mainLayout->addWidget(_imp->buttons);
    QObject::connect( _imp->buttons, SIGNAL(accepted()), this, SLOT(accept()) );
    QObject::connect( _imp->buttons, SIGNAL(rejected()), this, SLOT(reject()) );

    if ( !initialScript.isEmpty() ) {
        compileAndSetResult(initialScript);
    }
    QObject::connect( _imp->expressionEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()) );
    _imp->expressionEdit->setFocus();

    QString fontFamily = QString::fromUtf8( appPTR->getCurrentSettings()->getSEFontFamily().c_str() );
    int fontSize = appPTR->getCurrentSettings()->getSEFontSize();
    QFont font(fontFamily, fontSize);
    if ( font.exactMatch() ) {
        _imp->expressionEdit->setFont(font);
        _imp->resultEdit->setFont(font);
    }
    QFontMetrics fm = _imp->expressionEdit->fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    _imp->expressionEdit->setTabStopDistance( 4 * fm.horizontalAdvance( QLatin1Char(' ') ) );
#else
    _imp->expressionEdit->setTabStopWidth( 4 * fm.width( QLatin1Char(' ') ) );
#endif
} // EditScriptDialog::create

void
EditScriptDialog::compileAndSetResult(const QString& script)
{
    QString ret = compileExpression(script);

    _imp->resultEdit->setPlainText(ret);
}

QString
EditScriptDialog::getHelpPart1()
{
    return tr("<p>У каждого узла в области видимости уже есть переменная, объявленная с ее именем, например, если у вас есть узел с именем "
              "<b>Transform1</b> в вашем проекте, вы можете ввести <i>Transform1</i> для ссылки на этот узел.</p>"
              "<p>Обратите внимание, что область действия включает все узлы в той же группе, что и thisNode, и сам узел родительской группы. "
              "если узел принадлежит группе. Если узел сам по себе является группой, то он также может иметь выражения в зависимости "
              "от параметров его дочерних элементов.</p>"
              "<p>Каждый узел имеет все свои параметры, объявленные как поля, и вы можете сослаться на конкретный параметр, введя его <b>имя скрипта</b>, например:</p>"
              "<pre>Transform1.rotate</pre>"
              "<p>TСценарное имя параметра — это имя, выделенное жирным шрифтом, которое отображается во всплывающей подсказке при наведении мыши на параметр."
              "Это то, что идентифицирует параметр внутри.</p>");
}

QString
EditScriptDialog::getHelpThisNodeVariable()
{
    return tr("<p>Переменная может ссылаться на текущий Узел, выражение которого редактируется <i>этот Узел</i> для удобства.</p>");
}

QString
EditScriptDialog::getHelpThisGroupVariable()
{
    return tr("<p>Переменная может ссылаться на родительскую группу, содержащую этот Узел <i>thisGroup</i> для удобства,"
              "только если этот узел принадлежит к группе.</p>");
}

QString
EditScriptDialog::getHelpThisParamVariable()
{
    return tr("<p>Переменная <i>thisParam</i> определена для удобства редактирования выражения. Он относится к текущему параметру.</p>");
}

QString
EditScriptDialog::getHelpDimensionVariable()
{
    return tr("<p>Таким же образом была определена переменная <i>dimension</i>, которая ссылается на текущее измерение параметра, выражение которого задается."
              ".</p>"
              "<p> <i>dimension</i> это индекс, начинающийся с 0, идентифицирующий конкретное поле параметра. Например, если мы редактируем выражение Y "
              "поле параметра translate параметра Transform1, в котором <i>dimension</i> было бы 1. </p>");
}

QString
EditScriptDialog::getHelpPart2()
{
    return tr("<p>Для доступа к значениям параметра доступны несколько функций: </p>"
              "<p>The <b>get()</b> функция вернет цепочку, содержащую все значения для каждого измерения параметра. Например "
              "скажем, у нас есть узел Transform1 в нашей композиции, тогда мы могли бы ссылаться на значение x параметра <i>center</i> следующим образом:</p>"
              "<pre>Transform1.center.get().x</pre>"
              "<p><b>get(</b><i>frame</i><b>)</b> работает точно так же, как функция <b>get()</b>, за исключением того, что она требует дополнительных "
              "<i>frame</i> параметр, соответствующий времени, в которое мы хотим получить значение. Для параметров с анимацией "
              "затем он вернет их значение в соответствующей позиции на временной шкале. Затем это значение будет либо интерполировано "
              "с текущим фильтром интерполяции или точным ключевым кадром в тот момент, если он существует.</p>");
}

void
EditScriptDialog::onHelpRequested()
{
    QString help = getCustomHelp();
    Dialogs::informationDialog(tr("Помощь").toStdString(), help.toStdString(), true);
}

QString
EditScriptDialog::getExpression(bool* hasRetVariable) const
{
    if (hasRetVariable) {
        *hasRetVariable = _imp->useRetButton ? _imp->useRetButton->isChecked() : false;
    }

    return _imp->expressionEdit->toPlainText();
}

bool
EditScriptDialog::isUseRetButtonChecked() const
{
    return _imp->useRetButton->isChecked();
}

void
EditScriptDialog::onTextEditChanged()
{
    compileAndSetResult( _imp->expressionEdit->toPlainText() );
}

void
EditScriptDialog::onUseRetButtonClicked(bool useRet)
{
    compileAndSetResult( _imp->expressionEdit->toPlainText() );
    _imp->useRetButton->setDown(useRet);
}

EditScriptDialog::~EditScriptDialog()
{
}

void
EditScriptDialog::keyPressEvent(QKeyEvent* e)
{
    if ( (e->key() == Qt::Key_Return) || (e->key() == Qt::Key_Enter) ) {
        accept();
    } else if (e->key() == Qt::Key_Escape) {
        reject();
    } else {
        QDialog::keyPressEvent(e);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_EditScriptDialog.cpp"
