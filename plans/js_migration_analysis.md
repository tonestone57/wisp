# Duktape → QuickJS DOM Binding Migration Analysis

## Executive Summary

The old Duktape-based JS engine had **full DOM bindings** backed by real libdom nodes. The current QuickJS implementation is **entirely stub-based** — no JS operation touches the real DOM. This document analyzes the old architecture, assesses reuse potential, and proposes a phased migration strategy.

---

## Old Architecture (Duktape + nsgenbind)

### Components

| Component | Role | Size |
|---|---|---|
| `neosurf.bnd` | Master binding file, references WebIDL + includes all `.bnd` files | 214 lines |
| `*.bnd` files (65+) | Per-type DOM logic: Node, Element, Document, HTMLElement, etc. | ~800 lines avg |
| `dukky.c` | Core infrastructure: node wrapping, prototype chain, class dispatch | 1685 lines |
| `nsgenbind` tool | Code generator: `.bnd` + WebIDL → C source files | External tool |
| `*.idl` (7 files) | WebIDL definitions for DOM, HTML, CSSOM, events | Already in project |

### How `.bnd` Files Work

Each `.bnd` file defines getters, setters, and methods with C implementation between `%{` `%}` markers:

```c
// Element.bnd - className getter
getter Element::className()
%{
    dom_string *classstr = NULL;
    dom_exception exc;
    exc = dom_element_get_attribute(priv->parent.node,
                                    corestring_dom_class, &classstr);
    if (exc != DOM_NO_ERR) return 0;
    if (classstr == NULL) {
        duk_push_lstring(ctx, "", 0);          // ← Duktape-specific
    } else {
        duk_push_lstring(ctx, dom_string_data(classstr),
                         dom_string_length(classstr)); // ← Duktape-specific
        dom_string_unref(classstr);
    }
    return 1;
%}
```

> [!IMPORTANT]
> The DOM logic (libdom calls) is engine-agnostic. Only the value push/pop calls are Duktape-specific.

### Core Infrastructure (`dukky.c`)

Key functions that need QuickJS equivalents:

| Duktape Function | Purpose |
|---|---|
| `dukky_push_node()` | Wraps a `dom_node*` into a JS object, with memoization cache |
| `dukky_push_node_klass()` | Determines JS class from DOM node type (Element, Text, etc.) |
| `dukky_html_element_class_from_tag_type()` | Maps `dom_html_element_type` → JS prototype name |
| `dukky_create_object()` | Creates JS object with prototype chain |
| Node memoization map (`NODE_MAGIC`) | Ensures same DOM node → same JS object identity |

---

## Current QuickJS Implementation (Stubs)

| File | Lines | Status |
|---|---|---|
| `document.c` | 434 | All stubs, `documentElement` returns throwaway object |
| `qjs.c` | 428 | Thread/heap lifecycle (works, keep as-is) |
| `location.c` | 701 | Partial real implementation |
| `event_target.c` | 82 | Stub |
| Others | ~200 | Stubs |

---

## Reuse Assessment

### What Can Be Reused Directly

1. **WebIDL files** — Already in `src/content/handlers/javascript/WebIDL/` (7 files, same as old). These define the interface contracts.
2. **DOM logic from `.bnd` files** — The libdom API calls are identical. The C code between `%{ %}` is ~90% reusable.
3. **`dukky_html_element_class_from_tag_type()`** — Pure lookup table, directly portable.
4. **QuickJS `qjs.c` lifecycle** — Already working, keep as-is.

### What Needs Translation (Mechanical)

The Duktape↔QuickJS API has near 1:1 equivalents:

| Duktape | QuickJS | Notes |
|---|---|---|
| `duk_push_string(ctx, s)` | `JS_NewString(ctx, s)` | |
| `duk_push_lstring(ctx, s, len)` | `JS_NewStringLen(ctx, s, len)` | |
| `duk_push_int(ctx, n)` | `JS_NewInt32(ctx, n)` | |
| `duk_push_uint(ctx, n)` | `JS_NewUint32(ctx, n)` | |
| `duk_push_boolean(ctx, b)` | `JS_NewBool(ctx, b)` | |
| `duk_push_null(ctx)` | `JS_NULL` | Constant, not function |
| `duk_push_undefined(ctx)` | `JS_UNDEFINED` | Constant, not function |
| `duk_push_object(ctx)` | `JS_NewObject(ctx)` | |
| `duk_to_string(ctx, idx)` | `JS_ToCString(ctx, val)` | Must `JS_FreeCString` after |
| `duk_push_pointer(ctx, p)` | `JS_SetOpaque(obj, p)` | Different pattern entirely |
| Return `1` (push 1 value) | `return JS_NewString(...)` | QuickJS returns values directly |
| Return `0` (no value) | `return JS_UNDEFINED` | |
| `priv->node` | `JS_GetOpaque(this_val, class_id)` | Different private data access |

