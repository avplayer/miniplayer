#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "miniplayer/qt/QmlMiniPlayer.hpp"
#include "miniplayer/qt/QmlVideoSurface.hpp"

using namespace miniplayer;

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QmlMiniPlayer::init();

    qmlRegisterType<QmlMiniPlayer>("IPTV", 1, 0, "MiniPlayer");
    qmlRegisterType<QmlVideoSurface>("IPTV", 1, 0, "VideoSurface");
    qmlRegisterType<QmlDumpInfo>("IPTV", 1, 0, "DumpInfo");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));

    int ret = app.exec();

    QmlMiniPlayer::uninit();

    return ret;
}
