#include <check.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "content/handlers/javascript/js.h"
#include "content/handlers/javascript/quickjs/dom_bridge.h"
#include "quickjs.h"
#include "utils/libdom.h"

int main() {
    corestrings_init();
    js_initialise();
    jsheap *heap;
    js_newheap(5, &heap);
    jsthread *thread;

    dom_document *doc;
    dom_exception exc = dom_implementation_create_document(DOM_IMPLEMENTATION_HTML, NULL, NULL, NULL, NULL, NULL, &doc);
    if (exc != DOM_NO_ERR) { printf("Failed to create document: %d\n", exc); return 1; }

    js_newthread(heap, NULL, doc, &thread);

    const char *code = "console.log('document:', document); 1";
    bool result = js_exec(thread, (const uint8_t *)code, strlen(code), "test");
    printf("Result: %d\n", result);

    js_destroythread(thread);
    dom_node_unref(doc);
    js_destroyheap(heap);
    js_finalise();
    corestrings_fini();
    return result ? 0 : 1;
}
