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

#include "Settings.h"

#include <cassert>
#include <limits>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QThreadPool>
#include <QtCore/QThread>
#include <QtCore/QTextStream>

#ifdef WINDOWS
#include <tchar.h>
#endif

#include "Global/StrUtils.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/LibraryBinary.h"
#include "Engine/MemoryInfo.h" // getSystemTotalRAM, isApplication32Bits, printAsRAM
#include "Engine/Node.h"
#include "Engine/OSGLContext.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/StandardPaths.h"
#include "Engine/Utils.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"

#include "Gui/GuiDefines.h"

#include <SequenceParsing.h> // for removePath

#ifdef WINDOWS
#include <ofxhPluginCache.h>
#endif

#define NATRON_DEFAULT_OCIO_CONFIG_NAME "blender"


#define NATRON_CUSTOM_OCIO_CONFIG_NAME "Custom config"

#define NATRON_DEFAULT_APPEARANCE_VERSION 1

#define NATRON_CUSTOM_HOST_NAME_ENTRY "Custom..."

NATRON_NAMESPACE_ENTER

Settings::Settings()
    : KnobHolder( AppInstancePtr() ) // < Settings are process wide and do not belong to a single AppInstance
    , _restoringSettings(false)
    , _ocioRestored(false)
    , _settingsExisted(false)
    , _defaultAppearanceOutdated(false)
{
}

static QStringList
getDefaultOcioConfigPaths()
{
    QString binaryPath = appPTR->getApplicationBinaryPath();
    StrUtils::ensureLastPathSeparator(binaryPath);

#ifdef __NATRON_LINUX__
    QStringList ret;
    ret.push_back( QString::fromUtf8("/usr/share/OpenColorIO-Configs") );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../share/OpenColorIO-Configs") ) );
    ret.push_back( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );

    return ret;
#elif defined(__NATRON_WIN32__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#elif defined(__NATRON_OSX__)

    return QStringList( QString( binaryPath + QString::fromUtf8("../Resources/OpenColorIO-Configs") ) );
#endif
}

void
Settings::initializeKnobs()
{
    initializeKnobsGeneral();
    initializeKnobsThreading();
    initializeKnobsRendering();
    initializeKnobsGPU();
    initializeKnobsProjectSetup();
    initializeKnobsDocumentation();
    initializeKnobsUserInterface();
    initializeKnobsColorManagement();
    initializeKnobsCaching();
    initializeKnobsViewers();
    initializeKnobsNodeGraph();
    initializeKnobsPlugins();
    initializeKnobsPython();
    initializeKnobsAppearance();
    initializeKnobsGuiColors();
    initializeKnobsCurveEditorColors();
    initializeKnobsDopeSheetColors();
    initializeKnobsNodeGraphColors();
    initializeKnobsScriptEditorColors();

    setDefaultValues();
}

