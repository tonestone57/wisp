#include "quickjs.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/content/csp.h>
#include "content/handlers/html/private.h"
#include <string.h>

static JSValue js_trusted_types_require_trusted_types(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t || !t->doc_priv) {
        return JS_FALSE;
    }
    struct html_content *htmlc = t->doc_priv;
    if (t->win_priv && t->doc_priv && t->win_priv != t->doc_priv) {
        if (htmlc->csp) {
            bool required = csp_require_trusted_types_for_script(htmlc->csp);
            return JS_NewBool(ctx, required);
        }
    }
    return JS_FALSE;
}

static JSValue js_trusted_types_check_policy_allowed(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)
{
    if (argc < 1) return JS_FALSE;
    const char *name = JS_ToCString(ctx, argv[0]);
    if (!name) return JS_FALSE;

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (!t || !t->doc_priv) {
        JS_FreeCString(ctx, name);
        return JS_TRUE; // Assume allowed if no document context
    }
    if (t->win_priv && t->doc_priv && t->win_priv != t->doc_priv) {
        struct html_content *htmlc = t->doc_priv;
        if (htmlc->csp) {
            bool allowed = csp_trusted_types_policy_allowed(htmlc->csp, name);
            JS_FreeCString(ctx, name);
            return JS_NewBool(ctx, allowed);
        }
    }
    JS_FreeCString(ctx, name);
    return JS_TRUE;
}

