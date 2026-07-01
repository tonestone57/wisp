# QuickJS-ng WebIDL Bindings in Wisp

This document describes the automated WebIDL binding system used in the Wisp browser.

## Architecture

Wisp uses a custom WebIDL compiler (`utils/qjs_binding_generator.py`) to generate QuickJS-ng (v0.15.1) C bindings. This approach minimizes manual boilerplate and ensures consistent memory management and type safety.

The system is divided into two layers:
1.  **Generated Marshalling Layer**: Automatically handles the QuickJS stack, converting JS types to C types, and vice versa.
2.  **Manual Implementation Layer**: Contains the domain logic (e.g., calls to LibDOM).

### Directory Structure

- `src/content/handlers/javascript/WebIDL/`: Source `.idl` files.
- `utils/qjs_binding_generator.py`: The binding compiler.
- `build/quickjs/`: (Generated) Marshaller code (`JSNode.gen.c`, `JSNode.gen.h`, etc.).
- `src/content/handlers/javascript/quickjs/impl/`: Manual implementations (`node_impl.c`, `mutationobserver_impl.c`, etc.).

## Adding or Modifying Bindings

### 1. Update the WebIDL
Modify or add an `.idl` file in `src/content/handlers/javascript/WebIDL/`.

### 2. Implement the Logic
The generator creates "weak" stubs in `build/quickjs/JSInterface_stubs.gen.c`. You should provide a concrete implementation in the corresponding file within `src/content/handlers/javascript/quickjs/impl/`.

For a method like `Node.appendChild(Node node)`, the generator expects:

```c
JSValue wisp_node_appendChild_impl(JSContext *ctx, QJSNodePrivate *priv, void * node) {
    /* Implementation here */
}
```

### 3. Marshalling Details

- **Private Data**: All DOM objects store a `QJSNodePrivate` pointer as their QuickJS opaque data. Use `qjs_get_dom_priv(val)` to safely retrieve it.
- **Interface Arguments**: Arguments that are interfaces (like `Node`) are automatically converted to `void*` (the `node` pointer inside `QJSNodePrivate`) before being passed to your implementation.
- **Basic Types**: `DOMString` becomes `const char*`, `boolean` becomes `bool`, etc.
- **Variadic Arguments**: Handled as an array/JSValue in the implementation if specified in IDL.

## [Custom] Bindings

If an IDL member is marked with the `[Custom]` attribute, the generator will only produce a call to a custom marshaller that you must implement manually:

```c
/* In your impl file */
JSValue js_interface_method_custom(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
    /* Handle the QuickJS stack manually here */
}
```

## Global Scope

The global object in Wisp QuickJS threads inherits from the `Window` prototype. This means any method defined in `Window.idl` is automatically available in the global scope (e.g., `alert()`, `setTimeout()`).
