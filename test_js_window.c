#include <stdio.h>
#include <string.h>
#include "quickjs.h"

static JSValue window_method(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    printf("Window method called, this_val is undefined: %d\n", JS_IsUndefined(this_val));
    return JS_UNDEFINED;
}

int main() {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "window", JS_DupValue(ctx, global));

    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, proto, "alert", JS_NewCFunction(ctx, window_method, "alert", 1));

    JS_SetPrototype(ctx, global, proto);

    const char *code = "window.alert('hi')";
    JSValue res = JS_Eval(ctx, code, strlen(code), "test", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(ctx);
        const char *str = JS_ToCString(ctx, exc);
        printf("Exception: %s\n", str);
        JS_FreeCString(ctx, str);
        JS_FreeValue(ctx, exc);
    }

    JS_FreeValue(ctx, res);
    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
