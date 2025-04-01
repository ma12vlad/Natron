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

#ifndef ACTIONSHORTCUTS_H
#define ACTIONSHORTCUTS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

/**
 * @brief In this file all Natron's actions that can have their shortcut edited should be listed.
 **/

#include <map>
#include <list>
#include <vector>

#include "Global/Macros.h"

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QKeyEvent>
#include <QMouseEvent>
#include <QtCore/QString>
#include <QAction>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/PluginActionShortcut.h"
#include "Gui/GuiFwd.h"

#define kShortcutGroupGlobal "Global"
#define kShortcutGroupNodegraph "NodeGraph"
#define kShortcutGroupCurveEditor "CurveEditor"
#define kShortcutGroupDopeSheetEditor "DopeSheetEditor"
#define kShortcutGroupViewer "Viewer"
#define kShortcutGroupRoto "Roto"
#define kShortcutGroupTracking "Tracking"
#define kShortcutGroupPlayer "Player"
#define kShortcutGroupNodes "Nodes"
#define kShortcutGroupScriptEditor "ScriptEditor"

/////////GLOBAL SHORTCUTS
#define kShortcutIDActionNewProject "newProject"
#define kShortcutDescActionNewProject "Новый проект"

#define kShortcutIDActionOpenProject "openProject"
#define kShortcutDescActionOpenProject "Открыть проект..."

#define kShortcutIDActionCloseProject "closeProject"
#define kShortcutDescActionCloseProject "Закрыть проект"

#define kShortcutIDActionReloadProject "reloadProject"
#define kShortcutDescActionReloadProject "Перезагрузить проект"

#define kShortcutIDActionSaveProject "saveProject"
#define kShortcutDescActionSaveProject "Сохранить проект"

#define kShortcutIDActionSaveAsProject "saveAsProject"
#define kShortcutDescActionSaveAsProject "Сохранить проект как .."

#define kShortcutIDActionSaveAndIncrVersion "saveAndIncr"
#define kShortcutDescActionSaveAndIncrVersion "Новая версия проекта"

#define kShortcutIDActionExportProject "exportAsGroup"
#define kShortcutDescActionExportProject "Экспорт проекта как группа"

#define kShortcutIDActionPreferences "preferences"
#define kShortcutDescActionPreferences "Предпочтения ..."

#define kShortcutIDActionQuit "quit"
#define kShortcutDescActionQuit "Выйти"

#define kShortcutIDActionProjectSettings "projectSettings"
#define kShortcutDescActionProjectSettings "Настройки проекта ..."

#define kShortcutIDActionShowErrorLog "showErrorLog"
#define kShortcutDescActionShowErrorLog "Журнал ошибок проекта ..."

#define kShortcutIDActionNewViewer "newViewer"
#define kShortcutDescActionNewViewer "Новый просмотрщик"

#define kShortcutIDActionFullscreen "fullScreen"
#define kShortcutDescActionFullscreen "Полный экран"

#define kShortcutIDActionShowWindowsConsole "showApplicationConsole"
#define kShortcutDescActionShowWindowsConsole "Показать/скрыть консоль приложения"

#define kShortcutIDActionClearDiskCache "clearDiskCache"
#define kShortcutDescActionClearDiskCache "Очистить дисковый кэш"

#define kShortcutIDActionClearPlaybackCache "clearPlaybackCache"
#define kShortcutDescActionClearPlaybackCache "Очистить кэш воспроизведения"

#define kShortcutIDActionClearNodeCache "clearNodeCache"
#define kShortcutDescActionClearNodeCache "Очистить кэш для каждого узла"

#define kShortcutIDActionClearPluginsLoadCache "clearPluginsCache"
#define kShortcutDescActionClearPluginsLoadCache "Очистить кэш загрузки плагинов"

#define kShortcutIDActionClearAllCaches "clearAllCaches"
#define kShortcutDescActionClearAllCaches "Очистить все кэши"

#define kShortcutIDActionShowAbout "showAbout"
#define kShortcutDescActionShowAbout "О Natron"

#define kShortcutIDActionRenderSelected "renderSelect"
#define kShortcutDescActionRenderSelected "Рендер выбранных записей"

#define kShortcutIDActionEnableRenderStats "enableRenderStats"
#define kShortcutDescActionEnableRenderStats "Включить статистику рендеринга"

