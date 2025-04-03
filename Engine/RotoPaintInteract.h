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

#ifndef ROTOPAINTINTERACT_H
#define ROTOPAINTINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <ofxNatron.h>

#include "Engine/EngineFwd.h"
#include "Engine/BezierCP.h"
#include "Engine/Bezier.h"

NATRON_NAMESPACE_ENTER


#define kControlPointMidSize 3
#define kBezierSelectionTolerance 8
#define kControlPointSelectionTolerance 8
#define kXHairSelectedCpsTolerance 8
#define kXHairSelectedCpsBox 8
#define kTangentHandleSelectionTolerance 8
#define kTransformArrowLenght 10
#define kTransformArrowWidth 3
#define kTransformArrowOffsetFromPoint 15


// parameters


// The toolbar
#define kRotoUIParamToolbar "Toolbar"

#define kRotoUIParamSelectionToolButton "SelectionToolButton"
#define kRotoUIParamSelectionToolButtonLabel "Инструмент выбора"

#define kRotoUIParamSelectAllToolButtonAction "SelectAllTool"
#define kRotoUIParamSelectAllToolButtonActionLabel "Выберать все инструменты"
#define kRotoUIParamSelectAllToolButtonActionHint "Все может быть выбрано и перемещено"

#define kRotoUIParamSelectPointsToolButtonAction "SelectPointsTool"
#define kRotoUIParamSelectPointsToolButtonActionLabel "Выберать инструмент Точки"
#define kRotoUIParamSelectPointsToolButtonActionHint "Работает только для точек внутренней формы" \
    " точки растушевки учитываться не будут"

#define kRotoUIParamSelectShapesToolButtonAction "SelectShapesTool"
#define kRotoUIParamSelectShapesToolButtonActionLabel "Выберать инструмент Фигуры"
#define kRotoUIParamSelectShapesToolButtonActionHint "Могут быть выбраны только формы"

#define kRotoUIParamSelectFeatherPointsToolButtonAction "SelectFeatherTool"
#define kRotoUIParamSelectFeatherPointsToolButtonActionLabel "Выберать инструмент Точки растушевки"
#define kRotoUIParamSelectFeatherPointsToolButtonActionHint "Можно выбрать только точки растушевки"

#define kRotoUIParamEditPointsToolButton "EditPointsToolButton"
#define kRotoUIParamEditPointsToolButtonLabel "Инструмент редактирования очков"

#define kRotoUIParamAddPointsToolButtonAction "AddPointsTool"
#define kRotoUIParamAddPointsToolButtonActionLabel "Инструмент добавления точек"
#define kRotoUIParamAddPointsToolButtonActionHint "Добавьте контрольную точку к фигуре"

#define kRotoUIParamRemovePointsToolButtonAction "RemovePointsTool"
#define kRotoUIParamRemovePointsToolButtonActionLabel "Инструмент удаления точек"
#define kRotoUIParamRemovePointsToolButtonActionHint "Удалите контрольную точку с фигуры"

#define kRotoUIParamCuspPointsToolButtonAction "CuspPointsTool"
#define kRotoUIParamCuspPointsToolButtonActionLabel "Инструмент Точки пересечения"
#define kRotoUIParamCuspPointsToolButtonActionHint "Остроконечные точки на фигуре"

#define kRotoUIParamSmoothPointsToolButtonAction "SmoothPointsTool"
#define kRotoUIParamSmoothPointsToolButtonActionLabel "Инструмент сглаживания точек"
#define kRotoUIParamSmoothPointsToolButtonActionHint "Сглаживайте точки на фигуре"

#define kRotoUIParamOpenCloseCurveToolButtonAction "OpenCloseShapeTool"
#define kRotoUIParamOpenCloseCurveToolButtonActionLabel "Инструмент открытия/закрытия фигур"
#define kRotoUIParamOpenCloseCurveToolButtonActionHint "Открывайте или закрывайте фигуры"

#define kRotoUIParamRemoveFeatherToolButtonAction "RemoveFeatherPointTool"
#define kRotoUIParamRemoveFeatherToolButtonActionLabel "Инструмент удаления растушевки"
#define kRotoUIParamRemoveFeatherToolButtonActionHint "Удалите растушевку на точках"

#define kRotoUIParamBezierEditionToolButton "EditBezierToolButton"
#define kRotoUIParamBezierEditionToolButtonLabel "Инструмент рисования фигур"

