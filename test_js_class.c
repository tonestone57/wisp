#include <stdio.h>
#include "quickjs.h"

int main() {
    JSRuntime *rt = JS_NewRuntime();
    JSContext *ctx = JS_NewContext(rt);

    JSClassID id;
    JS_NewClassID(rt, &id);
    JSClassDef def = { "Test" };
    JS_NewClass(rt, id, &def);

    JSValue obj = JS_NewObjectClass(ctx, id);
    JSClassID actual_id = JS_GetClassID(obj);
    printf("Class ID: %u\n", actual_id);

    JS_FreeValue(ctx, obj);
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
