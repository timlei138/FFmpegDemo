#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>

#define APP_TAG "ffmpeg-inner"

extern "C" {
#include "libavcodec/version.h"
#include "libavcodec/avcodec.h"
#include "libavformat/version.h"
#include "libavutil/version.h"
#include "libavutil/imgutils.h"
#include "libavfilter/version.h"
#include "libswresample/version.h"
#include "libswscale/version.h"
#include "libswscale/swscale.h"
#include "libavformat/avformat.h"
};

#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE,APP_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,APP_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, APP_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN， APP_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,APP_TAG, __VA_ARGS__)


static ANativeWindow *pNativeWindow;

void swscaleFrame(int videoWidth,int videoHeight,int renderWidth,int renderHeight,enum AVPixelFormat srcFormat,AVFrame *frame);

static void log_callback(void *ptr,int level,const char *fmt,va_list vl){
    char buffer[1024];
    vsprintf(buffer,fmt,vl);
    LOGE("debug level[%d]-%s",level,buffer);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_Player_init(JNIEnv *env, jobject thiz) {
    av_log_set_level(AV_LOG_DEBUG);
    av_log_set_callback(&log_callback);
    LOGD("init is call %d",av_log_get_level());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_myapplication_Player_getVersion(JNIEnv *env, jobject thiz) {
    char strBuffer[1024 * 4] = {0};
    strcat(strBuffer, "libavcodec : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVCODEC_VERSION));
    strcat(strBuffer, "\nlibavformat : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFORMAT_VERSION));
    strcat(strBuffer, "\nlibavutil : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVUTIL_VERSION));
    strcat(strBuffer, "\nlibavfilter : ");
    strcat(strBuffer, AV_STRINGIFY(LIBAVFILTER_VERSION));
    strcat(strBuffer, "\nlibswresample : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWRESAMPLE_VERSION));
    strcat(strBuffer, "\nlibswscale : ");
    strcat(strBuffer, AV_STRINGIFY(LIBSWSCALE_VERSION));
    strcat(strBuffer, "\navcodec_configure : \n");
    strcat(strBuffer, avcodec_configuration());
    strcat(strBuffer, "\navcodec_license : ");
    strcat(strBuffer, avcodec_license());
    return env->NewStringUTF(strBuffer);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_myapplication_Player_playWithSurface(JNIEnv *env, jobject thiz, jstring path) {
    const char* input = env->GetStringUTFChars(path, nullptr);
    LOGD("input file path %s",input);

    AVFormatContext *pFormatCtx;
    pFormatCtx = avformat_alloc_context();

    AVCodecContext *pCodecCtx;
    const AVCodec *avCodec;

    AVPacket *packet;
    AVFrame *frame;
    AVFrame *rgbFrame;
    uint8_t *frameBuffer;
    SwsContext *pSwsCtx;
    ANativeWindow_Buffer rgbWindowBuff;

    int ret = 0;
    if ((ret = avformat_open_input(&pFormatCtx,input,NULL,NULL)) < 0){
        LOGE("Cannot open input file %s \n",input);
        return ret;
    }

    if ((ret = avformat_find_stream_info(pFormatCtx,NULL)) < 0){
        LOGE("Cannot find stream information\n");
        return ret;
    }
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &avCodec, 0);
    if (ret < 0){
        LOGE("annot find a video stream in the input file");
        return ret;
    }

    int video_stream_index = ret;
    LOGD("found video stream index %d",ret);

    AVCodecParameters *codeParams = pFormatCtx->streams[video_stream_index]->codecpar;

    pCodecCtx = avcodec_alloc_context3(avCodec);

    ret = avcodec_parameters_to_context(pCodecCtx,codeParams);

    if (ret != 0){
        LOGE("avcodec_parameters_to_context failed");
        return ret;
    }

    ret = avcodec_open2(pCodecCtx,avCodec,NULL);

    if (ret < 0){
        LOGE("open video failed");
        return ret;
    }

    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    int renderWidth = ANativeWindow_getWidth(pNativeWindow);
    int renderHeight = ANativeWindow_getHeight(pNativeWindow);

    ANativeWindow_setBuffersGeometry(pNativeWindow,videoWidth,videoHeight,WINDOW_FORMAT_RGBA_8888);
    rgbFrame = av_frame_alloc();

    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA ,videoWidth,videoHeight,1);
    frameBuffer = (uint8_t *)av_malloc(bufferSize * sizeof(uint8_t));

    av_image_fill_arrays(rgbFrame->data,rgbFrame->linesize,frameBuffer,AV_PIX_FMT_RGBA,videoWidth,videoHeight,1);

    pSwsCtx = sws_getContext(videoWidth,videoHeight,pCodecCtx->pix_fmt,renderWidth,renderHeight,AV_PIX_FMT_RGBA,SWS_FAST_BILINEAR,NULL,NULL,NULL);

    LOGD("video width %d,%d,renderWidth %d,%d",videoWidth,videoHeight,renderWidth,renderHeight);

    packet = av_packet_alloc();
    frame = av_frame_alloc();
//
    while (av_read_frame(pFormatCtx,packet) >=0 ){
        if (packet->stream_index == video_stream_index){
            if (avcodec_send_packet(pCodecCtx,packet) != 0){
                return -1;
            }
            while (avcodec_receive_frame(pCodecCtx,frame) == 0){
                sws_scale(pSwsCtx,frame->data,frame->linesize,0,videoHeight,rgbFrame->data,rgbFrame->linesize);
                //swscaleFrame(videoWidth,videoHeight,renderWidth,renderHeight,pCodecCtx->pix_fmt,frame);
                ANativeWindow_lock(pNativeWindow,&rgbWindowBuff, nullptr);
                uint8_t *dstBuffer = static_cast<uint8_t *>(rgbWindowBuff.bits);
                int srcLineSize = rgbFrame->linesize[0];
                int dstLineSize = rgbWindowBuff.stride * 4;

                for (int i = 0; i < videoHeight; ++i) {
                    memcpy(dstBuffer + i * dstLineSize,frameBuffer + i * srcLineSize,srcLineSize);
                }
                ANativeWindow_unlockAndPost(pNativeWindow);
            }
        }
        av_packet_unref(packet);
    }


    if (frame != nullptr){
        av_frame_free(&frame);
        frame = nullptr;
    }

    if (packet != nullptr){
        av_packet_free(&packet);
        packet = nullptr;
    }

    if (pCodecCtx != nullptr){
        avcodec_close(pCodecCtx);
        avcodec_free_context(&pCodecCtx);
        pCodecCtx = nullptr;
        avCodec = nullptr;
    }

    if (pFormatCtx != nullptr){
        avformat_close_input(&pFormatCtx);
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }

    if (rgbFrame != nullptr){
        av_frame_free(&rgbFrame);
        rgbFrame = nullptr;
    }

    if (frameBuffer != nullptr){
        free(frameBuffer);
        frameBuffer = nullptr;
    }

    if (pSwsCtx != nullptr){
        sws_freeContext(pSwsCtx);
        pSwsCtx = nullptr;
    }

    if (pNativeWindow){
        ANativeWindow_release(pNativeWindow);
    }


    return ret;
}

//void swscaleFrame(int videoWidth,int videoHeight,int renderWidth,int renderHeight,enum AVPixelFormat srcFormat,AVFrame *frame){
//    AVFrame *rgbFrame = av_frame_alloc();
//    int bufferSize = av_image_get_buffer_size(AV_PIX_FMT_RGBA ,videoWidth,videoHeight,1);
//    uint8_t *frameBuffer = (uint8_t *)av_malloc(bufferSize * sizeof(uint8_t));
//    av_image_fill_arrays(rgbFrame->data,rgbFrame->linesize,frameBuffer,AV_PIX_FMT_RGBA,videoWidth,videoHeight,1);
//    SwsContext *pSwsCtx = sws_getContext(videoWidth,videoHeight,srcFormat,renderWidth,renderHeight,AV_PIX_FMT_RGBA,SWS_FAST_BILINEAR,NULL,NULL,NULL);
//}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_myapplication_Player_surfaceCreated(JNIEnv *env, jobject thiz, jobject surface) {
    pNativeWindow = ANativeWindow_fromSurface(env,surface);
    LOGI("surface created");
}