#define kShortcutIDActionRenderAll "renderAll"
#define kShortcutDescActionRenderAll "Рендер все записи"

#define kShortcutIDActionConnectViewerToInput1 "connectViewerInput1"
#define kShortcutDescActionConnectViewerToInput1 "Подключить просмотрщик к входу 1"

#define kShortcutIDActionConnectViewerToInput2 "connectViewerInput2"
#define kShortcutDescActionConnectViewerToInput2 "Подключить просмотрщик к входу 2"

#define kShortcutIDActionConnectViewerToInput3 "connectViewerInput3"
#define kShortcutDescActionConnectViewerToInput3 "Подключить просмотрщик к входу 3"

#define kShortcutIDActionConnectViewerToInput4 "connectViewerInput4"
#define kShortcutDescActionConnectViewerToInput4 "Подключить просмотрщик к входу 4"

#define kShortcutIDActionConnectViewerToInput5 "connectViewerInput5"
#define kShortcutDescActionConnectViewerToInput5 "Подключить просмотрщик к входу 5"

#define kShortcutIDActionConnectViewerToInput6 "connectViewerInput6"
#define kShortcutDescActionConnectViewerToInput6 "Подключить просмотрщик к входу 6"

#define kShortcutIDActionConnectViewerToInput7 "connectViewerInput7"
#define kShortcutDescActionConnectViewerToInput7 "Подключить просмотрщик к входу 7"

#define kShortcutIDActionConnectViewerToInput8 "connectViewerInput8"
#define kShortcutDescActionConnectViewerToInput8 "Подключить просмотрщик к входу 8"

#define kShortcutIDActionConnectViewerToInput9 "connectViewerInput9"
#define kShortcutDescActionConnectViewerToInput9 "Подключить просмотрщик к входу 9"

#define kShortcutIDActionConnectViewerToInput10 "connectViewerInput10"
#define kShortcutDescActionConnectViewerToInput10 "Подключить просмотрщик к входу 10"

#define kShortcutIDActionConnectViewerBToInput1 "connectViewerBInput1"
#define kShortcutDescActionConnectViewerBToInput1 "Подключить просмотрщик В к входу 1"

#define kShortcutIDActionConnectViewerBToInput2 "connectViewerBInput2"
#define kShortcutDescActionConnectViewerBToInput2 "Подключить просмотрщик В к входу 2"

#define kShortcutIDActionConnectViewerBToInput3 "connectViewerBInput3"
#define kShortcutDescActionConnectViewerBToInput3 "Подключить просмотрщик В к входу 3"

#define kShortcutIDActionConnectViewerBToInput4 "connectViewerBInput4"
#define kShortcutDescActionConnectViewerBToInput4 "Подключить просмотрщик В к входу 4"

#define kShortcutIDActionConnectViewerBToInput5 "connectViewerBInput5"
#define kShortcutDescActionConnectViewerBToInput5 "Подключить просмотрщик В к входу 5"

#define kShortcutIDActionConnectViewerBToInput6 "connectViewerBInput6"
#define kShortcutDescActionConnectViewerBToInput6 "Подключить просмотрщик В к входу 6"

#define kShortcutIDActionConnectViewerBToInput7 "connectViewerBInput7"
#define kShortcutDescActionConnectViewerBToInput7 "Подключить просмотрщик В к входу 7"

#define kShortcutIDActionConnectViewerBToInput8 "connectViewerBInput8"
#define kShortcutDescActionConnectViewerBToInput8 "Подключить просмотрщик В к входу 8"

#define kShortcutIDActionConnectViewerBToInput9 "connectViewerBInput9"
#define kShortcutDescActionConnectViewerBToInput9 "Подключить просмотрщик В к входу 9"

#define kShortcutIDActionConnectViewerBToInput10 "connectViewerBInput10"
#define kShortcutDescActionConnectViewerBToInput10 "Подключить просмотрщик В к входу 10"

#define kShortcutIDActionShowPaneFullScreen "showPaneFullScreen"
#define kShortcutDescActionShowPaneFullScreen "Показывать панель во весь экран"

#define kShortcutIDActionImportLayout "importLayout"
#define kShortcutDescActionImportLayout "Импортировать макет..."

#define kShortcutIDActionExportLayout "exportLayout"
#define kShortcutDescActionExportLayout "Экспортировать макет..."