static const char *trusted_types_js =
"(function() {\n"
"    const privateToken = {};\n"
"    let requiredForTesting = false;\n"
"    let allowedPoliciesForTesting = null;\n"
"\n"
"    globalThis.__trustedTypesSetRequiredForTesting = function(val) {\n"
"        requiredForTesting = !!val;\n"
"    };\n"
"\n"
"    globalThis.__trustedTypesSetAllowedPoliciesForTesting = function(policies) {\n"
"        allowedPoliciesForTesting = policies ? new Set(policies) : null;\n"
"    };\n"
"\n"
"    class TrustedHTML {\n"
"        constructor(value, token) {\n"
"            if (token !== privateToken) throw new TypeError('TrustedHTML constructor is private');\n"
"            this.value = value;\n"
"        }\n"
"        toString() { return this.value; }\n"
"    }\n"
"\n"
"    class TrustedScript {\n"
"        constructor(value, token) {\n"
"            if (token !== privateToken) throw new TypeError('TrustedScript constructor is private');\n"
"            this.value = value;\n"
"        }\n"
"        toString() { return this.value; }\n"
"    }\n"
"\n"
"    class TrustedScriptURL {\n"
"        constructor(value, token) { \n"
"            if (token !== privateToken) throw new TypeError('TrustedScriptURL constructor is private');\n"
"            this.value = value;\n"
"        }\n"
"        toString() { return this.value; }\n"
"    }\n"
"\n"
"    class TrustedTypePolicy {\n"
"        constructor(name, rules, token) {\n"
"            if (token !== privateToken) throw new TypeError('TrustedTypePolicy constructor is private');\n"
"            this.name = name;\n"
"            this.rules = rules;\n"
"        }\n"
"        createHTML(input, ...args) {\n"
"            if (this.rules.createHTML) {\n"
"                const res = this.rules.createHTML(input, ...args);\n"
"                return new TrustedHTML(res, privateToken);\n"
"            }\n"
"            throw new TypeError('Policy ' + this.name + ' does not define createHTML');\n"
"        }\n"
"        createScript(input, ...args) {\n"
"            if (this.rules.createScript) {\n"
"                const res = this.rules.createScript(input, ...args);\n"
"                return new TrustedScript(res, privateToken);\n"
"            }\n"
"            throw new TypeError('Policy ' + this.name + ' does not define createScript');\n"
"        }\n"
"        createScriptURL(input, ...args) {\n"
"            if (this.rules.createScriptURL) {\n"
"                const res = this.rules.createScriptURL(input, ...args);\n"
"                return new TrustedScriptURL(res, privateToken);\n"
"            }\n"
"            throw new TypeError('Policy ' + this.name + ' does not define createScriptURL');\n"
"        }\n"
"    }\n"
"\n"
"    class TrustedTypePolicyFactory {\n"
"        constructor(token) {\n"
"            if (token !== privateToken) throw new TypeError('TrustedTypePolicyFactory constructor is private');\n"
"            this.policies = new Map();\n"
"            this.defaultPolicy = null;\n"
"        }\n"
"        createPolicy(name, rules) {\n"
"            if (allowedPoliciesForTesting !== null) {\n"
"                if (!allowedPoliciesForTesting.has('*') && !allowedPoliciesForTesting.has(name)) {\n"
"                    throw new TypeError('Policy ' + name + ' is not allowed by CSP');\n"
"                }\n"
"            } else if (!__trustedTypesCheckPolicyAllowed(name)) {\n"
"                throw new TypeError('Policy ' + name + ' is not allowed by CSP trusted-types directive');\n"
"            }\n"
"            if (this.policies.has(name)) {\n"
"                throw new TypeError('Policy ' + name + ' already exists');\n"
"            }\n"
"            const policy = new TrustedTypePolicy(name, rules, privateToken);\n"
"            this.policies.set(name, policy);\n"
"            if (name === 'default') {\n"
"                this.defaultPolicy = policy;\n"
"            }\n"
"            return policy;\n"
"        }\n"
"        isHTML(value) {\n"
"            return value instanceof TrustedHTML;\n"
"        }\n"
"        isScript(value) {\n"
"            return value instanceof TrustedScript;\n"
"        }\n"
"        isScriptURL(value) {\n"
"            return value instanceof TrustedScriptURL;\n"
"        }\n"
"        getAttributeType(tagName, attribute, elementNs, attrNs) {\n"
"            return null;\n"
"        }\n"
"        getPropertyType(tagName, property, elementNs) {\n"
"            return null;\n"
"        }\n"
"        get emptyHTML() {\n"
"            return new TrustedHTML('', privateToken);\n"
"        }\n"
"        get emptyScript() {\n"
"            return new TrustedScript('', privateToken);\n"
"        }\n"
"    }\n"
"\n"
"    globalThis.TrustedHTML = TrustedHTML;\n"
"    globalThis.TrustedScript = TrustedScript;\n"
"    globalThis.TrustedScriptURL = TrustedScriptURL;\n"
"    globalThis.TrustedTypePolicy = TrustedTypePolicy;\n"
"    globalThis.TrustedTypePolicyFactory = TrustedTypePolicyFactory;\n"
"    globalThis.trustedTypes = new TrustedTypePolicyFactory(privateToken);\n"
"\n"
"    // Sink wrappers helper function\n"
"    function trustedTypesCheckHTML(value, sinkName) {\n"
"        if (globalThis.trustedTypes.isHTML(value)) {\n"
"            return value.toString();\n"
"        }\n"
"        if (!__trustedTypesRequireTrustedTypes() && !requiredForTesting) {\n"
"            return String(value);\n"
"        }\n"
"        const defaultPolicy = globalThis.trustedTypes.defaultPolicy;\n"
"        if (defaultPolicy) {\n"
"            try {\n"
"                const result = defaultPolicy.createHTML(String(value));\n"
"                if (globalThis.trustedTypes.isHTML(result)) {\n"
"                    return result.toString();\n"
"                }\n"
"            } catch (e) {}\n"
"        }\n"
"        throw new TypeError('This document requires TrustedHTML assignment for ' + sinkName);\n"
"    }\n"
"\n"
"    function trustedTypesCheckScript(value, sinkName) {\n"
"        if (globalThis.trustedTypes.isScript(value)) {\n"
"            return value.toString();\n"
"        }\n"
"        if (!__trustedTypesRequireTrustedTypes() && !requiredForTesting) {\n"
"            return String(value);\n"
"        }\n"
"        const defaultPolicy = globalThis.trustedTypes.defaultPolicy;\n"
"        if (defaultPolicy) {\n"
"            try {\n"
"                const result = defaultPolicy.createScript(String(value));\n"
"                if (globalThis.trustedTypes.isScript(result)) {\n"
"                    return result.toString();\n"
"                }\n"
"            } catch (e) {}\n"
"        }\n"
"        throw new TypeError('This document requires TrustedScript assignment for ' + sinkName);\n"
"    }\n"
"\n"
"    function trustedTypesCheckScriptURL(value, sinkName) {\n"
"        if (globalThis.trustedTypes.isScriptURL(value)) {\n"
"            return value.toString();\n"
"        }\n"
"        if (!__trustedTypesRequireTrustedTypes() && !requiredForTesting) {\n"
"            return String(value);\n"
"        }\n"
"        const defaultPolicy = globalThis.trustedTypes.defaultPolicy;\n"
"        if (defaultPolicy) {\n"
"            try {\n"
"                const result = defaultPolicy.createScriptURL(String(value));\n"
"                if (globalThis.trustedTypes.isScriptURL(result)) {\n"
"                    return result.toString();\n"
"                }\n"
"            } catch (e) {}\n"
"        }\n"
"        throw new TypeError('This document requires TrustedScriptURL assignment for ' + sinkName);\n"
"    }\n"
"\n"
"    // Hook DOM / Sinks\n"
"    // 1. innerHTML, outerHTML, insertAdjacentHTML\n"
"    if (globalThis.Element) {\n"
"        const innerHTMLDescriptor = Object.getOwnPropertyDescriptor(globalThis.Element.prototype, 'innerHTML');\n"
"        if (innerHTMLDescriptor && innerHTMLDescriptor.set) {\n"
"            const origSet = innerHTMLDescriptor.set;\n"
"            Object.defineProperty(globalThis.Element.prototype, 'innerHTML', {\n"
"                get: innerHTMLDescriptor.get,\n"
"                set: function(value) {\n"
"                    const checked = trustedTypesCheckHTML(value, 'Element innerHTML');\n"
"                    origSet.call(this, checked);\n"
"                },\n"
"                configurable: true, \n"
"                enumerable: true\n"
"            });\n"
"        }\n"
"\n"
"        const outerHTMLDescriptor = Object.getOwnPropertyDescriptor(globalThis.Element.prototype, 'outerHTML');\n"
"        if (outerHTMLDescriptor && outerHTMLDescriptor.set) {\n"
"            const origSet = outerHTMLDescriptor.set;\n"
"            Object.defineProperty(globalThis.Element.prototype, 'outerHTML', {\n"
"                get: outerHTMLDescriptor.get,\n"
"                set: function(value) {\n"
"                    const checked = trustedTypesCheckHTML(value, 'Element outerHTML');\n"
"                    origSet.call(this, checked);\n"
"                },\n"
"                configurable: true, \n"
"                enumerable: true\n"
"            });\n"
"        }\n"
"\n"
"        const origInsertAdjacentHTML = globalThis.Element.prototype.insertAdjacentHTML;\n"
"        if (origInsertAdjacentHTML) {\n"
"            globalThis.Element.prototype.insertAdjacentHTML = function(position, text) {\n"
"                const checked = trustedTypesCheckHTML(text, 'Element insertAdjacentHTML');\n"
"                return origInsertAdjacentHTML.call(this, position, checked);\n"
"            };\n"
"        }\n"
"    }\n"
"\n"
"    // 2. HTMLScriptElement text and src\n"
"    if (globalThis.HTMLScriptElement) {\n"
"        const textDescriptor = Object.getOwnPropertyDescriptor(globalThis.HTMLScriptElement.prototype, 'text');\n"
"        if (textDescriptor && textDescriptor.set) {\n"
"            const origSet = textDescriptor.set;\n"
"            Object.defineProperty(globalThis.HTMLScriptElement.prototype, 'text', {\n"
"                get: textDescriptor.get,\n"
"                set: function(value) {\n"
"                    const checked = trustedTypesCheckScript(value, 'HTMLScriptElement text');\n"
"                    origSet.call(this, checked);\n"
"                },\n"
"                configurable: true,\n"
"                enumerable: true\n"
"            });\n"
"        }\n"
"\n"
"        const srcDescriptor = Object.getOwnPropertyDescriptor(globalThis.HTMLScriptElement.prototype, 'src');\n"
"        if (srcDescriptor && srcDescriptor.set) {\n"
"            const origSet = srcDescriptor.set;\n"
"            Object.defineProperty(globalThis.HTMLScriptElement.prototype, 'src', {\n"
"                get: srcDescriptor.get,\n"
"                set: function(value) {\n"
"                    const checked = trustedTypesCheckScriptURL(value, 'HTMLScriptElement src');\n"
"                    origSet.call(this, checked);\n"
"                },\n"
"                configurable: true,\n"
"                enumerable: true\n"
"            });\n"
"        }\n"
"    }\n"
"\n"
"    // 3. Document write and writeln\n"
"    if (globalThis.Document) {\n"
"        const origWrite = globalThis.Document.prototype.write;\n"
"        if (origWrite) {\n"
"            globalThis.Document.prototype.write = function(...text) {\n"
"                const checked = text.map(t => trustedTypesCheckHTML(t, 'Document write'));\n"
"                return origWrite.apply(this, checked);\n"
"            };\n"
"        }\n"
"        const origWriteln = globalThis.Document.prototype.writeln;\n"
"        if (origWriteln) {\n"
"            globalThis.Document.prototype.writeln = function(...text) {\n"
"                const checked = text.map(t => trustedTypesCheckHTML(t, 'Document writeln'));\n"
"                return origWriteln.apply(this, checked);\n"
"            };\n"
"        }\n"
"    }\n"
"\n"
"    // 4. eval, Function, setTimeout, setInterval\n"
"    const origEval = globalThis.eval;\n"
"    if (origEval) {\n"
"        globalThis.eval = function(code) {\n"
"            const checked = trustedTypesCheckScript(code, 'eval');\n"
"            return origEval.call(this, String(checked));\n"
"        };\n"
"    }\n"
"\n"
"    const origFunction = globalThis.Function;\n"
"    if (origFunction) {\n"
"        const HookedFunction = function(...args) {\n"
"            if (args.length > 0) {\n"
"                const body = args[args.length - 1];\n"
"                const checked = trustedTypesCheckScript(body, 'Function constructor');\n"
"                args[args.length - 1] = String(checked);\n"
"            }\n"
"            return origFunction.apply(this, args);\n"
"        };\n"
"        HookedFunction.prototype = origFunction.prototype;\n"
"        globalThis.Function = HookedFunction;\n"
"    }\n"
"\n"
"    const origSetTimeout = globalThis.setTimeout;\n"
"    if (origSetTimeout) {\n"
"        globalThis.setTimeout = function(handler, timeout, ...args) {\n"
"            if (typeof handler === 'string' || (handler && typeof handler.toString === 'function' && !globalThis.trustedTypes.isHTML(handler) && !globalThis.trustedTypes.isScript(handler) && !globalThis.trustedTypes.isScriptURL(handler))) {\n"
"                const checked = trustedTypesCheckScript(handler, 'setTimeout');\n"
"                const fn = new Function(String(checked));\n"
"                return origSetTimeout.call(this, fn, timeout, ...args);\n"
"            }\n"
"            return origSetTimeout.call(this, handler, timeout, ...args);\n"
"        };\n"
"    }\n"
"\n"
"    const origSetInterval = globalThis.setInterval;\n"
"    if (origSetInterval) {\n"
"        globalThis.setInterval = function(handler, timeout, ...args) {\n"
"            if (typeof handler === 'string' || (handler && typeof handler.toString === 'function' && !globalThis.trustedTypes.isHTML(handler) && !globalThis.trustedTypes.isScript(handler) && !globalThis.trustedTypes.isScriptURL(handler))) {\n"
"                const checked = trustedTypesCheckScript(handler, 'setInterval');\n"
"                const fn = new Function(String(checked));\n"
"                return origSetInterval.call(this, fn, timeout, ...args);\n"
"            }\n"
"            return origSetInterval.call(this, handler, timeout, ...args);\n"
"        };\n"
"    }\n"
"})();";