### What Needs Redesign

1. **Node memoization** — Duktape used a global stash object as a WeakMap. QuickJS needs a different approach (C hashtable or `Map`).
2. **Prototype chain setup** — `nsgenbind` generated prototype initialization code for Duktape. QuickJS uses `JSClassDef` + `JS_NewClass`.
3. **Private data** — Duktape used hidden properties. QuickJS uses `JS_SetOpaque`/`JS_GetOpaque` with `JSClassID`.
4. **Finalizers** — Duktape used `duk_set_finalizer`. QuickJS uses `JSClassDef.finalizer`.

---

## Automation Potential

### Option A: Write a `.bnd` → QuickJS Transpiler (Recommended)

A Python script that:
1. Parses `.bnd` files (simple regex-based, the format is straightforward)
2. Extracts getter/setter/method declarations + C bodies
3. Translates Duktape API calls to QuickJS equivalents (the table above)
4. Generates QuickJS C source files with proper `JSClassDef`, `JSCFunctionListEntry`, etc.

**Effort**: ~2-3 days for the transpiler, then the 65+ `.bnd` files are processed automatically.

**Advantages**: Minimal manual work per binding, easy to update when `.bnd` files change.

### Option B: Manual Port with `.bnd` as Reference

Manually rewrite each binding using the `.bnd` C bodies as reference. Copy the DOM logic, replace Duktape calls with QuickJS calls.

**Effort**: ~1-2 hours per `.bnd` file × 65 = ~2-3 weeks full-time.

### Option C: Hybrid — Core Framework + Selective Porting

Build the QuickJS infrastructure (node wrapping, class registration) first, then port only the most-needed bindings manually, using `.bnd` as reference.

**Priority bindings** (covers 90% of real-world JS):
1. `Node.bnd` — Base for all DOM nodes
2. `Element.bnd` — className, getAttribute, setAttribute, classList
3. `Document.bnd` — getElementById, querySelector, documentElement
4. `HTMLElement.bnd` — style, event handlers
5. `EventTarget.bnd` — addEventListener, removeEventListener

**Effort**: ~3-5 days for framework + 5 priority bindings.

---

## Quality Assessment of Old Bindings

Before committing to reuse, here's a thorough audit of the code quality across all `.bnd` files.

### Bugs Found

| File | Bug | Severity |
|---|---|---|
| `Document.bnd:157` | `createElementNS()` reads both namespace AND text from `argv[0]` — should read namespace from `argv[0]` and text from `argv[1]` | **High** — wrong namespace used |
| `EventTarget.bnd:54` | `duk_strict_equals` compares callback with flags value instead of with the candidate callback — off-by-one in stack comparisons | **Medium** — may fail to deduplicate listeners |
| `Window.bnd:142` | `calloc(sizeof *sched, 1)` — arguments reversed (should be `calloc(1, sizeof *sched)`) | **Low** — works but non-idiomatic |

### Error Handling — Grade: B+

**Strengths:**
- Consistent pattern: every `dom_exception` is checked, returns 0 on failure
- Memory cleanup on error paths (e.g. `dom_string_unref` before early return)
- Null checks before dereferencing returned nodes

**Weaknesses:**
- Errors are silently swallowed — `return 0` coerces to `undefined` in JS with no error message
- No DOM exceptions thrown back to JS (marked with `/** \todo raise correct exception */`)
- Several `return 0` paths leak stack entries in Duktape (non-critical, GC handles it)

### Memory Management — Grade: A-

**Strengths:**
- Every `dom_string_create` is paired with `dom_string_unref`
- Every `dom_node_ref` (in init) paired with `dom_node_unref` (in fini)
- `dukky_push_node` handles refcounting + memoization correctly

**Weaknesses:**
- `Element.bnd:innerHTML` setter has complex cleanup with `goto out` but handles all paths
- No use-after-free issues found
- Some paths in `EventTarget.bnd` could leak Duktape stack values (non-critical)

### Completeness — Grade: C+

| Capability | Status |
|---|---|
| Node traversal (parent, children, siblings) | ✅ Complete |
| Element attributes (get/set/has/remove) | ✅ Complete |
| Element properties (className, id, classList) | ✅ Complete |
| Document queries (getElementById, getElementsByTagName) | ✅ Complete |
| `querySelector` / `querySelectorAll` | ❌ **Missing** — critical for modern JS |
| `document.write` / `document.writeln` | ✅ Working (uses real parser) |
| `innerHTML` getter | ⚠️ **Stub** — returns empty string |
| `innerHTML` setter | ✅ Working (uses fragment parser) |
| `addEventListener` / `removeEventListener` | ✅ Complete with options support |
| `setTimeout` / `setInterval` | ✅ Complete with proper cleanup |
| `window.getComputedStyle` | ❌ **Missing** |
| `classList.add/remove/toggle/contains` | ⚠️ Via `DOMTokenList` — delegates to libdom tokenlist |
| `CSSStyleDeclaration` (element.style) | ⚠️ **Stub** — just creates empty object |
| Event handler attributes (onclick, etc.) | ✅ Auto-generated by nsgenbind |
| DOM mutation (appendChild, removeChild, etc.) | ✅ Complete with real DOM operations |