#define kShortcutIDActionDefaultLayout "restoreDefaultLayout"
#define kShortcutDescActionDefaultLayout "Восстановить макет по умолчанию"

#define kShortcutIDActionNextTab "nextTab"
#define kShortcutDescActionNextTab "Следующая вкладка"

#define kShortcutIDActionPrevTab "prevTab"
#define kShortcutDescActionPrevTab "Предыдущая вкладка"

#define kShortcutIDActionCloseTab "closeTab"
#define kShortcutDescActionCloseTab "Закрыть вкладку"

/////////VIEWER SHORTCUTS
#define kShortcutIDActionLuminance "luminance"
#define kShortcutDescActionLuminance "Показать яркость"

#define kShortcutIDActionRed "channelR"
#define kShortcutDescActionRed "Показать красный канал"

#define kShortcutIDActionGreen "channelG"
#define kShortcutDescActionGreen "Показать зелёный канал"

#define kShortcutIDActionBlue "channelB"
#define kShortcutDescActionBlue "Показать синий канал"

#define kShortcutIDActionAlpha "channelA"
#define kShortcutDescActionAlpha "Показать альфа канал"

#define kShortcutIDActionLuminanceA "luminanceA"
#define kShortcutDescActionLuminanceA "Показать яркость только на входе A"

#define kShortcutIDActionMatteOverlay "matteOverlay"
#define kShortcutDescActionMatteOverlay "Наложение альфа-канала"

#define kShortcutIDActionRedA "channelRA"
#define kShortcutDescActionRedA "Показать красный канал только на входе A"

#define kShortcutIDActionGreenA "channelGA"
#define kShortcutDescActionGreenA "Показать зелёный канал только на входе A"

#define kShortcutIDActionBlueA "channelBA"
#define kShortcutDescActionBlueA "Показать синий канал только на входе A"

#define kShortcutIDActionAlphaA "channelAA"
#define kShortcutDescActionAlphaA "Показать альфа канал только на входе A"

#define kShortcutIDActionFitViewer "fitViewer"
#define kShortcutDescActionFitViewer "Подогнать изображение в просмотрщике"

#define kShortcutIDActionClipEnabled "clipEnabled"
#define kShortcutDescActionClipEnabled "Включить отсечение для окна проекта"

#define kShortcutIDActionFullFrameProc "fullFrameProc"
#define kShortcutDescActionFullFrameProc "Полная обработка кадров"

#define kShortcutIDActionRefresh "refresh"
#define kShortcutDescActionRefresh "Обновить изображение"

#define kShortcutIDActionRefreshWithStats "refreshWithStats"
#define kShortcutDescActionRefreshWithStats "Обновить изображение и показать статистику рендеринга"

#define kShortcutIDActionROIEnabled "userRoiEnabled"
#define kShortcutDescActionROIEnabled "Вкл пользователя RoI"

#define kShortcutIDActionNewROI "newRoi"
#define kShortcutDescActionNewROI "Новый пользователь RoI"

#define kShortcutIDActionPauseViewer "pauseUpdates"
#define kShortcutDescActionPauseViewer "Приостановить обновление"

#define kShortcutIDActionPauseViewerInputA "pauseUpdatesA"
#define kShortcutDescActionPauseViewerInputA "Приостановить обновление только на входе A"

#define kShortcutIDActionProxyEnabled "proxyEnabled"
#define kShortcutDescActionProxyEnabled "Включить прокси -рендеринг"

#define kShortcutIDActionProxyLevel2 "proxy2"
#define kShortcutDescActionProxyLevel2 "Уровень прокси 2"

#define kShortcutIDActionProxyLevel4 "proxy4"
#define kShortcutDescActionProxyLevel4 "Уровень прокси 4"

#define kShortcutIDActionProxyLevel8 "proxy8"
#define kShortcutDescActionProxyLevel8 "Уровень прокси 8"

#define kShortcutIDActionProxyLevel16 "proxy16"
#define kShortcutDescActionProxyLevel16 "Уровень прокси 16"

#define kShortcutIDActionProxyLevel32 "proxy32"
#define kShortcutDescActionProxyLevel32 "Уровень прокси 32"

#define kShortcutIDActionZoomLevel100 "zoom100"
#define kShortcutDescActionZoomLevel100 "Установить масштаб 100%"

#define kShortcutIDActionZoomIn "zoomIn"
#define kShortcutDescActionZoomIn "Увеличить"

