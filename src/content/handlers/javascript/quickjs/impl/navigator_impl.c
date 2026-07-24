#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include <wisp/utils/log.h>
#include "JSNavigator.gen.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>

static JSValue js_navigator_probe_devices(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    bool audio = (argc > 0) ? JS_ToBool(ctx, argv[0]) : false;
    bool video = (argc > 1) ? JS_ToBool(ctx, argv[1]) : false;

    avdevice_register_all();

    bool audio_ok = false;
    bool video_ok = false;

    if (audio) {
        // Probe ALSA or PulseAudio
        const AVInputFormat *alsa_fmt = av_find_input_format("alsa");
        const AVInputFormat *pulse_fmt = av_find_input_format("pulse");
        if (alsa_fmt || pulse_fmt) {
            audio_ok = true; // Capability exists
            // Try to actually open default device if possible
            AVFormatContext *fmt_ctx = NULL;
            const AVInputFormat *use_fmt = pulse_fmt ? pulse_fmt : alsa_fmt;
            if (avformat_open_input(&fmt_ctx, "default", use_fmt, NULL) == 0) {
                avformat_close_input(&fmt_ctx);
            }
        }
    }

    if (video) {
        // Probe V4L2
        const AVInputFormat *v4l2_fmt = av_find_input_format("video4linux2");
        if (!v4l2_fmt) v4l2_fmt = av_find_input_format("v4l2");
        if (v4l2_fmt) {
            video_ok = true; // Capability exists
            // Try to actually open /dev/video0 if possible
            AVFormatContext *fmt_ctx = NULL;
            if (avformat_open_input(&fmt_ctx, "/dev/video0", v4l2_fmt, NULL) == 0) {
                avformat_close_input(&fmt_ctx);
            }
        }
    }

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "audio", JS_NewBool(ctx, audio_ok));
    JS_SetPropertyStr(ctx, obj, "video", JS_NewBool(ctx, video_ok));
    return obj;
}

JSValue wisp_navigator_cookieEnabled_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_TRUE;
}

JSValue wisp_navigator_userAgent_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 Wisp/1.0");
}

JSValue wisp_navigator_appCodeName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Mozilla"); }
JSValue wisp_navigator_appName_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Netscape"); }
JSValue wisp_navigator_appVersion_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "5.0 (Windows)"); }
JSValue wisp_navigator_platform_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Win32"); }
JSValue wisp_navigator_product_get_impl(JSContext *ctx, QJSNodePrivate *priv) { return JS_NewString(ctx, "Gecko"); }

JSValue wisp_navigator_language_get_impl(JSContext *ctx, QJSNodePrivate *priv)
{
    return JS_NewString(ctx, "en-US");
}

int qjs_init_navigator(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized on this global object */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_navigator_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    /* Initialize the class and prototype using the generated function */
    qjs_init_navigator_gen(ctx);
    JSValue navigator = qjs_new_navigator(ctx, NULL, false);
    if (JS_IsException(navigator)) {
        NSLOG(wisp, ERROR, "Failed to create navigator object");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    JS_DefinePropertyValueStr(ctx, navigator, "__probe_devices", JS_NewCFunction(ctx, js_navigator_probe_devices, "__probe_devices", 2), JS_PROP_C_W_E);

    if (JS_DefinePropertyValueStr(ctx, global_obj, "navigator", navigator, JS_PROP_C_W_E) < 0) {
        NSLOG(wisp, ERROR, "Failed to define navigator property");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }

    /* Mark as initialized */
    if (JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_navigator_init", JS_TRUE, 0) < 0) {
        NSLOG(wisp, ERROR, "Failed to define __wisp_navigator_init property");
        JS_FreeValue(ctx, global_obj);
        return -1;
    }
    JS_FreeValue(ctx, global_obj);

    return 0;
}
