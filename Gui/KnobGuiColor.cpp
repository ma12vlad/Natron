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

#include "KnobGuiColor.h"

#include <cfloat>
#include <algorithm> // min, max
#include <stdexcept>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QGridLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QToolTip>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QApplication>
#include <QScrollArea>
#include <QWidgetAction>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON
#include <QtCore/QDebug>
#include <QFontComboBox>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/Image.h"
#include "Engine/KnobTypes.h"
#include "Engine/Lut.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Utils.h" // convertFromPlainText
#include "Engine/ViewIdx.h"

#include "Gui/Button.h"
#include "Gui/ClickableLabel.h"
#include "Gui/ComboBox.h"
#include "Gui/CurveGui.h"
#include "Gui/DockablePanel.h"
#include "Gui/GroupBoxLabel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h"
#include "Gui/GuiDefines.h"
#include "Gui/GuiMacros.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/Label.h"
#include "Gui/NewLayerDialog.h"
#include "Gui/ProjectGui.h"
#include "Gui/ScaleSliderQWidget.h"
#include "Gui/SpinBox.h"
#include "Gui/SpinBoxValidator.h"
#include "Gui/TabGroup.h"

#include <ofxNatron.h>


NATRON_NAMESPACE_ENTER


ColorPickerLabel::ColorPickerLabel(KnobGuiColor* knob,
                                   QWidget* parent)
    : Label(parent)
    , _pickingEnabled(false)
    , _knob(knob)
{
    setMouseTracking(true);
}

void
ColorPickerLabel::mousePressEvent(QMouseEvent*)
{
    if (!_knob) {
        return;
    }
    _pickingEnabled = !_pickingEnabled;
    Q_EMIT pickingEnabled(_pickingEnabled);
    setColor(_currentColor); //< refresh the icon
}

void
ColorPickerLabel::enterEvent(QEvent*)
{
    QToolTip::showText( QCursor::pos(), toolTip() );
}

void
ColorPickerLabel::leaveEvent(QEvent*)
{
    QToolTip::hideText();
}

void
ColorPickerLabel::setPickingEnabled(bool enabled)
{
    if ( !isEnabled() ) {
        return;
    }
    _pickingEnabled = enabled;
    setColor(_currentColor);
}

void
ColorPickerLabel::setEnabledMode(bool enabled)
{
    setEnabled(enabled);
    if (!enabled && _pickingEnabled) {
        _pickingEnabled = false;
        setColor(_currentColor);
        if (_knob) {
            _knob->getGui()->removeColorPicker( std::dynamic_pointer_cast<KnobColor>( _knob->getKnob() ) );
        }
    }
}

