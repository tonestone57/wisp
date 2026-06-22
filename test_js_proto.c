#include <stdio.h>
#include "quickjs.h"

int main() {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue proto = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, proto, "testFunc", JS_NewCFunction(ctx, (JSCFunction *)NULL, "testFunc", 0));

    JS_SetPrototype(ctx, global, proto);

    JSValue res = JS_GetPropertyStr(ctx, global, "testFunc");
    printf("testFunc is function: %d\n", JS_IsFunction(ctx, res));

    JS_FreeValue(ctx, res);
    JS_FreeValue(ctx, proto);
    JS_FreeValue(ctx, global);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