#define kRotoUIParamDrawBezierToolButtonAction "DrawBezierTool"
#define kRotoUIParamDrawBezierToolButtonActionLabel "Инструмент Безье"
#define kRotoUIParamDrawBezierToolButtonActionHint "Нарисуйте Безье"

#define kRotoUIParamDrawEllipseToolButtonAction "DrawEllipseTool"
#define kRotoUIParamDrawEllipseToolButtonActionLabel "Инструмент эллипса"
#define kRotoUIParamDrawEllipseToolButtonActionHint "Нарисуйте эллипс"

#define kRotoUIParamDrawRectangleToolButtonAction "DrawRectangleTool"
#define kRotoUIParamDrawRectangleToolButtonActionLabel "Инструмент прямоугольника"
#define kRotoUIParamDrawRectangleToolButtonActionHint "Нарисуйте прямоугольник"


#define kRotoUIParamPaintBrushToolButton "PaintBrushToolButton"
#define kRotoUIParamPaintBrushToolButtonLabel "Инструмент кисть"
odge Tool
#define kRotoUIParamDrawBrushToolButtonAction "PaintSolidTool"
#define kRotoUIParamDrawBrushToolButtonActionLabel "Сплошная кисть"
#define kRotoUIParamDrawBrushToolButtonActionHint "Рисуйте с помощью пера"

#define kRotoUIParamPencilToolButtonAction "PencilTool"
#define kRotoUIParamPencilToolButtonActionLabel "Карандаш"
#define kRotoUIParamPencilToolButtonActionHint "Draw open bezier"

#define kRotoUIParamEraserToolButtonAction "EraserTool"
#define kRotoUIParamEraserToolButtonActionLabel "Ластик"
#define kRotoUIParamEraserToolButtonActionHint "Используйте ластик"

#define kRotoUIParamCloneBrushToolButton "CloneToolButton"
#define kRotoUIParamCloneBrushToolButtonLabel "Клон инструмента"

#define kRotoUIParamCloneToolButtonAction "CloneTool"
#define kRotoUIParamCloneToolButtonActionLabel "Клон инструмента"
#define kRotoUIParamCloneToolButtonActionHint "Клонировать области изображения пером. Удерживайте нажатой CTRL, чтобы сместить клонирование."

#define kRotoUIParamRevealToolButtonAction "RevealTool"
#define kRotoUIParamRevealToolButtonActionLabel "Показать инструмент"
#define kRotoUIParamRevealToolButtonActionHint "Нарисуйте пером, чтобы применить эффект к выбранному источнику. Удерживая нажатой CTRL переместите смещение"

#define kRotoUIParamEffectBrushToolButton "EffectToolButton"
#define kRotoUIParamEffectBrushToolButtonLabel "Инструмент эффекта"

#define kRotoUIParamBlurToolButtonAction "BlurTool"
#define kRotoUIParamBlurToolButtonActionLabel "Инструмент размытия"
#define kRotoUIParamBlurToolButtonActionHint "Нарисуйте пером, чтобы применить размытие"

#define kRotoUIParamSmearToolButtonAction "SmearTool"
#define kRotoUIParamSmearToolButtonActionLabel "Инструмент размазывания"
#define kRotoUIParamSmearToolButtonActionHint "Нарисуйте пером, чтобы размыть и сместить часть исходного изображения в направлении движения пера"

#define kRotoUIParamMergeBrushToolButton "MergeToolButton"
#define kRotoUIParamMergeBrushToolButtonLabel "Инструмент объединения"

#define kRotoUIParamDodgeToolButtonAction "DodgeTool"
#define kRotoUIParamDodgeToolButtonActionLabel "Инструмент Dodge"
#define kRotoUIParamDodgeToolButtonActionHint "Сделайте исходное изображение ярче"

#define kRotoUIParamBurnToolButtonAction "BurnTool"
#define kRotoUIParamBurnToolButtonActionLabel "Выжигание"
#define kRotoUIParamBurnToolButtonActionHint "Сделайте исходное изображение более темным"

// The right click menu
#define kRotoUIParamRightClickMenu kNatronOfxParamRightClickMenu

//For all items
#define kRotoUIParamRightClickMenuActionRemoveItems "removeItemsAction"
#define kRotoUIParamRightClickMenuActionRemoveItemsLabel "Удалить выбранные элементы"

#define kRotoUIParamRightClickMenuActionCuspItems "cuspItemsAction"
#define kRotoUIParamRightClickMenuActionCuspItemsLabel "Вырезать элементы"

