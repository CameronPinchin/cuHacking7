/*
 * Source file for connecting to a target drone, capturing frames, and then dumping them to /data/share/model_output
 * Cameron Pinchin <cwpinchin@outlook.com> Fisher Walsh <fisher-walsh-email>
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    time_t last_packet_time;
    int timeout_seconds;
} TimeoutContext;

static int interrupt_cb(void *ctx)
{
    TimeoutContext *timeout_ctx = (TimeoutContext *)ctx;
    if(time(NULL) - timeout_ctx->last_packet_time > timeout_ctx->timeout_seconds){
        fprintf(stderr, "[ERROR] Network timeout reached. Interuptting...\n");
        return 1;
    }
    return 0;
}

int main()
{
    const char *drone_url = "udp://172.19.10.1:8080";
    AVFormatContext *format_ctx = NULL;

    format_ctx = avformat_alloc_context();
    TimeoutContext timeout_ctx = { .last_packet_time = time(NULL), .timeout_seconds = 3};
    format_ctx->interrupt_callback.callback = interrupt_cb;
    format_ctx->interrupt_callback.opaque = &timeout_ctx;

    AVDictionary *options = NULL;
    av_dict_set(&options, "fflags", "nobuffer", 0);
    av_dict_set(&options, "flags", "low_delay", 0);

    if(avformat_open_input(&format_ctx, drone_url, NULL, &options) < 0){
        fprintf(stderr, "[ERROR] Could not open UDP stream.\n");
        av_dict_free(&options);
        return EXIT_FAILURE;
    }
    av_dict_free(&options);

    if(avformat_find_stream_info(format_ctx, NULL) < 0){
        fprintf(stderr, "[ERROR] Could not identify stream deatils.\n");
        avformat_close_input(&format_ctx);
        return EXIT_FAILURE;
    }

    int video_stream_idx = -1;
    for(unsigned int i = 0; i < format_ctx->nb_streams; i++){
        if(format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            video_stream_idx = i;
            break;
        }
    }

    if(video_stream_idx == -1){
        fprintf(stderr, "[ERROR] No video stream found. \n");
        avformat_close_input(&format_ctx);
        return EXIT_FAILURE;
    }

    const AVCodec *codec = avcodec_find_decoder(format_ctx->streams[video_stream_idx]->codecpar->codec_id);
    AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codec_ctx, format_ctx->streams[video_stream_idx]->codecpar);
    avcodec_open2(codec_ctx, codec, NULL);

    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();

    struct SwsContext *sws_ctx = NULL;
    AVFrame *resized_frame = av_frame_alloc();
    const int target_width = 448;
    const int target_height = 512;

    resized_frame->width = target_width;
    resized_frame->height = target_height;
    resized_frame->format = AV_PIX_FMT_YUV420P;

    av_frame_get_buffer(resized_frame, 0);


    while(1){
        // resets timeout clock before a blocking call
        timeout_ctx.last_packet_time = time(NULL);

        if(av_read_frame(format_ctx, packet) < 0){
            break;
        }

        if(packet->stream_index == video_stream_idx){
            // send encoded packet to decoder function
            if(avcodec_send_packet(codec_ctx, packet) >= 0){
                while(avcodec_receive_frame(codec_ctx, frame) == 0){
                    fprintf(stdout, "Captured Frame! Resolution: %dx%d,  Format: %d\n",
                            frame->width, frame->height, frame->format);

                    sws_ctx = sws_getCachedContext(
                        sws_ctx,
                        frame->width, frame->height, frame->format,
                        512, 448, resized_frame->format,
                        SWS_BILINEAR, NULL, NULL, NULL
                    );

                    if(sws_ctx){
                        sws_scale(
                            sws_ctx,
                            (const uint8_t *const *)frame->data, frame->linesize,
                                  0, frame->height,
                                  resized_frame->data, resized_frame->linesize
                        );
                        fprintf(stdout, "[SUCCESS] Downsized frame to: %dx%d\n", resized_frame->height, resized_frame->width);
                    }


                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(packet);
    }

    // Free resources
    sws_freeContext(sws_ctx);
    av_frame_free(&resized_frame);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec_ctx);
    avformat_close_input(&format_ctx);

    return 0;
}
