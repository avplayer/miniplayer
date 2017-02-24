#include "AudioOutputOpenAL.hpp"
#include <memory>
#include <thread>
#include <QDebug>

std::mutex AudioOutputOpenAL::globalMutex;

#define SCOPE_LOCK_CONTEXT() \
    std::lock_guard<std::mutex> l(globalMutex); \
    Q_UNUSED(l); \
    if (mContext) \
        alcMakeContextCurrent(mContext)

AudioOutputOpenAL::AudioOutputOpenAL() :
    mContext(nullptr),
    mDevice(nullptr),
    mSwrContext(nullptr),
    mSwrTempFrame(nullptr),
    mSwrBuffer(NULL),
    mSwrBufferSize(0),
    mSwrChannelLayout(0),
    mSwrChannels(0),
    mSwrSampleRate(0),
    mSwrFormat(AV_SAMPLE_FMT_NONE),
    mSwrNbSamples(0),
    mALBufferState(0)
{
    qDebug() << __FUNCTION__;
}

AudioOutputOpenAL::~AudioOutputOpenAL()
{
    qDebug() << __FUNCTION__;
    swrFree();
}

bool AudioOutputOpenAL::open(AVFrame *)
{
    qDebug() << __FUNCTION__;
    if (mContext)
        return true;

    bool success = false;

    std::unique_ptr<int, std::function<void (int *)>> scope((int *)1, [&](void*)
    {
        if(success)
            return;
        close();
    });


    const ALCchar* defaultDevice = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    qDebug() << __FUNCTION__ << "OpenAL opening default device:" << defaultDevice;
    mDevice = alcOpenDevice(NULL); //parameter: NULL or default_device
    if (!mDevice)
    {
        qWarning() << __FUNCTION__ << "OpenAL failed to open sound device:" << alcGetString(0, alcGetError(0));
        return false;
    }
    mContext = alcCreateContext(mDevice, NULL);
    SCOPE_LOCK_CONTEXT();
    qDebug() << __FUNCTION__ <<"OpenAL" << alGetString(AL_VERSION) << "vendor:" << alGetString(AL_VENDOR) << "renderer:" << alGetString(AL_RENDERER);
    ALCenum err = alcGetError(mDevice);
    if (err != ALC_NO_ERROR)
    {
        qWarning() << __FUNCTION__ << "AudioOutputOpenAL Error:" << alcGetString(mDevice, err);
        return false;
    }

    qDebug() << __FUNCTION__ << "device:" << mDevice << "context:" << mContext;

    mALFormat = AL_FORMAT_STEREO16;
    mALBufferState = 0;

    alGenBuffers(AL_NUM_BUFFERS, mALBuffers);
    err = alGetError();
    if (err != AL_NO_ERROR)
    {
        qWarning() << __FUNCTION__ << "Failed to generate OpenAL buffers: " << alGetString(err);
        return false;
    }

    alGenSources(1, &mALSource);
    err = alGetError();
    if (err != AL_NO_ERROR)
    {
        qWarning() << __FUNCTION__ << "Failed to generate OpenAL source: " << alGetString(err);
        alDeleteBuffers(AL_NUM_BUFFERS, mALBuffers);
        return false;
    }

    alSourcei(mALSource, AL_LOOPING, AL_FALSE);
    alSourcei(mALSource, AL_SOURCE_RELATIVE, AL_TRUE);
    alSourcei(mALSource, AL_ROLLOFF_FACTOR, 0);
    alSource3f(mALSource, AL_POSITION, 0.0, 0.0, 0.0);
    alSource3f(mALSource, AL_VELOCITY, 0.0, 0.0, 0.0);
    alListener3f(AL_POSITION, 0.0, 0.0, 0.0);

    success = true;
    return true;
}

bool AudioOutputOpenAL::isStopped()
{
    if(!mContext)
        return false;

    SCOPE_LOCK_CONTEXT();
    ALint state = 0;
    alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
    return state == AL_STOPPED;
}

bool AudioOutputOpenAL::stop()
{
    qDebug() << __FUNCTION__;
    if(!mContext)
        return false;

    SCOPE_LOCK_CONTEXT();

    ALint state = 0;
    alSourceStop(mALSource);
    do
    {
        alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING);

    ALint processed = 0;
    alGetSourcei(mALSource, AL_BUFFERS_PROCESSED, &processed);
    ALuint buf;
    while (processed-- > 0)
    {
        alSourceUnqueueBuffers(mALSource, 1, &buf);
    }
    mALBufferState = 0;

    return true;
}

bool AudioOutputOpenAL::close()
{
    qDebug() << __FUNCTION__;
    if(!mContext)
        return true;

    SCOPE_LOCK_CONTEXT();

    mALBufferState = 0;

    ALint state = 0;
    alSourceStop(mALSource);
    do
    {
        alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
    } while (alGetError() == AL_NO_ERROR && state == AL_PLAYING);

    ALint processed = 0;
    alGetSourcei(mALSource, AL_BUFFERS_PROCESSED, &processed);
    ALuint buf;
    while (processed-- > 0)
    {
        alSourceUnqueueBuffers(mALSource, 1, &buf);
    }
    alDeleteSources(1, &mALSource);
    alDeleteBuffers(AL_NUM_BUFFERS, mALBuffers);
    mALBufferState = 0;

    alcMakeContextCurrent(NULL);
    if(mContext)
    {
        alcDestroyContext(mContext);
        mContext = nullptr;
    }
    if(mDevice)
    {
        alcCloseDevice(mDevice);
        mDevice = nullptr;
    }
    swrFree();

    return true;
}