void
Settings::initializeKnobsGeneral()
{
    _generalTab = AppManager::createKnob<KnobPage>( this, tr("Общие") );

    _natronSettingsExist = AppManager::createKnob<KnobBool>( this, tr("Существующие настройки") );
    _natronSettingsExist->setName("existingSettings");
    _natronSettingsExist->setSecretByDefault(true);
    _generalTab->addKnob(_natronSettingsExist);

    _saveSettings = AppManager::createKnob<KnobBool>( this, tr("Сохранять настройки при изменении") );
    _saveSettings->setName("saveSettings");
    _saveSettings->setDefaultValue(true);
    _saveSettings->setSecretByDefault(true);
    _generalTab->addKnob(_saveSettings);

    _checkForUpdates = AppManager::createKnob<KnobBool>( this, tr("Всегда проверяйте наличие обновлений при запуске") );
    _checkForUpdates->setName("checkForUpdates");
    _checkForUpdates->setHintToolTip( tr("Если флажок установлен, %1 будет проверять наличие новых обновлений при запуске приложения.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_checkForUpdates);

#ifdef NATRON_USE_BREAKPAD
    _enableCrashReports = AppManager::createKnob<KnobBool>( this, tr("Включить отчеты о сбоях") );
    _enableCrashReports->setName("enableCrashReports");
    _enableCrashReports->setHintToolTip( tr("Если этот флажок установлен, то в случае сбоя %1 появится всплывающее окно с вопросом, "
                                            "хотите ли вы загрузить дамп сбоя разработчикам или нет. "
                                            "Это может помочь им отследить ошибку.\n"
                                            "Если вам нужно отключить систему отчетов о сбоях, снимите этот флажок.\n"
                                            "Обратите внимание, что при использовании приложения в режиме командной строки, если отчеты о сбоях "
                                            "включено, они будут загружены автоматически.\n"
                                            "Для изменения этого параметра требуется перезапуск приложения.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableCrashReports->setAddNewLine(false);
    _generalTab->addKnob(_enableCrashReports);

    _testCrashReportButton = AppManager::createKnob<KnobButton>( this, tr("Отчеты о сбоях тестирования") );
    _testCrashReportButton->setName("testCrashReporting");
    _testCrashReportButton->setHintToolTip( tr("Эта кнопка предназначена только для разработчиков, чтобы проверить, правильно ли работает "
                                               "система отчетов о сбоях. Не используйте это.") );
    _generalTab->addKnob(_testCrashReportButton);
#endif

    _autoSaveDelay = AppManager::createKnob<KnobInt>( this, tr("Задержка триггера автосохранения") );
    _autoSaveDelay->setName("autoSaveDelay");
    _autoSaveDelay->disableSlider();
    _autoSaveDelay->setMinimum(0);
    _autoSaveDelay->setMaximum(60);
    _autoSaveDelay->setHintToolTip( tr("Количество секунд которое %1 должен подождать перед автосохранением. "
                                       " если рендеринг выполняется, %1 будет ждать, пока он не завершится, "
                                       " чтобы выполнить автоматическое сохранение.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveDelay);


    _autoSaveUnSavedProjects = AppManager::createKnob<KnobBool>( this, tr("Включить автосохранение для несохраненных проектов") );
    _autoSaveUnSavedProjects->setName("autoSaveUnSavedProjects");
    _autoSaveUnSavedProjects->setHintToolTip( tr("При активации %1 будет автоматически сохранять проекты и при запуске "
                                                 "будет выдаваться сообщение об обнаружении автосохранения этого несохраненного проекта. "
                                                 "Отключение этого параметра больше не будет сохранять несохраненный проект.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _generalTab->addKnob(_autoSaveUnSavedProjects);

    _saveVersions = AppManager::createKnob<KnobInt>( this, tr("Сохранение версий") );
    _saveVersions->setName("saveVersions");
    _saveVersions->disableSlider();
    _saveVersions->setMinimum(0);
    _saveVersions->setMaximum(32);
    _saveVersions->setHintToolTip( tr("Количество версий, созданных (для резервного копирования) при сохранении новых версий файла.\n"
                                      "Эта опция сохраняет сохраненные версии вашего файла в том же каталоге, добавляя "
                                      ".~1~, .~2~ и т. д., причем число увеличивается до количества указанных вами версий.\n"
                                      "Старым файлам будет присвоен больший номер. Например, с настройкой по умолчанию 2 "
                                      "у вас будет три версии вашего файла: *.ntp (last saved), *.ntp.~1~ (second "
                                      "last saved), *.~2~ (third last saved).") );
    _generalTab->addKnob(_saveVersions);

    _hostName = AppManager::createKnob<KnobChoice>( this, tr("Отображаются в плагинах как") );
    _hostName->setName("pluginHostName");
    _hostName->setHintToolTip( tr("%1 появится с именем выбранного приложения для плагинов OpenFX. "
                                  "Изменение его на имя другого приложения может помочь загрузить плагины, "
                                  "использование конкретными хостами OpenFX. "
                                  "Если хост здесь не указан, используйте запись \"Custom\" чтобы ввести собственное имя."
                                  "Для изменения этого параметра требуется перезапуск "
                                  "приложения и очистка кеша плагинов OpenFX из меню Кэш.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _knownHostNames.clear();
    std::vector<ChoiceOption> visibleHostEntries;
    assert(visibleHostEntries.size() == (int)eKnownHostNameNatron);
    visibleHostEntries.push_back(ChoiceOption(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME, NATRON_APPLICATION_NAME, ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNuke);
    visibleHostEntries.push_back(ChoiceOption("uk.co.thefoundry.nuke", "Nuke", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameFusion);
    visibleHostEntries.push_back(ChoiceOption("com.eyeonline.Fusion", "Fusion", "")); // or com.blackmagicdesign.Fusion
    assert(visibleHostEntries.size() == (int)eKnownHostNameCatalyst);
    visibleHostEntries.push_back(ChoiceOption("com.sony.Catalyst.Edit", "Sony Catalyst Edit", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameVegas);
    visibleHostEntries.push_back(ChoiceOption("com.sonycreativesoftware.vegas", "Sony Vegas", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameToxik);
    visibleHostEntries.push_back(ChoiceOption("Autodesk Toxik", "Toxik", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameScratch);
    visibleHostEntries.push_back(ChoiceOption("Assimilator", "Scratch", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameDustBuster);
    visibleHostEntries.push_back(ChoiceOption("Dustbuster", "DustBuster", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameResolve);
    visibleHostEntries.push_back(ChoiceOption("DaVinciResolve", "Da Vinci Resolve", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameResolveLite);
    visibleHostEntries.push_back(ChoiceOption("DaVinciResolveLite", "Da Vinci Resolve Lite", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameMistika);
    visibleHostEntries.push_back(ChoiceOption("Mistika", "SGO Mistika", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNamePablo);
    visibleHostEntries.push_back(ChoiceOption("com.quantel.genq", "Quantel Pablo Rio", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameMotionStudio);
    visibleHostEntries.push_back(ChoiceOption("com.idtvision.MotionStudio", "IDT Motion Studio", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameShake);
    visibleHostEntries.push_back(ChoiceOption("com.apple.shake", "Shake", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameBaselight);
    visibleHostEntries.push_back(ChoiceOption("Baselight", "Baselight", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameFrameCycler);
    visibleHostEntries.push_back(ChoiceOption("IRIDAS Framecycler", "FrameCycler", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNucoda);
    visibleHostEntries.push_back(ChoiceOption("Nucoda", "Nucoda Film Master", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameAvidDS);
    visibleHostEntries.push_back(ChoiceOption("DS OFX HOST", "Avid DS", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameDX);
    visibleHostEntries.push_back(ChoiceOption("com.chinadigitalvideo.dx", "China Digital Video DX", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameTitlerPro);
    visibleHostEntries.push_back(ChoiceOption("com.newblue.titlerpro", "NewBlueFX Titler Pro", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameNewBlueOFXBridge);
    visibleHostEntries.push_back(ChoiceOption("com.newblue.ofxbridge", "NewBlueFX OFX Bridge", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameRamen);
    visibleHostEntries.push_back(ChoiceOption("Ramen", "Ramen", ""));
    assert(visibleHostEntries.size() == (int)eKnownHostNameTuttleOfx);
    visibleHostEntries.push_back(ChoiceOption("TuttleOfx", "TuttleOFX", ""));

    _knownHostNames = visibleHostEntries;

    visibleHostEntries.push_back(ChoiceOption(NATRON_CUSTOM_HOST_NAME_ENTRY, "Custom host name", ""));

    _hostName->populateChoices(visibleHostEntries);
    _hostName->setAddNewLine(false);
    _generalTab->addKnob(_hostName);

    _customHostName = AppManager::createKnob<KnobString>( this, tr("Пользовательское имя хоста") );
    _customHostName->setName("customHostName");
    _customHostName->setHintToolTip( tr("Это имя хоста OpenFX, которое отображается в плагинах OpenFX. "
                                        "Изменение его на имя другого приложения может помочь загрузить некоторые плагины, "
                                        "которые ограничивают их использование определенными хостами OpenFX. "
                                        "Нужно оставить значение по умолчанию, если только конкретный плагин не отказывается загружаться."
                                        "Изменение этого параметра вступит в силу при следующем запуске приложения  "
                                        "и потребует очистки кеша плагинов OpenFX из меню Кэш "
                                        "Имя хоста по умолчанию: \n%1").arg( QString::fromUtf8(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME) ) );
    _customHostName->setSecretByDefault(true);
    _generalTab->addKnob(_customHostName);
} // Settings::initializeKnobsGeneral

void
Settings::initializeKnobsThreading()
{
    _threadingPage = AppManager::createKnob<KnobPage>( this, tr("Ванизывание") );

    _numberOfThreads = AppManager::createKnob<KnobInt>( this, tr("Количество потоков рендеринга (0=\"guess\")") );
    _numberOfThreads->setName("noRenderThreads");

    QString numberOfThreadsToolTip = tr("Управляет количеством потоков %1, которые следует использовать для рендеринга. \n"
                                        "-1: Полностью отключить многопоточность (полезно для отладки) \n"
                                        "0: Установите количество потоков по количеству ядер и доступной памяти (min(num_cores,memory/3.5Gb)). Идеальное количество потоков для этого оборудования составляет %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QThread::idealThreadCount() );
    _numberOfThreads->setHintToolTip( numberOfThreadsToolTip.toStdString() );
    _numberOfThreads->disableSlider();
    _numberOfThreads->setMinimum(-1);
    _numberOfThreads->setDisplayMinimum(-1);
    _threadingPage->addKnob(_numberOfThreads);

#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders = AppManager::createKnob<KnobInt>( this, tr("Количество параллельных рендеров (0=\"guess\")") );
    _numberOfParallelRenders->setHintToolTip( tr("Количество параллельных кадров, которые отобразятся средством рендеринга одновременно "
                                                 "Значение 0 указывает, что %1 должен автоматически определить "
                                                 "Лучшее количество параллельных рендерингов для запуска с учетом вашего процессора.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _numberOfParallelRenders->setName("nParallelRenders");
    _numberOfParallelRenders->setMinimum(0);
    _numberOfParallelRenders->disableSlider();
    _threadingPage->addKnob(_numberOfParallelRenders);
#endif

    _useThreadPool = AppManager::createKnob<KnobBool>( this, tr("Эффекты используют пул потоков") );
    _useThreadPool->setName("useThreadPool");
    _useThreadPool->setHintToolTip( tr("Флажок - все эффекты будут использовать глобальный пул потоков вместо запуска собственных потоков."
                                       "В результате рендеринг в системах может происходить быстрее с большим количеством ядер (>= 8). \n"
                                       "Не работает при использовании плагинов The Foundry's Furnace (и, возможно, некоторых других плагинов). При использовании этих плагинов обязательно снимите флажок с этой опции, иначе произойдет сбой %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_useThreadPool);

    _nThreadsPerEffect = AppManager::createKnob<KnobInt>( this, tr("Максимальное количество потоков на эффект (0=\"guess\")") );
    _nThreadsPerEffect->setName("nThreadsPerEffect");
    _nThreadsPerEffect->setHintToolTip( tr("Количество потоков, которые конкретный эффект может использовать максимум для своей обработки. "
                                           "Высокое значение позволит 1 эффекту порождать много потоков и может оказаться неэффективным, "
                                           "время, затраченное на запуск всех потоков, превысит время, затраченное на фактическую обработку.  "
                                           "По умолчанию (0) просмотрщик применяет эвристику, чтобы определить оптимальное количество "
                                           "потоков для достижения эффекта.") );

    _nThreadsPerEffect->setMinimum(0);
    _nThreadsPerEffect->disableSlider();
    _threadingPage->addKnob(_nThreadsPerEffect);

    _renderInSeparateProcess = AppManager::createKnob<KnobBool>( this, tr("Рендеринг в отдельном процессе") );
    _renderInSeparateProcess->setName("renderNewProcess");
    _renderInSeparateProcess->setHintToolTip( tr("Если это правда, %1 будет отображать кадры на диск в отдельном процессе, "
                                                 "поэтому в случае сбоя основного приложения рендеринг продолжится.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _threadingPage->addKnob(_renderInSeparateProcess);

    _queueRenders = AppManager::createKnob<KnobBool>( this, tr("Добавить новые рендеры в очередь") );
    _queueRenders->setHintToolTip( tr("Если этот флажок установлен, рендеринг будет поставлен в очередь на панели выполнения  "
                                      "выполнения всех остальных предыдущих задач.") );
    _queueRenders->setName("queueRenders");
    _threadingPage->addKnob(_queueRenders);
} // Settings::initializeKnobsThreading

void
Settings::initializeKnobsRendering()
{
    _renderingPage = AppManager::createKnob<KnobPage>( this, tr("Рендеринг") );

    _convertNaNValues = AppManager::createKnob<KnobBool>( this, tr("Преобразование значений NaN") );
    _convertNaNValues->setName("convertNaNs");
    _convertNaNValues->setHintToolTip( tr("При активации любой пиксель будет преобразован в 1, чтобы избежать сбоев на нижестоящих узлах."
                                          "Эти значения могут быть созданы ошибочными плагинами, когда они используют неправильную арифметику, например, деление на ноль."
                                          "Отключение этой опции сохранит NaN(s) в буферах: может привести к неопределенным ппоследствиям.") );
    _renderingPage->addKnob(_convertNaNValues);

    _pluginUseImageCopyForSource = AppManager::createKnob<KnobBool>( this, tr("Скопируйте входное изображение перед рендерингом любого плагина") );
    _pluginUseImageCopyForSource->setName("copyInputImage");
    _pluginUseImageCopyForSource->setHintToolTip( tr("Если флажок установлен, то перед рендерингом любого узла% 1 скопирует входное изображение "
                                                     "в локальное временное изображение. Это делается для того, чтобы обойтинекоторые плагины, "
                                                     "которые записывают данные в исходное изображение, изменяя таким образом выходные данные узла, расположенного выше по потоку в кэше. "
                                                     "Например, это известная ошибка старой версии REVisionFX REMap. "
                                                     "По умолчанию параметр не установлен, так как это потребует дополнительного выделения "
                                                     "изображения и его копирования перед рендерингом любого подключаемого модуля.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _renderingPage->addKnob(_pluginUseImageCopyForSource);

    _activateRGBSupport = AppManager::createKnob<KnobBool>( this, tr("Поддержка компонентов RGB") );
    _activateRGBSupport->setHintToolTip( tr("Если установлен флажок %1, то можно обрабатывать изображения только с использованием компонентов RGB "
                                            "(поддержка изображений с компонентами RGBA и Alpha всегда включена). "
                                            "Снятие флажка с этой опции может предотвратить сбой %1 плагинов, которые плохо поддерживают компоненты RGB. "
                                            "Изменение этой опции требует перезапуска приложения.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateRGBSupport->setName("rgbSupport");
    _renderingPage->addKnob(_activateRGBSupport);


    _activateTransformConcatenationSupport = AppManager::createKnob<KnobBool>( this, tr("Преобразует поддержку конкатенации") );
    _activateTransformConcatenationSupport->setHintToolTip( tr("Если этот флажок установлен, %1 может объединять эффекты "
                                                               "преобразования, когда они объединены в цепочку в дереве композиции.  "
                                                               "Это дает лучшие результаты и сокращает время рендеринга, поскольку изображение фильтруется только один раз, "
                                                               "а не столько раз, сколько требуется преобразований.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _activateTransformConcatenationSupport->setName("transformCatSupport");
    _renderingPage->addKnob(_activateTransformConcatenationSupport);
}

void
Settings::populateOpenGLRenderers(const std::list<OpenGLRendererInfo>& renderers)
{
    if ( renderers.empty() ) {
        _availableOpenGLRenderers->setSecret(true);
        _nOpenGLContexts->setSecret(true);
        _enableOpenGL->setSecret(true);
        return;
    }

    _nOpenGLContexts->setSecret(false);
    _enableOpenGL->setSecret(false);

    std::vector<ChoiceOption> entries( renderers.size() );
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        std::string option = it->vendorName + ' ' + it->rendererName + ' ' + it->glVersionString;
        entries[i] = ChoiceOption(option);
    }
    _availableOpenGLRenderers->populateChoices(entries);
    _availableOpenGLRenderers->setSecret(renderers.size() == 1);
}

bool
Settings::isOpenGLRenderingEnabled() const
{
    if (_enableOpenGL->getIsSecret()) {
        return false;
    }
    EnableOpenGLEnum enableOpenGL = (EnableOpenGLEnum)_enableOpenGL->getValue();
    return enableOpenGL == eEnableOpenGLEnabled || (enableOpenGL == eEnableOpenGLDisabledIfBackground && !appPTR->isBackground());
}

int
Settings::getMaxOpenGLContexts() const
{
    return _nOpenGLContexts->getValue();
}

GLRendererID
Settings::getActiveOpenGLRendererID() const
{
    if ( _availableOpenGLRenderers->getIsSecret() ) {
        // We were not able to detect multiple renderers, use default
        return GLRendererID();
    }
    int activeIndex = _availableOpenGLRenderers->getValue();
    const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
    if ( (activeIndex < 0) || ( activeIndex >= (int)renderers.size() ) ) {
        // Invalid index
        return GLRendererID();
    }
    int i = 0;
    for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
        if (i == activeIndex) {
            return it->rendererID;
        }
    }

    return GLRendererID();
}

void
Settings::initializeKnobsGPU()
{
    _gpuPage = AppManager::createKnob<KnobPage>( this, tr("GPU Рендеринг") );
    _openglRendererString = AppManager::createKnob<KnobString>( this, tr("Активный рендерер OpenGL") );
    _openglRendererString->setName("activeOpenGLRenderer");
    _openglRendererString->setHintToolTip( tr("Текущий активный рендерер OpenGL.") );
    _openglRendererString->setAsLabel();
    _gpuPage->addKnob(_openglRendererString);

    _availableOpenGLRenderers = AppManager::createKnob<KnobChoice>( this, tr("OpenGL-рендерер") );
    _availableOpenGLRenderers->setName("chooseOpenGLRenderer");
    _availableOpenGLRenderers->setHintToolTip( tr("Средство рендеринга, используемое для рендеринга OpenGL. Изменение средства визуализации OpenGL требует перезапуска приложения.") );
    _gpuPage->addKnob(_availableOpenGLRenderers);

    _nOpenGLContexts = AppManager::createKnob<KnobInt>( this, tr("Количество контекстов OpenGL") );
    _nOpenGLContexts->setName("maxOpenGLContexts");
    _nOpenGLContexts->setMinimum(1);
    _nOpenGLContexts->setDisplayMinimum(1);
    _nOpenGLContexts->setDisplayMaximum(8);
    _nOpenGLContexts->setMaximum(8);
    _nOpenGLContexts->setHintToolTip( tr("Количество контекстов OpenGL, созданных для выполнения рендеринга OpenGL. Каждый контекст OpenGL может быть прикреплен к потоку ЦП, что позволяет одновременно отображать больше кадров. Увеличение этого значения может повысить производительность графов со смешанными узлами ЦП/ГП, но может значительно снизить производительность, если слишком много контекстов OpenGL активны одновременно.
") );
    _gpuPage->addKnob(_nOpenGLContexts);


    _enableOpenGL = AppManager::createKnob<KnobChoice>( this, tr("OpenGL-рендеринг") );
    _enableOpenGL->setName("enableOpenGLRendering");
    {
        std::vector<ChoiceOption> entries;
        assert(entries.size() == (int)Settings::eEnableOpenGLEnabled);
        entries.push_back(ChoiceOption("enabled",
                                       tr("Включить").toStdString(),
                                       tr("Если плагин поддерживает рендеринг с помощью графического процессора, по возможности отдавайте предпочтение рендерингу с использованием графического процессора.").toStdString()));
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabled);
        entries.push_back(ChoiceOption("disabled",
                                       tr("Отключить").toStdString(),
                                       tr("Отключите рендеринг графического процессора для всех плагинов.").toStdString()));
        assert(entries.size() == (int)Settings::eEnableOpenGLDisabledIfBackground);
        entries.push_back(ChoiceOption("foreground",
                                       tr("Отключен, если Фоновый").toStdString(),
                                       tr("Отключите рендеринг с помощью ГП при рендеринге с помощью NatronRenderer, но не в режиме графического интерфейса.").toStdString()));
        _enableOpenGL->populateChoices(entries);
    }
    _enableOpenGL->setHintToolTip( tr("Выберите, активировать рендеринг OpenGL или нет. Если этот параметр отключен, даже если проект включает рендеринг с помощью графического процессора, он не будет активирован") );
    _gpuPage->addKnob(_enableOpenGL);
}

void
Settings::initializeKnobsProjectSetup()
{
    _projectsPage = AppManager::createKnob<KnobPage>( this, tr("Настройка проекта") );

    _firstReadSetProjectFormat = AppManager::createKnob<KnobBool>( this, tr("Первое чтение изображения установило формат проекта") );
    _firstReadSetProjectFormat->setName("autoProjectFormat");
    _firstReadSetProjectFormat->setHintToolTip( tr("Если этот флажок установлен, размер проекта устанавливается равным размеру первого изображения или видео, считываемого в проекте.") );
    _projectsPage->addKnob(_firstReadSetProjectFormat);


    _autoPreviewEnabledForNewProjects = AppManager::createKnob<KnobBool>( this, tr("Авто предпросмотр включен по умолчанию для новых проектов") );
    _autoPreviewEnabledForNewProjects->setName("enableAutoPreviewNewProjects");
    _autoPreviewEnabledForNewProjects->setHintToolTip( tr("Если отмечено, то при создании нового проекта включается "
                                                          "опция Автопросмотр.") );
    _projectsPage->addKnob(_autoPreviewEnabledForNewProjects);


    _fixPathsOnProjectPathChanged = AppManager::createKnob<KnobBool>( this, tr("Автоисправление относительных путей к файлам") );
    _fixPathsOnProjectPathChanged->setHintToolTip( tr("Если этот флажок установлен, то при изменении пути проекта (либо имени, либо значения, на которое указано), %1 проверяет все параметры пути к файлу в проекте и пытается их исправить.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _fixPathsOnProjectPathChanged->setName("autoFixRelativePaths");

    _projectsPage->addKnob(_fixPathsOnProjectPathChanged);

    _enableMappingFromDriveLettersToUNCShareNames = AppManager::createKnob<KnobBool>( this, tr("Используйте буквы дисков вместо имен серверов (только для Windows)") );
    _enableMappingFromDriveLettersToUNCShareNames->setHintToolTip( tr("Если флажок установлен, %1 не будет преобразовывать путь, начинающийся с буквы диска из диалогового окна файла, в имя общего сетевого ресурса. Вы можете использовать это, если, например, вы хотите поделиться одним и тем же проектом с несколькими пользователями на объектах с разными серверами, но когда у всех пользователей есть один и тот же диск, подключенный к серверу.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _enableMappingFromDriveLettersToUNCShareNames->setName("useDriveLetters");
#ifndef __NATRON_WIN32__
    _enableMappingFromDriveLettersToUNCShareNames->setAllDimensionsEnabled(false);
#endif
    _projectsPage->addKnob(_enableMappingFromDriveLettersToUNCShareNames);
}

void
Settings::initializeKnobsDocumentation()
{
    _documentationPage = AppManager::createKnob<KnobPage>( this, tr("Документация") );

#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource = AppManager::createKnob<KnobChoice>( this, tr("Источник документации") );
    _documentationSource->setName("documentationSource");
    _documentationSource->setHintToolTip( tr("Источник документации.") );
    _documentationSource->appendChoice(ChoiceOption("local",
                                                    tr("Локально").toStdString(),
                                                    tr("Используйте документацию, прилагаемую к ПО.").toStdString()));
    _documentationSource->appendChoice(ChoiceOption("online",
                                                    tr("Онлайн").toStdString(),
                                                    tr("Используйте онлайн-версию документации (требуется Интернет).").toStdString()));
    _documentationSource->appendChoice(ChoiceOption("none",
                                                    tr("Нет").toStdString(),
                                                    tr("Отключить документацию").toStdString()));
    _documentationPage->addKnob(_documentationSource);
#endif

    /// used to store temp port for local webserver
    _wwwServerPort = AppManager::createKnob<KnobInt>( this, tr("Документация локальный порт (0=auto)") );
    _wwwServerPort->setName("webserverPort");
    _wwwServerPort->setHintToolTip( tr("Порт, который будет прослушивать сервер документации. Значение 0 указывает, что документация должна автоматически найти порт самостоятельно.") );
    _documentationPage->addKnob(_wwwServerPort);
}

void
Settings::initializeKnobsUserInterface()
{
    _uiPage = AppManager::createKnob<KnobPage>( this, tr("Пользовательский интерфейс") );
    _uiPage->setName("userInterfacePage");

    _notifyOnFileChange = AppManager::createKnob<KnobBool>( this, tr("Предупреждать, когда файл изменяется извне") );
    _notifyOnFileChange->setName("warnOnExternalChange");
    _notifyOnFileChange->setHintToolTip( tr("Если флажок установлен, то если файл, считанный из параметра файла, изменится извне, в средстве просмотра будет отображено предупреждение. "
                                            "Отключение параметра приведет к приостановке работы системы уведомлений.
") );
    _uiPage->addKnob(_notifyOnFileChange);

#ifdef NATRON_ENABLE_IO_META_NODES
    _filedialogForWriters = AppManager::createKnob<KnobBool>( this, tr("Запрос в диалоговом окне файла при создании узла записи") );
    _filedialogForWriters->setName("writeUseDialog");
    _filedialogForWriters->setDefaultValue(true);
    _filedialogForWriters->setHintToolTip( tr("Если флажок установлен, открывается диалоговое окно файла при создании узла записи.") );
    _uiPage->addKnob(_filedialogForWriters);
#endif


    _renderOnEditingFinished = AppManager::createKnob<KnobBool>( this, tr("Обновлять программу просмотра только после завершения редактирования.") );
    _renderOnEditingFinished->setName("renderOnEditingFinished");
    _renderOnEditingFinished->setHintToolTip( tr("Если этот флажок установлен, средство просмотра запускает новую визуализацию только при отпускании мыши при редактировании параметров, "
                                                 " кривых или временной шкалы. Этот параметр не применяется к редактированию рото-сплайнов.") );
    _uiPage->addKnob(_renderOnEditingFinished);


    _linearPickers = AppManager::createKnob<KnobBool>( this, tr("Линейные палитры цветов") );
    _linearPickers->setName("linearPickers");
    _linearPickers->setHintToolTip( tr("При активации все цвета, выбранные из цветовых параметров, перед выборкой линеаризуются. "
                                       "В противном случае они находятся в том же цветовом пространстве, что и средство просмотра, "
                                       "из которого они были выбраны.") );
    _uiPage->addKnob(_linearPickers);

    _maxPanelsOpened = AppManager::createKnob<KnobInt>( this, tr("Максимальное количество открытых панелей настроек (0=\"unlimited\")") );
    _maxPanelsOpened->setName("maxPanels");
    _maxPanelsOpened->setHintToolTip( tr("Это свойство содержит максимальное количество панелей настроек, которые могут одновременно "
                                         "удерживаться на панели свойств. Специальное значение 0 указывает, что может "
                                         "быть открыто неограниченное количество панелей.") );
    _maxPanelsOpened->disableSlider();
    _maxPanelsOpened->setMinimum(0);
    _maxPanelsOpened->setMaximum(99);
    _uiPage->addKnob(_maxPanelsOpened);

    _useCursorPositionIncrements = AppManager::createKnob<KnobBool>( this, tr("Приращение значения в зависимости от положения курсора") );
    _useCursorPositionIncrements->setName("cursorPositionAwareFields");
    _useCursorPositionIncrements->setHintToolTip( tr("Если эта функция включена, увеличение полей значений "
                                                     "параметров с помощью колеса мыши или клавиш со стрелками "
                                                     "приведет к увеличению цифр справа от курсора. \n"
                                                     "Если этот параметр отключен, поля значений увеличиваются в соответствии с тем, "
                                                     "что плагин решил, что это должно быть. Вы можете изменить это приращение, "
                                                     "удерживая Shift (x10) или Control (/10) во время увеличения.") );
    _uiPage->addKnob(_useCursorPositionIncrements);

    _defaultLayoutFile = AppManager::createKnob<KnobFile>( this, tr("Файл макета по умолчанию") );
    _defaultLayoutFile->setName("defaultLayout");
    _defaultLayoutFile->setHintToolTip( tr("Если параметр установлен, %1 использует данный файл макета по умолчанию для новых проектов."
                                           "Вы можете экспортировать/импортировать макет в/из файла из меню Макет.  "
                                           "Если оно пустое, используется макет приложения по умолчанию.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _uiPage->addKnob(_defaultLayoutFile);

    _loadProjectsWorkspace = AppManager::createKnob<KnobBool>( this, tr("Загрузка рабочей области, встроенной в проекты") );
    _loadProjectsWorkspace->setName("loadProjectWorkspace");
    _loadProjectsWorkspace->setHintToolTip( tr("Если флажок установлен, при загрузке проекта также будет загружена рабочая область (макет окна), "
                                               "в противном случае будет использоваться ваш текущий макет.") );
    _uiPage->addKnob(_loadProjectsWorkspace);

#ifdef Q_OS_WIN
    _enableConsoleWindow = AppManager::createKnob<KnobBool>( this, tr("Включить окно консоли") );
    _enableConsoleWindow->setName("enableConsoleWindow");
    _enableConsoleWindow->setHintToolTip( tr("Если флажок установлен, показывать окно консоли в Windows.") );
    _uiPage->addKnob(_enableConsoleWindow);
#endif
} // Settings::initializeKnobsUserInterface

void
Settings::initializeKnobsColorManagement()
{
    _ocioTab = AppManager::createKnob<KnobPage>( this, tr("Управление цветом") );
    _ocioConfigKnob = AppManager::createKnob<KnobChoice>( this, tr("Конфигурация OpenColorIO") );
    _ocioConfigKnob->setName("ocioConfig");

    std::vector<ChoiceOption> configs;
    int defaultIndex = 0;
    QStringList defaultOcioConfigsPaths = getDefaultOcioConfigPaths();
    Q_FOREACH(const QString &defaultOcioConfigsDir, defaultOcioConfigsPaths) {
        QDir ocioConfigsDir(defaultOcioConfigsDir);

        if ( ocioConfigsDir.exists() ) {
            QStringList entries = ocioConfigsDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            for (int j = 0; j < entries.size(); ++j) {
                if ( entries[j] == QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ) {
                    defaultIndex = j;
                }
                configs.push_back(ChoiceOption( entries[j].toStdString() ));
            }

            break; //if we found 1 OpenColorIO-Configs directory, skip the next
        }
    }
    configs.push_back(ChoiceOption(NATRON_CUSTOM_OCIO_CONFIG_NAME));
    _ocioConfigKnob->populateChoices(configs);
    _ocioConfigKnob->setDefaultValue(defaultIndex, 0);
    _ocioConfigKnob->setHintToolTip( tr("Выберите конфигурацию OpenColorIO, которую вы хотите использовать глобально для всех операторов "
                                        "и плагинов, использующих OpenColorIO, установив переменную среды \"OCIO\"."
                                        "Его будут учитывать только узлы, созданные после изменения этого параметра, "
                                        "i и после его изменения лучше перезапустить приложение. Если выбран \"%1\" "
                                        "используется параметр \"Custom OpenColorIO config file\").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );

    _ocioTab->addKnob(_ocioConfigKnob);

    _customOcioConfigFile = AppManager::createKnob<KnobFile>( this, tr("Пользовательский файл конфигурации OpenColorIO") );
    _customOcioConfigFile->setName("ocioCustomConfigFile");

    if (_ocioConfigKnob->getNumEntries() == 1) {
        _customOcioConfigFile->setDefaultAllDimensionsEnabled(true);
    } else {
        _customOcioConfigFile->setDefaultAllDimensionsEnabled(false);
    }

    _customOcioConfigFile->setHintToolTip( tr("Файл конфигурации OpenColorIO (config.ocio), который будет использоваться, когда "
                                              "в качестве конфигурации OpenColorIO выбран \"%1\"").arg( QString::fromUtf8(NATRON_CUSTOM_OCIO_CONFIG_NAME) ) );
    _ocioTab->addKnob(_customOcioConfigFile);

    _warnOcioConfigKnobChanged = AppManager::createKnob<KnobBool>( this, tr("Предупреждать об изменении конфигурации OpenColorIO") );
    _warnOcioConfigKnobChanged->setName("warnOCIOChanged");
    _warnOcioConfigKnobChanged->setHintToolTip( tr("Показывать окно с предупреждением при изменении конфигурации OpenColorIO, чтобы помнить о необходимости перезагрузки.") );
    _ocioTab->addKnob(_warnOcioConfigKnobChanged);

    _ocioStartupCheck = AppManager::createKnob<KnobBool>( this, tr("Предупреждать при запуске, если конфигурация OpenColorIO не является конфигурацией по умолчанию") );
    _ocioStartupCheck->setName("startupCheckOCIO");
    _ocioTab->addKnob(_ocioStartupCheck);
} // Settings::initializeKnobsColorManagement

void
Settings::initializeKnobsAppearance()
{
    //////////////APPEARANCE TAB/////////////////
    _appearanceTab = AppManager::createKnob<KnobPage>( this, tr("Внешность") );

    _defaultAppearanceVersion = AppManager::createKnob<KnobInt>( this, tr("версия Внешности") );
    _defaultAppearanceVersion->setName("appearanceVersion");
    _defaultAppearanceVersion->setSecretByDefault(true);
    _appearanceTab->addKnob(_defaultAppearanceVersion);

    _systemFontChoice = AppManager::createKnob<KnobChoice>( this, tr("Шрифт") );
    _systemFontChoice->setHintToolTip( tr("Список всех шрифтов, доступных в системе") );
    _systemFontChoice->setName("systemFont");
    _systemFontChoice->setAddNewLine(false);
    _appearanceTab->addKnob(_systemFontChoice);

    _fontSize = AppManager::createKnob<KnobInt>( this, tr("Размер шрифта") );
    _fontSize->setName("fontSize");
    _appearanceTab->addKnob(_fontSize);

    _qssFile = AppManager::createKnob<KnobFile>( this, tr("Stylesheet file (.qss)") );
    _qssFile->setName("stylesheetFile");
    _qssFile->setHintToolTip( tr("При указании на допустимый qss-файл таблица стилей приложения будет установлена в соответствии с этим файлом вместо таблицы стилей по умолчанию. "
                                 " Можно адаптировать таблицу стилей по умолчанию, которую можно найти в вашем дистрибутиве %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _appearanceTab->addKnob(_qssFile);
} // Settings::initializeKnobsAppearance

void
Settings::initializeKnobsGuiColors()
{
    _guiColorsTab = AppManager::createKnob<KnobPage>( this, tr("Главное окно") );
    _appearanceTab->addKnob(_guiColorsTab);

    _useBWIcons = AppManager::createKnob<KnobBool>( this, tr("Используйте черно-белые значки кнопок инструментов") );
    _useBWIcons->setName("useBwIcons");
    _useBWIcons->setHintToolTip( tr("Если флажок установлен, значки инструментов на левой панели инструментов отображаются в оттенках серого "
                                    "Изменение эпараметра вступит в силу при следующем запуске приложения.") );
    _guiColorsTab->addKnob(_useBWIcons);


    _sunkenColor =  AppManager::createKnob<KnobColor>(this, tr("Погруженный"), 3);
    _sunkenColor->setName("sunken");
    _sunkenColor->setSimplified(true);
    _guiColorsTab->addKnob(_sunkenColor);

    _baseColor =  AppManager::createKnob<KnobColor>(this, tr("Базовый"), 3);
    _baseColor->setName("base");
    _baseColor->setSimplified(true);
    _guiColorsTab->addKnob(_baseColor);

    _raisedColor =  AppManager::createKnob<KnobColor>(this, tr("Поднятый"), 3);
    _raisedColor->setName("raised");
    _raisedColor->setSimplified(true);
    _guiColorsTab->addKnob(_raisedColor);

    _selectionColor =  AppManager::createKnob<KnobColor>(this, tr("Выбор"), 3);
    _selectionColor->setName("selection");
    _selectionColor->setSimplified(true);
    _guiColorsTab->addKnob(_selectionColor);

    _textColor =  AppManager::createKnob<KnobColor>(this, tr("Текст"), 3);
    _textColor->setName("text");
    _textColor->setSimplified(true);
    _guiColorsTab->addKnob(_textColor);

    _altTextColor =  AppManager::createKnob<KnobColor>(this, tr("Неизменённый текст"), 3);
    _altTextColor->setName("unmodifiedText");
    _altTextColor->setSimplified(true);
    _guiColorsTab->addKnob(_altTextColor);

    _timelinePlayheadColor =  AppManager::createKnob<KnobColor>(this, tr("Временная шкала воспроизведения"), 3);
    _timelinePlayheadColor->setName("timelinePlayhead");
    _timelinePlayheadColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelinePlayheadColor);


    _timelineBGColor =  AppManager::createKnob<KnobColor>(this, tr("Фон временной шкалы"), 3);
    _timelineBGColor->setName("timelineBG");
    _timelineBGColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBGColor);

    _timelineBoundsColor =  AppManager::createKnob<KnobColor>(this, tr("Границы временной шкалы"), 3);
    _timelineBoundsColor->setName("timelineBound");
    _timelineBoundsColor->setSimplified(true);
    _guiColorsTab->addKnob(_timelineBoundsColor);

    _cachedFrameColor =  AppManager::createKnob<KnobColor>(this, tr("Кэшированный кадр"), 3);
    _cachedFrameColor->setName("cachedFrame");
    _cachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_cachedFrameColor);

    _diskCachedFrameColor =  AppManager::createKnob<KnobColor>(this, tr("Кэшированный на диске кадр"), 3);
    _diskCachedFrameColor->setName("diskCachedFrame");
    _diskCachedFrameColor->setSimplified(true);
    _guiColorsTab->addKnob(_diskCachedFrameColor);

    _interpolatedColor =  AppManager::createKnob<KnobColor>(this, tr("Интерполированное значение"), 3);
    _interpolatedColor->setName("interpValue");
    _interpolatedColor->setSimplified(true);
    _guiColorsTab->addKnob(_interpolatedColor);

    _keyframeColor =  AppManager::createKnob<KnobColor>(this, tr("Ключевой кадр"), 3);
    _keyframeColor->setName("keyframe");
    _keyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_keyframeColor);

    _trackerKeyframeColor =  AppManager::createKnob<KnobColor>(this, tr("Отслеживание ключевых кадров пользователя"), 3);
    _trackerKeyframeColor->setName("trackUserKeyframe");
    _trackerKeyframeColor->setSimplified(true);
    _guiColorsTab->addKnob(_trackerKeyframeColor);

    _exprColor =  AppManager::createKnob<KnobColor>(this, tr("Выражение"), 3);
    _exprColor->setName("exprColor");
    _exprColor->setSimplified(true);
    _guiColorsTab->addKnob(_exprColor);

    _sliderColor =  AppManager::createKnob<KnobColor>(this, tr("Ползунок"), 3);
    _sliderColor->setName("slider");
    _sliderColor->setSimplified(true);
    _guiColorsTab->addKnob(_sliderColor);
} // Settings::initializeKnobsGuiColors

void
Settings::initializeKnobsCurveEditorColors()
{
    _curveEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Редактор кривых") );
    _appearanceTab->addKnob(_curveEditorColorsTab);

    _curveEditorBGColor =  AppManager::createKnob<KnobColor>(this, tr("Цвет фона"), 3);
    _curveEditorBGColor->setName("curveEditorBG");
    _curveEditorBGColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_curveEditorBGColor);

    _gridColor =  AppManager::createKnob<KnobColor>(this, tr("Цвет сетки"), 3);
    _gridColor->setName("curveditorGrid");
    _gridColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_gridColor);

    _curveEditorScaleColor =  AppManager::createKnob<KnobColor>(this, tr("Цвет шкалы"), 3);
    _curveEditorScaleColor->setName("curveeditorScale");
    _curveEditorScaleColor->setSimplified(true);
    _curveEditorColorsTab->addKnob(_curveEditorScaleColor);
}

void
Settings::initializeKnobsDopeSheetColors()
{
    _dopeSheetEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Шкала времени") );
    _appearanceTab->addKnob(_dopeSheetEditorColorsTab);

    _dopeSheetEditorBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Цвет фона листа"), 3);
    _dopeSheetEditorBackgroundColor->setName("dopesheetBackground");
    _dopeSheetEditorBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorBackgroundColor);

    _dopeSheetEditorRootSectionBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Цвет фона корневого раздела"), 4);
    _dopeSheetEditorRootSectionBackgroundColor->setName("dopesheetRootSectionBackground");
    _dopeSheetEditorRootSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorRootSectionBackgroundColor);

    _dopeSheetEditorKnobSectionBackgroundColor = AppManager::createKnob<KnobColor>(this, tr("Цвет фона раздела ручки"), 4);
    _dopeSheetEditorKnobSectionBackgroundColor->setName("dopesheetKnobSectionBackground");
    _dopeSheetEditorKnobSectionBackgroundColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorKnobSectionBackgroundColor);

    _dopeSheetEditorScaleColor = AppManager::createKnob<KnobColor>(this, tr("Цвет шкалы листа"), 3);
    _dopeSheetEditorScaleColor->setName("dopesheetScale");
    _dopeSheetEditorScaleColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorScaleColor);

    _dopeSheetEditorGridColor = AppManager::createKnob<KnobColor>(this, tr("Цвет сетки листа"), 3);
    _dopeSheetEditorGridColor->setName("dopesheetGrid");
    _dopeSheetEditorGridColor->setSimplified(true);
    _dopeSheetEditorColorsTab->addKnob(_dopeSheetEditorGridColor);
}

void
Settings::initializeKnobsNodeGraphColors()
{
    _nodegraphColorsTab = AppManager::createKnob<KnobPage>( this, tr("Схема Узлов") );
    _appearanceTab->addKnob(_nodegraphColorsTab);

    _usePluginIconsInNodeGraph = AppManager::createKnob<KnobBool>( this, tr("Отображение значка плагина на Схеме узлов") );
    _usePluginIconsInNodeGraph->setName("usePluginIcons");
    _usePluginIconsInNodeGraph->setHintToolTip( tr("Флажок установлен-каждый узел, имеющий значок плагина, будет отображать его в Схеме узлов. "
                                                   "Опция не повлияет на уже существующие узлы, если не будет произведен перезапуск Natron.") );
    _usePluginIconsInNodeGraph->setAddNewLine(false);
    _nodegraphColorsTab->addKnob(_usePluginIconsInNodeGraph);

    _useAntiAliasing = AppManager::createKnob<KnobBool>( this, tr("Сглаживание") );
    _useAntiAliasing->setName("antiAliasing");
    _useAntiAliasing->setHintToolTip( tr("Если флажок установлен, Схема узлов будет рисоваться со сглаживанием. Снятие флажка может повысить производительность. "
                                         " Чтобы изменить это, потребуется перезапустить Natron.") );
    _nodegraphColorsTab->addKnob(_useAntiAliasing);


    _defaultNodeColor = AppManager::createKnob<KnobColor>(this, tr("Цвет узла по умолчанию"), 3);
    _defaultNodeColor->setName("defaultNodeColor");
    _defaultNodeColor->setSimplified(true);
    _defaultNodeColor->setHintToolTip( tr("Цвет по умолчанию, используемый для вновь создаваемых узлов.") );

    _nodegraphColorsTab->addKnob(_defaultNodeColor);


    _defaultBackdropColor =  AppManager::createKnob<KnobColor>(this, tr("Цвет фона по умолчанию"), 3);
    _defaultBackdropColor->setName("backdropColor");
    _defaultBackdropColor->setSimplified(true);
    _defaultBackdropColor->setHintToolTip( tr("Цвет по умолчанию, используемый для вновь созданных узлов фона.") );
    _nodegraphColorsTab->addKnob(_defaultBackdropColor);

    _defaultReaderColor =  AppManager::createKnob<KnobColor>(this, tr(PLUGIN_GROUP_IMAGE_READERS), 3);
    _defaultReaderColor->setName("readerColor");
    _defaultReaderColor->setSimplified(true);
    _defaultReaderColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлом Чтения.") );
    _nodegraphColorsTab->addKnob(_defaultReaderColor);

    _defaultWriterColor =  AppManager::createKnob<KnobColor>(this, tr(PLUGIN_GROUP_IMAGE_WRITERS), 3);
    _defaultWriterColor->setName("writerColor");
    _defaultWriterColor->setSimplified(true);
    _defaultWriterColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлом Запись.") );
    _nodegraphColorsTab->addKnob(_defaultWriterColor);

    _defaultGeneratorColor =  AppManager::createKnob<KnobColor>(this, tr("Генераторы"), 3);
    _defaultGeneratorColor->setName("generatorColor");
    _defaultGeneratorColor->setSimplified(true);
    _defaultGeneratorColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов Генератора.") );
    _nodegraphColorsTab->addKnob(_defaultGeneratorColor);

    _defaultColorGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Цветовая группа"), 3);
    _defaultColorGroupColor->setName("colorNodesColor");
    _defaultColorGroupColor->setSimplified(true);
    _defaultColorGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов цвета.") );
    _nodegraphColorsTab->addKnob(_defaultColorGroupColor);

    _defaultFilterGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Фильтров"), 3);
    _defaultFilterGroupColor->setName("filterNodesColor");
    _defaultFilterGroupColor->setSimplified(true);
    _defaultFilterGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов фильтра.") );
    _nodegraphColorsTab->addKnob(_defaultFilterGroupColor);

    _defaultTransformGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Tрансформации"), 3);
    _defaultTransformGroupColor->setName("transformNodesColor");
    _defaultTransformGroupColor->setSimplified(true);
    _defaultTransformGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов Трансформации.") );
    _nodegraphColorsTab->addKnob(_defaultTransformGroupColor);

    _defaultTimeGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Времени"), 3);
    _defaultTimeGroupColor->setName("timeNodesColor");
    _defaultTimeGroupColor->setSimplified(true);
    _defaultTimeGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных временных узлов.") );
    _nodegraphColorsTab->addKnob(_defaultTimeGroupColor);

    _defaultDrawGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Рисования"), 3);
    _defaultDrawGroupColor->setName("drawNodesColor");
    _defaultDrawGroupColor->setSimplified(true);
    _defaultDrawGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов рисования.") );
    _nodegraphColorsTab->addKnob(_defaultDrawGroupColor);

    _defaultKeyerGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Ключей"), 3);
    _defaultKeyerGroupColor->setName("keyerNodesColor");
    _defaultKeyerGroupColor->setSimplified(true);
    _defaultKeyerGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных ключевых узлов.") );
    _nodegraphColorsTab->addKnob(_defaultKeyerGroupColor);

    _defaultChannelGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Каналов"), 3);
    _defaultChannelGroupColor->setName("channelNodesColor");
    _defaultChannelGroupColor->setSimplified(true);
    _defaultChannelGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов канала.") );
    _nodegraphColorsTab->addKnob(_defaultChannelGroupColor);

    _defaultMergeGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Слияния"), 3);
    _defaultMergeGroupColor->setName("defaultMergeColor");
    _defaultMergeGroupColor->setSimplified(true);
    _defaultMergeGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов слияния.") );
    _nodegraphColorsTab->addKnob(_defaultMergeGroupColor);

    _defaultViewsGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Просмотра"), 3);
    _defaultViewsGroupColor->setName("defaultViewsColor");
    _defaultViewsGroupColor->setSimplified(true);
    _defaultViewsGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов представлений.") );
    _nodegraphColorsTab->addKnob(_defaultViewsGroupColor);

    _defaultDeepGroupColor =  AppManager::createKnob<KnobColor>(this, tr("Группа Глубины"), 3);
    _defaultDeepGroupColor->setName("defaultDeepColor");
    _defaultDeepGroupColor->setSimplified(true);
    _defaultDeepGroupColor->setHintToolTip( tr("Цвет, используемый для вновь созданных узлов глубины.") );
    _nodegraphColorsTab->addKnob(_defaultDeepGroupColor);
} // Settings::initializeKnobsNodeGraphColors

void
Settings::initializeKnobsScriptEditorColors()
{
    _scriptEditorColorsTab = AppManager::createKnob<KnobPage>( this, tr("Редактор скриптов") );
    _scriptEditorColorsTab->setParentKnob(_appearanceTab);

    _scriptEditorFontChoice = AppManager::createKnob<KnobChoice>( this, tr("Шрифт") );
    _scriptEditorFontChoice->setHintToolTip( tr("Список всех шрифтов, доступных в вашей системе") );
    _scriptEditorFontChoice->setName("scriptEditorFont");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontChoice);

    _scriptEditorFontSize = AppManager::createKnob<KnobInt>( this, tr("Размер шрифта") );
    _scriptEditorFontSize->setHintToolTip( tr("Размер шрифта") );
    _scriptEditorFontSize->setName("scriptEditorFontSize");
    _scriptEditorColorsTab->addKnob(_scriptEditorFontSize);

    _curLineColor = AppManager::createKnob<KnobColor>(this, tr("Текущий цвет линии"), 3);
    _curLineColor->setName("currentLineColor");
    _curLineColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_curLineColor);

    _keywordColor = AppManager::createKnob<KnobColor>(this, tr("Цвет ключевого слова"), 3);
    _keywordColor->setName("keywordColor");
    _keywordColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_keywordColor);

    _operatorColor = AppManager::createKnob<KnobColor>(this, tr("Цвет оператора"), 3);
    _operatorColor->setName("operatorColor");
    _operatorColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_operatorColor);

    _braceColor = AppManager::createKnob<KnobColor>(this, tr("Цвет скобки"), 3);
    _braceColor->setName("braceColor");
    _braceColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_braceColor);

    _defClassColor = AppManager::createKnob<KnobColor>(this, tr("Цвет определения класса"), 3);
    _defClassColor->setName("classDefColor");
    _defClassColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_defClassColor);

    _stringsColor = AppManager::createKnob<KnobColor>(this, tr("Цвет струн"), 3);
    _stringsColor->setName("stringsColor");
    _stringsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_stringsColor);

    _commentsColor = AppManager::createKnob<KnobColor>(this, tr("Цвет комментариев"), 3);
    _commentsColor->setName("commentsColor");
    _commentsColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_commentsColor);

    _selfColor = AppManager::createKnob<KnobColor>(this, tr("Свой цвет"), 3);
    _selfColor->setName("selfColor");
    _selfColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_selfColor);

    _numbersColor = AppManager::createKnob<KnobColor>(this, tr("Цвет цифр"), 3);
    _numbersColor->setName("numbersColor");
    _numbersColor->setSimplified(true);
    _scriptEditorColorsTab->addKnob(_numbersColor);
} // Settings::initializeKnobsScriptEditorColors

void
Settings::initializeKnobsViewers()
{
    _viewersTab = AppManager::createKnob<KnobPage>( this, tr("Просмотрщик") );

    _texturesMode = AppManager::createKnob<KnobChoice>( this, tr("Разрядность просмотрщика текстур") );
    _texturesMode->setName("texturesBitDepth");
    std::vector<ChoiceOption> textureModes;
    textureModes.push_back(ChoiceOption("8u",
                                        tr("8-бит").toStdString(),
                                        tr("Постобработка, выполняемая средством просмотра (например, преобразование цветового пространства) "
                                           "выполняется процессором. Размер кэшированных текстур становится меньше.").toStdString() ));

    //textureModes.push_back("16bits half-float");
    //helpStringsTextureModes.push_back("Not available yet. Similar to 32bits fp.");
    textureModes.push_back(ChoiceOption("32f",
                                        tr("32-бит плавающая запятая").toStdString(),
                                        tr("Постобработка, выполняемая средством просмотра (например, преобразование цветового пространства)"
                                           "выполняется ГП с использованием GLSL. Размер кэшированных текстур становится больше.").toStdString()));
    _texturesMode->populateChoices(textureModes);


    _texturesMode->setHintToolTip( tr("Разрядность текстур средства просмотра, используемых для рендеринга. "
                                      "Наведите курсор мыши на каждый вариант, чтобы увидеть подробное описание.") );
    _viewersTab->addKnob(_texturesMode);

    _powerOf2Tiling = AppManager::createKnob<KnobInt>( this, tr("Размер плитки просмотрщика равен 2 в степени...") );
    _powerOf2Tiling->setName("viewerTiling");
    _powerOf2Tiling->setHintToolTip( tr("Размер плиток средства просмотра составляет 2^n на 2^n (т. е. 256 на 256 пикселей для n=8).  "
                                        "Высокое значение означает, что средство просмотра отображает большие плитки, поэтому "
                                        "рендеринг выполняется реже, но на более крупных площадях.") );
    _powerOf2Tiling->disableSlider();
    _powerOf2Tiling->setMinimum(4);
    _powerOf2Tiling->setDisplayMinimum(4);
    _powerOf2Tiling->setMaximum(9);
    _powerOf2Tiling->setDisplayMaximum(9);

    _viewersTab->addKnob(_powerOf2Tiling);

    _checkerboardTileSize = AppManager::createKnob<KnobInt>( this, tr("Размер плитки шахматной доски (в пикселях)") );
    _checkerboardTileSize->setName("checkerboardTileSize");
    _checkerboardTileSize->setMinimum(1);
    _checkerboardTileSize->setHintToolTip( tr("Размер (в пикселях экрана) одного тайла шахматной доски.") );
    _viewersTab->addKnob(_checkerboardTileSize);

    _checkerboardColor1 = AppManager::createKnob<KnobColor>(this, tr("Цвет шахматной доски 1"), 4);
    _checkerboardColor1->setName("checkerboardColor1");
    _checkerboardColor1->setHintToolTip( tr("Первый цвет, используемый шахматной доской.") );
    _viewersTab->addKnob(_checkerboardColor1);

    _checkerboardColor2 = AppManager::createKnob<KnobColor>(this, tr("Цвет шахматной доски 2"), 4);
    _checkerboardColor2->setName("checkerboardColor2");
    _checkerboardColor2->setHintToolTip( tr("Второй цвет, используемый шахматной доской.") );
    _viewersTab->addKnob(_checkerboardColor2);

    _autoWipe = AppManager::createKnob<KnobBool>( this, tr("Автоматически включать очистку") );
    _autoWipe->setName("autoWipeForViewer");
    _autoWipe->setHintToolTip( tr("Если флажок установлен, инструмент вытеснения средства просмотра будет автоматически включаться, когда "
                                  "мышь наводит курсор на средство просмотра и меняет ввод просмотрщика." ) );
    _viewersTab->addKnob(_autoWipe);


    _autoProxyWhenScrubbingTimeline = AppManager::createKnob<KnobBool>( this, tr("Автоматически включать прокси при очистке временной шкалы") );
    _autoProxyWhenScrubbingTimeline->setName("autoProxyScrubbing");
    _autoProxyWhenScrubbingTimeline->setHintToolTip( tr("Если флажок установлен, режим прокси будет как минимум на уровне, "
                                                        "указанном параметром auto-proxy.") );
    _autoProxyWhenScrubbingTimeline->setAddNewLine(false);
    _viewersTab->addKnob(_autoProxyWhenScrubbingTimeline);


    _autoProxyLevel = AppManager::createKnob<KnobChoice>( this, tr("Уровень авто-прокси") );
    _autoProxyLevel->setName("autoProxyLevel");
    std::vector<ChoiceOption> autoProxyChoices;
    autoProxyChoices.push_back(ChoiceOption("2", "",""));
    autoProxyChoices.push_back(ChoiceOption("4", "",""));
    autoProxyChoices.push_back(ChoiceOption("8", "",""));
    autoProxyChoices.push_back(ChoiceOption("16", "",""));
    autoProxyChoices.push_back(ChoiceOption("32", "",""));
    _autoProxyLevel->populateChoices(autoProxyChoices);

    _viewersTab->addKnob(_autoProxyLevel);

    _maximumNodeViewerUIOpened = AppManager::createKnob<KnobInt>( this, tr("Макс. открытый интерфейс просмотра узлов") );
    _maximumNodeViewerUIOpened->setName("maxNodeUiOpened");
    _maximumNodeViewerUIOpened->setMinimum(1);
    _maximumNodeViewerUIOpened->disableSlider();
    _maximumNodeViewerUIOpened->setHintToolTip( tr("Управляет максимальным количеством узлов, интерфейс которых может одновременно отображаться в окне просмотра") );
    _viewersTab->addKnob(_maximumNodeViewerUIOpened);

    _viewerNumberKeys = AppManager::createKnob<KnobBool>( this, tr("Используйте цифровые клавиши для просмотра") );
    _viewerNumberKeys->setName("viewerNumberKeys");
    _viewerNumberKeys->setHintToolTip( tr("Когда эта функция включена, ряд цифровых клавиш на клавиатуре используется для переключения ввода "
                                    "(<клавиша> подключает ввод к боковой панели, <клавиша shift> подключает ввод к боковой панели сбоку), "
                                    "даже если соответствующий символ в текущей раскладке клавиатуры не является цифрой."
                                    "character in the current keyboard layout is not a number.\n"
                                    "Возможно, для этого потребуется может быть отключен при использовании удаленного подключения дисплея "
                                    "к Linux из другой операционной системы.") );
    _viewersTab->addKnob(_viewerNumberKeys);

    _viewerOverlaysPath = AppManager::createKnob<KnobBool>( this, tr("Отображать наложения только для пути Просмотрщика") );
    _viewerOverlaysPath->setName("viewerOverlaysPath");
    _viewerOverlaysPath->setHintToolTip( tr("Если параметр отключен, отображаются наложения для всех не свернутых открытых панелей свойств. "
                                            "Если этот параметр включен, наложения отображаются только  "
                                            "для пути рендеринга для текущих входных данных программы просмотра.") );
    _viewersTab->addKnob(_viewerOverlaysPath);
} // Settings::initializeKnobsViewers

void
Settings::initializeKnobsNodeGraph()
{
    /////////// Nodegraph tab
    _nodegraphTab = AppManager::createKnob<KnobPage>( this, tr("Схема Узлов") );

    _autoScroll = AppManager::createKnob<KnobBool>( this, tr("Автопрокрутка") );
    _autoScroll->setName("autoScroll");
    _autoScroll->setHintToolTip( tr("Если флажок установлен, Схема Узлов будет автоматически прокручиваться, если вы переместите узел за пределы текущего представления графика.") );
    _nodegraphTab->addKnob(_autoScroll);

    _autoTurbo = AppManager::createKnob<KnobBool>( this, tr("Авто-турбо") );
    _autoTurbo->setName("autoTurbo");
    _autoTurbo->setHintToolTip( tr("Если флажок установлен, турбо-режим будет автоматически включаться при запуске "
                                   "воспроизведения и отключаться по его завершении.") );
    _nodegraphTab->addKnob(_autoTurbo);

    _snapNodesToConnections = AppManager::createKnob<KnobBool>( this, tr("Привязка к узлу") );
    _snapNodesToConnections->setName("enableSnapToNode");
    _snapNodesToConnections->setHintToolTip( tr("При перемещении узлов на графе узлов привязывайтесь к позициям, в которых "
                                                "они совпадают с входными и выходными узлами.") );
    _nodegraphTab->addKnob(_snapNodesToConnections);


    _maxUndoRedoNodeGraph = AppManager::createKnob<KnobInt>( this, tr("Максимальное количество операций отмены/повтора для Схема узла") );
    _maxUndoRedoNodeGraph->setName("maxUndoRedo");
    _maxUndoRedoNodeGraph->disableSlider();
    _maxUndoRedoNodeGraph->setMinimum(0);
    _maxUndoRedoNodeGraph->setHintToolTip( tr("Установите максимальное количество событий, связанных с графом узлов, которые запоминает %1. "
                                              "При превышении этого ограничения более старые события "
                                              "будут удалены навсегда. \n"
                                              "Это позволит повторно использовать оперативную память для других целей.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _nodegraphTab->addKnob(_maxUndoRedoNodeGraph);


    _disconnectedArrowLength = AppManager::createKnob<KnobInt>( this, tr("Длина отключенной стрелы") );
    _disconnectedArrowLength->setName("disconnectedArrowLength");
    _disconnectedArrowLength->setHintToolTip( tr("Размер стрелки ввода отключенного узла в пикселях.") );
    _disconnectedArrowLength->disableSlider();

    _nodegraphTab->addKnob(_disconnectedArrowLength);

    _hideOptionalInputsAutomatically = AppManager::createKnob<KnobBool>( this, tr("Автоскрытие входных данных масок") );
    _hideOptionalInputsAutomatically->setName("autoHideInputs");
    _hideOptionalInputsAutomatically->setHintToolTip( tr("Флажок-любые данные, вводимые с помощью несвязанной маски для узла в Схеме Узлов, "
                                                         "будут видны только при наведении курсора мыши на узел "
                                                         "или при его выборе.") );
    _nodegraphTab->addKnob(_hideOptionalInputsAutomatically);

    _useInputAForMergeAutoConnect = AppManager::createKnob<KnobBool>( this, tr("Узел слияния, подключение к входу A") );
    _useInputAForMergeAutoConnect->setName("mergeConnectToA");
    _useInputAForMergeAutoConnect->setHintToolTip( tr("Если флажок установлен, то при создании нового узла слияния или любого другого узла "
                                                      "с входами с именами A и B вход A будет предпочтительным для автоматического подключения."
                                                      "Когда узел отключен, B всегда выводится, независимо от того, отмечено это или нет.") );
    _nodegraphTab->addKnob(_useInputAForMergeAutoConnect);
} // Settings::initializeKnobsNodeGraph

void
Settings::initializeKnobsCaching()
{
    /////////// Caching tab
    _cachingTab = AppManager::createKnob<KnobPage>( this, tr("Кэширование") );

    _aggressiveCaching = AppManager::createKnob<KnobBool>( this, tr("Агрессивное кэширование") );
    _aggressiveCaching->setName("aggressiveCaching");
    _aggressiveCaching->setHintToolTip( tr("Если флажок установлен, %1 будет кэшировать выходные данные всех изображений,  "
                                           "отображаемых всеми узлами, независимо от их параметра \"Force caching\". Для включения этой опции "
                                           "необходимо иметь как минимум 8 ГБ ОЗУ, рекомендуется 16 ГБ.\n"
                                           "Если флажок не установлен, %1 будет кэшировать только те узлы, которые имеют несколько выходов, "
                                           "или их параметр \"Force caching\" отмечен, или если у одного из его выходов "
                                           "открыта панель настроек.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_aggressiveCaching);

    _maxRAMPercent = AppManager::createKnob<KnobInt>( this, tr("Максимальный объем ОЗУ, используемый для кэширования (% of total RAM)") );
    _maxRAMPercent->setName("maxRAMPercent");
    _maxRAMPercent->disableSlider();
    _maxRAMPercent->setMinimum(0);
    _maxRAMPercent->setMaximum(100);
    QString ramHint( tr("Этот параметр указывает процент от общего объема оперативной памяти, который может использоваться кэшами памяти."
                        "Эта система имеет %1 ОЗУ.").arg( printAsRAM( getSystemTotalRAM() ) ) );
    if ( isApplication32Bits() && (getSystemTotalRAM() > 4ULL * 1024ULL * 1024ULL * 1024ULL) ) {
        ramHint.append( QString::fromUtf8("\n") );
        ramHint.append( tr("Версия %1, которую вы используете, составляет 32 бита, что означает доступную оперативную память "
                           "ограничено 4 ГБ. Объем ОЗУ, используемой для кэширования, составляет 4 ГБ * MaxRamPercent.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    }

    _maxRAMPercent->setHintToolTip(ramHint);
    _maxRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_maxRAMPercent);

    _maxRAMLabel = AppManager::createKnob<KnobString>( this, std::string() );
    _maxRAMLabel->setName("maxRamLabel");
    _maxRAMLabel->setIsPersistent(false);
    _maxRAMLabel->setAsLabel();
    _cachingTab->addKnob(_maxRAMLabel);


    _unreachableRAMPercent = AppManager::createKnob<KnobInt>( this, tr("Системная ОЗУ должна оставаться свободной (% of total RAM)") );
    _unreachableRAMPercent->setName("unreachableRAMPercent");
    _unreachableRAMPercent->disableSlider();
    _unreachableRAMPercent->setMinimum(0);
    _unreachableRAMPercent->setMaximum(90);
    _unreachableRAMPercent->setHintToolTip(tr("Это определяет, сколько оперативной памяти должно оставаться свободным для других "
                                              "приложений, работающих в той же системе. "
                                              "Когда этот предел достигнут, кэши начинают перерабатывать память, а не увеличиваться. "
                                              //"A reasonable value should be set for it allowing the caches to stay in physical RAM " // users don't understand what swap is
                                              //"and avoid being swapped-out on disk. "
                                              "Это значение должно отражать объем памяти, который вы хотите оставить "
                                              "на своем компьютере для другого использования. "
                                              "Низкое значение может привести к замедлению работы и интенсивному использованию диска.")
                                           );
    _unreachableRAMPercent->setAddNewLine(false);
    _cachingTab->addKnob(_unreachableRAMPercent);
    _unreachableRAMLabel = AppManager::createKnob<KnobString>( this, std::string() );
    _unreachableRAMLabel->setName("unreachableRAMLabel");
    _unreachableRAMLabel->setIsPersistent(false);
    _unreachableRAMLabel->setAsLabel();
    _cachingTab->addKnob(_unreachableRAMLabel);

    _maxViewerDiskCacheGB = AppManager::createKnob<KnobInt>( this, tr("Максимальный размер кэша диска воспроизведения (ГиБ)") );
    _maxViewerDiskCacheGB->setName("maxViewerDiskCache");
    _maxViewerDiskCacheGB->disableSlider();
    _maxViewerDiskCacheGB->setMinimum(0);
    _maxViewerDiskCacheGB->setMaximum(100);
    _maxViewerDiskCacheGB->setHintToolTip( tr("Максимальный размер, который может использовать кэш воспроизведения на диске (в ГиБ)") );
    _cachingTab->addKnob(_maxViewerDiskCacheGB);

    _maxDiskCacheNodeGB = AppManager::createKnob<KnobInt>( this, tr("Максимальное использование диска узла DiskCache (ГиБ)") );
    _maxDiskCacheNodeGB->setName("maxDiskCacheNode");
    _maxDiskCacheNodeGB->disableSlider();
    _maxDiskCacheNodeGB->setMinimum(0);
    _maxDiskCacheNodeGB->setMaximum(100);
    _maxDiskCacheNodeGB->setHintToolTip( tr("Максимальный размер, который может использовать узел DiskCache на диске. (в ГиБ)") );
    _cachingTab->addKnob(_maxDiskCacheNodeGB);


    _diskCachePath = AppManager::createKnob<KnobPath>( this, tr("Путь к кэшу диска") );
    _diskCachePath->setName("diskCachePath");
    _diskCachePath->setMultiPath(false);

    QString defaultLocation = StandardPaths::writableLocation(StandardPaths::eStandardLocationCache);
    QString diskCacheTt( tr("ВНИМАНИЕ: изменение этого параметра требует перезапуска приложения. \n"
                            "Это указывает на место, где будут находиться кэши %1 на диске. "
                            "Эта переменная должна указывать на ваш самый быстрый диск. "
                            "Этот параметр можно переопределить значением переменной среды %2.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DISK_CACHE_PATH_ENV_VAR) ) );

    QString diskCacheTt2( tr("Если параметр оставить пустым или набор местоположений недействителен, "
                             "будет использовано местоположение по умолчанию. Местоположение по умолчанию: %1.").arg(defaultLocation) );

    _diskCachePath->setHintToolTip( diskCacheTt + QLatin1Char('\n') + diskCacheTt2 );
    _cachingTab->addKnob(_diskCachePath);

    _wipeDiskCache = AppManager::createKnob<KnobButton>( this, tr("Очистить кэш диска") );
    _wipeDiskCache->setHintToolTip( tr("Очищает все кеши, удаляя все папки, которые могут содержать кешированные данные."
                                       "Это предусмотрено на тот случай, если %1 по какой-либо причине "
                                       "потерял след кешированных изображений.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _cachingTab->addKnob(_wipeDiskCache);
} // Settings::initializeKnobsCaching

void
Settings::initializeKnobsPlugins()
{
    _pluginsTab = AppManager::createKnob<KnobPage>( this, tr("Плагины") );
    _pluginsTab->setName("plugins");

#if defined(__linux__) || defined(__FreeBSD__)
    std::string searchPath("/usr/OFX/Plugins");
#elif defined(__APPLE__)
    std::string searchPath("/Library/OFX/Plugins");
#elif defined(WINDOWS)

    std::wstring basePath = std::wstring( OFX::Host::PluginCache::getStdOFXPluginPath() );
    basePath.append( std::wstring(L" and C:\\Program Files\\Common Files\\OFX\\Plugins") );
    std::string searchPath = StrUtils::utf16_to_utf8(basePath);

#endif

    _loadBundledPlugins = AppManager::createKnob<KnobBool>( this, tr("Используйте встроенные плагины") );
    _loadBundledPlugins->setName("useBundledPlugins");
    _loadBundledPlugins->setHintToolTip( tr("Если этот флажок установлен, %1 также использует плагины, "
                                            "входящие в состав двоичного дистрибутива.\n"
                                            "Если флажок снят, загружаются только общесистемные найденные плагины (более подробную информацию "
                                            "можно найти в справке по настройке \"Extra plug-ins search paths\").").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_loadBundledPlugins);

    _preferBundledPlugins = AppManager::createKnob<KnobBool>( this, tr("Предпочитайте встроенные плагины общесистемным плагинам.") );
    _preferBundledPlugins->setName("preferBundledPlugins");
    _preferBundledPlugins->setHintToolTip( tr("Если этот флажок установлен, а также если также установлен флажок \"Use bundled plug-ins\" плагины, связанные с двоичным дистрибутивом %1, будут иметь приоритет над общесистемными плагинами,"
                                              "если у них одинаковый внутренний ID.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pluginsTab->addKnob(_preferBundledPlugins);

    _useStdOFXPluginsLocation = AppManager::createKnob<KnobBool>( this, tr("Включить расположение плагинов OpenFX по умолчанию") );
    _useStdOFXPluginsLocation->setName("useStdOFXPluginsLocation");
    _useStdOFXPluginsLocation->setHintToolTip( tr("Если флажок установлен, %1 также использует подключаемые модули OpenFX, находящиеся в расположении по умолчанию (%2).").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _pluginsTab->addKnob(_useStdOFXPluginsLocation);

    _extraPluginPaths = AppManager::createKnob<KnobPath>( this, tr("Путь поиска плагинов OpenFX") );
    _extraPluginPaths->setName("extraPluginsSearchPaths");
    _extraPluginPaths->setHintToolTip( tr("Дополнительные пути поиска, по которым %1 должен искать подключаемые модули OpenFX. "
                                          "Дополнительные пути поиска плагинов также можно указать с помощью переменной среды OFX_PLUGIN_PATH.\n"
                                          "The priority order for system-wide plug-ins, from high to low, is:\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is checked)\n"
                                          "- plug-ins found in OFX_PLUGIN_PATH\n"
                                          "- plug-ins found in %2 (if \"Enable default OpenFX plug-ins location\" is checked)\n"
                                          "- plugins bundled with the binary distribution of %1 (if \"Prefer bundled plug-ins over "
                                          "system-wide plug-ins\" is not checked)\n"
                                          "Any change will take effect on the next launch of %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8( searchPath.c_str() ) ) );
    _extraPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_extraPluginPaths);


    _templatesPluginPaths = AppManager::createKnob<KnobPath>( this, tr("Путь поиска PyPlugs") );
    _templatesPluginPaths->setName("groupPluginsSearchPath");
    _templatesPluginPaths->setHintToolTip( tr("Путь поиска, по которому %1 должен искать сценарии группы Python (PyPlugs). "
                                              "Пути поиска групп также можно указать с помощью "
                                              "переменной среды NATRON_PLUGIN_PATH.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _templatesPluginPaths->setMultiPath(true);
    _pluginsTab->addKnob(_templatesPluginPaths);

} // Settings::initializeKnobsPlugins

void
Settings::initializeKnobsPython()
{
    _pythonPage = AppManager::createKnob<KnobPage>( this, tr("Python") );


    _onProjectCreated = AppManager::createKnob<KnobString>( this, tr("После создания проекта") );
    _onProjectCreated->setName("afterProjectCreated");
    _onProjectCreated->setHintToolTip( tr("Callback called once a new project is created (this is never called "
                                          "when \"After project loaded\" is called.)\n"
                                          "The signature of the callback is: callback(app) where:\n"
                                          "- app: points to the current application instance\n") );
    _pythonPage->addKnob(_onProjectCreated);


    _defaultOnProjectLoaded = AppManager::createKnob<KnobString>( this, tr("По умолчанию после загрузки проекта") );
    _defaultOnProjectLoaded->setName("defOnProjectLoaded");
    _defaultOnProjectLoaded->setHintToolTip( tr("Обратный вызов afterProjectLoad по умолчанию, который будет установлен для новых проектов.") );
    _pythonPage->addKnob(_defaultOnProjectLoaded);

    _defaultOnProjectSave = AppManager::createKnob<KnobString>( this, tr("По умолчанию перед сохранением проекта") );
    _defaultOnProjectSave->setName("defOnProjectSave");
    _defaultOnProjectSave->setHintToolTip( tr("Обратный вызов beforeProjectSave по умолчанию, который будет установлен для новых проектов.") );
    _pythonPage->addKnob(_defaultOnProjectSave);


    _defaultOnProjectClose = AppManager::createKnob<KnobString>( this, tr("По умолчанию перед закрытием проекта") );
    _defaultOnProjectClose->setName("defOnProjectClose");
    _defaultOnProjectClose->setHintToolTip( tr("Обратный вызов beforeProjectClose по умолчанию, который будет установлен для новых проектов.") );
    _pythonPage->addKnob(_defaultOnProjectClose);


    _defaultOnNodeCreated = AppManager::createKnob<KnobString>( this, tr("По умолчанию после создания узла") );
    _defaultOnNodeCreated->setName("defOnNodeCreated");
    _defaultOnNodeCreated->setHintToolTip( tr("Обратный вызов afterNodeCreated по умолчанию, который будет установлен для новых проектов.") );
    _pythonPage->addKnob(_defaultOnNodeCreated);


    _defaultOnNodeDelete = AppManager::createKnob<KnobString>( this, tr("По умолчанию перед удалением узла") );
    _defaultOnNodeDelete->setName("defOnNodeDelete");
    _defaultOnNodeDelete->setHintToolTip( tr("Обратный вызов beforeNodeRemoval по умолчанию, который будет установлен для новых проектов.") );
    _pythonPage->addKnob(_defaultOnNodeDelete);

    _loadPyPlugsFromPythonScript = AppManager::createKnob<KnobBool>( this, tr("Загрузите PyPlugs в проекты из .py, если это возможно.") );
    _loadPyPlugsFromPythonScript->setName("loadFromPyFile");
    _loadPyPlugsFromPythonScript->setHintToolTip( tr("Флажок-если проект содержит PyPlug, он попытается сначала загрузить PyPlug из файла .py. "
                                                     "Если версия PyPlug изменилась, Natron спросит вас, хотите ли вы перейти на новую версию "
                                                     "PyPlug в своем проекте. Если файл .py не найден, он вернется к тому же поведению "
                                                     "что и при снятии этого параметра. Если флажок снят, PyPlug будет"
                                                     "загружаться как обычная группа с информацией, встроенной в файл проекта.") );
    _loadPyPlugsFromPythonScript->setDefaultValue(true);
    _pythonPage->addKnob(_loadPyPlugsFromPythonScript);

    _echoVariableDeclarationToPython = AppManager::createKnob<KnobBool>( this, tr("Печать автоматически объявленных переменных в редакторе скриптов") );
    _echoVariableDeclarationToPython->setName("printAutoDeclaredVars");
    _echoVariableDeclarationToPython->setHintToolTip( tr("Если флажок установлен, %1 будет печатать в редакторе сценариев все автоматически "
                                                         "объявленные переменные,такие как переменная приложения или атрибуты узла.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ) );
    _pythonPage->addKnob(_echoVariableDeclarationToPython);
} // initializeKnobs

void
Settings::setCachingLabels()
{
    int maxTotalRam = _maxRAMPercent->getValue();
    U64 systemTotalRam = getSystemTotalRAM();
    U64 maxRAM = (U64)( ( (double)maxTotalRam / 100. ) * systemTotalRam );

    _maxRAMLabel->setValue( printAsRAM(maxRAM).toStdString() );
    _unreachableRAMLabel->setValue( printAsRAM( (double)systemTotalRam * ( (double)_unreachableRAMPercent->getValue() / 100. ) ).toStdString() );
}

void
Settings::setDefaultValues()
{
    beginChanges();

    _natronSettingsExist->setDefaultValue(false);

    // General
    _checkForUpdates->setDefaultValue(false);
#ifdef NATRON_USE_BREAKPAD
    _enableCrashReports->setDefaultValue(true);
#endif
    _autoSaveUnSavedProjects->setDefaultValue(true);
    _autoSaveDelay->setDefaultValue(5, 0);
    _saveVersions->setDefaultValue(1);
    _hostName->setDefaultValue(0);
    _customHostName->setDefaultValue(NATRON_ORGANIZATION_DOMAIN_TOPLEVEL "." NATRON_ORGANIZATION_DOMAIN_SUB "." NATRON_APPLICATION_NAME);

    // General/Threading
    _numberOfThreads->setDefaultValue(0, 0);
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders->setDefaultValue(0, 0);
#endif
    _useThreadPool->setDefaultValue(true);
    _nThreadsPerEffect->setDefaultValue(0);
    _renderInSeparateProcess->setDefaultValue(false, 0);
    _queueRenders->setDefaultValue(false);

    // General/Rendering
    _convertNaNValues->setDefaultValue(true);
    _pluginUseImageCopyForSource->setDefaultValue(false);
    _activateRGBSupport->setDefaultValue(true);
    _activateTransformConcatenationSupport->setDefaultValue(true);

    // General/GPU rendering
    //_openglRendererString
    _nOpenGLContexts->setDefaultValue(2);
#if NATRON_VERSION_MAJOR < 2 || (NATRON_VERSION_MAJOR == 2 && NATRON_VERSION_MINOR < 2)
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabled);
#else
    _enableOpenGL->setDefaultValue((int)eEnableOpenGLDisabledIfBackground);
#endif

    // General/Projects setup
    _firstReadSetProjectFormat->setDefaultValue(true);
    _autoPreviewEnabledForNewProjects->setDefaultValue(true, 0);
    _fixPathsOnProjectPathChanged->setDefaultValue(true);
    //_enableMappingFromDriveLettersToUNCShareNames

    // General/Documentation
    _wwwServerPort->setDefaultValue(0);
#ifdef NATRON_DOCUMENTATION_ONLINE
    _documentationSource->setDefaultValue(0);
#endif

    // General/User Interface
    _notifyOnFileChange->setDefaultValue(true);
#ifdef NATRON_ENABLE_IO_META_NODES
    //_filedialogForWriters
#endif
    _renderOnEditingFinished->setDefaultValue(false);
    _linearPickers->setDefaultValue(true, 0);
    _maxPanelsOpened->setDefaultValue(10, 0);
    _useCursorPositionIncrements->setDefaultValue(true);
    //_defaultLayoutFile
    _loadProjectsWorkspace->setDefaultValue(false);
#ifdef Q_OS_WIN
    _enableConsoleWindow->setDefaultValue(false);
#endif

    // Color-Management
    //_ocioConfigKnob
    _warnOcioConfigKnobChanged->setDefaultValue(true);
    _ocioStartupCheck->setDefaultValue(true);
    //_customOcioConfigFile

    // Caching
    _aggressiveCaching->setDefaultValue(false);
    _maxRAMPercent->setDefaultValue(50, 0);
    _unreachableRAMPercent->setDefaultValue(20); // see https://github.com/NatronGitHub/Natron/issues/486
    _maxViewerDiskCacheGB->setDefaultValue(5, 0);
    _maxDiskCacheNodeGB->setDefaultValue(10, 0);
    //_diskCachePath
    setCachingLabels();

    // Viewer
    _texturesMode->setDefaultValue(0, 0);
    _powerOf2Tiling->setDefaultValue(8, 0);
    _checkerboardTileSize->setDefaultValue(5);
    _checkerboardColor1->setDefaultValue(0.5, 0);
    _checkerboardColor1->setDefaultValue(0.5, 1);
    _checkerboardColor1->setDefaultValue(0.5, 2);
    _checkerboardColor1->setDefaultValue(0.5, 3);
    _checkerboardColor2->setDefaultValue(0., 0);
    _checkerboardColor2->setDefaultValue(0., 1);
    _checkerboardColor2->setDefaultValue(0., 2);
    _checkerboardColor2->setDefaultValue(0., 3);
    _autoWipe->setDefaultValue(true);
    _autoProxyWhenScrubbingTimeline->setDefaultValue(true);
    _autoProxyLevel->setDefaultValue(1);
    _maximumNodeViewerUIOpened->setDefaultValue(2);
    _viewerNumberKeys->setDefaultValue(true);
    _viewerOverlaysPath->setDefaultValue(true);

    // Nodegraph
    _autoScroll->setDefaultValue(false);
    _autoTurbo->setDefaultValue(false);
    _snapNodesToConnections->setDefaultValue(true);
    _useBWIcons->setDefaultValue(false);
    _maxUndoRedoNodeGraph->setDefaultValue(20, 0);
    _disconnectedArrowLength->setDefaultValue(30);
    _hideOptionalInputsAutomatically->setDefaultValue(true);
    _useInputAForMergeAutoConnect->setDefaultValue(true);
    _usePluginIconsInNodeGraph->setDefaultValue(true);
    _useAntiAliasing->setDefaultValue(true);

    // Plugins
    _extraPluginPaths->setDefaultValue("", 0);
    _useStdOFXPluginsLocation->setDefaultValue(true);
    //_templatesPluginPaths
    _preferBundledPlugins->setDefaultValue(true);
    _loadBundledPlugins->setDefaultValue(true);

    // Python
    //_onProjectCreated;
    //_defaultOnProjectLoaded;
    //_defaultOnProjectSave;
    //_defaultOnProjectClose;
    //_defaultOnNodeCreated;
    //_defaultOnNodeDelete;
    //_loadPyPlugsFromPythonScript;
    _echoVariableDeclarationToPython->setDefaultValue(false);

    // Appearance
    _systemFontChoice->setDefaultValue(0);
    _fontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);
    //_qssFile
    //_defaultAppearanceVersion

    // Appearance/Main Window
    _sunkenColor->setDefaultValue(0.12, 0);
    _sunkenColor->setDefaultValue(0.12, 1);
    _sunkenColor->setDefaultValue(0.12, 2);
    _baseColor->setDefaultValue(0.19, 0);
    _baseColor->setDefaultValue(0.19, 1);
    _baseColor->setDefaultValue(0.19, 2);
    _raisedColor->setDefaultValue(0.28, 0);
    _raisedColor->setDefaultValue(0.28, 1);
    _raisedColor->setDefaultValue(0.28, 2);
    _selectionColor->setDefaultValue(0.95, 0);
    _selectionColor->setDefaultValue(0.54, 1);
    _selectionColor->setDefaultValue(0., 2);
    _textColor->setDefaultValue(0.78, 0);
    _textColor->setDefaultValue(0.78, 1);
    _textColor->setDefaultValue(0.78, 2);
    _altTextColor->setDefaultValue(0.6, 0);
    _altTextColor->setDefaultValue(0.6, 1);
    _altTextColor->setDefaultValue(0.6, 2);
    _timelinePlayheadColor->setDefaultValue(0.95, 0);
    _timelinePlayheadColor->setDefaultValue(0.54, 1);
    _timelinePlayheadColor->setDefaultValue(0., 2);
    _timelineBGColor->setDefaultValue(0, 0);
    _timelineBGColor->setDefaultValue(0, 1);
    _timelineBGColor->setDefaultValue(0., 2);
    _timelineBoundsColor->setDefaultValue(0.81, 0);
    _timelineBoundsColor->setDefaultValue(0.27, 1);
    _timelineBoundsColor->setDefaultValue(0.02, 2);
    _interpolatedColor->setDefaultValue(0.34, 0);
    _interpolatedColor->setDefaultValue(0.46, 1);
    _interpolatedColor->setDefaultValue(0.6, 2);
    _keyframeColor->setDefaultValue(0.08, 0);
    _keyframeColor->setDefaultValue(0.38, 1);
    _keyframeColor->setDefaultValue(0.97, 2);
    _trackerKeyframeColor->setDefaultValue(0.7, 0);
    _trackerKeyframeColor->setDefaultValue(0.78, 1);
    _trackerKeyframeColor->setDefaultValue(0.39, 2);
    _exprColor->setDefaultValue(0.7, 0);
    _exprColor->setDefaultValue(0.78, 1);
    _exprColor->setDefaultValue(0.39, 2);
    _cachedFrameColor->setDefaultValue(0.56, 0);
    _cachedFrameColor->setDefaultValue(0.79, 1);
    _cachedFrameColor->setDefaultValue(0.4, 2);
    _diskCachedFrameColor->setDefaultValue(0.27, 0);
    _diskCachedFrameColor->setDefaultValue(0.38, 1);
    _diskCachedFrameColor->setDefaultValue(0.25, 2);
    _sliderColor->setDefaultValue(0.33, 0);
    _sliderColor->setDefaultValue(0.45, 1);
    _sliderColor->setDefaultValue(0.44, 2);

    // Apprance/Curve Editor
    _curveEditorBGColor->setDefaultValue(0., 0);
    _curveEditorBGColor->setDefaultValue(0., 1);
    _curveEditorBGColor->setDefaultValue(0., 2);
    _gridColor->setDefaultValue(0.46, 0);
    _gridColor->setDefaultValue(0.84, 1);
    _gridColor->setDefaultValue(0.35, 2);
    _curveEditorScaleColor->setDefaultValue(0.26, 0);
    _curveEditorScaleColor->setDefaultValue(0.48, 1);
    _curveEditorScaleColor->setDefaultValue(0.2, 2);

    // Appearance/Dope Sheet
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 0);
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 1);
    _dopeSheetEditorBackgroundColor->setDefaultValue(0.208, 2);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 0);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 1);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.204, 2);
    _dopeSheetEditorRootSectionBackgroundColor->setDefaultValue(0.2, 3);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 0);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 1);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.443, 2);
    _dopeSheetEditorKnobSectionBackgroundColor->setDefaultValue(0.2, 3);
    _dopeSheetEditorScaleColor->setDefaultValue(0.714, 0);
    _dopeSheetEditorScaleColor->setDefaultValue(0.718, 1);
    _dopeSheetEditorScaleColor->setDefaultValue(0.714, 2);
    _dopeSheetEditorGridColor->setDefaultValue(0.714, 0);
    _dopeSheetEditorGridColor->setDefaultValue(0.714, 1);
    _dopeSheetEditorGridColor->setDefaultValue(0.714, 2);

    // Appearance/Script Editor
    _curLineColor->setDefaultValue(0.35, 0);
    _curLineColor->setDefaultValue(0.35, 1);
    _curLineColor->setDefaultValue(0.35, 2);
    _keywordColor->setDefaultValue(0.7, 0);
    _keywordColor->setDefaultValue(0.7, 1);
    _keywordColor->setDefaultValue(0., 2);
    _operatorColor->setDefaultValue(0.78, 0);
    _operatorColor->setDefaultValue(0.78, 1);
    _operatorColor->setDefaultValue(0.78, 2);
    _braceColor->setDefaultValue(0.85, 0);
    _braceColor->setDefaultValue(0.85, 1);
    _braceColor->setDefaultValue(0.85, 2);
    _defClassColor->setDefaultValue(0.7, 0);
    _defClassColor->setDefaultValue(0.7, 1);
    _defClassColor->setDefaultValue(0., 2);
    _stringsColor->setDefaultValue(0.8, 0);
    _stringsColor->setDefaultValue(0.2, 1);
    _stringsColor->setDefaultValue(0., 2);
    _commentsColor->setDefaultValue(0.25, 0);
    _commentsColor->setDefaultValue(0.6, 1);
    _commentsColor->setDefaultValue(0.25, 2);
    _selfColor->setDefaultValue(0.7, 0);
    _selfColor->setDefaultValue(0.7, 1);
    _selfColor->setDefaultValue(0., 2);
    _numbersColor->setDefaultValue(0.25, 0);
    _numbersColor->setDefaultValue(0.8, 1);
    _numbersColor->setDefaultValue(0.9, 2);
    _scriptEditorFontChoice->setDefaultValue(0);
    _scriptEditorFontSize->setDefaultValue(NATRON_FONT_SIZE_DEFAULT);


    // Appearance/Node Graph
    _defaultNodeColor->setDefaultValue(0.7, 0);
    _defaultNodeColor->setDefaultValue(0.7, 1);
    _defaultNodeColor->setDefaultValue(0.7, 2);
    _defaultBackdropColor->setDefaultValue(0.45, 0);
    _defaultBackdropColor->setDefaultValue(0.45, 1);
    _defaultBackdropColor->setDefaultValue(0.45, 2);
    _defaultGeneratorColor->setDefaultValue(0.3, 0);
    _defaultGeneratorColor->setDefaultValue(0.5, 1);
    _defaultGeneratorColor->setDefaultValue(0.2, 2);
    _defaultReaderColor->setDefaultValue(0.7, 0);
    _defaultReaderColor->setDefaultValue(0.7, 1);
    _defaultReaderColor->setDefaultValue(0.7, 2);
    _defaultWriterColor->setDefaultValue(0.75, 0);
    _defaultWriterColor->setDefaultValue(0.75, 1);
    _defaultWriterColor->setDefaultValue(0., 2);
    _defaultColorGroupColor->setDefaultValue(0.48, 0);
    _defaultColorGroupColor->setDefaultValue(0.66, 1);
    _defaultColorGroupColor->setDefaultValue(1., 2);
    _defaultFilterGroupColor->setDefaultValue(0.8, 0);
    _defaultFilterGroupColor->setDefaultValue(0.5, 1);
    _defaultFilterGroupColor->setDefaultValue(0.3, 2);
    _defaultTransformGroupColor->setDefaultValue(0.7, 0);
    _defaultTransformGroupColor->setDefaultValue(0.3, 1);
    _defaultTransformGroupColor->setDefaultValue(0.1, 2);
    _defaultTimeGroupColor->setDefaultValue(0.7, 0);
    _defaultTimeGroupColor->setDefaultValue(0.65, 1);
    _defaultTimeGroupColor->setDefaultValue(0.35, 2);
    _defaultDrawGroupColor->setDefaultValue(0.75, 0);
    _defaultDrawGroupColor->setDefaultValue(0.75, 1);
    _defaultDrawGroupColor->setDefaultValue(0.75, 2);
    _defaultKeyerGroupColor->setDefaultValue(0., 0);
    _defaultKeyerGroupColor->setDefaultValue(1, 1);
    _defaultKeyerGroupColor->setDefaultValue(0., 2);
    _defaultChannelGroupColor->setDefaultValue(0.6, 0);
    _defaultChannelGroupColor->setDefaultValue(0.24, 1);
    _defaultChannelGroupColor->setDefaultValue(0.39, 2);
    _defaultMergeGroupColor->setDefaultValue(0.3, 0);
    _defaultMergeGroupColor->setDefaultValue(0.37, 1);
    _defaultMergeGroupColor->setDefaultValue(0.776, 2);
    _defaultViewsGroupColor->setDefaultValue(0.5, 0);
    _defaultViewsGroupColor->setDefaultValue(0.9, 1);
    _defaultViewsGroupColor->setDefaultValue(0.7, 2);
    _defaultDeepGroupColor->setDefaultValue(0., 0);
    _defaultDeepGroupColor->setDefaultValue(0., 1);
    _defaultDeepGroupColor->setDefaultValue(0.38, 2);


    endChanges();
} // setDefaultValues

void
Settings::warnChangedKnobs(const std::vector<KnobI*>& knobs)
{
    bool didFontWarn = false;
    bool didOCIOWarn = false;
    bool didOFXCacheWarn = false;

    for (U32 i = 0; i < knobs.size(); ++i) {
        if ( ( ( knobs[i] == _fontSize.get() ) ||
               ( knobs[i] == _systemFontChoice.get() ) )
             && !didFontWarn ) {
            didFontWarn = true;
            Dialogs::warningDialog( tr("Изменение шрифта").toStdString(),
                                    tr("Изменение шрифта требует перезапуска %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        } else if ( ( ( knobs[i] == _ocioConfigKnob.get() ) ||
                      ( knobs[i] == _customOcioConfigFile.get() ) )
                    && !didOCIOWarn ) {
            didOCIOWarn = true;
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if (warnOcioChanged) {
                bool stopAsking = false;
                Dialogs::warningDialog(tr("Конфигурация OCIO изменена").toStdString(),
                                       tr("Чтобы изменение конфигурации OpenColorIO вступило в силу, требуется перезапуск %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), &stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false);
                    saveSetting( _warnOcioConfigKnobChanged.get() );
                }
            }
        } else if ( knobs[i] == _texturesMode.get() ) {
            AppInstanceVec apps = appPTR->getAppInstances();
            for (AppInstanceVec::iterator it = apps.begin(); it != apps.end(); ++it) {
                std::list<ViewerInstance*> allViewers;
                (*it)->getProject()->getViewers(&allViewers);
                for (std::list<ViewerInstance*>::iterator it = allViewers.begin(); it != allViewers.end(); ++it) {
                    (*it)->renderCurrentFrame(true);
                }
            }
        } else if ( ( ( knobs[i] == _loadBundledPlugins.get() ) ||
                      ( knobs[i] == _preferBundledPlugins.get() ) ||
                      ( knobs[i] == _useStdOFXPluginsLocation.get() ) ||
                      ( knobs[i] == _extraPluginPaths.get() ) )
                    && !didOFXCacheWarn ) {
            didOFXCacheWarn = true;
            appPTR->clearPluginsLoadedCache(); // clear the cache for next restart
            Dialogs::warningDialog( tr("Путь к плагинам OpenFX изменен").toStdString(),
                                    tr("Для изменения пути плагинов OpenFX требуется перезапуск %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        }
    }
} // Settings::warnChangedKnobs

void
Settings::saveAllSettings()
{
    if ( !_saveSettings->getValue() ) {
        return;
    }
    const KnobsVec &knobs = getKnobs();
    std::vector<KnobI*> k( knobs.size() );

    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    saveSettings(k, false, true);
}

void
Settings::restorePluginSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        if ( it->first.empty() ) {
            continue;
        }
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            Plugin* plugin  = *itver;
            assert(plugin);

            if ( plugin->getIsForInternalUseOnly() ) {
                continue;
            }


            {
                QString pluginIDKey = plugin->getPluginID() + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
                QString enabledKey = pluginIDKey + QString::fromUtf8("_enabled");
                if ( settings.contains(enabledKey) ) {
                    bool enabled = settings.value(enabledKey).toBool();
                    plugin->setActivated(enabled);
                } else {
                    settings.setValue( enabledKey, plugin->isActivated() );
                }

                QString rsKey = pluginIDKey + QString::fromUtf8("_rs");
                if ( settings.contains(rsKey) ) {
                    bool renderScaleEnabled = settings.value(rsKey).toBool();
                    plugin->setRenderScaleEnabled(renderScaleEnabled);
                } else {
                    settings.setValue( rsKey, plugin->isRenderScaleEnabled() );
                }

                QString mtKey = pluginIDKey + QString::fromUtf8("_mt");
                if ( settings.contains(mtKey) ) {
                    bool multiThreadingEnabled = settings.value(mtKey).toBool();
                    plugin->setMultiThreadingEnabled(multiThreadingEnabled);
                } else {
                    settings.setValue( mtKey, plugin->isMultiThreadingEnabled() );
                }

                QString glKey = pluginIDKey + QString::fromUtf8("_gl");
                if (settings.contains(glKey)) {
                    bool openglEnabled = settings.value(glKey).toBool();
                    plugin->setOpenGLEnabled(openglEnabled);
                } else {
                    settings.setValue(glKey, plugin->isOpenGLEnabled());
                }

            }
        }
    }
} // Settings::restorePluginSettings

void
Settings::savePluginsSettings()
{
    const PluginsMap& plugins = appPTR->getPluginsList();
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (PluginsMap::const_iterator it = plugins.begin(); it != plugins.end(); ++it) {
        assert(it->second.size() > 0);

        for (PluginVersionsOrdered::const_reverse_iterator itver = it->second.rbegin(); itver != it->second.rend(); ++itver) {
            Plugin* plugin  = *itver;
            assert(plugin);

            QString pluginID = plugin->getPluginID() + QString::fromUtf8("_") + QString::number( plugin->getMajorVersion() ) + QString::fromUtf8("_") + QString::number( plugin->getMinorVersion() );
            QString enabledKey = pluginID + QString::fromUtf8("_enabled");
            settings.setValue( enabledKey, plugin->isActivated() );

            QString rsKey = pluginID + QString::fromUtf8("_rs");
            settings.setValue( rsKey, plugin->isRenderScaleEnabled() );

            QString mtKey = pluginID + QString::fromUtf8("_mt");
            settings.setValue(mtKey, plugin->isMultiThreadingEnabled());

            QString glKey = pluginID + QString::fromUtf8("_gl");
            settings.setValue(glKey, plugin->isOpenGLEnabled());

        }
    }
}

void
Settings::setSaveSettings(bool v)
{
    _saveSettings->setValue(v);
}

bool
Settings::getSaveSettings() const
{
    return _saveSettings->getValue();
}

void
Settings::saveSettings(const std::vector<KnobI*>& knobs,
                       bool doWarnings,
                       bool pluginSettings)
{
    if ( !_saveSettings->getValue() ) {
        return;
    }
    if (pluginSettings) {
        savePluginsSettings();
    }
    std::vector<KnobI*> changedKnobs;
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    settings.setValue(QString::fromUtf8(kQSettingsSoftwareMajorVersionSettingName), NATRON_VERSION_MAJOR);
    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(knobs[i]);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(knobs[i]);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(knobs[i]);

        const std::string& name = knobs[i]->getName();
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            QString dimensionName;
            if (knobs[i]->getDimension() > 1) {
                dimensionName =  QString::fromUtf8( name.c_str() ) + QLatin1Char('.') + QString::fromUtf8( knobs[i]->getDimensionName(j).c_str() );
            } else {
                dimensionName = QString::fromUtf8( name.c_str() );
            }
            try {
                if (isString) {
                    QString old = settings.value(dimensionName).toString();
                    QString newValue = QString::fromUtf8( isString->getValue(j).c_str() );
                    if (old != newValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        int newIndex = isChoice->getValue(j);
                        const std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        if ( newIndex < (int)entries.size() ) {
                            QString oldValue = settings.value(dimensionName).toString();
                            QString newValue = QString::fromUtf8( entries[newIndex].id.c_str());
                            if (oldValue != newValue) {
                                changedKnobs.push_back(knobs[i]);
                            }
                            settings.setValue( dimensionName, QVariant(newValue) );
                        }
                    } else {
                        int newValue = isInt->getValue(j);
                        int oldValue = settings.value( dimensionName, QVariant(std::numeric_limits<int>::min()) ).toInt();
                        if (newValue != oldValue) {
                            changedKnobs.push_back(knobs[i]);
                        }
                        settings.setValue( dimensionName, QVariant(newValue) );
                    }
                } else if (isDouble) {
                    double newValue = isDouble->getValue(j);
                    double oldValue = settings.value( dimensionName, QVariant(std::numeric_limits<int>::min()) ).toDouble();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else if (isBool) {
                    bool newValue = isBool->getValue(j);
                    bool oldValue = settings.value(dimensionName).toBool();
                    if (newValue != oldValue) {
                        changedKnobs.push_back(knobs[i]);
                    }
                    settings.setValue( dimensionName, QVariant(newValue) );
                } else {
                    assert(false);
                }
            } catch (std::logic_error&) {
                // ignore
            }
        } // for (int j = 0; j < knobs[i]->getDimension(); ++j) {
    } // for (U32 i = 0; i < knobs.size(); ++i) {

    if (doWarnings) {
        warnChangedKnobs(changedKnobs);
    }
} // saveSettings

void
Settings::restoreKnobsFromSettings(const std::vector<KnobI*>& knobs)
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    for (U32 i = 0; i < knobs.size(); ++i) {
        KnobStringBase* isString = dynamic_cast<KnobStringBase*>(knobs[i]);
        KnobIntBase* isInt = dynamic_cast<KnobIntBase*>(knobs[i]);
        KnobChoice* isChoice = dynamic_cast<KnobChoice*>(knobs[i]);
        KnobDoubleBase* isDouble = dynamic_cast<KnobDoubleBase*>(knobs[i]);
        KnobBoolBase* isBool = dynamic_cast<KnobBoolBase*>(knobs[i]);

        const std::string& name = knobs[i]->getName();

        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            std::string dimensionName = knobs[i]->getDimension() > 1 ? name + '.' + knobs[i]->getDimensionName(j) : name;
            QString qDimName = QString::fromUtf8( dimensionName.c_str() );

            if ( settings.contains(qDimName) ) {
                if (isString) {
                    isString->setValue(settings.value(qDimName).toString().toStdString(), ViewSpec::all(), j);
                } else if (isInt) {
                    if (isChoice) {
                        ///For choices,serialize the choice name instead
                        std::string value = settings.value(qDimName).toString().toStdString();
                        const std::vector<ChoiceOption> entries = isChoice->getEntries_mt_safe();
                        int found = -1;

                        for (U32 k = 0; k < entries.size(); ++k) {
                            if (entries[k].id == value) {
                                found = (int)k;
                                break;
                            }
                        }

                        if (found >= 0) {
                            isChoice->setValue(found, ViewSpec::all(), j);
                        }
                    } else {
                        isInt->setValue(settings.value(qDimName).toInt(), ViewSpec::all(), j);
                    }
                } else if (isDouble) {
                    isDouble->setValue(settings.value(qDimName).toDouble(), ViewSpec::all(), j);
                } else if (isBool) {
                    isBool->setValue(settings.value(qDimName).toBool(), ViewSpec::all(), j);
                } else {
                    assert(false);
                }
            }
        }
    }
} // Settings::restoreKnobsFromSettings

void
Settings::restoreKnobsFromSettings(const KnobsVec& knobs)
{
    std::vector<KnobI*> k( knobs.size() );

    for (U32 i = 0; i < knobs.size(); ++i) {
        k[i] = knobs[i].get();
    }
    restoreKnobsFromSettings(k);
}

void
Settings::restoreSettings(bool useDefault)
{
    _restoringSettings = true;

    // call restoreKnobsFromSettings only if --no-settings is not part of the command-line arguments
    if (!useDefault) {
        const KnobsVec& knobs = getKnobs();
        restoreKnobsFromSettings(knobs);
    }

    if (!_ocioRestored) {
        ///Load even though there's no settings!
        tryLoadOpenColorIOConfig();
    }

    // Restore opengl renderer
    {
        std::vector<ChoiceOption> availableRenderers = _availableOpenGLRenderers->getEntries_mt_safe();
        QString missingGLError;
        bool hasGL = appPTR->hasOpenGLForRequirements(eOpenGLRequirementsTypeRendering, &missingGLError);

        if ( availableRenderers.empty() || !hasGL) {
            if (missingGLError.isEmpty()) {
                _openglRendererString->setValue( tr("Отрисовка OpenGL отключена: не удалось найти устройство, отвечающее требованиям %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
            } else {
                _openglRendererString->setValue(missingGLError.toStdString());
            }
        }
        int curIndex = _availableOpenGLRenderers->getValue();
        if ( (curIndex >= 0) && ( curIndex < (int)availableRenderers.size() ) ) {
            const std::list<OpenGLRendererInfo>& renderers = appPTR->getOpenGLRenderers();
            int i = 0;
            for (std::list<OpenGLRendererInfo>::const_iterator it = renderers.begin(); it != renderers.end(); ++it, ++i) {
                if (i == curIndex) {
                    QString maxMemoryString = it->maxMemBytes == 0 ? tr("Неизвестный") : printAsRAM(it->maxMemBytes);
                    QString curRenderer = (QString::fromUtf8("<p><h2>") +
                                           tr("Информация о рендерере OpenGL:") +
                                           QString::fromUtf8("</h2></p><p><b>") +
                                           tr("Продавец:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->vendorName.c_str() ) ) +
                                           tr("Рендер:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->rendererName.c_str() ) ) +
                                           tr("Версия OpenGL:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glVersionString.c_str() ) ) +
                                           tr("Версия GLSL:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg( QString::fromUtf8( it->glslVersionString.c_str() ) ) +
                                           tr("Макс. Память:") +
                                           QString::fromUtf8("</b> %1</p><p><b>").arg(maxMemoryString) +
                                           tr("Макс. Размер текстуры (пикс):") +
                                           QString::fromUtf8("</b> %5</p<").arg(it->maxTextureSize));
                    _openglRendererString->setValue( curRenderer.toStdString() );
                    break;
                }
            }
        }
    }

    if (!appPTR->isTextureFloatSupported()) {
        if (_texturesMode) {
            _texturesMode->setSecret(true);
        }
    }

    _settingsExisted = false;
    try {
        _settingsExisted = _natronSettingsExist->getValue();

        if (!_settingsExisted && !useDefault) {
            _natronSettingsExist->setValue(true);
            saveSetting( _natronSettingsExist.get() );
        }

        int appearanceVersion = _defaultAppearanceVersion->getValue();
        if ( _settingsExisted && (appearanceVersion < NATRON_DEFAULT_APPEARANCE_VERSION) ) {
            _defaultAppearanceOutdated = true;
            _defaultAppearanceVersion->setValue(NATRON_DEFAULT_APPEARANCE_VERSION);
            saveSetting( _defaultAppearanceVersion.get() );
        }

        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
        appPTR->setNThreadsToRender( getNumberOfThreads() );
        appPTR->setUseThreadPool( _useThreadPool->getValue() );
        appPTR->setPluginsUseInputImageCopyToRender( _pluginUseImageCopyForSource->getValue() );
    } catch (std::logic_error&) {
        // ignore
    }

    _restoringSettings = false;
} // restoreSettings

bool
Settings::tryLoadOpenColorIOConfig()
{
    // the default value is the environment variable "OCIO"
    QString configFile = QFile::decodeName( qgetenv(NATRON_OCIO_ENV_VAR_NAME) );

    // OCIO environment variable overrides everything, then try the custom config...
    if ( configFile.isEmpty() && _customOcioConfigFile->isEnabled(0) ) {
        ///try to load from the file
        std::string file;
        try {
            file = _customOcioConfigFile->getValue();
        } catch (...) {
            // ignore exceptions
        }
        if ( file.empty() ) {
            return false;
        }
        configFile = QString::fromUtf8( file.c_str() );
    }
    if ( !configFile.isEmpty() ) {
        if ( !QFile::exists(configFile) )  {
            Dialogs::errorDialog( "OpenColorIO", tr("%1: Нет такого файла.").arg(configFile).toStdString() );

            return false;
        }
    } else {
        // ... and finally try the setting from the choice menu.
        try {
            ///try to load from the combobox
            QString activeEntryText  = QString::fromUtf8( _ocioConfigKnob->getActiveEntry().id.c_str() );
            QString configFileName = QString( activeEntryText + QString::fromUtf8(".ocio") );
            QStringList defaultConfigsPaths = getDefaultOcioConfigPaths();
            Q_FOREACH(const QString &defaultConfigsDirStr, defaultConfigsPaths) {
                QDir defaultConfigsDir(defaultConfigsDirStr);

                if ( !defaultConfigsDir.exists() ) {
                    qDebug() << "Attempt to read an OpenColorIO configuration but the configuration directory"
                             << defaultConfigsDirStr << "does not exist.";
                    continue;
                }
                ///try to open the .ocio config file first in the defaultConfigsDir
                ///if we can't find it, try to look in a subdirectory with the name of the config for the file config.ocio
                if ( !defaultConfigsDir.exists(configFileName) ) {
                    QDir subDir(defaultConfigsDirStr + QDir::separator() + activeEntryText);
                    if ( !subDir.exists() ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: Такого файла или каталога нет").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    if ( !subDir.exists( QString::fromUtf8("config.ocio") ) ) {
                        Dialogs::errorDialog( "OpenColorIO", tr("%1: Такого файла или каталога нет").arg( subDir.absoluteFilePath( QString::fromUtf8("config.ocio") ) ).toStdString() );

                        return false;
                    }
                    configFile = subDir.absoluteFilePath( QString::fromUtf8("config.ocio") );
                } else {
                    configFile = defaultConfigsDir.absoluteFilePath(configFileName);
                }
            }
        } catch (...) {
            // ignore exceptions
        }

        if ( configFile.isEmpty() ) {
            return false;
        }
    }
    _ocioRestored = true;
#ifdef DEBUG
    qDebug() << "setting OCIO=" << configFile;
#endif
    std::string stdConfigFile = configFile.toStdString();
#if 0 //def __NATRON_WIN32__ // commented out in https://github.com/NatronGitHub/Natron/commit/3445d671f15fbd97bca164b53ceb41cef47c61c3
    _wputenv_s(L"OCIO", StrUtils::utf8_to_utf16(stdConfigFile).c_str());
#else
    qputenv( NATRON_OCIO_ENV_VAR_NAME, stdConfigFile.c_str() );
#endif

    std::string configPath = SequenceParsing::removePath(stdConfigFile);
    if ( !configPath.empty() && (configPath[configPath.size() - 1] == '/') ) {
        configPath.erase(configPath.size() - 1, 1);
    }
    appPTR->onOCIOConfigPathChanged(configPath);

    return true;
} // tryLoadOpenColorIOConfig

#ifdef NATRON_USE_BREAKPAD
inline
void
crash_application()
{
    std::cerr << "CRASHING APPLICATION NOW UPON USER REQUEST!" << std::endl;
    volatile int* a = (int*)(NULL);

    // coverity[var_deref_op]
    *a = 1;
}
#endif

bool
Settings::onKnobValueChanged(KnobI* k,
                             ValueChangedReasonEnum reason,
                             double /*time*/,
                             ViewSpec /*view*/,
                             bool /*originatedFromMainThread*/)
{
    Q_EMIT settingChanged(k);
    bool ret = true;

    if ( k == _maxViewerDiskCacheGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumViewerDiskSpace( getMaximumViewerDiskCacheSize() );
        }
    } else if ( k == _maxDiskCacheNodeGB.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumDiskSpace( getMaximumDiskCacheNodeSize() );
        }
    } else if ( k == _maxRAMPercent.get() ) {
        if (!_restoringSettings) {
            appPTR->setApplicationsCachesMaximumMemoryPercent( getRamMaximumPercent() );
        }
        setCachingLabels();
    } else if ( k == _diskCachePath.get() ) {
        QString path = QString::fromUtf8(_diskCachePath->getValue().c_str());
        qputenv(NATRON_DISK_CACHE_PATH_ENV_VAR, path.toUtf8());
        appPTR->refreshDiskCacheLocation();
    } else if ( k == _wipeDiskCache.get() ) {
        appPTR->wipeAndCreateDiskCacheStructure();
    } else if ( k == _numberOfThreads.get() ) {
        int nbThreads = getNumberOfThreads();
        appPTR->setNThreadsToRender(nbThreads);
        if (nbThreads == -1) {
            QThreadPool::globalInstance()->setMaxThreadCount(1);
            appPTR->abortAnyProcessing();
        } else if (nbThreads == 0) {
            // See https://github.com/NatronGitHub/Natron/issues/554
            // min(num_cores, RAM/3.5Gb)
            int maxThread = getSystemTotalRAM() / ( (3ULL * 1024ULL + 512ULL) * 1024ULL * 1024ULL );
            maxThread = std::min( std::max(1, maxThread), QThread::idealThreadCount() );
            QThreadPool::globalInstance()->setMaxThreadCount(maxThread);
        } else {
            QThreadPool::globalInstance()->setMaxThreadCount(nbThreads);
        }
    } else if ( k == _nThreadsPerEffect.get() ) {
        appPTR->setNThreadsPerEffect( getNumberOfThreadsPerEffect() );
    } else if ( k == _ocioConfigKnob.get() ) {
        if (_ocioConfigKnob->getActiveEntry().id == NATRON_CUSTOM_OCIO_CONFIG_NAME) {
            _customOcioConfigFile->setAllDimensionsEnabled(true);
        } else {
            _customOcioConfigFile->setAllDimensionsEnabled(false);
        }
        tryLoadOpenColorIOConfig();
    } else if ( k == _useThreadPool.get() ) {
        bool useTP = _useThreadPool->getValue();
        appPTR->setUseThreadPool(useTP);
    } else if ( k == _customOcioConfigFile.get() ) {
        if ( _customOcioConfigFile->isEnabled(0) ) {
            tryLoadOpenColorIOConfig();
            bool warnOcioChanged = _warnOcioConfigKnobChanged->getValue();
            if ( warnOcioChanged && appPTR->getTopLevelInstance() ) {
                bool stopAsking = false;
                Dialogs::warningDialog(tr("Изменена конфигурация OCIO").toStdString(),
                                       tr("Чтобы изменение конфигурации OpenColorIO вступило в силу, требуется перезапуск %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), &stopAsking);
                if (stopAsking) {
                    _warnOcioConfigKnobChanged->setValue(false);
                }
            }
        }
    } else if ( k == _maxUndoRedoNodeGraph.get() ) {
        appPTR->setUndoRedoStackLimit( _maxUndoRedoNodeGraph->getValue() );
    } else if ( k == _maxPanelsOpened.get() ) {
        appPTR->onMaxPanelsOpenedChanged( _maxPanelsOpened->getValue() );
    } else if ( k == _queueRenders.get() ) {
        appPTR->onQueueRendersChanged( _queueRenders->getValue() );
    } else if ( ( k == _checkerboardTileSize.get() ) || ( k == _checkerboardColor1.get() ) || ( k == _checkerboardColor2.get() ) ) {
        appPTR->onCheckerboardSettingsChanged();
    } else if ( k == _powerOf2Tiling.get() && !_restoringSettings) {
        appPTR->onViewerTileCacheSizeChanged();
    } else if ( k == _texturesMode.get() &&  !_restoringSettings) {
         appPTR->onViewerTileCacheSizeChanged();
    } else if ( ( k == _hideOptionalInputsAutomatically.get() ) && !_restoringSettings && (reason == eValueChangedReasonUserEdited) ) {
        appPTR->toggleAutoHideGraphInputs();
    } else if ( k == _autoProxyWhenScrubbingTimeline.get() ) {
        _autoProxyLevel->setSecret( !_autoProxyWhenScrubbingTimeline->getValue() );
    } else if ( !_restoringSettings &&
                ( ( k == _sunkenColor.get() ) ||
                  ( k == _baseColor.get() ) ||
                  ( k == _raisedColor.get() ) ||
                  ( k == _selectionColor.get() ) ||
                  ( k == _textColor.get() ) ||
                  ( k == _altTextColor.get() ) ||
                  ( k == _timelinePlayheadColor.get() ) ||
                  ( k == _timelineBoundsColor.get() ) ||
                  ( k == _timelineBGColor.get() ) ||
                  ( k == _interpolatedColor.get() ) ||
                  ( k == _keyframeColor.get() ) ||
                  ( k == _trackerKeyframeColor.get() ) ||
                  ( k == _cachedFrameColor.get() ) ||
                  ( k == _diskCachedFrameColor.get() ) ||
                  ( k == _curveEditorBGColor.get() ) ||
                  ( k == _gridColor.get() ) ||
                  ( k == _curveEditorScaleColor.get() ) ||
                  ( k == _dopeSheetEditorBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorRootSectionBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorKnobSectionBackgroundColor.get() ) ||
                  ( k == _dopeSheetEditorScaleColor.get() ) ||
                  ( k == _dopeSheetEditorGridColor.get() ) ||
                  ( k == _keywordColor.get() ) ||
                  ( k == _operatorColor.get() ) ||
                  ( k == _curLineColor.get() ) ||
                  ( k == _braceColor.get() ) ||
                  ( k == _defClassColor.get() ) ||
                  ( k == _stringsColor.get() ) ||
                  ( k == _commentsColor.get() ) ||
                  ( k == _selfColor.get() ) ||
                  ( k == _sliderColor.get() ) ||
                  ( k == _numbersColor.get() ) ) ) {
        appPTR->reloadStylesheets();
    } else if ( k == _qssFile.get() ) {
        appPTR->reloadStylesheets();
    } else if ( k == _hostName.get() ) {
        std::string hostName = _hostName->getActiveEntry().id;
        bool isCustom = hostName == NATRON_CUSTOM_HOST_NAME_ENTRY;
        _customHostName->setSecret(!isCustom);
#ifdef NATRON_USE_BREAKPAD
    } else if ( ( k == _testCrashReportButton.get() ) && (reason == eValueChangedReasonUserEdited) ) {
        StandardButtonEnum reply = Dialogs::questionDialog( tr("Краш тест").toStdString(),
                                                            tr("Вы собираетесь вызвать сбой %1 для проверки системы отчетов.\n"
                                                               "Вы действительно хотите разбиться?").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString(), false,
                                                            StandardButtons(eStandardButtonYes | eStandardButtonNo) );
        if (reply == eStandardButtonYes) {
            crash_application();
        }
#endif
    } else if ( ( k == _scriptEditorFontChoice.get() ) || ( k == _scriptEditorFontSize.get() ) ) {
        appPTR->reloadScriptEditorFonts();
    } else if ( k == _pluginUseImageCopyForSource.get() ) {
        appPTR->setPluginsUseInputImageCopyToRender( _pluginUseImageCopyForSource->getValue() );
    } else if ( k == _enableOpenGL.get() ) {
        appPTR->refreshOpenGLRenderingFlagOnAllInstances();
        if (!_restoringSettings) {
            appPTR->clearPluginsLoadedCache();
        }
    } else {
        ret = false;
    }
    if (ret) {
        if ( ( ( k == _hostName.get() ) || ( k == _customHostName.get() ) ) && !_restoringSettings ) {
            Dialogs::warningDialog( tr("Изменение имени хоста").toStdString(), tr("Для изменения этого параметра требуется перезапуск %1 и очистка кэша загрузки подключаемых модулей OpenFX из меню Кэш.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).toStdString() );
        }
    }

    return ret;
} // onKnobValueChanged

////////////////////////////////////////////////////////
// "Viewers" pane

ImageBitDepthEnum
Settings::getViewersBitDepth() const
{
    if (!appPTR->isTextureFloatSupported()) {
        return eImageBitDepthByte;
    }
    int v = _texturesMode->getValue();

    if (v == 0) {
        return eImageBitDepthByte;
    } else if (v == 1) {
        return eImageBitDepthFloat;
    } else {
        return eImageBitDepthByte;
    }
}

int
Settings::getViewerTilesPowerOf2() const
{
    return _powerOf2Tiling->getValue();
}

int
Settings::getCheckerboardTileSize() const
{
    return _checkerboardTileSize->getValue();
}

void
Settings::getCheckerboardColor1(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _checkerboardColor1->getValue(0);
    *g = _checkerboardColor1->getValue(1);
    *b = _checkerboardColor1->getValue(2);
    *a = _checkerboardColor1->getValue(3);
}

void
Settings::getCheckerboardColor2(double* r,
                                double* g,
                                double* b,
                                double* a) const
{
    *r = _checkerboardColor2->getValue(0);
    *g = _checkerboardColor2->getValue(1);
    *b = _checkerboardColor2->getValue(2);
    *a = _checkerboardColor2->getValue(3);
}

bool
Settings::isAutoWipeEnabled() const
{
    return _autoWipe->getValue();
}

bool
Settings::isAutoProxyEnabled() const
{
    return _autoProxyWhenScrubbingTimeline->getValue();
}

unsigned int
Settings::getAutoProxyMipmapLevel() const
{
    return (unsigned int)_autoProxyLevel->getValue() + 1;
}

int
Settings::getMaxOpenedNodesViewerContext() const
{
    return _maximumNodeViewerUIOpened->getValue();
}

bool
Settings::viewerNumberKeys() const
{
    return _viewerNumberKeys->getValue();
}

bool
Settings::viewerOverlaysPath() const
{
    return _viewerOverlaysPath->getValue();
}

///////////////////////////////////////////////////////
// "Caching" pane

bool
Settings::isAggressiveCachingEnabled() const
{
    return _aggressiveCaching->getValue();
}

double
Settings::getRamMaximumPercent() const
{
    return (double)_maxRAMPercent->getValue() / 100.;
}

U64
Settings::getMaximumViewerDiskCacheSize() const
{
    return (U64)( _maxViewerDiskCacheGB->getValue() ) * 1024 * 1024 * 1024;
}

U64
Settings::getMaximumDiskCacheNodeSize() const
{
    return (U64)( _maxDiskCacheNodeGB->getValue() ) * 1024 * 1024 * 1024;
}

///////////////////////////////////////////////////

double
Settings::getUnreachableRamPercent() const
{
    return (double)_unreachableRAMPercent->getValue() / 100.;
}

bool
Settings::getColorPickerLinear() const
{
    return _linearPickers->getValue();
}

int
Settings::getNumberOfThreadsPerEffect() const
{
    return _nThreadsPerEffect->getValue();
}

int
Settings::getNumberOfThreads() const
{
    return _numberOfThreads->getValue();
}

void
Settings::setNumberOfThreads(int threadsNb)
{
    _numberOfThreads->setValue(threadsNb);
}

bool
Settings::isAutoPreviewOnForNewProjects() const
{
    return _autoPreviewEnabledForNewProjects->getValue();
}

#ifdef NATRON_DOCUMENTATION_ONLINE
int
Settings::getDocumentationSource() const
{
    return _documentationSource->getValue();
}
#endif

int
Settings::getServerPort() const
{
    return _wwwServerPort->getValue();
}

void
Settings::setServerPort(int port) const
{
    _wwwServerPort->setValue(port);
}

bool
Settings::isAutoScrollEnabled() const
{
    return _autoScroll->getValue();
}

QString
Settings::makeHTMLDocumentation(bool genHTML) const
{
    QString ret;
    QString markdown;
    QTextStream ts(&ret);
    QTextStream ms(&markdown);

    ms << ( QString::fromUtf8("<!--") + tr("Не редактируйте этот файл! Он генерируется автоматически самим %1.").arg ( QString::fromUtf8( NATRON_APPLICATION_NAME) ) + QString::fromUtf8(" -->\n\n") );
    ms << tr("Предпочтения") << "\n==========\n\n";

    const KnobsVec& knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->getDefaultIsSecret() ) {
            continue;
        }
        //QString knobScriptName = QString::fromUtf8( (*it)->getName().c_str() );
        QString knobLabel = convertFromPlainTextToMarkdown( QString::fromStdString( (*it)->getLabel() ), genHTML, false );
        QString knobHint = convertFromPlainTextToMarkdown( QString::fromStdString( (*it)->getHintToolTip() ), genHTML, false );
        KnobPage* isPage = dynamic_cast<KnobPage*>( it->get() );
        KnobSeparator* isSep = dynamic_cast<KnobSeparator*>( it->get() );
        if (isPage) {
            if (isPage->getParentKnob()) {
                ms << "### " << knobLabel << "\n\n";
            } else {
                ms << knobLabel << "\n----------\n\n";
            }
        } else if (isSep) {
            ms << "**" << knobLabel << "**\n\n";
        } else if ( !knobLabel.isEmpty() && !knobHint.isEmpty() ) {
            if ( ( knobLabel != QString::fromUtf8("Enabled") ) && ( knobLabel != QString::fromUtf8("Zoom support") ) ) {
                ms << "**" << knobLabel << "**\n\n";
                ms << knobHint << "\n\n";
            }
        }
    }

    if (genHTML) {
        QString prefs_head =
        QString::fromUtf8(
        "<!DOCTYPE html>\n"
        "<!--[if IE 8]><html class=\"no-js lt-ie9\" lang=\"en\" > <![endif]-->\n"
        "<!--[if gt IE 8]><!--> <html class=\"no-js\" lang=\"en\" > <!--<![endif]-->\n"
        "<head>\n"
        "<meta charset=\"utf-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<title>Preferences &mdash; NATRON_DOCUMENTATION</title>\n"
        "<script type=\"text/javascript\" src=\"_static/js/modernizr.min.js\"></script>\n"
        "<script type=\"text/javascript\" id=\"documentation_options\" data-url_root=\"./\" src=\"_static/documentation_options.js\"></script>\n"
        "<script src=\"_static/jquery.js\"></script>\n"
        "<script src=\"_static/underscore.js\"></script>\n"
        "<script src=\"_static/doctools.js\"></script>\n"
        "<script src=\"_static/language_data.js\"></script>\n"
        "<script async=\"async\" src=\"https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/latest.js?config=TeX-AMS-MML_HTMLorMML\"></script>\n"
        "<script type=\"text/javascript\" src=\"_static/js/theme.js\"></script>\n"
        "<link rel=\"stylesheet\" href=\"_static/css/theme.css\" type=\"text/css\" />\n"
        "<link rel=\"stylesheet\" href=\"_static/pygments.css\" type=\"text/css\" />\n"
        "<link rel=\"stylesheet\" href=\"_static/theme_overrides.css\" type=\"text/css\" />\n"
        "<link rel=\"index\" title=\"Index\" href=\"genindex.html\" />\n"
        "<link rel=\"search\" title=\"Search\" href=\"search.html\" />\n"
        "<link rel=\"next\" title=\"Environment Variables\" href=\"_environment.html\" />\n"
        "<link rel=\"prev\" title=\"Reference Guide\" href=\"_group.html\" />\n"
        "</head>\n"
        "<body class=\"wy-body-for-nav\">\n"
        "<div class=\"wy-grid-for-nav\">\n"
        "<nav data-toggle=\"wy-nav-shift\" class=\"wy-nav-side\">\n"
        "<div class=\"wy-side-scroll\">\n"
        "<div class=\"wy-side-nav-search\" >\n"
        "<a href=\"index.html\" class=\"icon icon-home\"> Natron\n"
        "<img src=\"_static/logo.png\" class=\"logo\" alt=\"Logo\"/>\n"
        "</a>\n"
        "<div class=\"version\">\n"
        "2.3\n"
        "</div>\n"
        "<div role=\"search\">\n"
        "<form id=\"rtd-search-form\" class=\"wy-form\" action=\"search.html\" method=\"get\">\n"
        "<input type=\"text\" name=\"q\" placeholder=\"Search docs\" />\n"
        "<input type=\"hidden\" name=\"check_keywords\" value=\"yes\" />\n"
        "<input type=\"hidden\" name=\"area\" value=\"default\" />\n"
        "</form>\n"
        "</div>\n"
        "</div>\n"
        "<div class=\"wy-menu wy-menu-vertical\" data-spy=\"affix\" role=\"navigation\" aria-label=\"main navigation\">\n"
        "<ul class=\"current\">\n"
        "<li class=\"toctree-l1\"><a class=\"reference internal\" href=\"guide/index.html\">User Guide</a></li>\n"
        "<li class=\"toctree-l1 current\"><a class=\"reference internal\" href=\"_group.html\">Reference Guide</a><ul class=\"current\">\n"
        "<li class=\"toctree-l2 current\"><a class=\"current reference internal\" href=\"#\">Preferences</a><ul>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#general\">General</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#threading\">Threading</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#rendering\">Rendering</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#gpu-rendering\">GPU Rendering</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#project-setup\">Project Setup</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#documentation\">Documentation</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#user-interface\">User Interface</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#color-management\">Color Management</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#caching\">Caching</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#viewer\">Viewer</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#nodegraph\">Nodegraph</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#plug-ins\">Plug-ins</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#python\">Python</a></li>\n"
        "<li class=\"toctree-l3\"><a class=\"reference internal\" href=\"#appearance\">Appearance</a><ul>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#main-window\">Main Window</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#curve-editor\">Curve Editor</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#dope-sheet\">Dope Sheet</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#node-graph\">Node Graph</a></li>\n"
        "<li class=\"toctree-l4\"><a class=\"reference internal\" href=\"#script-editor\">Script Editor</a></li>\n"
        "</ul>\n"
        "</li>\n"
        "</ul>\n"
        "</li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_environment.html\">Environment Variables</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupImage.html\">Image nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupDraw.html\">Draw nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupTime.html\">Time nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupChannel.html\">Channel nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupColor.html\">Color nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupFilter.html\">Filter nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupKeyer.html\">Keyer nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupMerge.html\">Merge nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupTransform.html\">Transform nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupViews.html\">Views nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupOther.html\">Other nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupGMIC.html\">GMIC nodes</a></li>\n"
        "<li class=\"toctree-l2\"><a class=\"reference internal\" href=\"_groupExtra.html\">Extra nodes</a></li>\n"
        "</ul>\n"
        "</li>\n"
        "<li class=\"toctree-l1\"><a class=\"reference internal\" href=\"devel/index.html\">Developers Guide</a></li>\n"
        "</ul>\n"
        "</div>\n"
        "</div>\n"
        "</nav>\n"
        "<section data-toggle=\"wy-nav-shift\" class=\"wy-nav-content-wrap\">\n"
        "<nav class=\"wy-nav-top\" aria-label=\"top navigation\">\n"
        "<i data-toggle=\"wy-nav-top\" class=\"fa fa-bars\"></i>\n"
        "<a href=\"index.html\">Natron</a>\n"
        "</nav>\n"
        "<div class=\"wy-nav-content\">\n"
        "<div class=\"rst-content\">\n"
        "<div role=\"navigation\" aria-label=\"breadcrumbs navigation\">\n"
        "<ul class=\"wy-breadcrumbs\">\n"
        "<li><a href=\"index.html\">Docs</a> &raquo;</li>\n"
        "<li><a href=\"_group.html\">Reference Guide</a> &raquo;</li>\n"
        "<li>Preferences</li>\n"
        "<li class=\"wy-breadcrumbs-aside\">\n"
        "<a href=\"_sources/_prefs.rst.txt\" rel=\"nofollow\"> View page source</a>\n"
        "</li>\n"
        "</ul>\n"
        "<hr/>\n"
        "</div>\n"
        "<div role=\"main\" class=\"document\" itemscope=\"itemscope\" itemtype=\"http://schema.org/Article\">\n"
        "<div itemprop=\"articleBody\">\n"
        "<div class=\"section\" id=\"preferences\">\n");

        QString prefs_foot =
        QString::fromUtf8(
        "</div>\n"
        "</div>\n"
        "</div>\n"
        "<footer>\n"
        "<div class=\"rst-footer-buttons\" role=\"navigation\" aria-label=\"footer navigation\">\n"
        "<a href=\"_environment.html\" class=\"btn btn-neutral float-right\" title=\"Environment Variables\" accesskey=\"n\" rel=\"next\">Next <span class=\"fa fa-arrow-circle-right\"></span></a>\n"
        "<a href=\"_group.html\" class=\"btn btn-neutral float-left\" title=\"Reference Guide\" accesskey=\"p\" rel=\"prev\"><span class=\"fa fa-arrow-circle-left\"></span> Previous</a>\n"
        "</div>\n"
        "<hr/>\n"
        "<div role=\"contentinfo\">\n"
        "<p>\n"
        "&copy; Copyright 2013-2023 The Natron documentation authors, licensed under CC BY-SA 4.0\n"
        "</p>\n"
        "</div>\n"
        "Built with <a href=\"http://sphinx-doc.org/\">Sphinx</a> using a <a href=\"https://github.com/rtfd/sphinx_rtd_theme\">theme</a> provided by <a href=\"https://readthedocs.org\">Read the Docs</a>.\n"
        "</footer>\n"
        "</div>\n"
        "</div>\n"
        "</section>\n"
        "</div>\n"
        "<script type=\"text/javascript\">\n"
        "jQuery(function () {\n"
        "SphinxRtdTheme.Navigation.enable(true);\n"
        "});\n"
        "</script>\n"
        "</body>\n"
        "</html>\n");

        ts << prefs_head;
        QString html = Markdown::convert2html(markdown);
        ts << Markdown::fixSettingsHTML(html);
        ts << prefs_foot;
    } else {
        ts << markdown;
    }

    return ret;
} // Settings::makeHTMLDocumentation

void
Settings::populateSystemFonts(const QSettings& settings,
                              const std::vector<std::string>& fonts)
{
    std::vector<ChoiceOption> options(fonts.size());
    for (std::size_t i = 0; i < fonts.size(); ++i) {
        options[i].id = fonts[i];
    }
    _systemFontChoice->populateChoices(options);
    _scriptEditorFontChoice->populateChoices(options);

    for (U32 i = 0; i < fonts.size(); ++i) {
        if (fonts[i] == NATRON_FONT) {
            _systemFontChoice->setDefaultValue(i);
        }
        if (fonts[i] == NATRON_SCRIPT_FONT) {
            _scriptEditorFontChoice->setDefaultValue(i);
        }
    }
    ///Now restore properly the system font choice
    {
        QString name = QString::fromUtf8( _systemFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _systemFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
    {
        QString name = QString::fromUtf8( _scriptEditorFontChoice->getName().c_str() );
        if ( settings.contains(name) ) {
            std::string value = settings.value(name).toString().toStdString();
            for (U32 i = 0; i < fonts.size(); ++i) {
                if (fonts[i] == value) {
                    _scriptEditorFontChoice->setValue(i);
                    break;
                }
            }
        }
    }
}

void
Settings::getOpenFXPluginsSearchPaths(std::list<std::string>* paths) const
{
    assert(paths);
    try {
        _extraPluginPaths->getPaths(paths);
    } catch (std::logic_error&) {
        paths->clear();
    }
}

bool
Settings::getUseStdOFXPluginsLocation() const
{
    return _useStdOFXPluginsLocation->getValue();
}

void
Settings::restoreDefault()
{
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );

    if ( !QFile::remove( settings.fileName() ) ) {
        qDebug() << "Failed to remove settings ( " << settings.fileName() << " ).";
    }

    beginChanges();
    const KnobsVec & knobs = getKnobs();
    for (U32 i = 0; i < knobs.size(); ++i) {
        for (int j = 0; j < knobs[i]->getDimension(); ++j) {
            knobs[i]->resetToDefaultValue(j);
        }
    }
    setCachingLabels();
    endChanges();
}

bool
Settings::isRenderInSeparatedProcessEnabled() const
{
    return _renderInSeparateProcess->getValue();
}

int
Settings::getMaximumUndoRedoNodeGraph() const
{
    return _maxUndoRedoNodeGraph->getValue();
}

int
Settings::getAutoSaveDelayMS() const
{
    return _autoSaveDelay->getValue() * 1000;
}

bool
Settings::isAutoSaveEnabledForUnsavedProjects() const
{
    return _autoSaveUnSavedProjects->getValue();
}

int
Settings::saveVersions() const
{
    return _saveVersions->getValue();
}


bool
Settings::isSnapToNodeEnabled() const
{
    return _snapNodesToConnections->getValue();
}

bool
Settings::isCheckForUpdatesEnabled() const
{
    return _checkForUpdates->getValue();
}

void
Settings::setCheckUpdatesEnabled(bool enabled)
{
    _checkForUpdates->setValue(enabled);
    saveSetting( _checkForUpdates.get() );
}

#ifdef NATRON_USE_BREAKPAD
bool
Settings::isCrashReportingEnabled() const
{
    return _enableCrashReports->getValue();
}
#endif

int
Settings::getMaxPanelsOpened() const
{
    return _maxPanelsOpened->getValue();
}

void
Settings::setMaxPanelsOpened(int maxPanels)
{
    _maxPanelsOpened->setValue(maxPanels);
    saveSetting( _maxPanelsOpened.get() );
}

bool
Settings::loadBundledPlugins() const
{
    return _loadBundledPlugins->getValue();
}

bool
Settings::preferBundledPlugins() const
{
    return _preferBundledPlugins->getValue();
}

void
Settings::getDefaultNodeColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _defaultNodeColor->getValue(0);
    *g = _defaultNodeColor->getValue(1);
    *b = _defaultNodeColor->getValue(2);
}

void
Settings::getDefaultBackdropColor(float *r,
                                  float *g,
                                  float *b) const
{
    *r = _defaultBackdropColor->getValue(0);
    *g = _defaultBackdropColor->getValue(1);
    *b = _defaultBackdropColor->getValue(2);
}

void
Settings::getGeneratorColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultGeneratorColor->getValue(0);
    *g = _defaultGeneratorColor->getValue(1);
    *b = _defaultGeneratorColor->getValue(2);
}

void
Settings::getReaderColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _defaultReaderColor->getValue(0);
    *g = _defaultReaderColor->getValue(1);
    *b = _defaultReaderColor->getValue(2);
}

void
Settings::getWriterColor(float *r,
                         float *g,
                         float *b) const
{
    *r = _defaultWriterColor->getValue(0);
    *g = _defaultWriterColor->getValue(1);
    *b = _defaultWriterColor->getValue(2);
}

void
Settings::getColorGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultColorGroupColor->getValue(0);
    *g = _defaultColorGroupColor->getValue(1);
    *b = _defaultColorGroupColor->getValue(2);
}

void
Settings::getFilterGroupColor(float *r,
                              float *g,
                              float *b) const
{
    *r = _defaultFilterGroupColor->getValue(0);
    *g = _defaultFilterGroupColor->getValue(1);
    *b = _defaultFilterGroupColor->getValue(2);
}

void
Settings::getTransformGroupColor(float *r,
                                 float *g,
                                 float *b) const
{
    *r = _defaultTransformGroupColor->getValue(0);
    *g = _defaultTransformGroupColor->getValue(1);
    *b = _defaultTransformGroupColor->getValue(2);
}

void
Settings::getTimeGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultTimeGroupColor->getValue(0);
    *g = _defaultTimeGroupColor->getValue(1);
    *b = _defaultTimeGroupColor->getValue(2);
}

void
Settings::getDrawGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultDrawGroupColor->getValue(0);
    *g = _defaultDrawGroupColor->getValue(1);
    *b = _defaultDrawGroupColor->getValue(2);
}

void
Settings::getKeyerGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultKeyerGroupColor->getValue(0);
    *g = _defaultKeyerGroupColor->getValue(1);
    *b = _defaultKeyerGroupColor->getValue(2);
}

void
Settings::getChannelGroupColor(float *r,
                               float *g,
                               float *b) const
{
    *r = _defaultChannelGroupColor->getValue(0);
    *g = _defaultChannelGroupColor->getValue(1);
    *b = _defaultChannelGroupColor->getValue(2);
}

void
Settings::getMergeGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultMergeGroupColor->getValue(0);
    *g = _defaultMergeGroupColor->getValue(1);
    *b = _defaultMergeGroupColor->getValue(2);
}

void
Settings::getViewsGroupColor(float *r,
                             float *g,
                             float *b) const
{
    *r = _defaultViewsGroupColor->getValue(0);
    *g = _defaultViewsGroupColor->getValue(1);
    *b = _defaultViewsGroupColor->getValue(2);
}

void
Settings::getDeepGroupColor(float *r,
                            float *g,
                            float *b) const
{
    *r = _defaultDeepGroupColor->getValue(0);
    *g = _defaultDeepGroupColor->getValue(1);
    *b = _defaultDeepGroupColor->getValue(2);
}

int
Settings::getDisconnectedArrowLength() const
{
    return _disconnectedArrowLength->getValue();
}

std::string
Settings::getHostName() const
{
    int entry_i =  _hostName->getValue();
    std::vector<ChoiceOption> entries = _hostName->getEntries_mt_safe();

    if ( (entry_i >= 0) && ( entry_i < (int)entries.size() ) && (entries[entry_i].id == NATRON_CUSTOM_HOST_NAME_ENTRY) ) {
        return _customHostName->getValue();
    } else {
        if ( (entry_i >= 0) && ( entry_i < (int)_knownHostNames.size() ) ) {
            return _knownHostNames[entry_i].id;
        }

        return std::string();
    }
}

bool
Settings::getRenderOnEditingFinishedOnly() const
{
    return _renderOnEditingFinished->getValue();
}

void
Settings::setRenderOnEditingFinishedOnly(bool render)
{
    _renderOnEditingFinished->setValue(render);
}

bool
Settings::getIconsBlackAndWhite() const
{
    return _useBWIcons->getValue();
}

std::string
Settings::getDefaultLayoutFile() const
{
    return _defaultLayoutFile->getValue();
}

bool
Settings::getLoadProjectWorkspce() const
{
    return _loadProjectsWorkspace->getValue();
}

#ifdef Q_OS_WIN
bool
Settings::getEnableConsoleWindow() const
{
    return _enableConsoleWindow->getValue();
}
#endif

bool
Settings::useCursorPositionIncrements() const
{
    return _useCursorPositionIncrements->getValue();
}

bool
Settings::isAutoProjectFormatEnabled() const
{
    return _firstReadSetProjectFormat->getValue();
}

bool
Settings::isAutoFixRelativeFilePathEnabled() const
{
    return _fixPathsOnProjectPathChanged->getValue();
}

int
Settings::getNumberOfParallelRenders() const
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL

    return _numberOfParallelRenders->getValue();
#else

    return 1;
#endif
}

void
Settings::setNumberOfParallelRenders(int nb)
{
#ifndef NATRON_PLAYBACK_USES_THREAD_POOL
    _numberOfParallelRenders->setValue(nb);
#endif
}

bool
Settings::areRGBPixelComponentsSupported() const
{
    return _activateRGBSupport->getValue();
}

bool
Settings::isTransformConcatenationEnabled() const
{
    return _activateTransformConcatenationSupport->getValue();
}

bool
Settings::useGlobalThreadPool() const
{
    return _useThreadPool->getValue();
}

void
Settings::setUseGlobalThreadPool(bool use)
{
    _useThreadPool->setValue(use);
}

bool
Settings::useInputAForMergeAutoConnect() const
{
    return _useInputAForMergeAutoConnect->getValue();
}

void
Settings::doOCIOStartupCheckIfNeeded()
{
    bool docheck = _ocioStartupCheck->getValue();
    AppInstancePtr mainInstance = appPTR->getTopLevelInstance();

    if (!mainInstance) {
        qDebug() << "WARNING: doOCIOStartupCheckIfNeeded() called without a AppInstance";

        return;
    }

    if (docheck && mainInstance) {
        int entry_i = _ocioConfigKnob->getValue();
        std::vector<ChoiceOption> entries = _ocioConfigKnob->getEntries_mt_safe();
        std::string warnText;
        if ( (entry_i < 0) || ( entry_i >= (int)entries.size() ) ) {
            warnText = tr("Текущая конфигурация OCIO, выбранная в настройках, недействительна. Хотите установить для нее конфигурацию по умолчанию (%1)?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else if (entries[entry_i].id != NATRON_DEFAULT_OCIO_CONFIG_NAME) {
            warnText = tr("Текущая конфигурация OCIO, выбранная в настройках, не является конфигурацией по умолчанию (%1). Установить для нее конфигурацию по умолчанию?").arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString();
        } else {
            return;
        }

        bool stopAsking = false;
        StandardButtonEnum reply = mainInstance->questionDialog("OCIO config", warnText, false,
                                                                StandardButtons(eStandardButtonYes | eStandardButtonNo),
                                                                eStandardButtonYes,
                                                                &stopAsking);
        if (stopAsking != !docheck) {
            _ocioStartupCheck->setValue(!stopAsking);
            saveSetting( _ocioStartupCheck.get() );
        }

        if (reply == eStandardButtonYes) {
            int defaultIndex = -1;
            for (unsigned i = 0; i < entries.size(); ++i) {
                if (entries[i].id.find(NATRON_DEFAULT_OCIO_CONFIG_NAME) != std::string::npos) {
                    defaultIndex = i;
                    break;
                }
            }
            if (defaultIndex != -1) {
                _ocioConfigKnob->setValue(defaultIndex);
                saveSetting( _ocioConfigKnob.get() );
            } else {
                Dialogs::warningDialog( "OCIO config", tr("Не удалось найти конфигурацию OCIO %2.\n"
                                                          "Это связано с тем, что вы не используете папку OpenColorIO-Configs, которая должна быть "
                                                          "в комплекте с вашей установкой %1.").arg( QString::fromUtf8(NATRON_APPLICATION_NAME) ).arg( QString::fromUtf8(NATRON_DEFAULT_OCIO_CONFIG_NAME) ).toStdString() );
            }
        }
    }
} // Settings::doOCIOStartupCheckIfNeeded

bool
Settings::didSettingsExistOnStartup() const
{
    return _settingsExisted;
}

bool
Settings::notifyOnFileChange() const
{
    return _notifyOnFileChange->getValue();
}

bool
Settings::isAutoTurboEnabled() const
{
    return _autoTurbo->getValue();
}

void
Settings::setAutoTurboModeEnabled(bool e)
{
    _autoTurbo->setValue(e);
}

void
Settings::setOptionalInputsAutoHidden(bool hidden)
{
    _hideOptionalInputsAutomatically->setValue(hidden);
}

bool
Settings::areOptionalInputsAutoHidden() const
{
    return _hideOptionalInputsAutomatically->getValue();
}

void
Settings::getPythonGroupsSearchPaths(std::list<std::string>* templates) const
{
    _templatesPluginPaths->getPaths(templates);
}

void
Settings::appendPythonGroupsPath(const std::string& path)
{
    _templatesPluginPaths->appendPath(path);
    QSettings settings( QString::fromUtf8(NATRON_ORGANIZATION_NAME), QString::fromUtf8(NATRON_APPLICATION_NAME) );
    settings.setValue( QString::fromUtf8( _templatesPluginPaths->getName().c_str() ), QVariant( QString::fromUtf8( _templatesPluginPaths->getValue(0).c_str() ) ) );
}

std::string
Settings::getDefaultOnProjectLoadedCB()
{
    return _defaultOnProjectLoaded->getValue();
}

std::string
Settings::getDefaultOnProjectSaveCB()
{
    return _defaultOnProjectSave->getValue();
}

std::string
Settings::getDefaultOnProjectCloseCB()
{
    return _defaultOnProjectClose->getValue();
}

std::string
Settings::getDefaultOnNodeCreatedCB()
{
    return _defaultOnNodeCreated->getValue();
}

std::string
Settings::getDefaultOnNodeDeleteCB()
{
    return _defaultOnNodeDelete->getValue();
}

std::string
Settings::getOnProjectCreatedCB()
{
    return _onProjectCreated->getValue();
}

bool
Settings::isLoadFromPyPlugsEnabled() const
{
    return _loadPyPlugsFromPythonScript->getValue();
}

bool
Settings::isAutoDeclaredVariablePrintActivated() const
{
    return _echoVariableDeclarationToPython->getValue();
}

void
Settings::setAutoDeclaredVariablePrintEnabled(bool enabled)
{
    _echoVariableDeclarationToPython->setValue(enabled);
    saveSetting( _echoVariableDeclarationToPython.get() );
}

bool
Settings::isPluginIconActivatedOnNodeGraph() const
{
    return _usePluginIconsInNodeGraph->getValue();
}

bool
Settings::isNodeGraphAntiAliasingEnabled() const
{
    return _useAntiAliasing->getValue();
}

void
Settings::getSunkenColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _sunkenColor->getValue(0);
    *g = _sunkenColor->getValue(1);
    *b = _sunkenColor->getValue(2);
}

void
Settings::getBaseColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _baseColor->getValue(0);
    *g = _baseColor->getValue(1);
    *b = _baseColor->getValue(2);
}

void
Settings::getRaisedColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _raisedColor->getValue(0);
    *g = _raisedColor->getValue(1);
    *b = _raisedColor->getValue(2);
}

void
Settings::getSelectionColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _selectionColor->getValue(0);
    *g = _selectionColor->getValue(1);
    *b = _selectionColor->getValue(2);
}

void
Settings::getInterpolatedColor(double* r,
                               double* g,
                               double* b) const
{
    *r = _interpolatedColor->getValue(0);
    *g = _interpolatedColor->getValue(1);
    *b = _interpolatedColor->getValue(2);
}

void
Settings::getKeyframeColor(double* r,
                           double* g,
                           double* b) const
{
    *r = _keyframeColor->getValue(0);
    *g = _keyframeColor->getValue(1);
    *b = _keyframeColor->getValue(2);
}

void
Settings::getTrackerKeyframeColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _trackerKeyframeColor->getValue(0);
    *g = _trackerKeyframeColor->getValue(1);
    *b = _trackerKeyframeColor->getValue(2);
}

void
Settings::getExprColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _exprColor->getValue(0);
    *g = _exprColor->getValue(1);
    *b = _exprColor->getValue(2);
}

void
Settings::getTextColor(double* r,
                       double* g,
                       double* b) const
{
    *r = _textColor->getValue(0);
    *g = _textColor->getValue(1);
    *b = _textColor->getValue(2);
}

void
Settings::getAltTextColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _altTextColor->getValue(0);
    *g = _altTextColor->getValue(1);
    *b = _altTextColor->getValue(2);
}

void
Settings::getTimelinePlayheadColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _timelinePlayheadColor->getValue(0);
    *g = _timelinePlayheadColor->getValue(1);
    *b = _timelinePlayheadColor->getValue(2);
}

void
Settings::getTimelineBoundsColor(double* r,
                                 double* g,
                                 double* b) const
{
    *r = _timelineBoundsColor->getValue(0);
    *g = _timelineBoundsColor->getValue(1);
    *b = _timelineBoundsColor->getValue(2);
}

void
Settings::getTimelineBGColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _timelineBGColor->getValue(0);
    *g = _timelineBGColor->getValue(1);
    *b = _timelineBGColor->getValue(2);
}

void
Settings::getCachedFrameColor(double* r,
                              double* g,
                              double* b) const
{
    *r = _cachedFrameColor->getValue(0);
    *g = _cachedFrameColor->getValue(1);
    *b = _cachedFrameColor->getValue(2);
}

void
Settings::getDiskCachedColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _diskCachedFrameColor->getValue(0);
    *g = _diskCachedFrameColor->getValue(1);
    *b = _diskCachedFrameColor->getValue(2);
}

void
Settings::getCurveEditorBGColor(double* r,
                                double* g,
                                double* b) const
{
    *r = _curveEditorBGColor->getValue(0);
    *g = _curveEditorBGColor->getValue(1);
    *b = _curveEditorBGColor->getValue(2);
}

void
Settings::getCurveEditorGridColor(double* r,
                                  double* g,
                                  double* b) const
{
    *r = _gridColor->getValue(0);
    *g = _gridColor->getValue(1);
    *b = _gridColor->getValue(2);
}

void
Settings::getCurveEditorScaleColor(double* r,
                                   double* g,
                                   double* b) const
{
    *r = _curveEditorScaleColor->getValue(0);
    *g = _curveEditorScaleColor->getValue(1);
    *b = _curveEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorBackgroundColor(double *r,
                                            double *g,
                                            double *b) const
{
    *r = _dopeSheetEditorBackgroundColor->getValue(0);
    *g = _dopeSheetEditorBackgroundColor->getValue(1);
    *b = _dopeSheetEditorBackgroundColor->getValue(2);
}

void
Settings::getDopeSheetEditorRootRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _dopeSheetEditorRootSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorRootSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorRootSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorRootSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorKnobRowBackgroundColor(double *r,
                                                   double *g,
                                                   double *b,
                                                   double *a) const
{
    *r = _dopeSheetEditorKnobSectionBackgroundColor->getValue(0);
    *g = _dopeSheetEditorKnobSectionBackgroundColor->getValue(1);
    *b = _dopeSheetEditorKnobSectionBackgroundColor->getValue(2);
    *a = _dopeSheetEditorKnobSectionBackgroundColor->getValue(3);
}

void
Settings::getDopeSheetEditorScaleColor(double *r,
                                       double *g,
                                       double *b) const
{
    *r = _dopeSheetEditorScaleColor->getValue(0);
    *g = _dopeSheetEditorScaleColor->getValue(1);
    *b = _dopeSheetEditorScaleColor->getValue(2);
}

void
Settings::getDopeSheetEditorGridColor(double *r,
                                      double *g,
                                      double *b) const
{
    *r = _dopeSheetEditorGridColor->getValue(0);
    *g = _dopeSheetEditorGridColor->getValue(1);
    *b = _dopeSheetEditorGridColor->getValue(2);
}

void
Settings::getSEKeywordColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _keywordColor->getValue(0);
    *g = _keywordColor->getValue(1);
    *b = _keywordColor->getValue(2);
}

void
Settings::getSEOperatorColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _operatorColor->getValue(0);
    *g = _operatorColor->getValue(1);
    *b = _operatorColor->getValue(2);
}

void
Settings::getSEBraceColor(double* r,
                          double* g,
                          double* b) const
{
    *r = _braceColor->getValue(0);
    *g = _braceColor->getValue(1);
    *b = _braceColor->getValue(2);
}

void
Settings::getSEDefClassColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _defClassColor->getValue(0);
    *g = _defClassColor->getValue(1);
    *b = _defClassColor->getValue(2);
}

void
Settings::getSEStringsColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _stringsColor->getValue(0);
    *g = _stringsColor->getValue(1);
    *b = _stringsColor->getValue(2);
}

void
Settings::getSECommentsColor(double* r,
                             double* g,
                             double* b) const
{
    *r = _commentsColor->getValue(0);
    *g = _commentsColor->getValue(1);
    *b = _commentsColor->getValue(2);
}

void
Settings::getSESelfColor(double* r,
                         double* g,
                         double* b) const
{
    *r = _selfColor->getValue(0);
    *g = _selfColor->getValue(1);
    *b = _selfColor->getValue(2);
}

void
Settings::getSENumbersColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _numbersColor->getValue(0);
    *g = _numbersColor->getValue(1);
    *b = _numbersColor->getValue(2);
}

void
Settings::getSECurLineColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _curLineColor->getValue(0);
    *g = _curLineColor->getValue(1);
    *b = _curLineColor->getValue(2);
}

void
Settings::getSliderColor(double* r,
                            double* g,
                            double* b) const
{
    *r = _sliderColor->getValue(0);
    *g = _sliderColor->getValue(1);
    *b = _sliderColor->getValue(2);
}

int
Settings::getSEFontSize() const
{
    return _scriptEditorFontSize->getValue();
}

std::string
Settings::getSEFontFamily() const
{
    return _scriptEditorFontChoice->getActiveEntry().id;
}

void
Settings::getPluginIconFrameColor(int *r,
                                  int *g,
                                  int *b) const
{
    *r = 50;
    *g = 50;
    *b = 50;
}

int
Settings::getDopeSheetEditorNodeSeparationWith() const
{
    return 4;
}

bool
Settings::isNaNHandlingEnabled() const
{
    return _convertNaNValues->getValue();
}

bool
Settings::isCopyInputImageForPluginRenderEnabled() const
{
    return _pluginUseImageCopyForSource->getValue();
}

void
Settings::setOnProjectCreatedCB(const std::string& func)
{
    _onProjectCreated->setValue(func);
}

void
Settings::setOnProjectLoadedCB(const std::string& func)
{
    _defaultOnProjectLoaded->setValue(func);
}

bool
Settings::isDefaultAppearanceOutdated() const
{
    return _defaultAppearanceOutdated;
}

void
Settings::restoreDefaultAppearance()
{
    std::vector<KnobIPtr> children = _appearanceTab->getChildren();

    for (std::size_t i = 0; i < children.size(); ++i) {
        KnobColor* isColorKnob = dynamic_cast<KnobColor*>( children[i].get() );
        if ( isColorKnob && isColorKnob->isSimplified() ) {
            isColorKnob->blockValueChanges();
            for (int j = 0; j < isColorKnob->getDimension(); ++j) {
                isColorKnob->resetToDefaultValue(j);
            }
            isColorKnob->unblockValueChanges();
        }
    }
    _defaultAppearanceOutdated = false;
    appPTR->reloadStylesheets();
}

std::string
Settings::getUserStyleSheetFilePath() const
{
    return _qssFile->getValue();
}

void
Settings::setRenderQueuingEnabled(bool enabled)
{
    _queueRenders->setValue(enabled);
    saveSetting( _queueRenders.get() );
}

bool
Settings::isRenderQueuingEnabled() const
{
    return _queueRenders->getValue();
}

bool
Settings::isFileDialogEnabledForNewWriters() const
{
#ifdef NATRON_ENABLE_IO_META_NODES

    return _filedialogForWriters->getValue();
#else

    return true;
#endif
}

bool
Settings::isDriveLetterToUNCPathConversionEnabled() const
{
    return !_enableMappingFromDriveLettersToUNCShareNames->getValue();
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_Settings.cpp"