void
ColorPickerLabel::setColor(const QColor & color)
{
    _currentColor = color;

    if (_pickingEnabled && _knob) {
        //draw the picker on top of the label
        QPixmap pickerIcon;
        appPTR->getIcon(NATRON_PIXMAP_COLOR_PICKER, &pickerIcon);
        QImage pickerImg = pickerIcon.toImage();
        QImage img(pickerIcon.width(), pickerIcon.height(), QImage::Format_ARGB32);
        img.fill( color.rgb() );


        int h = pickerIcon.height();
        int w = pickerIcon.width();
        for (int i = 0; i < h; ++i) {
            const QRgb* data = (QRgb*)pickerImg.scanLine(i);
            if ( (i == 0) || (i == h - 1) ) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel( j, i, qRgb(0, 0, 0) );
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ( (j == 0) || (j == w - 1) ) {
                        img.setPixel( j, i, qRgb(0, 0, 0) );
                    } else {
                        int alpha = qAlpha(data[j]);
                        if (alpha > 0) {
                            img.setPixel(j, i, data[j]);
                        }
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img); //.scaled(20, 20);
        setPixmap(pix);
    } else {
        QImage img(NATRON_MEDIUM_BUTTON_SIZE, NATRON_MEDIUM_BUTTON_SIZE, QImage::Format_ARGB32);
        int h = img.height();
        int w = img.width();
        QRgb c = color.rgb();
        for (int i = 0; i < h; ++i) {
            if ( (i == 0) || (i == h - 1) ) {
                for (int j = 0; j < w; ++j) {
                    img.setPixel( j, i, qRgb(0, 0, 0) );
                }
            } else {
                for (int j = 0; j < w; ++j) {
                    if ( (j == 0) || (j == w - 1) ) {
                        img.setPixel( j, i, qRgb(0, 0, 0) );
                    } else {
                        img.setPixel(j, i, c);
                    }
                }
            }
        }
        QPixmap pix = QPixmap::fromImage(img);
        setPixmap(pix);
    }
} // ColorPickerLabel::setColor

KnobGuiColor::KnobGuiColor(KnobIPtr knob,
                           KnobGuiContainerI *container)
    : KnobGuiValue(knob, container)
    , _knob( std::dynamic_pointer_cast<KnobColor>(knob) )
    , _colorLabel(0)
    , _colorSelector(0)
    , _colorSelectorButton(0)
    , _lastColor()
    , _useSimplifiedUI(true)
{
    KnobColorPtr k = _knob.lock();
    assert(k);
    if (!k) {
        throw std::logic_error(__func__);
    }
    int nDims = k->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    _lastColor.resize(nDims);
    _useSimplifiedUI = isViewerUIKnob() || ( k && k->isSimplified() );
}

void
KnobGuiColor::connectKnobSignalSlots()
{
    KnobColorPtr knob = _knob.lock();
    QObject::connect( this, SIGNAL(dimensionSwitchToggled(bool)), knob.get(), SLOT(onDimensionSwitchToggled(bool)) );
    QObject::connect( knob.get(), SIGNAL(mustActivateAllDimensions()), this, SLOT(onMustShowAllDimension()) );
    QObject::connect( knob.get(), SIGNAL(pickingEnabled(bool)), this, SLOT(setPickingEnabled(bool)) );
}

void
KnobGuiColor::getIncrements(std::vector<double>* increments) const
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    increments->resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        (*increments)[i] = 0.001;
    }
}

void
KnobGuiColor::getDecimals(std::vector<int>* decimals) const
{
    KnobColorPtr knob = _knob.lock();
    if (!knob) {
        return;
    }
    int nDims = knob->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    decimals->resize(nDims);
    for (int i = 0; i < nDims; ++i) {
        (*decimals)[i] = 6;
    }
}

