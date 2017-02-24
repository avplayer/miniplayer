#include "QmlMiniPlayer.hpp"
#include "QmlVideoSurface.hpp"


QmlMiniPlayer::QmlMiniPlayer(QQuickItem *parent)
    : QObject(parent)
{
    qDebug() << __FUNCTION__;
    qRegisterMetaType<QmlMiniPlayer::State>("State");
    mPlayer = std::make_shared<MiniPlayer>(dynamic_cast<Callback*>(this),&mAudioOutput);
}

QmlMiniPlayer::~QmlMiniPlayer()
{
    qDebug() << __FUNCTION__ << "start";

    while(mAttachedSurfaces.size() > 0)
    {
        auto & surface = *mAttachedSurfaces.begin();
        surface->setSource(nullptr);
    }
    qDebug() << __FUNCTION__ << "end";
}

void QmlMiniPlayer::init()
{
    MiniPlayer::init();
}

void QmlMiniPlayer::uninit()
{
    MiniPlayer::uninit();
}

void QmlMiniPlayer::registerVideoSurface(QmlVideoSurface* videoSurface)
{
    qDebug() << __FUNCTION__;
    const auto & iter =  std::find(mAttachedSurfaces.begin(), mAttachedSurfaces.end(), videoSurface);
    if(iter != mAttachedSurfaces.end())
        return;
    mAttachedSurfaces.push_back(videoSurface);
}

void QmlMiniPlayer::unregisterVideoSurface(QmlVideoSurface* videoSurface)
{
    qDebug() << __FUNCTION__;
    const auto & iter =  std::find(mAttachedSurfaces.begin(), mAttachedSurfaces.end(), videoSurface);
    mAttachedSurfaces.erase(iter);
}


void QmlMiniPlayer::videoFrameUpdated()
{
    std::lock_guard<std::mutex> l(mVideoRenderMutex);
    std::shared_ptr<const QI420VideoFrame> frame = mVideoRenderFrame;
    for (auto& surface : mAttachedSurfaces)
        surface->presentFrame(frame);
}

void QmlMiniPlayer::onVideoRender(AVFrame * avframe)
{
    std::lock_guard<std::mutex> l(mVideoRenderMutex);
    mVideoRenderFrame = std::make_shared<QI420VideoFrame>(avframe);
    QMetaObject::invokeMethod(this, "videoFrameUpdated");
}

void QmlMiniPlayer::onPositionChanged(double pos)
{
    emit positionChanged(pos);
}

void QmlMiniPlayer::onEndReached()
{
    emit endReached();
}

void QmlMiniPlayer::onStateChanged(int state)
{
    emit stateChanged((State)state);
}

double QmlMiniPlayer::position()
{
    return mPlayer->getPosition();
}

void QmlMiniPlayer::setPosition(double pos)
{
    mPlayer->seek(pos);
}

double QmlMiniPlayer::duration()
{
    return mPlayer->getDuration();
}

bool QmlMiniPlayer::seekable()
{
    return mPlayer->isSeekable();
}

QmlMiniPlayer::State QmlMiniPlayer::state()
{
    return static_cast<State>(mPlayer->getState());
}

void QmlMiniPlayer::dump(QmlDumpInfo * info)
{
    mPlayer->dump(&info->data);
}

void QmlMiniPlayer::open(const QString & mediaPath)
{
    mPlayer->open(mediaPath.toStdString());
}

void QmlMiniPlayer::stop()
{
    mPlayer->stop();
}

void QmlMiniPlayer::play()
{
    mPlayer->play();
}

void QmlMiniPlayer::pause()
{
    mPlayer->pause();
}

void QmlMiniPlayer::togglePause()
{
    mPlayer->togglePause();
}