#define kShortcutIDActionZoomOut "zoomOut"
#define kShortcutDescActionZoomOut "Уменишить"

#define kShortcutIDActionHideOverlays "hideOverlays"
#define kShortcutDescActionHideOverlays "Показать/скрыть наложения"

#define kShortcutIDActionHidePlayer "hidePlayer"
#define kShortcutDescActionHidePlayer "Показать/скрыть плеер"

#define kShortcutIDActionHideTimeline "hideTimeline"
#define kShortcutDescActionHideTimeline "Показать/скрыть шкалу времени"

#define kShortcutIDActionHideLeft "hideLeft"
#define kShortcutDescActionHideLeft "Показать/скрыть левую панель инструментов"

#define kShortcutIDActionHideRight "hideRight"
#define kShortcutDescActionHideRight "Показать/скрыть правую панель инструментов"

#define kShortcutIDActionHideTop "hideTop"
#define kShortcutDescActionHideTop "Показать/скрыть верхнюю панель инструментов"

#define kShortcutIDActionHideInfobar "hideInfo"
#define kShortcutDescActionHideInfobar "Показать/скрыть инфопанель"

#define kShortcutIDActionHideAll "hideAll"
#define kShortcutDescActionHideAll "Скрыть всё"

#define kShortcutIDActionShowAll "showAll"
#define kShortcutDescActionShowAll "Показать всё"

#define kShortcutIDMousePickColor "pick"
#define kShortcutDescMousePickColor "Выберать цвет"

#define kShortcutIDMousePickInputColor "pickInput"
#define kShortcutDescMousePickInputColor "Выберать цвет из входных данных просматриваемого узла"

#define kShortcutIDMouseRectanglePick "rectanglePick"
#define kShortcutDescMouseRectanglePick "Средство выбора цвета прямоугольника"

#define kShortcutIDToggleWipe "toggleWipe"
#define kShortcutDescToggleWipe "Переключить режим очистки"

#define kShortcutIDCenterWipe "centerWipe"
#define kShortcutDescCenterWipe "Центр с мышью"

#define kShortcutIDNextLayer "nextLayer"
#define kShortcutDescNextLayer "Следующий слой"

#define kShortcutIDPrevLayer "prevLayer"
#define kShortcutDescPrevLayer "Предыдущий слой"

#define kShortcutIDSwitchInputAAndB "switchAB"
#define kShortcutDescSwitchInputAAndB "Переключить входы A и B"

#define kShortcutIDPrevView "prevView"
#define kShortcutDescPrevView "Предыдущий вид"

#define kShortcutIDNextView "nextView"
#define kShortcutDescNextView "Следующий вид"
///////////PLAYER SHORTCUTS

#define kShortcutIDActionPlayerPrevious "prev"
#define kShortcutDescActionPlayerPrevious "Предыдущий кадр"

#define kShortcutIDActionPlayerNext "next"
#define kShortcutDescActionPlayerNext "Следующий кадр"

#define kShortcutIDActionPlayerBackward "backward"
#define kShortcutDescActionPlayerBackward "Воспроизведение назад"

#define kShortcutIDActionPlayerForward "forward"
#define kShortcutDescActionPlayerForward "Воспроизведение вперёд"

#define kShortcutIDActionPlayerStop "stop"
#define kShortcutDescActionPlayerStop "Стоп"

#define kShortcutIDActionPlayerPrevIncr "prevIncr"
#define kShortcutDescActionPlayerPrevIncr "Перейти к текущему кадру без приращения"

#define kShortcutIDActionPlayerNextIncr "nextIncr"
#define kShortcutDescActionPlayerNextIncr "Перейти к текущей раме плюс приращение"

#define kShortcutIDActionPlayerPrevKF "prevKF"
#define kShortcutDescActionPlayerPrevKF "Перейти к предыдущему ключевому кадру"

#define kShortcutIDActionPlayerNextKF "nextKF"
#define kShortcutDescActionPlayerNextKF "Переход к следующему ключевому кадру"

#define kShortcutIDActionPlayerFirst "first"
#define kShortcutDescActionPlayerFirst "Перейти к первому кадру"

#define kShortcutIDActionPlayerLast "last"
#define kShortcutDescActionPlayerLast "Перейти к последнему кадру"

#define kShortcutIDActionPlayerPlaybackIn "pbIn"
#define kShortcutDescActionPlayerPlaybackIn "Set Playback \"In\" Point"

