/*
 * Copyright 2011 John-Mark Bell <jmb@netsurf-browser.org>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#include <wisp/content/content_protected.h>
#include <wisp/content/llcache.h>
#include <wisp/content.h>
#include <wisp/plotters.h>
#include <wisp/desktop/gui_internal.h>
#include <wisp/bitmap.h>
#include <wisp/audio.h>
#include <wisp/utils/log.h>
#include <wisp/utils/utils.h>
#include "utils/http/parameter.h"
#include "content/content_factory.h"

#include "content/handlers/image/video.h"

#define VIDEO_BUFFER_SIZE (4096 * 1024)

typedef struct nsvideo_content {
    struct content base;

    AVFormatContext *format_ctx;
    AVCodecContext *video_codec_ctx;
    AVCodecContext *audio_codec_ctx;
    int video_stream_idx;
    int audio_stream_idx;

    AVIOContext *avio_ctx;
    unsigned char *avio_buffer;

    struct {
        unsigned char *data;
        size_t size;
        size_t capacity;
        size_t pos;
        pthread_mutex_t lock;
    } buffer;

    pthread_t decode_thread;
    bool decoding;
    bool abort;
    bool paused;

    float volume;
    double video_clock; /* PTS of last decoded video frame */
    double audio_clock; /* PTS of last played audio sample */
    int64_t start_time;

    bool seek_requested;
    double seek_time;

    void *current_bitmap;
    pthread_mutex_t bitmap_lock;
    bool mutexes_initialized;
    bool thread_created;
} nsvideo_content;

static int nsvideo_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    nsvideo_content *video = (nsvideo_content *)opaque;
    int read_size = 0;

    pthread_mutex_lock(&video->buffer.lock);

    while (video->buffer.pos >= video->buffer.size && !video->abort && video->decoding) {
        pthread_mutex_unlock(&video->buffer.lock);
        usleep(10000);
        pthread_mutex_lock(&video->buffer.lock);
    }

    if (video->abort || !video->decoding) {
        pthread_mutex_unlock(&video->buffer.lock);
        return AVERROR_EOF;
    }

    read_size = (int)(video->buffer.size - video->buffer.pos);
    if (read_size > buf_size)
        read_size = buf_size;

    memcpy(buf, video->buffer.data + video->buffer.pos, read_size);
    video->buffer.pos += (size_t)read_size;

    pthread_mutex_unlock(&video->buffer.lock);

    return read_size;
}

static double get_master_clock(nsvideo_content *video)
{
    if (video->audio_stream_idx != -1) {
        return video->audio_clock;
    }
    return (av_gettime() - video->start_time) / 1000000.0;
}

static int64_t nsvideo_seek(void *opaque, int64_t offset, int whence)
{
    nsvideo_content *video = (nsvideo_content *)opaque;
    int64_t new_pos = -1;

    pthread_mutex_lock(&video->buffer.lock);

    switch (whence) {
    case SEEK_SET:
        new_pos = offset;
        break;
    case SEEK_CUR:
        new_pos = (int64_t)video->buffer.pos + offset;
        break;
    case SEEK_END:
        new_pos = (int64_t)video->buffer.size + offset;
        break;
    case AVSEEK_SIZE:
        new_pos = (int64_t)video->buffer.size;
        pthread_mutex_unlock(&video->buffer.lock);
        return new_pos;
    }

    if (new_pos < 0 || new_pos > (int64_t)video->buffer.size) {
        pthread_mutex_unlock(&video->buffer.lock);
        return -1;
    }

    video->buffer.pos = (size_t)new_pos;
    pthread_mutex_unlock(&video->buffer.lock);

    return new_pos;
}

