#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "McapController.h"
#include "McapImageProvider.h"

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setApplicationName("McapViewer");

    McapController controller;
    auto* provider = new McapImageProvider(&controller);  // engine takes ownership

    QQmlApplicationEngine engine;
    engine.addImageProvider("mcap", provider);
    engine.rootContext()->setContextProperty("controller", &controller);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