void
KnobGuiColor::addExtraWidgets(QHBoxLayout* containerLayout)
{
    containerLayout->addSpacing( TO_DPIX(10) );
    KnobColorPtr knob = _knob.lock();
    _colorLabel = new ColorPickerLabel( _useSimplifiedUI ? NULL : this, containerLayout->widget() );
    if (!_useSimplifiedUI) {
        _colorLabel->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Чтобы выбрать цвет для средства просмотра, щелкните его, а затем нажмите Control + щелкните ЛКМ по любому средству просмотра.\n"
                                                                   "Вы также можете выбрать средний цвет данного прямоугольника, удерживая Control + Shift + щелчок ЛКМ\n. "
                                                                   "Чтобы отменить выбор средства выбора, щелкните левой кнопкой мыши в любом месте"
                                                                   "По умолчанию %1 преобразует выбранный цвет в линейный\n"
                                                                   "потому что весь конвейер обработки линейный, но вы можете отключить это в\n"
                                                                   "Панели настроек.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ), NATRON_NAMESPACE::WhiteSpaceNormal) );
    }

    QSize medSize( TO_DPIX(NATRON_MEDIUM_BUTTON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_SIZE) );
    QSize medIconSize( TO_DPIX(NATRON_MEDIUM_BUTTON_ICON_SIZE), TO_DPIY(NATRON_MEDIUM_BUTTON_ICON_SIZE) );

    _colorLabel->setFixedSize(medSize);
    QObject::connect( _colorLabel, SIGNAL(pickingEnabled(bool)), this, SLOT(onPickingEnabled(bool)) );
    containerLayout->addWidget(_colorLabel);

    if (_useSimplifiedUI) {
        containerLayout->addSpacing( TO_DPIX(5) );
    }

    // add color selector popup
    QPixmap colorSelectorPix;
    appPTR->getIcon(NATRON_PIXMAP_COLORWHEEL, NATRON_MEDIUM_BUTTON_ICON_SIZE, &colorSelectorPix);

    _colorSelectorButton = new QToolButton( containerLayout->widget() );
    _colorSelectorButton->setObjectName( QString::fromUtf8("ColorSelectorButton") );
    _colorSelectorButton->setIcon( QIcon(colorSelectorPix) );
    _colorSelectorButton->setFixedSize(medSize);
    _colorSelectorButton->setIconSize(medIconSize);
    _colorSelectorButton->setPopupMode(QToolButton::InstantPopup);
    _colorSelectorButton->setArrowType(Qt::NoArrow);
    _colorSelectorButton->setAutoRaise(false);
    _colorSelectorButton->setCheckable(false);
    _colorSelectorButton->setToolTip( NATRON_NAMESPACE::convertFromPlainText(tr("Открыть выбор цвета"), NATRON_NAMESPACE::WhiteSpaceNormal) );
    _colorSelectorButton->setFocusPolicy(Qt::NoFocus);

    _colorSelector = new ColorSelectorWidget( knob->getDimension() == 4, containerLayout->widget() );
    QObject::connect( _colorSelector, SIGNAL( colorChanged(float, float, float, float) ),
                      this, SLOT( onColorSelectorChanged(float, float, float, float) ) );
    QObject::connect( _colorSelector, SIGNAL( updateColor() ),
                      this, SLOT( updateColorSelector() ) );

    QWidgetAction *colorPopupAction = new QWidgetAction( containerLayout->widget() );
    colorPopupAction->setDefaultWidget(_colorSelector);
    _colorSelectorButton->addAction(colorPopupAction);
    containerLayout->addWidget(_colorSelectorButton);

    if (_useSimplifiedUI) {
        KnobGuiValue::_hide();
        enableRightClickMenu(_colorLabel, -1);
    }
}

void
KnobGuiColor::onMustShowAllDimension()
{
    onDimensionSwitchClicked(true);
}

void
KnobGuiColor::onColorSelectorChanged(float r,
                                     float g,
                                     float b,
                                     float a)
{
    KnobColorPtr knob = _knob.lock();
    int nDims = knob->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    if (nDims == 1) {
        knob->setValue(r, ViewSpec::all(), 0);
    } else if (nDims == 3) {
        knob->setValues(r, g, b,
                        ViewSpec::all(),
                        eValueChangedReasonNatronInternalEdited);
    } else if (nDims == 4) {
        knob->setValues(r, g, b, a,
                        ViewSpec::all(),
                        eValueChangedReasonNatronInternalEdited);
    }
    if ( getGui() ) {
        getGui()->setDraftRenderEnabled(true);
    }
}

void
KnobGuiColor::updateLabel(double r,
                          double g,
                          double b,
                          double a)
{
    QColor color;
    KnobColorPtr knob = _knob.lock();
    bool simple = _useSimplifiedUI;

    color.setRgbF( Image::clamp<qreal>(simple ? r : Color::to_func_srgb(r), 0., 1.),
                   Image::clamp<qreal>(simple ? g : Color::to_func_srgb(g), 0., 1.),
                   Image::clamp<qreal>(simple ? b : Color::to_func_srgb(b), 0., 1.),
                   Image::clamp<qreal>(a, 0., 1.) );
    _colorLabel->setColor(color);
}

void
KnobGuiColor::setPickingEnabled(bool enabled)
{
    if (!_colorLabel) {
        return;
    }
    if (_colorLabel->isPickingEnabled() == enabled) {
        return;
    }
    _colorLabel->setPickingEnabled(enabled);
    onPickingEnabled(enabled);
}

