#include <jni.h>
#include <string>
#include <android/log.h>


#define TAG "wangyi" // 这个是自定义的LOG的标识
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,TAG ,__VA_ARGS__) // 定义LOGD类型
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG ,__VA_ARGS__) // 定义LOGI类型
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,TAG ,__VA_ARGS__) // 定义LOGW类型
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,TAG ,__VA_ARGS__) // 定义LOGE类型
#define LOGF(...) __android_log_print(ANDROID_LOG_FATAL,TAG ,__VA_ARGS__) // 定义LOGF类型


#define MAX_AUDIO_FRAME_SIZE 48000*4

extern "C"{
    #include "include/libavutil/avutil.h"
    // 封装格式
    #include "include/libavformat/avformat.h"
    //解码
    #include "include/libavcodec/avcodec.h"
    //缩放
    #include "include/libswscale/swscale.h"
    //重采样
   #include "include/libswresample/swresample.h"
}
extern "C" JNIEXPORT jstring JNICALL
Java_com_tianzhuan_musicplayer_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    const char* bytes ="Hello from C++";
    return env->NewStringUTF(av_version_info());
}
extern "C"
JNIEXPORT void JNICALL
Java_com_tianzhuan_musicplayer_WangyiPlayer_sound(JNIEnv *env, jobject instance, jstring inpiut_,
                                                  jstring output_) {
    const char *inpiut = env->GetStringUTFChars(inpiut_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);
//    //初始化网络
    avformat_network_init();
    //总上下文
    AVFormatContext *formatContext=avformat_alloc_context();
    //打开音频文件
    if(avformat_open_input(&formatContext,inpiut,NULL,NULL)!=0){
        LOGI("无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if(avformat_find_stream_info(formatContext,NULL)<0){
        LOGI("无法获取输入文件信息");
        return;
    }
    //找到音频索引
    int audio_stream_idx=-1;
    for (int i = 0; i < formatContext->nb_streams; ++i) {
        if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            audio_stream_idx=i;
            break;
        }
    }
    LOGE("找到音频索引 %d",audio_stream_idx);
    //从音频流中获取解码器参数
    AVCodecParameters *codecpar=formatContext->streams[audio_stream_idx]->codecpar;
    //找到解码器
    AVCodec *dec=avcodec_find_decoder(codecpar->codec_id);
    //创建上下文
    AVCodecContext *codecContext=avcodec_alloc_context3(dec);
    //将解码器参数copy到解码器上下文
    avcodec_parameters_to_context(codecContext,codecpar);
    //打开解码器
    avcodec_open2(codecContext,dec,NULL);
//    //获取音频的转换器上下文
    SwrContext *swrContext=swr_alloc();

    //输入参数是多变的

    //输入采样位数
    AVSampleFormat in_sample= codecContext->sample_fmt;
    //输入采样率
    int in_sample_rate = codecContext->sample_rate;
    //输入声道布局
    uint64_t  in_ch_layout = codecContext->channel_layout;

    //因为都要输出到统一格式的喇叭，所以输出参数是固定的。
    //输出采样位数
    AVSampleFormat  out_sample=AV_SAMPLE_FMT_S16;
    //输出采样率
    int out_sample_rate=44100;
    //输出声道布局
    uint64_t  out_ch_layout = AV_CH_LAYOUT_STEREO;

    //这个转化上下文设置参数（设置转换器的输入参数和输出参数）
    swr_alloc_set_opts(swrContext,out_ch_layout,out_sample,out_sample_rate
            ,in_ch_layout,in_sample,in_sample_rate,0,NULL);
    //初始化转换器其他的默认参数
    swr_init(swrContext);
//
    //创建缓冲区
    uint8_t *out_buffer = (uint8_t *)(av_malloc(2 * 44100 ));
    //打开mp3文件
    FILE *fp_pcm = fopen(output,"wb");

    //声明数据包
    AVPacket *packet=av_packet_alloc();
    int count=0;
//    //从音频流中读取数据包
    while (av_read_frame(formatContext,packet)>=0){
        //从音频流中取出已经解码好的packet
        avcodec_send_packet(codecContext,packet);
        //什么AVFrame frame为未压缩数据
        AVFrame *frame=av_frame_alloc();
        //将AVpacket转化为AVframe
       int ret= avcodec_receive_frame(codecContext,frame);
        LOGE("ret: %d",ret);
       if(ret== AVERROR(EAGAIN)){
           continue;
       } else if(ret < 0){
            LOGE("解码完成");
           break;
       }
       if(packet->stream_index!=audio_stream_idx){//不是音频流
           continue;
       }
       LOGE("正在解码%d",count++);

        //frame --->统一的格式
        swr_convert(swrContext, &out_buffer,2*44100,
                    (const uint8_t **)(frame->data), frame->nb_samples);

        int out_channel_nb=av_get_channel_layout_nb_channels(out_ch_layout);
        //out_buffer -->file（将缓冲区输出到文件中）
        int out_buffer_size=av_samples_get_buffer_size(NULL,out_channel_nb,frame->nb_samples,out_sample,1);
        fwrite(out_buffer,1,out_buffer_size,fp_pcm);


    }
//
    fclose(fp_pcm);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecContext);
    avformat_close_input(&formatContext);

    env->ReleaseStringUTFChars(inpiut_, inpiut);
    env->ReleaseStringUTFChars(output_, output);
}