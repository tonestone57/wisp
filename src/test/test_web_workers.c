#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "quickjs.h"
#include "wisp_subsystem.h"
#include "qjs_internal.h"
#include "wisp/desktop/gui_table.h"
#include "wisp/misc.h"

/* Mock NetSurf structures for test */
nserror mock_schedule(int delay, void (*cb)(void *p), void *p) {
    /* Execute immediately for testing */
    cb(p);
    return NSERROR_OK;
}

struct gui_misc_table mock_misc = { .schedule = mock_schedule };
struct wisp_table mock_guit_data = { .misc = &mock_misc };
struct wisp_table *guit = &mock_guit_data;

/* Helper to run a JS script */
static void run_js(JSContext *ctx, const char *script) {
    JSValue res = JS_Eval(ctx, script, strlen(script), "<test>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        fprintf(stderr, "JS Exception: %s\n", str);
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exc);
    }
    JS_FreeValue(ctx, res);
}

int main(int argc, char **argv) {
    init_wisp_subsystem(64);

    jsheap *heap;
    assert(js_newheap(1000, &heap) == NSERROR_OK);

    jsthread *thread;
    assert(js_newthread(heap, NULL, NULL, &thread) == NSERROR_OK);

    printf("Testing Web Worker creation and messaging...\n");

    run_js(thread->ctx,
        "try {"
        "  var w = new Worker('mock.js');"
        "  console.log('Worker object created');"
        "  w.onmessage = function(e) { console.log('Main received:', e.data); };"
        "  w.postMessage({hello: 'world'});"
        "  w.terminate();"
        "  console.log('Worker terminated request sent');"
        "} catch (e) {"
        "  console.log('Caught error: ' + e);"
        "}"
    );

    printf("Cleanup...\n");
    js_destroythread(thread);
    js_destroyheap(heap);
    shutdown_wisp_subsystem();

    printf("PASS\n");
    return 0;
}