#define kRotoUIParamRightClickMenuActionSmoothItems "SmoothItemsAction"
#define kRotoUIParamRightClickMenuActionSmoothItemsLabel "Сгладить выбранные элементы"

#define kRotoUIParamRightClickMenuActionRemoveItemsFeather "removeItemsFeatherAction"
#define kRotoUIParamRightClickMenuActionRemoveItemsFeatherLabel "Удалить элементы с помощью пера"


#define kRotoUIParamRightClickMenuActionNudgeLeft "nudgeLeftAction"
#define kRotoUIParamRightClickMenuActionNudgeLeftLabel "Подтолкнуть влево"

#define kRotoUIParamRightClickMenuActionNudgeRight "nudgeRightAction"
#define kRotoUIParamRightClickMenuActionNudgeRightLabel "Подтолкнуть вправо"

#define kRotoUIParamRightClickMenuActionNudgeBottom "nudgeBottomAction"
#define kRotoUIParamRightClickMenuActionNudgeBottomLabel "Подтолкнуть вниз"

#define kRotoUIParamRightClickMenuActionNudgeTop "nudgeTopAction"
#define kRotoUIParamRightClickMenuActionNudgeTopLabel "Подтолкнуть вверх"

// just for shapes
#define kRotoUIParamRightClickMenuActionSelectAll "selectAllAction"
#define kRotoUIParamRightClickMenuActionSelectAllLabel "Выбрать все"

#define kRotoUIParamRightClickMenuActionOpenClose "openCloseAction"
#define kRotoUIParamRightClickMenuActionOpenCloseLabel "Открытая/закрытая форма"

#define kRotoUIParamRightClickMenuActionLockShapes "lockShapesAction"
#define kRotoUIParamRightClickMenuActionLockShapesLabel "Закрыть формы"

// Viewer UI buttons

// Roto
#define kRotoUIParamAutoKeyingEnabled "autoKeyingEnabledButton"
#define kRotoUIParamAutoKeyingEnabledLabel "Включить автоввод данных"
#define kRotoUIParamAutoKeyingEnabledHint "Любое изменение, внесенное в контрольную точку, установит ключевой кадр на текущий момент времени"

#define kRotoUIParamFeatherLinkEnabled "featherLinkEnabledButton"
#define kRotoUIParamFeatherLinkEnabledLabel "Включить растушевку"
#define kRotoUIParamFeatherLinkEnabledHint "Соединение перьев: при активации наконечники перьев будут двигаться так же," \
    " как и их противоположные части"

#define kRotoUIParamDisplayFeather "displayFeatherButton"
#define kRotoUIParamDisplayFeatherLabel "Показать перо"
#define kRotoUIParamDisplayFeatherHint "При проверке кривая перья, приложенная к форме (-ам), будет видна и редактируется"

#define kRotoUIParamStickySelectionEnabled "stickySelectionEnabledButton"
#define kRotoUIParamStickySelectionEnabledLabel "Включить прилипание"
#define kRotoUIParamStickySelectionEnabledHint "Выделение прилипанием: при активации " \
    " щелчок за пределами любой фигуры не приведет к удалению текущего выделения"

#define kRotoUIParamStickyBbox "stickyBboxButton"
#define kRotoUIParamStickyBboxLabel "Включить липкую ограничивающую рамку"
#define kRotoUIParamStickyBboxHint "Простое управление ограничивающим прямоугольником: при активации " \
    " щелчок внутри ограничивающего прямоугольника выбранных точек приведет к перемещению точек." \
    " А при деактивации точки перемещаются только при нажатии на крестик"

#define kRotoUIParamRippleEdit "rippleEditButton"
#define kRotoUIParamRippleEditLabel "Включить редактирование пульсаций"
#define kRotoUIParamRippleEditLabelHint "Ripple-редактирование: при активации перемещение контрольной точки" \
    " переместит ее на одинаковую величину для всех ключевых кадров " \
    " которые у нее есть"

#define kRotoUIParamAddKeyFrame "addKeyframeButton"
#define kRotoUIParamAddKeyFrameLabel "Добавить ключевой кадр"
#define kRotoUIParamAddKeyFrameHint "Установите ключевой кадр в текущий момент времени для выбранных фигур"

#define kRotoUIParamRemoveKeyframe "removeKeyframeButton"
#define kRotoUIParamRemoveKeyframeLabel "Удалить ключевой кадр"
#define kRotoUIParamRemoveKeyframeHint "Удалите ключевой кадр в текущий момент времени для выбранных фигур"

