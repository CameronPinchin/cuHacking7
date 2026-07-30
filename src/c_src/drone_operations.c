/*
 * Source file for connecting to a target drone, capturing frames, and then dumping them to /data/share/model_output
 * Cameron Pinchin <cwpinchin@outlook.com> Fisher Walsh <fisher-walsh-email>
 */

#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavformat/avcodec.h>

int main()
{
    const char *drone_url = "udp://0.0.0.0:8080";
    AVFormatContext *format_ctx = NULL;

    if(avformat_open_input(&format_ctx, drone_url, NULL, NULL) < 0){
        fprintf(stderr, "[ERROR] Could not open UDP stream.\n");
        return EXIT_FAILURE;
    }

    if(avformat_find_stream_info(format_ctx, NULL) < 0){
        fprintf(stderr, "[ERROR] Could not identify stream deatils.\n");
        return EXIT_FAILURE;
    }

    // At this point, the stream should be open
    av_dump_format(format_ctx, 0, drone_url, 0);

    AVPacket *packet = av_packet_alloc();

    while(av_read_frame(format_ctx, packet) >= 0){
        fprintf(stdout, "Received packet size: %d on stream %d\n", packet->size, packet->stream_index);

        // wipe the payload for the next block
        av_packet_unref(packet);
    }


    av_packet_free(&packet);
    avformat_close_input(&format_ctx);

    return 0;
}