void
KnobGuiColor::onPickingEnabled(bool enabled)
{
    if ( getKnob()->getHolder()->getApp() ) {
        if (enabled) {
            getGui()->registerNewColorPicker( _knob.lock() );
        } else {
            getGui()->removeColorPicker( _knob.lock() );
        }
    }
}

void
KnobGuiColor::_hide()
{
    if (!_useSimplifiedUI) {
        KnobGuiValue::_hide();
    }
    _colorLabel->hide();
    _colorSelectorButton->hide();
}

void
KnobGuiColor::_show()
{
    if (!_useSimplifiedUI) {
        KnobGuiValue::_show();
    }
    _colorLabel->show();
    _colorSelectorButton->show();
}

void
KnobGuiColor::updateExtraGui(const std::vector<double>& values)
{
    assert(values.size() > 0);
    double r = values[0];
    double g = r;
    double b = r;
    double a = r;
    if (values.size() > 1) {
        g = values[1];
    }
    if (values.size() > 2) {
        b = values[2];
    }
    if (values.size() > 3) {
        a = values[3];
    }
    updateLabel(r, g, b, a);
}

void
KnobGuiColor::onDimensionsFolded()
{
    KnobColorPtr knob = _knob.lock();
    int nDims = knob->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    for (int i = 0; i < nDims; ++i) {
        SpinBox* sb = 0;
        getSpinBox(i, &sb);
        assert(sb);
        sb->setUseLineColor(false, Qt::red);
    }
    Q_EMIT dimensionSwitchToggled(false);
}

void
KnobGuiColor::onDimensionsExpanded()
{
    QColor colors[4];

    colors[0].setRgbF(0.851643, 0.196936, 0.196936);
    colors[1].setRgbF(0, 0.654707, 0);
    colors[2].setRgbF(0.345293, 0.345293, 1);
    colors[3].setRgbF(0.398979, 0.398979, 0.398979);

    KnobColorPtr knob = _knob.lock();
    int nDims = knob->getDimension();

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    for (int i = 0; i < nDims; ++i) {
        SpinBox* sb = 0;
        Label* label = 0;
        getSpinBox(i, &sb, &label);
        assert(sb);
        sb->setUseLineColor(true, colors[i]);
    }
    Q_EMIT dimensionSwitchToggled(true);
}

void
KnobGuiColor::setEnabledExtraGui(bool enabled)
{
    _colorLabel->setEnabledMode(enabled);
    _colorSelectorButton->setEnabled(enabled);
}

void
KnobGuiColor::updateColorSelector()
{
    KnobColorPtr knob = _knob.lock();
    const int nDims = knob->getDimension();
    double curR = knob->getValue(0);

    assert(nDims == 1 || nDims == 3 || nDims == 4);
    if (nDims != 1 && nDims != 3 && nDims != 4) {
        throw std::logic_error("A color Knob can only have dimension 1, 3 or 4");
    }

    double curG = curR;
    double curB = curR;
    double curA = 1.;
    if (nDims > 1) {
        curG = knob->getValue(1);
        curB = knob->getValue(2);
    }
    if (nDims > 3) {
        curA = knob->getValue(3);
    }

    _colorSelector->setColor( Image::clamp<qreal>(curR, 0., 1.),
                              Image::clamp<qreal>(curG, 0., 1.),
                              Image::clamp<qreal>(curB, 0., 1.),
                              Image::clamp<qreal>(curA, 0., 1.) );
}

bool
KnobGuiColor::isAutoFoldDimensionsEnabled() const
{
    // [FD] I think all color parameters should be folded automatically:
    // Grade and ColorCorrect are some of the most used nodes,
    // and they have "A" channel uncked by default, so we don't care if colors
    // are folded.
    // The only case where this could matter are the "Draw" nodes (Rectangle, Constant, etc.),
    // but these are rarely used compared to "Color" nodes.
    return true;
    //KnobColorPtr knob = _knob.lock();
    //if (!knob) {
    //    return false;
    //}
    //return knob->getDimension() == 3;
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_KnobGuiColor.cpp"