static void *nsvideo_decode_loop(void *arg)
{
    nsvideo_content *video = (nsvideo_content *)arg;
    AVPacket *packet = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    struct SwsContext *sws_ctx = NULL;
    int ret;

    if (avformat_open_input(&video->format_ctx, NULL, NULL, NULL) < 0) {
        NSLOG(wisp, ERROR, "FFmpeg: Failed to open input");
        goto cleanup;
    }

    if (avformat_find_stream_info(video->format_ctx, NULL) < 0) {
        NSLOG(wisp, ERROR, "FFmpeg: Failed to find stream info");
        goto cleanup;
    }

    video->video_stream_idx = -1;
    video->audio_stream_idx = -1;
    for (unsigned int i = 0; i < video->format_ctx->nb_streams; i++) {
        if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video->video_stream_idx == -1) {
            video->video_stream_idx = (int)i;
        } else if (video->format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && video->audio_stream_idx == -1) {
            video->audio_stream_idx = (int)i;
        }
    }

    if (video->video_stream_idx != -1) {
        const AVCodec *vcodec = avcodec_find_decoder(video->format_ctx->streams[video->video_stream_idx]->codecpar->codec_id);
        if (vcodec) {
            video->video_codec_ctx = avcodec_alloc_context3(vcodec);
            avcodec_parameters_to_context(video->video_codec_ctx, video->format_ctx->streams[video->video_stream_idx]->codecpar);
            avcodec_open2(video->video_codec_ctx, vcodec, NULL);
        }
    }

    struct SwrContext *swr_ctx = NULL;
    if (video->audio_stream_idx != -1) {
        const AVCodec *acodec = avcodec_find_decoder(video->format_ctx->streams[video->audio_stream_idx]->codecpar->codec_id);
        if (acodec) {
            video->audio_codec_ctx = avcodec_alloc_context3(acodec);
            avcodec_parameters_to_context(video->audio_codec_ctx, video->format_ctx->streams[video->audio_stream_idx]->codecpar);
            avcodec_open2(video->audio_codec_ctx, acodec, NULL);

            if (guit->audio) {
                AVChannelLayout out_ch_layout = AV_CHANNEL_LAYOUT_STEREO;
                swr_alloc_set_opts2(&swr_ctx, &out_ch_layout, AV_SAMPLE_FMT_FLT, video->audio_codec_ctx->sample_rate,
                                   &video->audio_codec_ctx->ch_layout, video->audio_codec_ctx->sample_fmt, video->audio_codec_ctx->sample_rate, 0, NULL);
                swr_init(swr_ctx);
                guit->audio->init(video->audio_codec_ctx->sample_rate, 2);
            }
        }
    }

    video->start_time = av_gettime();

    while (!video->abort) {
        if (video->paused) {
            usleep(10000);
            continue;
        }

        ret = av_read_frame(video->format_ctx, packet);
        if (ret < 0) break;

        if (packet->stream_index == video->video_stream_idx && video->video_codec_ctx) {
            ret = avcodec_send_packet(video->video_codec_ctx, packet);
            if (ret >= 0) {
                while (avcodec_receive_frame(video->video_codec_ctx, frame) >= 0) {
                    double pts = frame->best_effort_timestamp * av_q2d(video->format_ctx->streams[video->video_stream_idx]->time_base);

                    /* Sync logic against master clock */
                    double master_clock = get_master_clock(video);
                    double diff = pts - master_clock;

                    if (diff > 0.01) {
                        av_usleep((unsigned int)(diff * 1000000.0));
                    } else if (diff < -0.1) {
                        /* Video too slow, skip frame */
                        continue;
                    }

                    video->video_clock = pts;

                    pthread_mutex_lock(&video->bitmap_lock);
                    if (video->current_bitmap == NULL) {
                        video->current_bitmap = guit->bitmap->create(frame->width, frame->height, BITMAP_OPAQUE);
                    }

                    if (video->current_bitmap != NULL) {
                        enum bitmap_layout layout = BITMAP_LAYOUT_ARGB8888;
#ifdef __HAIKU__
                        layout = BITMAP_LAYOUT_B8G8R8A8;
#elif defined(WIN32)
                        layout = BITMAP_LAYOUT_B8G8R8A8;
#endif
                        enum AVPixelFormat dst_pix_fmt = (layout == BITMAP_LAYOUT_ARGB8888) ? AV_PIX_FMT_RGBA : AV_PIX_FMT_BGRA;

                        sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height, (enum AVPixelFormat)frame->format,
                            frame->width, frame->height, dst_pix_fmt, SWS_BILINEAR, NULL, NULL, NULL);

                        uint8_t *dst_data[4] = { (uint8_t *)guit->bitmap->get_buffer(video->current_bitmap), NULL, NULL, NULL };
                        int dst_linesize[4] = { (int)guit->bitmap->get_rowstride(video->current_bitmap), 0, 0, 0 };

                        sws_scale(sws_ctx, (const uint8_t *const *)frame->data, frame->linesize, 0, frame->height, dst_data, dst_linesize);
                        guit->bitmap->modified(video->current_bitmap);
                    }
                    pthread_mutex_unlock(&video->bitmap_lock);

                    /* Trigger redraw of the content */
                    content__request_redraw(&video->base, 0, 0, video->base.width, video->base.height);
                }
            }
        } else if (packet->stream_index == video->audio_stream_idx && video->audio_codec_ctx) {
            ret = avcodec_send_packet(video->audio_codec_ctx, packet);
            if (ret >= 0) {
                while (avcodec_receive_frame(video->audio_codec_ctx, frame) >= 0) {
                    if (guit->audio && swr_ctx) {
                        uint8_t *out_data[1];
                        int out_samples = (int)av_rescale_rnd(swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples, video->audio_codec_ctx->sample_rate, frame->sample_rate, AV_ROUND_UP);
                        av_samples_alloc(out_data, NULL, 2, out_samples, AV_SAMPLE_FMT_FLT, 0);
                        int converted = swr_convert(swr_ctx, out_data, out_samples, (const uint8_t **)frame->data, frame->nb_samples);

                        /* Apply software volume scaling (Float) */
                        if (video->volume != 1.0f) {
                            float *samples = (float *)out_data[0];
                            for (int i = 0; i < converted * 2; i++) {
                                samples[i] *= video->volume;
                            }
                        }

                        guit->audio->play(out_data[0], (size_t)converted * 4 * 2);
                        av_freep(&out_data[0]);

                        if (frame->pts != AV_NOPTS_VALUE) {
                            video->audio_clock = frame->pts * av_q2d(video->format_ctx->streams[video->audio_stream_idx]->time_base);
                        } else {
                            video->audio_clock += (double)converted / (double)video->audio_codec_ctx->sample_rate;
                        }
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

cleanup:
    if (sws_ctx) sws_freeContext(sws_ctx);
    if (swr_ctx) swr_free(&swr_ctx);
    if (guit->audio) guit->audio->fini();
    av_frame_free(&frame);
    av_packet_free(&packet);
    video->decoding = false;
    return NULL;
}

static nserror nsvideo_create(const struct content_handler *handler, lwc_string *imime_type, const struct http_parameter *params,
    struct llcache_handle *llcache, const char *fallback_charset, bool quirks, struct content **c)
{
    nsvideo_content *video;
    nserror error;

    video = (nsvideo_content *)calloc(1, sizeof(nsvideo_content));
    if (video == NULL)
        return NSERROR_NOMEM;

    error = content__init(&video->base, handler, imime_type, params, llcache, fallback_charset, quirks);
    if (error != NSERROR_OK) {
        free(video);
        return error;
    }

    error = llcache_handle_force_stream(llcache);
    if (error != NSERROR_OK) {
        content_destroy(&video->base);
        return error;
    }

    video->buffer.capacity = VIDEO_BUFFER_SIZE;
    video->buffer.data = (unsigned char *)malloc(video->buffer.capacity);
    if (video->buffer.data == NULL) {
        content_destroy(&video->base);
        return NSERROR_NOMEM;
    }
    pthread_mutex_init(&video->buffer.lock, NULL);
    pthread_mutex_init(&video->bitmap_lock, NULL);
    video->mutexes_initialized = true;

    video->decoding = true;
    video->volume = 1.0f;
    video->avio_buffer = (unsigned char *)av_malloc(4096);
    video->avio_ctx = avio_alloc_context(video->avio_buffer, 4096, 0, video, nsvideo_read_packet, NULL, nsvideo_seek);
    video->format_ctx = avformat_alloc_context();
    if (video->format_ctx) {
        video->format_ctx->pb = video->avio_ctx;
    }

    if (pthread_create(&video->decode_thread, NULL, nsvideo_decode_loop, video) == 0) {
        video->thread_created = true;
    }

    *c = (struct content *)video;

    return NSERROR_OK;
}

static bool nsvideo_process_data(struct content *c, const char *data, unsigned int size)
{
    nsvideo_content *video = (nsvideo_content *)c;

    pthread_mutex_lock(&video->buffer.lock);
    if (video->buffer.size + size > video->buffer.capacity) {
        video->buffer.capacity = (video->buffer.size + size) * 2;
        video->buffer.data = (unsigned char *)realloc(video->buffer.data, video->buffer.capacity);
    }
    memcpy(video->buffer.data + video->buffer.size, data, size);
    video->buffer.size += size;
    pthread_mutex_unlock(&video->buffer.lock);

    return true;
}

static bool nsvideo_convert(struct content *c)
{
    return true;
}

static void nsvideo_destroy(struct content *c)
{
    nsvideo_content *video = (nsvideo_content *)c;

    video->abort = true;

    if (video->mutexes_initialized) {
        pthread_mutex_lock(&video->buffer.lock);
        video->decoding = false;
        pthread_mutex_unlock(&video->buffer.lock);
    } else {
        video->decoding = false;
    }

    if (video->thread_created) {
        pthread_join(video->decode_thread, NULL);
    }

    if (video->mutexes_initialized) {
        pthread_mutex_destroy(&video->buffer.lock);
        pthread_mutex_destroy(&video->bitmap_lock);
    }

    if (video->video_codec_ctx) avcodec_free_context(&video->video_codec_ctx);
    if (video->audio_codec_ctx) avcodec_free_context(&video->audio_codec_ctx);
    if (video->format_ctx) avformat_close_input(&video->format_ctx);
    if (video->avio_ctx) av_free(video->avio_ctx);
    if (video->avio_buffer) av_free(video->avio_buffer);
    if (video->buffer.data) free(video->buffer.data);
    if (video->current_bitmap) guit->bitmap->destroy(video->current_bitmap);

    free(video);
}

static bool nsvideo_redraw(
    struct content *c, struct content_redraw_data *data, const struct rect *clip, const struct redraw_context *ctx)
{
    nsvideo_content *video = (nsvideo_content *)c;

    pthread_mutex_lock(&video->bitmap_lock);
    if (video->current_bitmap) {
        if (ctx->plot->bitmap)
            ctx->plot->bitmap(ctx, (struct bitmap *)video->current_bitmap, data->x, data->y, data->width, data->height, 0xffffffff, 0);
    }
    pthread_mutex_unlock(&video->bitmap_lock);

    return true;
}

static nserror nsvideo_clone(const struct content *old, struct content **newc)
{
    return NSERROR_CLONE_FAILED;
}

static content_type nsvideo_type(void)
{
    return CONTENT_IMAGE;
}

static void *nsvideo_get_internal(const struct content *c, void *context)
{
    nsvideo_content *video = (nsvideo_content *)c;
    if (context && strcmp((const char *)context, "media_interface") == 0) {
        return video;
    }
    return video->current_bitmap;
}




void nsvideo_play(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    video->paused = false;
    video->start_time = av_gettime() - (int64_t)(video->video_clock * 1000000.0);
}

void nsvideo_pause(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    video->paused = true;
}

void nsvideo_seek_to(struct content *c, double time) {
    nsvideo_content *video = (nsvideo_content *)c;
    video->seek_time = time;
    video->seek_requested = true;
}

void nsvideo_set_volume(struct content *c, float volume) {
    nsvideo_content *video = (nsvideo_content *)c;
    video->volume = volume;
}

double nsvideo_get_duration(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    if (video->format_ctx && video->format_ctx->duration != AV_NOPTS_VALUE)
        return (double)video->format_ctx->duration / AV_TIME_BASE;
    return 0.0;
}

double nsvideo_get_time(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    return video->video_clock;
}

bool nsvideo_is_paused(struct content *c) {
    nsvideo_content *video = (nsvideo_content *)c;
    return video->paused;
}

static const struct content_handler nsvideo_content_handler = {.create = nsvideo_create,
    .process_data = nsvideo_process_data,
    .data_complete = nsvideo_convert,
    .destroy = nsvideo_destroy,
    .redraw = nsvideo_redraw,
    .clone = nsvideo_clone,
    .type = nsvideo_type,
    .get_internal = nsvideo_get_internal,
    .no_share = true};

static const char *nsvideo_types[] = {"video/mp4", "video/webm", "video/ogg", "application/ogg"};

CONTENT_FACTORY_REGISTER_TYPES(nsvideo, nsvideo_types, nsvideo_content_handler);