#define kShortcutIDActionPlayerPlaybackOut "pbOut"
#define kShortcutDescActionPlayerPlaybackOut "Set Playback \"Out\" Point"


///////////NODEGRAPH SHORTCUTS
#ifndef NATRON_ENABLE_IO_META_NODES

#define kShortcutIDActionGraphCreateReader "createReader"
#define kShortcutDescActionGraphCreateReader "Слздать чтение"

#define kShortcutIDActionGraphCreateWriter "createWriter"
#define kShortcutDescActionGraphCreateWriter "Создать запись"

#endif // #ifdef NATRON_ENABLE_IO_META_NODES

#define kShortcutIDActionGraphRearrangeNodes "rearrange"
#define kShortcutDescActionGraphRearrangeNodes "Переставить узлы"

#define kShortcutIDActionGraphRemoveNodes "remove"
#define kShortcutDescActionGraphRemoveNodes "Удаление узлов"

#define kShortcutIDActionGraphShowExpressions "displayExp"
#define kShortcutDescActionGraphShowExpressions "Показывать ссылки на выражения"

#define kShortcutIDActionGraphNavigateUpstream "navigateUp"
#define kShortcutDescActionGraphNavigateUpstream "Переместиться по дереву вверх"

#define kShortcutIDActionGraphNavigateDownstream "navigateDown"
#define kShortcutDescActionGraphNavigateDownstram "Переместиться по дереву вниз"

#define kShortcutIDActionGraphSelectUp "selUp"
#define kShortcutDescActionGraphSelectUp "Выбрать дерево вверх"

#define kShortcutIDActionGraphSelectDown "selDown"
#define kShortcutDescActionGraphSelectDown "Выбрать дерево вниз"

#define kShortcutIDActionGraphSelectAll "selectAll"
#define kShortcutDescActionGraphSelectAll "Выбрать все узлы"

#define kShortcutIDActionGraphSelectAllVisible "selectAllVisible"
#define kShortcutDescActionGraphSelectAllVisible "Выберать все видимые узлы"

#define kShortcutIDActionGraphAutoHideInputs "autoHideInputs"
#define kShortcutDescActionGraphAutoHideInputs "Автоскрытие доп входных данных"

#define kShortcutIDActionGraphHideInputs "hideInputs"
#define kShortcutDescActionGraphHideInputs "Скрывать входные данные"

#define kShortcutIDActionGraphSwitchInputs "switchInputs"
#define kShortcutDescActionGraphSwitchInputs "Переключить входы 1 и 2"

#define kShortcutIDActionGraphCopy "copy"
#define kShortcutDescActionGraphCopy "Копировать узлы"

#define kShortcutIDActionGraphPaste "paste"
#define kShortcutDescActionGraphPaste "Вставить узлы"

#define kShortcutIDActionGraphClone "clone"
#define kShortcutDescActionGraphClone "Клонировать узлы"

#define kShortcutIDActionGraphDeclone "declone"
#define kShortcutDescActionGraphDeclone "Деклонировать узлы"

#define kShortcutIDActionGraphCut "cut"
#define kShortcutDescActionGraphCut "Вырезать узлы"

#define kShortcutIDActionGraphDuplicate "duplicate"
#define kShortcutDescActionGraphDuplicate "Дублировать узлы"

#define kShortcutIDActionGraphDisableNodes "disable"
#define kShortcutDescActionGraphDisableNodes "Отключить узлы"

#define kShortcutIDActionGraphToggleAutoPreview "toggleAutoPreview"
#define kShortcutDescActionGraphToggleAutoPreview "Переключение авто-предпросмотра"

#define kShortcutIDActionGraphToggleAutoTurbo "toggleAutoTurbo"
#define kShortcutDescActionGraphToggleAutoTurbo "Переключение авто турбо"

#define kShortcutIDActionGraphTogglePreview "togglePreview"
#define kShortcutDescActionGraphTogglePreview "Переключение предпросмотра изображений"

#define kShortcutIDActionGraphForcePreview "preview"
#define kShortcutDescActionGraphForcePreview "Обновить предпросмотр изображений"

#define kShortcutIDActionGraphShowCacheSize "cacheSize"
#define kShortcutDescActionGraphShowCacheSize "Отображение потребления кэш-памяти"