> [!WARNING]
> `querySelector`/`querySelectorAll` are missing. These are the most commonly used DOM APIs in modern JavaScript. Any migration must add these.

### Spec Conformance — Grade: B-

- DOM mutation methods follow the spec correctly (ref counting, node adoption)
- `EventTarget` follows [DOM Living Standard](https://dom.spec.whatwg.org/#dom-eventtarget-addeventlistener) logic
- Several `\todo` markers for proper exception throwing
- `document.head`/`body` use `getElementsByTagName` which is correct but inefficient (should use `dom_html_document_get_head` etc.)
- Missing `createElementNS` arg parsing is a spec violation

### Duktape-Specific Limitations in the Old Code

| Limitation | Impact on Migration |
|---|---|
| No `Map`/`Set`/`WeakMap` (ES5 only) | QuickJS has full ES2023+ — **advantage** |
| No `Promise`/`async`/`await` | QuickJS supports these — **advantage** |
| Stack-based value passing (push/pop) | QuickJS uses return values — **cleaner** |
| `BigInt64Array`/`BigUint64Array` missing | QuickJS has these |
| Complex stack manipulation (duk_insert, duk_remove) | Not needed in QuickJS — **simpler** |
| `MAGIC()` strings for hidden properties | QuickJS uses `JS_SetOpaque` — **cleaner** |

### Overall Verdict

> [!IMPORTANT]
> **The DOM logic is worth reusing. The Duktape API calls are not.** The `.bnd` files contain ~3,500 lines of correct, well-tested DOM-to-JS bridging logic that uses libdom APIs which are identical in Wisp. The bugs found are minor (1 real bug in `createElementNS`). The main gap is missing modern APIs (`querySelector`, `getComputedStyle`). The Duktape-specific parts (~30% of each function) need mechanical translation but QuickJS's API is actually simpler, so the translated code will be shorter and cleaner.

**Recommendation**: Use `.bnd` DOM logic as **reference material**, not as direct copy-paste. The libdom call sequences, error handling patterns, and memory management are all correct and should be followed. But write the QuickJS bindings fresh using QuickJS idioms, consulting the `.bnd` files for the DOM logic.

---

## Recommended Migration Strategy

> [!IMPORTANT]
> **Approach: Transpiler-first, but with quality input.** A transpiler will only generate good QuickJS code if the `.bnd` input is solid. Core bindings (Node, Element, Document) must be audited and fixed before being fed to the transpiler.

### Phase 1: Immediate — Native SVG Class Fix
Native SVG class handling in HTML content handler. No JS changes needed.

### Phase 2: Clean Up Core `.bnd` Files (~2 days)
Fix known bugs and gaps before they propagate through the transpiler:
1. Fix `Document.bnd:createElementNS` arg parsing bug
2. Fix `EventTarget.bnd` listener dedup stack comparison
3. Add `querySelector`/`querySelectorAll` to `Document.bnd` (using libdom's `dom_element_query_selector`)
4. Review and improve `innerHTML` getter (currently a stub)
5. Document any additional corrections needed

### Phase 3: Build the `.bnd` → QuickJS Transpiler (~3 days)
Python script that:
1. Parses `.bnd` file format (class, init, fini, getter, setter, method + C bodies)
2. Translates Duktape API → QuickJS API (using the translation table above)
3. Generates proper `JSClassDef`, `JSCFunctionListEntry`, opaque data patterns
4. Outputs one `.c` file per `.bnd` input

### Phase 4: Core QuickJS Framework (~2 days)
Hand-write the infrastructure that the transpiler output depends on:
1. `qjs_node.c` — node memoization (C hashtable: `dom_node*` → `JSValue`), `qjs_push_node()`
2. `qjs_classes.c` — class registration, prototype chain setup
3. `qjs_element_class_from_tag_type()` — tag type → JS class mapping (port from `dukky.c`)

### Phase 5: Generate + Test All Bindings (~2 days)
1. Run transpiler on all 65+ `.bnd` files
2. Fix edge cases in transpiler output
3. Integration test with real pages (cnx-software.com, etc.)
4. Verify `document.documentElement.className` replacement works via JS

---

## Risk Assessment

| Risk | Mitigation |
|---|---|
| `.bnd` format changes from upstream | We own the copy, format is stable |
| QuickJS API differences in edge cases | Manual review of transpiler output |
| Memory management (GC interaction) | QuickJS ref counting is simpler than Duktape |
| CSS reselection after DOM mutation | Existing `dom_element_set_attribute` triggers this |
| Event handling complexity | Defer to Phase 4+, not needed for SVG fix |