#define kRotoUIParamShowTransform "showTransformButton"
#define kRotoUIParamShowTransformLabel "Показать ручку трасформ"
#define kRotoUIParamShowTransformHint "Если флажок снят, даже если вкладка Трансформ видна на панели, дескриптор трансформ будет скрыт"

// RotoPaint
#define kRotoUIParamColorWheel "strokeColorButton"
#define kRotoUIParamColorWheelLabel "Цвет обводки"
#define kRotoUIParamColorWheelHint "Цвет следующего мазка кистью, который будет нанесен"

#define kRotoUIParamBlendingOp "blendingModeButton"
#define kRotoUIParamBlendingOpLabel "Режим смешивания"
#define kRotoUIParamBlendingOpHint "Режим наложения следующего мазка кистью"

#define kRotoUIParamOpacity "opacitySpinbox"
#define kRotoUIParamOpacityLabel "Непрозрачность"
#define kRotoUIParamOpacityHint "Непрозрачность следующего мазка кисти. Сочетание клавиш CTRL + SHIFT + перетаск " \
    "чтобы изменить непрозрачность, щелкните мышью."

#define kRotoUIParamPressureOpacity "pressureOpacityButton"
#define kRotoUIParamPressureOpacityLabel "Давление влияет на непрозрачность"
#define kRotoUIParamPressureOpacityHint "Если этот флажок установлен, нажатие пера будет динамически изменять непрозрачность  " \
    "следующего мазка кистью"

#define kRotoUIParamSize "sizeSpinbox"
#define kRotoUIParamSizeLabel "Размер кисти"
#define kRotoUIParamSizeHint "Размер следующего мазка кисти, который нужно нарисовать. Используйте SHIFT + перетаск " \
    "чтобы изменить размер"

#define kRotoUIParamPressureSize "pressureSizeButton"
#define kRotoUIParamPressureSizeLabel "Давление влияет на размер"
#define kRotoUIParamPressureSizeHint "Если этот флажок установлен, нажим пера будет динамически изменять размер " \
    "следующего мазка кисти. "

#define kRotoUIParamHardness "hardnessSpinbox"
#define kRotoUIParamHardnessLabel "Жесткость щетки"
#define kRotoUIParamHardnessHint "Жесткость следующего мазка кистью, который будет нанесен"

#define kRotoUIParamPressureHardness "pressureHardnessButton"
#define kRotoUIParamPressureHardnessLabel "Давление влияет на твердость"
#define kRotoUIParamPressureHardnessHint "Если этот флажок установлен, давление пера будет динамически изменять жесткость " \
    "следующего мазка кистью"

#define kRotoUIParamBuildUp "buildUpButton"
#define kRotoUIParamBuildUpLabel "Наращивание"
#define kRotoUIParamBuildUpHint "Когда включена функция наращивания, следующий мазок кисти будет наноситься " \
    "при закрашивании самого себя"

#define kRotoUIParamAutoConnectViewer "autoConnectViewerButton"
#define kRotoUIParamAutoConnectViewerLabel "Просмотрщик с автоподключением"
#define kRotoUIParamAutoConnectViewerHint "Автоподключение просмотрщика к узлу RotoPaint при использовании инструментов рисования"

#define kRotoUIParamEffect "effectSpinbox"
#define kRotoUIParamEffectLabel "Сила эффекта"
#define kRotoUIParamEffectHint "Сила следующего эффекта кисти"

#define kRotoUIParamTimeOffset "timeOffsetSpinbox"
#define kRotoUIParamTimeOffsetLabel "Смещение по времени"
#define kRotoUIParamTimeOffsetHint "Если используется инструмент клонирования то он определяет, от режима смещения по времени " \
    "исходный кадр для клонирования. В абсолютном режиме это " \
    "номер кадра источника, в относительном режиме это смещение относительно " \
    "текущего кадра"

#define kRotoUIParamTimeOffsetMode "timeOffsetModeChoice"
#define kRotoUIParamTimeOffsetModeLabel "Режим смещения по времени"
#define kRotoUIParamTimeOffsetModeHint "В абсолютном режиме это номер кадра источника, " \
    "в относительном режиме это смещение относительно " \
    "текущего кадра" \