int qjs_init_trusted_types(JSContext *ctx)
{
    JSValue global_obj = JS_GetGlobalObject(ctx);

    /* Check if already initialized */
    JSValue check = JS_GetPropertyStr(ctx, global_obj, "__wisp_trusted_types_init");
    if (JS_ToBool(ctx, check)) {
        JS_FreeValue(ctx, check);
        JS_FreeValue(ctx, global_obj);
        return 0;
    }
    JS_FreeValue(ctx, check);

    JS_SetPropertyStr(ctx, global_obj, "__trustedTypesRequireTrustedTypes", JS_NewCFunction(ctx, js_trusted_types_require_trusted_types, "__trustedTypesRequireTrustedTypes", 0));
    JS_SetPropertyStr(ctx, global_obj, "__trustedTypesCheckPolicyAllowed", JS_NewCFunction(ctx, js_trusted_types_check_policy_allowed, "__trustedTypesCheckPolicyAllowed", 1));

    JS_FreeValue(ctx, global_obj);

    /* Evaluate JS library */
    JSValue res = JS_Eval(ctx, trusted_types_js, strlen(trusted_types_js), "<trusted-types>", JS_EVAL_TYPE_GLOBAL);
    if (JS_IsException(res)) {
        JSValue exc = JS_GetException(ctx);
        const char *exc_str = JS_ToCString(ctx, exc);
        NSLOG(wisp, ERROR, "Failed to initialize Trusted Types JS library: %s", exc_str ? exc_str : "unknown error");
        if (exc_str) JS_FreeCString(ctx, exc_str);
        JS_FreeValue(ctx, exc);
        JS_FreeValue(ctx, res);
        return -1;
    }
    JS_FreeValue(ctx, res);

    /* Mark as initialized */
    global_obj = JS_GetGlobalObject(ctx);
    JS_DefinePropertyValueStr(ctx, global_obj, "__wisp_trusted_types_init", JS_TRUE, 0);
    JS_FreeValue(ctx, global_obj);

    return 0;
}