#define kShortcutIDActionGraphOpenNodePanel "openSettingsPanel"
#define kShortcutDescActionGraphOpenNodePanel "Открыть панель настроек узла"

#define kShortcutIDActionGraphFrameNodes "frameNodes"
#define kShortcutDescActionGraphFrameNodes "Центрировать по всем узлам"

#define kShortcutIDActionGraphFindNode "findNode"
#define kShortcutDescActionGraphFindNode "Искать"

#define kShortcutIDActionGraphCreateNode "createNode"
#define kShortcutDescActionGraphCreateNode "Создать узел"

#define kShortcutIDActionGraphRenameNode "renameNode"
#define kShortcutDescActionGraphRenameNode "Переименовать узел"

#define kShortcutIDActionGraphExtractNode "extractNode"
#define kShortcutDescActionGraphExtractNode "Извлечь узел"

#define kShortcutIDActionGraphMakeGroup "makeGroup"
#define kShortcutDescActionGraphMakeGroup "Группа из выборки"

#define kShortcutIDActionGraphExpandGroup "expandGroup"
#define kShortcutDescActionGraphExpandGroup "Расширить группу"

///////////CURVEEDITOR SHORTCUTS
#define kShortcutIDActionCurveEditorRemoveKeys "remove"
#define kShortcutDescActionCurveEditorRemoveKeys "Удалить ключевые кадры"

#define kShortcutIDActionCurveEditorConstant "constant"
#define kShortcutDescActionCurveEditorConstant "Постоянная интерполяция"

#define kShortcutIDActionCurveEditorLinear "linear"
#define kShortcutDescActionCurveEditorLinear "Линейная интерполяция"

#define kShortcutIDActionCurveEditorSmooth "smooth"
#define kShortcutDescActionCurveEditorSmooth "Плавная интерполяция"

#define kShortcutIDActionCurveEditorCatmullrom "catmullrom"
#define kShortcutDescActionCurveEditorCatmullrom "Интерполяция Catmull-Rom"

#define kShortcutIDActionCurveEditorCubic "cubic"
#define kShortcutDescActionCurveEditorCubic "Кубическая интерполяция"

#define kShortcutIDActionCurveEditorHorizontal "horiz"
#define kShortcutDescActionCurveEditorHorizontal "Горизонтальная интерполяция"

#define kShortcutIDActionCurveEditorBreak "break"
#define kShortcutDescActionCurveEditorBreak "Разрыв"

#define kShortcutIDActionCurveEditorSelectAll "selectAll"
#define kShortcutDescActionCurveEditorSelectAll "Выберать все ключевые кадры"

#define kShortcutIDActionCurveEditorCenterAll "frameAll"
#define kShortcutDescActionCurveEditorCenterAll "Обрамление всех кривых"

#define kShortcutIDActionCurveEditorCenter "center"
#define kShortcutDescActionCurveEditorCenter "Центр на кривой"

#define kShortcutIDActionCurveEditorCopy "copy"
#define kShortcutDescActionCurveEditorCopy "Копировать ключевые кадры"

#define kShortcutIDActionCurveEditorPaste "paste"
#define kShortcutDescActionCurveEditorPaste "Вставить ключевые кадры"

// Dope Sheet Editor shortcuts
#define kShortcutIDActionDopeSheetEditorDeleteKeys "deleteKeys"
#define kShortcutDescActionDopeSheetEditorDeleteKeys "Удалить выбранные ключевые кадры"

#define kShortcutIDActionDopeSheetEditorFrameSelection "frameonselection"
#define kShortcutDescActionDopeSheetEditorFrameSelection "Кадр на выборе"

#define kShortcutIDActionDopeSheetEditorSelectAllKeyframes "selectall"
#define kShortcutDescActionDopeSheetEditorSelectAllKeyframes "Выбрать всё"

#define kShortcutIDActionDopeSheetEditorRenameNode "renamenode"
#define kShortcutDescActionDopeSheetEditorRenameNode "Переименовать узел"

#define kShortcutIDActionDopeSheetEditorCopySelectedKeyframes "copyselectedkeyframes"
#define kShortcutDescActionDopeSheetEditorCopySelectedKeyframes "Копировать выбранные ключевые кадры"

#define kShortcutIDActionDopeSheetEditorPasteKeyframes "pastekeyframes"
#define kShortcutDescActionDopeSheetEditorPasteKeyframes "Вставить ключевые кадры"