#define kRotoUIParamSourceType "sourceTypeChoice"
#define kRotoUIParamSourceTypeLabel "Источник"
#define kRotoUIParamSourceTypeHint "Цвет для закрашивания обводки при использовании инструментов Раскрытия/клонирования: \n" \
    "- передний план: результат закрашивания в этой точке изображения. иерархия,\n" \
    "- фон: исходное неокрашенное изображение, подключенное к bg,\n" \
    "- фоновое изображение: исходное неокрашенное изображение, подключенное к bgN"

#define kRotoUIParamResetCloneOffset "resetCloneOffsetButton"
#define kRotoUIParamResetCloneOffsetLabel "Сброс Трансформ"
#define kRotoUIParamResetCloneOffsetHint "Сбросьте преобразование, примененное перед клонированием, в identity"

#define kRotoUIParamMultiStrokeEnabled "multiStrokeEnabledButton"
#define kRotoUIParamMultiStrokeEnabledLabel "Multi-Stroke"
#define kRotoUIParamMultiStrokeEnabledHint "Если этот флажок установлен, штрихи будут добавляться к одному и тому же элементу " \
    "в иерархии до тех пор, пока выбран один и тот же инструмент.\n" \
    "Выберите другой инструмент, чтобы создать новый предмет."


// Shortcuts

#define kShortcutIDActionRotoDelete "delete"
#define kShortcutDescActionRotoDelete "Delete Element"

#define kShortcutIDActionRotoCloseBezier "closeBezier"
#define kShortcutDescActionRotoCloseBezier "Close Bezier"

#define kShortcutIDActionRotoSelectAll "selectAll"
#define kShortcutDescActionRotoSelectAll "Select All"

#define kShortcutIDActionRotoSelectionTool "selectionTool"
#define kShortcutDescActionRotoSelectionTool "Switch to Selection Mode"

#define kShortcutIDActionRotoAddTool "addTool"
#define kShortcutDescActionRotoAddTool "Switch to Add Mode"

#define kShortcutIDActionRotoEditTool "editTool"
#define kShortcutDescActionRotoEditTool "Switch to Edition Mode"

#define kShortcutIDActionRotoBrushTool "brushTool"
#define kShortcutDescActionRotoBrushTool "Switch to Brush Mode"

#define kShortcutIDActionRotoCloneTool "cloneTool"
#define kShortcutDescActionRotoCloneTool "Switch to Clone Mode"

#define kShortcutIDActionRotoEffectTool "EffectTool"
#define kShortcutDescActionRotoEffectTool "Switch to Effect Mode"

#define kShortcutIDActionRotoColorTool "colorTool"
#define kShortcutDescActionRotoColorTool "Switch to Color Mode"

#define kShortcutIDActionRotoNudgeLeft "nudgeLeft"
#define kShortcutDescActionRotoNudgeLeft "Move Bezier to the Left"

#define kShortcutIDActionRotoNudgeRight "nudgeRight"
#define kShortcutDescActionRotoNudgeRight "Move Bezier to the Right"

#define kShortcutIDActionRotoNudgeBottom "nudgeBottom"
#define kShortcutDescActionRotoNudgeBottom "Move Bezier to the Bottom"

#define kShortcutIDActionRotoNudgeTop "nudgeTop"
#define kShortcutDescActionRotoNudgeTop "Move Bezier to the Top"

#define kShortcutIDActionRotoSmooth "smooth"
#define kShortcutDescActionRotoSmooth "Smooth Bezier"

#define kShortcutIDActionRotoCuspBezier "cusp"
#define kShortcutDescActionRotoCuspBezier "Cusp Bezier"

#define kShortcutIDActionRotoRemoveFeather "rmvFeather"
#define kShortcutDescActionRotoRemoveFeather "Remove Feather"

#define kShortcutIDActionRotoLockCurve "lock"
#define kShortcutDescActionRotoLockCurve "Lock Shape"

class RotoPaintInteract;
struct RotoPaintPrivate
{
    RotoPaint* publicInterface;
    bool isPaintByDefault;
    KnobBoolWPtr premultKnob;
    KnobBoolWPtr enabledKnobs[4];
    RotoPaintInteractPtr ui;

    RotoPaintPrivate(RotoPaint* publicInterface,
                     bool isPaintByDefault);
};

///A list of points and their counter-part, that is: either a control point and its feather point, or
///the feather point and its associated control point
typedef std::pair<BezierCPPtr, BezierCPPtr> SelectedCP;
typedef std::list<SelectedCP > SelectedCPs;
typedef std::list<RotoDrawableItemPtr> SelectedItems;

