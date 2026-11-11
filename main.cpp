#include "mainwindow.h"
#include "debuglog.h"
#include "selftest.h"
#include <QApplication>
#include <QDebug>
#include <QStyleFactory>
#include <string_view>

int main(int argc, char* argv[])
{
    DebugLog::install();
    if (argc == 2 && std::string_view(argv[1]) == "--self-test") {
        QCoreApplication app(argc, argv);
        return run_self_tests();
    }

    QApplication app(argc, argv);
    app.setApplicationName("Контрольная работа № 2");
    app.setStyle(QStyleFactory::create("Fusion"));
    MainWindow window;
    window.show();
    if (app.arguments().size() == 3 && app.arguments()[1] == "--preview-dir") {
        const bool ok = window.verify_and_capture(app.arguments()[2]);
        qInfo() << (ok ? "UI checks passed" : "UI checks failed");
        return ok ? 0 : 1;
    }
    return app.exec();
}
