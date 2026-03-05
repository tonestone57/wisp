Wait, the reviewer says: "The logic for addEventListener is incomplete because it fails to call dom_event_target_add_event_listener to register the JS listener with LibDOM... js_fire_event incorrectly invokes dispatchEvent strictly on the global window instead of retrieving the correct JS object for the provided target parameter."

How can we achieve this when we have NO node mapping?
We MUST implement a node mapping.
When `qjs_init_document` is called, it adds `createElement` which creates a JSObject.
But wait! If QuickJS bindings are incomplete and elements are just generic objects, `js_fire_event` receives a `dom_node *target`. If we don't have a map from `dom_node` to `JSValue`, we can't dispatch to it.
Since the QuickJS integration is a **stub** (and there are no real class IDs for Elements), we can maintain a mapping in `jsthread`.
Wait! In QuickJS, `JSValue` doesn't have an address that stays constant. We can't safely use it as a key in a C hash table without `JS_DupValue` and `JS_FreeValue`.
Actually, if we want to add an opaque pointer to an object, we can create a specific `JSClassID` for `Element`!
Let's add `dom_node_class_id` to `qjs.c`!

Wait, `qjs.c` is the entry point.
If we add `JSClassID qjs_dom_node_class_id;` and initialize it, we can create elements as instances of this class and set their opaque pointer to `dom_node *`.
But wait! Where does `js_document_createElement` get the `dom_node *` from?
It doesn't! `js_document_createElement` is just a stub that returns a fake object. It doesn't actually call LibDOM's `dom_document_create_element`!
Ah! If `document.createElement` is just a stub that DOESN'T call LibDOM, then the JS elements HAVE NO LibDOM node!
If JS elements have no LibDOM node, how can `addEventListener` call `dom_event_target_add_event_listener`?
It CANNOT!
If the JS element is a pure JS mock object, we can't register it with LibDOM.

BUT wait, `js_dom_event_add_listener` is called from C when the parser parses `<button onclick="foo()">`. In this case, `js_dom_event_add_listener` is passed the REAL `dom_node *` and the JS function!
And `js_fire_event` is called from C when LibDOM fires an event. It receives the REAL `dom_node *` target.

If `addEventListener` is called from JS on the `window` or `document` object, those DO have underlying C equivalents (`thread->win_priv` and `thread->doc_priv`).
Wait! `thread->doc_priv` is a `struct dom_document *`.
We can register listeners on `doc_priv`!

Let's check what the reviewer exactly said:
"When a script calls addEventListener in JS, it only stores the callback in a hidden __listeners JS object; it never registers this listener with the underlying C DOM (libdom). Consequently, real browser events triggered in C will never fire JS listeners attached via addEventListener."

Wait, what if `addEventListener` is called on the global object (`window`)? The global object is NOT an `Element` but it represents the `window` (or `Document`). We can pass `thread->doc_priv` to `dom_event_target_add_event_listener`.
And for `js_fire_event`, we should find the JS object that corresponds to the `target`. If we don't have a mapping, we can't find it.
Is there a way to map `dom_node` to `JSValue`?
The original `duktape` backend had a mapping.
If I implement a mapping table `dom_node_to_js_object` inside `jsthread`, when we need a JS object for a `dom_node` (like in `js_fire_event`), we can look it up or create a new JS object for it!
Yes! `JSValue js_get_dom_node_object(jsthread *thread, struct dom_node *node)`
It checks the map. If it doesn't exist, it creates a new JS object, sets `__node = node`, and stores it in the map.

Let's do this:
1. In `jsthread`, add `struct qjs_node_map *node_map;`
2. `JSValue qjs_get_node_object(JSContext *ctx, struct dom_node *node)`
   - looks up `node_map`. If found, returns `JS_DupValue(js_obj)`.
   - if not found, creates `JS_NewObject(ctx)`, adds to map.
   - BUT wait, `dom_node` can be Garbage Collected by LibDOM. If we hold a reference, it will leak.
   - For now, just a simple map.

Wait, if we do this, `js_addEventListener` inside `event_target.c` can extract the `dom_node *` from the JS object (e.g. from an opaque pointer, or we just pass the JS object to a `qjs.c` function).
Wait, `event_target.c` is inside `quickjs`. It can call a function in `qjs.c`.
Instead of putting `dom_event_target_add_event_listener` in `js_addEventListener`, we can export a function from `qjs.c` to do it, OR we can just do it in `event_target.c` if we include `libdom.h`.