enum EventStateEnum
{
    eEventStateNone = 0,
    eEventStateDraggingControlPoint,
    eEventStateDraggingSelectedControlPoints,
    eEventStateBuildingBezierControlPointTangent,
    eEventStateBuildingEllipse,
    eEventStateBuildingRectangle,
    eEventStateDraggingLeftTangent,
    eEventStateDraggingRightTangent,
    eEventStateDraggingFeatherBar,
    eEventStateDraggingBBoxTopLeft,
    eEventStateDraggingBBoxTopRight,
    eEventStateDraggingBBoxBtmRight,
    eEventStateDraggingBBoxBtmLeft,
    eEventStateDraggingBBoxMidTop,
    eEventStateDraggingBBoxMidRight,
    eEventStateDraggingBBoxMidBtm,
    eEventStateDraggingBBoxMidLeft,
    eEventStateBuildingStroke,
    eEventStateDraggingCloneOffset,
    eEventStateDraggingBrushSize,
    eEventStateDraggingBrushOpacity,
};

enum HoverStateEnum
{
    eHoverStateNothing = 0,
    eHoverStateBboxTopLeft,
    eHoverStateBboxTopRight,
    eHoverStateBboxBtmRight,
    eHoverStateBboxBtmLeft,
    eHoverStateBboxMidTop,
    eHoverStateBboxMidRight,
    eHoverStateBboxMidBtm,
    eHoverStateBboxMidLeft,
    eHoverStateBbox
};

enum SelectedCpsTransformModeEnum
{
    eSelectedCpsTransformModeTranslateAndScale = 0,
    eSelectedCpsTransformModeRotateAndSkew = 1
};

enum RotoRoleEnum
{
    eRotoRoleSelection = 0,
    eRotoRolePointsEdition,
    eRotoRoleBezierEdition,
    eRotoRolePaintBrush,
    eRotoRoleCloneBrush,
    eRotoRoleEffectBrush,
    eRotoRoleMergeBrush
};

enum RotoToolEnum
{
    eRotoToolSelectAll = 0,
    eRotoToolSelectPoints,
    eRotoToolSelectCurves,
    eRotoToolSelectFeatherPoints,

    eRotoToolAddPoints,
    eRotoToolRemovePoints,
    eRotoToolRemoveFeatherPoints,
    eRotoToolOpenCloseCurve,
    eRotoToolSmoothPoints,
    eRotoToolCuspPoints,

    eRotoToolDrawBezier,
    eRotoToolDrawBSpline,
    eRotoToolDrawEllipse,
    eRotoToolDrawRectangle,

    eRotoToolSolidBrush,
    eRotoToolOpenBezier,
    eRotoToolEraserBrush,

    eRotoToolClone,
    eRotoToolReveal,

    eRotoToolBlur,
    eRotoToolSharpen,
    eRotoToolSmear,

    eRotoToolDodge,
    eRotoToolBurn
};

