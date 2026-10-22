#include "diagnosticswindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Legatto Device Studio"));
    app.setApplicationVersion(QStringLiteral(LEGATTO_VERSION));
    app.setOrganizationName(QStringLiteral("Legatto2"));
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Read-only Android device diagnostics. No commands run until you choose a tool and start an operation."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(app);
    DiagnosticsWindow window;
    window.show();
    return app.exec();
}
