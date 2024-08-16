#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

int main() {
    const char *filename = "../video.mp4"; // 视频文件路径
    AVFormatContext *fmt_ctx = NULL;
    AVCodecContext *codec_ctx = nullptr;
    const AVCodec *codec = nullptr;
    AVFrame *frame = nullptr;

    // 打开输入文件
    if (avformat_open_input(&fmt_ctx, filename, NULL, NULL) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "无法打开文件 %s\n", filename);
        return -1;
    }

    // 读取流信息
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "无法获取流信息\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 打印格式信息
    av_dump_format(fmt_ctx, 0, filename, 0);

    // 查找视频流
    int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream_index < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "没有找到视频流\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }
    AVCodecParameters *codecpar = fmt_ctx->streams[video_stream_index]->codecpar;

    // 查找解码器
    codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec)
    {
        av_log(NULL, AV_LOG_ERROR, "找不到解码器\n");
        avformat_close_input(&fmt_ctx);
        return -1;
    }

    // 初始化解码器上下文
    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx)
    {
        fprintf(stderr, "Failed to allocate codec context\n");
        return -1;
    }

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0)
    {
        fprintf(stderr, "Failed to copy codec parameters to decoder context\n");
        return -1;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
    {
        fprintf(stderr, "Failed to open codec\n");
        return -1;
    }

    // 读取视频帧
    AVPacket pkt;
    while (av_read_frame(fmt_ctx, &pkt) >= 0)
    {
        if (pkt.stream_index == video_stream_index)
        {
            // 处理视频帧
            // printf("read a frame, size is %d\n", pkt.size);
            if (avcodec_send_packet(codec_ctx, &pkt) >= 0)
            {
                while (avcodec_receive_frame(codec_ctx, frame) >= 0)
                {
                    // 将帧保存为图片

                }
            }
        }
        av_packet_unref(&pkt); // 释放packet
    }

    // 释放资源
    avformat_close_input(&fmt_ctx);
    return 0;
}

void save_frame_as_image(AVFrame *pFrame, int width, int height, int iFrame)
{
    // 创建一个 OpenCV Mat 对象
    cv::Mat img(height, width, CV_8UC3);

    // 将 YUV420P 格式的 AVFrame 转换为 RGB 格式
    struct SwsContext *sws_ctx = sws_getContext(width, height, AV_PIX_FMT_YUV420P,
                                                width, height, AV_PIX_FMT_BGR24,
                                                SWS_BILINEAR, NULL, NULL, NULL);

    uint8_t *data[1] = {img.data};
    int linesize[1] = {static_cast<int>(img.step[0])};

    sws_scale(sws_ctx, pFrame->data, pFrame->linesize, 0, height, data, linesize);

    // 生成文件名
    char filename[32];
    snprintf(filename, sizeof(filename), "frame%d.jpg", iFrame);

    // 保存图片
    cv::imwrite(filename, img);

    // 释放 SwsContext
    sws_freeContext(sws_ctx);
}