class RotoPaintInteract
    : public std::enable_shared_from_this<RotoPaintInteract>
{
public:
    RotoPaintPrivate* p;
    SelectedItems selectedItems;
    SelectedCPs selectedCps;
    QRectF selectedCpsBbox;
    bool showCpsBbox;

    ////This is by default eSelectedCpsTransformModeTranslateAndScale. When clicking the cross-hair in the center this will toggle the transform mode
    ////like it does in inkscape.
    SelectedCpsTransformModeEnum transformMode;
    BezierPtr builtBezier; //< the bezier currently being built
    BezierPtr bezierBeingDragged;
    SelectedCP cpBeingDragged; //< the cp being dragged
    BezierCPPtr tangentBeingDragged; //< the control point whose tangent is being dragged.
    //only relevant when the state is DRAGGING_X_TANGENT
    SelectedCP featherBarBeingDragged, featherBarBeingHovered;
    RotoStrokeItemPtr strokeBeingPaint;
    int strokeBeingPaintedTimelineFrame;// the frame at which we painted the last brush stroke
    std::pair<double, double> cloneOffset;
    QPointF click; // used for drawing ellipses and rectangles, to handle center/constrain. May also be used for the selection bbox.
    RotoToolEnum selectedTool;
    RotoRoleEnum selectedRole;
    KnobButtonWPtr lastPaintToolAction;
    EventStateEnum state;
    HoverStateEnum hoverState;
    QPointF lastClickPos;
    QPointF lastMousePos;
    bool evaluateOnPenUp; //< if true the next pen up will call context->evaluateChange()
    bool evaluateOnKeyUp;  //< if true the next key up will call context->evaluateChange()
    bool iSelectingwithCtrlA;
    int shiftDown;
    int ctrlDown;
    int altDown;
    bool lastTabletDownTriggeredEraser;
    QPointF mouseCenterOnSizeChange;


    //////// Toolbar
    KnobPageWPtr toolbarPage;
    KnobGroupWPtr selectedToolRole;
    KnobButtonWPtr selectedToolAction;
    KnobGroupWPtr selectToolGroup;
    KnobButtonWPtr selectAllAction;
    KnobButtonWPtr selectPointsAction;
    KnobButtonWPtr selectCurvesAction;
    KnobButtonWPtr selectFeatherPointsAction;
    KnobGroupWPtr pointsEditionToolGroup;
    KnobButtonWPtr addPointsAction;
    KnobButtonWPtr removePointsAction;
    KnobButtonWPtr cuspPointsAction;
    KnobButtonWPtr smoothPointsAction;
    KnobButtonWPtr openCloseCurveAction;
    KnobButtonWPtr removeFeatherAction;
    KnobGroupWPtr bezierEditionToolGroup;
    KnobButtonWPtr drawBezierAction;
    KnobButtonWPtr drawEllipseAction;
    KnobButtonWPtr drawRectangleAction;
    KnobGroupWPtr paintBrushToolGroup;
    KnobButtonWPtr brushAction;
    KnobButtonWPtr pencilAction;
    KnobButtonWPtr eraserAction;
    KnobGroupWPtr cloneBrushToolGroup;
    KnobButtonWPtr cloneAction;
    KnobButtonWPtr revealAction;
    KnobGroupWPtr effectBrushToolGroup;
    KnobButtonWPtr blurAction;
    KnobButtonWPtr smearAction;
    KnobGroupWPtr mergeBrushToolGroup;
    KnobButtonWPtr dodgeAction;
    KnobButtonWPtr burnAction;

    //////Right click menu
    KnobChoiceWPtr rightClickMenuKnob;

    //Right click on point
    KnobButtonWPtr removeItemsMenuAction;
    KnobButtonWPtr cuspItemMenuAction;
    KnobButtonWPtr smoothItemMenuAction;
    KnobButtonWPtr removeItemFeatherMenuAction;
    KnobButtonWPtr nudgeLeftMenuAction, nudgeRightMenuAction, nudgeBottomMenuAction, nudgeTopMenuAction;

    // Right click on curve
    KnobButtonWPtr selectAllMenuAction;
    KnobButtonWPtr openCloseMenuAction;
    KnobButtonWPtr lockShapeMenuAction;

    // Roto buttons
    KnobButtonWPtr autoKeyingEnabledButton;
    KnobButtonWPtr featherLinkEnabledButton;
    KnobButtonWPtr displayFeatherEnabledButton;
    KnobButtonWPtr stickySelectionEnabledButton;
    KnobButtonWPtr bboxClickAnywhereButton;
    KnobButtonWPtr rippleEditEnabledButton;
    KnobButtonWPtr addKeyframeButton;
    KnobButtonWPtr removeKeyframeButton;
    KnobButtonWPtr showTransformHandle;

    // RotoPaint buttons
    KnobColorWPtr colorWheelButton;
    KnobChoiceWPtr compositingOperatorChoice;
    KnobDoubleWPtr opacitySpinbox;
    KnobButtonWPtr pressureOpacityButton;
    KnobDoubleWPtr sizeSpinbox;
    KnobButtonWPtr pressureSizeButton;
    KnobDoubleWPtr hardnessSpinbox;
    KnobButtonWPtr pressureHardnessButton;
    KnobButtonWPtr buildUpButton;
    KnobButtonWPtr autoConnectViewerButton;
    KnobDoubleWPtr effectSpinBox;
    KnobIntWPtr timeOffsetSpinBox;
    KnobChoiceWPtr timeOffsetModeChoice;
    KnobChoiceWPtr sourceTypeChoice;
    KnobButtonWPtr resetCloneOffsetButton;
    KnobBoolWPtr multiStrokeEnabled;


private:
    struct MakeSharedEnabler;

    // constructors should be privatized in any class that derives from std::enable_shared_from_this<>

    RotoPaintInteract(RotoPaintPrivate* p);

public:
    static RotoPaintInteractPtr create(RotoPaintPrivate* p);

    bool isFeatherVisible() const;

    RotoContextPtr getContext();

    RotoToolEnum getSelectedTool() const
    {
        return selectedTool;
    }

    bool isStickySelectionEnabled() const;

    bool isMultiStrokeEnabled() const;

    bool getRoleForGroup(const KnobGroupPtr& group, RotoRoleEnum* role) const;
    bool getToolForAction(const KnobButtonPtr& action, RotoToolEnum* tool) const;

    bool onRoleChangedInternal(const KnobGroupPtr& roleGroup);

    bool onToolChangedInternal(const KnobButtonPtr& actionButton);

    void clearSelection();

    void clearCPSSelection();

    void clearBeziersSelection();

    bool hasSelection() const;

    void onCurveLockedChangedRecursive(const RotoItemPtr & item, bool* ret);

    bool removeItemFromSelection(const RotoDrawableItemPtr& b);

    void computeSelectedCpsBBOX();

    QPointF getSelectedCpsBBOXCenter();

    void drawSelectedCpsBBOX();

    void drawEllipse(double x,
                     double y,
                     double radiusX,
                     double radiusY,
                     int l,
                     double r,
                     double g,
                     double b,
                     double a);

    ///by default draws a vertical arrow, which can be rotated by rotate amount.
    void drawArrow(double centerX, double centerY, double rotate, bool hovered, const std::pair<double, double> & pixelScale);

    ///same as drawArrow but the two ends will make an angle of 90 degrees
    void drawBendedArrow(double centerX, double centerY, double rotate, bool hovered, const std::pair<double, double> & pixelScale);

    void handleBezierSelection(const BezierPtr & curve);

    void handleControlPointSelection(const std::pair<BezierCPPtr, BezierCPPtr> & p);

    void drawSelectedCp(double time,
                        const BezierCPPtr & cp,
                        double x, double y,
                        const Transform::Matrix3x3& transform);

    std::pair<BezierCPPtr, BezierCPPtr>isNearbyFeatherBar(double time, const std::pair<double, double> & pixelScale, const QPointF & pos) const;

    bool isNearbySelectedCpsCrossHair(const QPointF & pos) const;

    bool isWithinSelectedCpsBBox(const QPointF& pos) const;

    bool isNearbyBBoxTopLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxTopRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxBtmLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxBtmRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;

    bool isNearbyBBoxMidTop(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidRight(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidBtm(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;
    bool isNearbyBBoxMidLeft(const QPointF & p, double tolerance, const std::pair<double, double> & pixelScale) const;

    bool isNearbySelectedCpsBoundingBox(const QPointF & pos, double tolerance) const;

    EventStateEnum isMouseInteractingWithCPSBbox(const QPointF& pos, double tolerance, const std::pair<double, double>& pixelScale) const;

    bool isBboxClickAnywhereEnabled() const;

    void makeStroke(bool prepareForLater, const RotoPoint& p);

    void checkViewersAreDirectlyConnected();

    void showMenuForControlPoint(const BezierCPPtr& cp);

    void showMenuForCurve(const BezierPtr & curve);

    void setCurrentTool(const KnobButtonPtr& tool);

    void onBreakMultiStrokeTriggered();


    /**
     * @brief Set the selection to be the given beziers and the given control points.
     * This can only be called on the main-thread.
     **/
    void setSelection(const std::list<RotoDrawableItemPtr> & selectedBeziers,
                      const std::list<std::pair<BezierCPPtr, BezierCPPtr> > & selectedCps);
    void setSelection(const BezierPtr & curve,
                      const std::pair<BezierCPPtr, BezierCPPtr> & point);

    void getSelection(std::list<RotoDrawableItemPtr>* selectedBeziers,
                      std::list<std::pair<BezierCPPtr, BezierCPPtr> >* selectedCps);

    void setBuiltBezier(const BezierPtr & curve);

    BezierPtr getBezierBeingBuild() const;

    bool smoothSelectedCurve();
    bool cuspSelectedCurve();
    bool removeFeatherForSelectedCurve();
    bool lockSelectedCurves();


    /**
     *@brief Moves of the given pixel the selected control points.
     * This takes into account the zoom factor.
     **/
    bool moveSelectedCpsWithKeyArrows(int x, int y);


    void evaluate(bool redraw);
    void autoSaveAndRedraw();

    void redrawOverlays();


    /**
     * @brief Calls RotoContext::removeItem but also clears some pointers if they point to
     * this curve. For undo/redo purpose.
     **/
    void removeCurve(const RotoDrawableItemPtr& curve);
};

NATRON_NAMESPACE_EXIT

#endif // ROTOPAINTINTERACT_H
