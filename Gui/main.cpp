
#include <QtCore/QTranslator>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
            // Changes the file loaded from the resource container
            // to force a different language
            if (arg == "--ru") {
                QLocale::setDefault(QLocale::Russian);
                langUpdated = true;
            } else if (arg == "--no-stdout") {
                logToStdout = false;
            } else {
                langUpdated = false;
            }

            if (langUpdated) {
                qDebug() << "Changed locale to " << QLocale::languageToString(QLocale().language());
            } else {
    // Load the translation file from the resource container and use it
    QTranslator translator;
    translator.load(":translations");
    QApplication::installTranslator(&translator);
}