void AudioOutputOpenAL::swrFree()
{
    if(mSwrBuffer)
    {
        av_free(mSwrBuffer);
        mSwrBuffer = NULL;
    }

    if(mSwrTempFrame)
    {
        av_frame_free(&mSwrTempFrame);
        mSwrTempFrame = nullptr;
    }

    if(mSwrContext)
    {
        swr_free(&mSwrContext);
        mSwrContext = nullptr;
    }
    mSwrBufferSize = 0;
    mSwrChannelLayout = 0;
    mSwrChannels = 0;
    mSwrSampleRate = 0;
    mSwrFormat = AV_SAMPLE_FMT_NONE;
    mSwrNbSamples = 0;
}

bool AudioOutputOpenAL::swrConvert(AVFrame * src)
{
    //qDebug() << __FUNCTION__;

    uint64_t channelLayout = src->channel_layout;
    int channels = src->channels;
    if (channels > 2)
    {
        channelLayout = AV_CH_LAYOUT_STEREO;
        channels = 2;
    }

    AVFrame * dst = mSwrTempFrame;

    if(mSwrChannelLayout != channelLayout || mSwrChannels != channels || mSwrSampleRate != src->sample_rate ||
            mSwrFormat != src->format || mSwrNbSamples != src->nb_samples)
    {
        qDebug() << __FUNCTION__ << "reset swr context";
        swrFree();
        mSwrChannelLayout = channelLayout;
        mSwrChannels = channels;
        mSwrSampleRate = src->sample_rate;
        mSwrFormat = (AVSampleFormat) src->format;
        mSwrNbSamples = src->nb_samples;

        dst = mSwrTempFrame = av_frame_alloc();
        mSwrBufferSize = mSwrNbSamples * mSwrChannels * av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        mSwrBuffer = reinterpret_cast<uint8_t*>(av_malloc(mSwrBufferSize));

        dst->nb_samples = mSwrNbSamples;
        dst->format = AV_SAMPLE_FMT_S16;
        dst->channel_layout = mSwrChannelLayout;
        dst->channels = mSwrChannels;
        dst->sample_rate = mSwrSampleRate;
        dst->data[0] = mSwrBuffer;
        dst->data[1] = dst->data[0] + (mSwrBufferSize / mSwrChannels);

        mSwrContext = swr_alloc_set_opts(NULL,
            mSwrChannelLayout, AV_SAMPLE_FMT_S16, mSwrSampleRate,
            mSwrChannelLayout, mSwrFormat, mSwrSampleRate,0, NULL);

        if (swr_init(mSwrContext) < 0)
        {
            qWarning() << __FUNCTION__ << "Cannot create sample rate converter for conversion";
            return false;
        }
    }

    int outLinesize;
    int neededBufferSize = av_samples_get_buffer_size(&outLinesize, src->channels, dst->nb_samples, AV_SAMPLE_FMT_S16, 16);

    int ret = swr_convert(mSwrContext, dst->data, mSwrNbSamples, (const uint8_t**)src->extended_data, src->nb_samples);
    if (ret == mSwrNbSamples)
    {
        ret = avcodec_fill_audio_frame(dst, mSwrChannels, AV_SAMPLE_FMT_S16, mSwrBuffer, neededBufferSize, 1);
        if (ret < 0)
        {
            qWarning() << __FUNCTION__ << "swr_convert() failed";
            return false;
        }
    }

    return true;
}

bool AudioOutputOpenAL::render(AVFrame * avFrame)
{
    //qDebug() << __FUNCTION__;
    if(!mContext)
        return false;

    AVFrame * dstFrame = avFrame;
    AVSampleFormat format = (AVSampleFormat)avFrame->format;
    if(format != AV_SAMPLE_FMT_S16 || avFrame->channels > 2)
    {
        if(swrConvert(avFrame))
            dstFrame = mSwrTempFrame;
        else
            return false;
    }

    if(!(dstFrame->linesize[0] > 0))
    {
        qWarning() << __FUNCTION__ << "linesize:" << dstFrame->linesize[0];
        return false;
    }

    SCOPE_LOCK_CONTEXT();

    //ALint processed,queued;
    //alGetSourcei(mALSource, AL_BUFFERS_QUEUED, &queued);

    ALuint buffer = 0;
    if(mALBufferState < AL_NUM_BUFFERS)
        buffer = mALBuffers[mALBufferState ++];
    else
    {
        ALint processed;
        while(true)
        {
            alGetSourcei(mALSource, AL_BUFFERS_PROCESSED, &processed);
            if(processed > 0)
            {
//                if(processed > 1)
//                    qDebug() << processed;
                alSourceUnqueueBuffers(mALSource, 1, &buffer);
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    //qDebug() << buffer - mALBuffers[0];

    //qDebug() << dstFrame->linesize[0] << buffer << processed << queued;

    alBufferData(buffer, mALFormat, dstFrame->data[0], dstFrame->linesize[0], dstFrame->sample_rate);
    alSourceQueueBuffers(mALSource, 1, &buffer);

    ALCenum err = alGetError();
    if (err != AL_NO_ERROR)
    {
        qWarning() << __FUNCTION__ << "Failed to buffering: " << alGetString(err);
        return false;
    }

    ALint sourceState;
    alGetSourcei(mALSource, AL_SOURCE_STATE, &sourceState);
    if(sourceState != AL_PLAYING)
        alSourcePlay(mALSource);

    return true;
}