#define kShortcutIDActionDopeSheetEditorPasteKeyframesAbsolute "pastekeyframesAbs"
#define kShortcutDescActionDopeSheetEditorPasteKeyframesAbsolute "Вставить ключевые кадры Absolute"

// Script editor shortcuts
#define kShortcutIDActionScriptEditorPrevScript "prevScript"
#define kShortcutDescActionScriptEditorPrevScript "Предыдущий сценарий"

#define kShortcutIDActionScriptEditorNextScript "nextScript"
#define kShortcutDescActionScriptEditorNextScript "Следующий сценарий"

#define kShortcutIDActionScriptEditorClearHistory "clearHistory"
#define kShortcutDescActionScriptEditorClearHistory "Очистить историю"

#define kShortcutIDActionScriptExecScript "execScript"
#define kShortcutDescActionScriptExecScript "Выполнить сценарий"

#define kShortcutIDActionScriptClearOutput "clearOutput"
#define kShortcutDescActionScriptClearOutput "Очистить окно вывода"

#define kShortcutIDActionScriptShowOutput "showHideOutput"
#define kShortcutDescActionScriptShowOutput "Показать/скрыть окно вывода"

NATRON_NAMESPACE_ENTER

inline
QKeySequence
makeKeySequence(const Qt::KeyboardModifiers & modifiers,
                Qt::Key key)
{
    int keys = 0;

    if ( modifiers.testFlag(Qt::ControlModifier) ) {
        keys |= Qt::CTRL;
    }
    if ( modifiers.testFlag(Qt::ShiftModifier) ) {
        keys |= Qt::SHIFT;
    }
    if ( modifiers.testFlag(Qt::AltModifier) ) {
        keys |= Qt::ALT;
    }
    if ( modifiers.testFlag(Qt::MetaModifier) ) {
        keys |= Qt::META;
    }
    if ( modifiers.testFlag(Qt::KeypadModifier) ) {
        keys |= Qt::KeypadModifier;
    }
    if (key != (Qt::Key)0) {
        keys |= key;
    }

    return QKeySequence(keys);
}

///This is tricky to do, what we do is we try to find the native strings of the modifiers
///in the sequence native's string. If we find them, we remove them. The last character
///is then the key symbol, we just have to call seq[0] to retrieve it.
inline void
extractKeySequence(const QKeySequence & seq,
                   Qt::KeyboardModifiers & modifiers,
                   Qt::Key & symbol)
{
    const QString nativeMETAStr = QKeySequence(Qt::META).toString(QKeySequence::NativeText);
    const QString nativeCTRLStr = QKeySequence(Qt::CTRL).toString(QKeySequence::NativeText);
    const QString nativeSHIFTStr = QKeySequence(Qt::SHIFT).toString(QKeySequence::NativeText);
    const QString nativeALTStr = QKeySequence(Qt::ALT).toString(QKeySequence::NativeText);
    const QString nativeKeypadStr = QKeySequence(Qt::KeypadModifier).toString(QKeySequence::NativeText);
    QString nativeSeqStr = seq.toString(QKeySequence::NativeText);

    if (nativeSeqStr.indexOf(nativeMETAStr) != -1) {
        modifiers |= Qt::MetaModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeMETAStr);
    }
    if (nativeSeqStr.indexOf(nativeCTRLStr) != -1) {
        modifiers |= Qt::ControlModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeCTRLStr);
    }
    if (nativeSeqStr.indexOf(nativeSHIFTStr) != -1) {
        modifiers |= Qt::ShiftModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeSHIFTStr);
    }
    if (nativeSeqStr.indexOf(nativeALTStr) != -1) {
        modifiers |= Qt::AltModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeALTStr);
    }
    if (!nativeKeypadStr.isEmpty() && nativeSeqStr.indexOf(nativeKeypadStr) != -1) {
        modifiers |= Qt::KeypadModifier;
        nativeSeqStr = nativeSeqStr.remove(nativeKeypadStr);
    }

    ///The nativeSeqStr now contains only the symbol
    QKeySequence newSeq(nativeSeqStr, QKeySequence::NativeText);
    if (newSeq.count() > 0) {
        symbol = (Qt::Key)newSeq[0];
    } else {
        symbol = (Qt::Key)0;
    }
}

class BoundAction
{
public:

    bool editable;
    QString grouping; //< the grouping of the action, such as CurveEditor/
    QString actionID; //< the unique ID within the grouping
    QString description; //< the description that will be in the shortcut editor

    //There might be multiple combinations
    std::list<Qt::KeyboardModifiers> modifiers; //< the keyboard modifiers that must be held down during the action
    std::list<Qt::KeyboardModifiers> defaultModifiers; //< the default keyboard modifiers
    Qt::KeyboardModifiers ignoreMask; ///Mask of modifiers to ignore for this shortcut

    BoundAction()
        : editable(true)
        , ignoreMask(Qt::NoModifier)
    {
    }

    virtual ~BoundAction()
    {
    }
};


class ActionWithShortcut
    : public QAction
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

private:
    QString _group;

protected:

    std::vector<std::pair<QString, QKeySequence> > _shortcuts;

public:

    ActionWithShortcut(const std::string & group,
                       const std::string & actionID,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    ActionWithShortcut(const std::string & group,
                       const std::list<std::string> & actionIDs,
                       const std::string & actionDescription,
                       QObject* parent,
                       bool setShortcutOnAction = true);


    const std::vector<std::pair<QString, QKeySequence> >& getShortcuts() const
    {
        return _shortcuts;
    }

    virtual ~ActionWithShortcut();
    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut);
};

/**
 * @brief Set the widget's tooltip and append in the tooltip the shortcut associated to the action.
 * This will be dynamically changed when the user edits the shortcuts from the editor.
 **/
#define setToolTipWithShortcut(group, actionID, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionID, tooltip, widget) ) )
#define setToolTipWithShortcut2(group, actionIDs, tooltip, widget) ( widget->addAction( new ToolTipActionShortcut(group, actionIDs, tooltip, widget) ) )

class ToolTipActionShortcut
    : public ActionWithShortcut
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

    QWidget* _widget;
    QString _originalToolTip;
    bool _tooltipSetInternally;

public:

    /**
     * @brief Set a dynamic shortcut in the tooltip. Reference it with %1 where you want to place the shortcut.
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::string & actionID,
                          const std::string & toolip,
                          QWidget* parent);

    /**
     * @brief Same as above except that the tooltip can contain multiple shortcuts.
     * In that case the tooltip should reference shortcuts by doing so %1, %2 etc... where
     * %1 references the first actionID, %2 the second ,etc...
     **/
    ToolTipActionShortcut(const std::string & group,
                          const std::list<std::string> & actionIDs,
                          const std::string & toolip,
                          QWidget* parent);

    virtual ~ToolTipActionShortcut()
    {
    }

    virtual void setShortcutWrapper(const QString& actionID, const QKeySequence& shortcut) OVERRIDE FINAL;

private:

    virtual bool eventFilter(QObject* watched, QEvent* event) OVERRIDE FINAL;


    void setToolTipFromOriginalToolTip();
};

class KeyBoundAction
    : public BoundAction
{
public:

    //There might be multiple shortcuts
    std::list<Qt::Key> currentShortcut; //< the actual shortcut for the keybind
    std::list<Qt::Key> defaultShortcut; //< the default shortcut proposed by the dev team
    std::list<ActionWithShortcut*> actions; //< list of actions using this shortcut

    KeyBoundAction()
        : BoundAction()
        , currentShortcut()
        , defaultShortcut()
    {
    }

    virtual ~KeyBoundAction()
    {
    }

    void updateActionsShortcut()
    {
        for (std::list<ActionWithShortcut*>::iterator it = actions.begin(); it != actions.end(); ++it) {
            if ( !modifiers.empty() ) {
                (*it)->setShortcutWrapper( actionID, makeKeySequence( modifiers.front(), currentShortcut.front() ) );
            }
        }
    }
};

class MouseAction
    : public BoundAction
{
public:

    Qt::MouseButton button; //< the button that must be held down for the action. This cannot be edited!

    MouseAction()
        : BoundAction()
        , button(Qt::NoButton)
    {
    }

    virtual ~MouseAction()
    {
    }
};


///All the shortcuts of a group matched against their
///internal id to find and match the action in the event handlers
typedef std::map<QString, BoundAction*> GroupShortcuts;

///All groups shortcuts mapped against the name of the group
typedef std::map<QString, GroupShortcuts> AppShortcuts;

NATRON_NAMESPACE_EXIT

#endif // ACTIONSHORTCUTS_H
