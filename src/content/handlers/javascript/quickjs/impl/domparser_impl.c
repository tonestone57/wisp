#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <strings.h>
#include "quickjs.h"
#include "dom_bridge.h"
#include "qjs_internal.h"
#include <wisp/utils/log.h>
#include <wisp/utils/corestrings.h>
#include "utils/libdom.h"

// From libdom and bindings
#include <dom/dom.h>
#include <dom/core/document.h>
#include <dom/core/element.h>
#include <dom/core/text.h>
#include <dom/core/implementation.h>
#include <libdom/bindings/hubbub/parser.h>
#include <libdom/bindings/xml/xmlparser.h>
#include "JSDOMParser.gen.h"

static void ignore_dom_msg(uint32_t severity, void *ctx, const char *msg, ...)
{
    (void)severity;
    (void)ctx;
    (void)msg;
}

static JSValue parse_xml_to_js_document(JSContext *ctx, const char *input_str)
{
    dom_xml_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_xml_error error;

    parser = dom_xml_parser_create(NULL, NULL, ignore_dom_msg, NULL, &doc);
    if (!parser) {
        return JS_ThrowInternalError(ctx, "DOMParser.parseFromString: Failed to create XML parser");
    }

    size_t len = input_str ? strlen(input_str) : 0;
    error = dom_xml_parser_parse_chunk(parser, (uint8_t *)input_str, len);
    if (error == DOM_XML_OK) {
        error = dom_xml_parser_completed(parser);
    }

    if (error != DOM_XML_OK) {
        // Parsing failed. Conforming to W3C/WHATWG <parsererror> spec:
        // We must not throw an exception on invalid XML markup.
        // We must:
        // 1. Create a valid XML document.
        // 2. Populate it with a <parsererror> root element.
        // 3. Set the namespace of <parsererror> to "http://www.mozilla.org/newlayout/xml/parsererror.xml".
        // 4. Provide error text inside that element.

        if (doc) {
            dom_node_unref((dom_node *)doc);
            doc = NULL;
        }
        dom_xml_parser_destroy(parser);

        // Let's create a fresh, clean XML Document.
        dom_exception err = dom_implementation_create_document(DOM_IMPLEMENTATION_XML, NULL, NULL, NULL, NULL, NULL, &doc);
        if (err != DOM_NO_ERR || !doc) {
            return JS_ThrowInternalError(ctx, "DOMParser.parseFromString: Failed to create parsererror document");
        }

        // Programmatically create "parsererror" element
        dom_string *ns_str = NULL;
        dom_string *qname_str = NULL;
        dom_exception ns_err = dom_string_create_interned((const uint8_t *)"http://www.mozilla.org/newlayout/xml/parsererror.xml", 52, &ns_str);
        dom_exception qname_err = dom_string_create_interned((const uint8_t *)"parsererror", 11, &qname_str);

        dom_element *error_element = NULL;
        if (ns_err == DOM_NO_ERR && qname_err == DOM_NO_ERR) {
            dom_document_create_element_ns(doc, ns_str, qname_str, &error_element);
        }
        if (ns_str) dom_string_unref(ns_str);
        if (qname_str) dom_string_unref(qname_str);

        if (error_element) {
            // Append error text description as a text node child
            const char *msg = "XML parsing error.";
            dom_string *text_str = NULL;
            dom_string_create((const uint8_t *)msg, strlen(msg), &text_str);
            if (text_str) {
                dom_text *text_node = NULL;
                dom_document_create_text_node(doc, text_str, &text_node);
                if (text_node) {
                    dom_node *inserted = NULL;
                    dom_node_append_child((dom_node *)error_element, (dom_node *)text_node, &inserted);
                    if (inserted) dom_node_unref(inserted);
                    dom_node_unref((dom_node *)text_node);
                }
                dom_string_unref(text_str);
            }

            // Append error_element to document
            dom_node *inserted_elem = NULL;
            dom_node_append_child((dom_node *)doc, (dom_node *)error_element, &inserted_elem);
            if (inserted_elem) dom_node_unref(inserted_elem);
            dom_node_unref((dom_node *)error_element);
        }

        struct jsthread *t = JS_GetContextOpaque(ctx);
        if (t && t->win_priv && t->doc_priv && t->win_priv != t->doc_priv && corestring_dom___ns_key_html_content_data) {
            dom_node_set_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, t->doc_priv, NULL, NULL);
        }

        JSValue wrap = qjs_wrap_node(ctx, (dom_node *)doc);
        dom_node_unref((dom_node *)doc);
        return wrap;
    }

    // Success!
    // Increment reference count to take ownership before destroying parser
    dom_node_ref((dom_node *)doc);
    dom_xml_parser_destroy(parser);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->win_priv && t->doc_priv && t->win_priv != t->doc_priv && corestring_dom___ns_key_html_content_data) {
        dom_node_set_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, t->doc_priv, NULL, NULL);
    }

    JSValue wrap = qjs_wrap_node(ctx, (dom_node *)doc);
    dom_node_unref((dom_node *)doc);
    return wrap;
}

static JSValue parse_html_to_js_document(JSContext *ctx, const char *input_str)
{
    dom_hubbub_parser_params parse_params;
    memset(&parse_params, 0, sizeof(parse_params));
    parse_params.enc = "UTF-8";
    parse_params.fix_enc = true;
    parse_params.enable_script = false; // spec compliance: script flag is completely disabled
    parse_params.idname = corestring_dom_id;
    parse_params.msg = ignore_dom_msg;

    dom_hubbub_parser *parser = NULL;
    dom_document *doc = NULL;
    dom_hubbub_error err = dom_hubbub_parser_create(&parse_params, &parser, &doc);
    if (err != DOM_HUBBUB_OK || !doc) {
        return JS_ThrowInternalError(ctx, "DOMParser.parseFromString: Failed to create HTML parser");
    }

    size_t len = input_str ? strlen(input_str) : 0;
    err = dom_hubbub_parser_parse_chunk(parser, (const uint8_t *)input_str, len);
    if (err == DOM_HUBBUB_OK) {
        err = dom_hubbub_parser_completed(parser);
    }

    if (err != DOM_HUBBUB_OK) {
        dom_hubbub_parser_destroy(parser);
        if (doc) dom_node_unref((dom_node *)doc);
        return JS_ThrowInternalError(ctx, "DOMParser.parseFromString: HTML parsing failed");
    }

    // Increment reference count to take ownership before destroying parser
    dom_node_ref((dom_node *)doc);
    dom_hubbub_parser_destroy(parser);

    struct jsthread *t = JS_GetContextOpaque(ctx);
    if (t && t->win_priv && t->doc_priv && t->win_priv != t->doc_priv && corestring_dom___ns_key_html_content_data) {
        dom_node_set_user_data((dom_node *)doc, corestring_dom___ns_key_html_content_data, t->doc_priv, NULL, NULL);
    }

    JSValue wrap = qjs_wrap_node(ctx, (dom_node *)doc);
    dom_node_unref((dom_node *)doc);
    return wrap;
}

JSValue wisp_domparser_constructor_impl(JSContext *ctx)
{
    return qjs_new_domparser(ctx, NULL, false);
}

JSValue wisp_domparser_parseFromString_impl(JSContext *ctx, QJSNodePrivate *priv, const char * str, JSValue type)
{
    (void)priv;
    const char *mime_type = JS_ToCString(ctx, type);
    if (!mime_type) {
        return JS_ThrowTypeError(ctx, "DOMParser.parseFromString: invalid MIME type");
    }

    JSValue result;
    if (strcmp(mime_type, "text/html") == 0) {
        result = parse_html_to_js_document(ctx, str);
    } else if (strcmp(mime_type, "text/xml") == 0 ||
               strcmp(mime_type, "application/xml") == 0 ||
               strcmp(mime_type, "application/xhtml+xml") == 0 ||
               strcmp(mime_type, "image/svg+xml") == 0) {
        result = parse_xml_to_js_document(ctx, str);
    } else {
        JS_FreeCString(ctx, mime_type);
        return JS_ThrowTypeError(ctx, "DOMParser.parseFromString: Unsupported MIME type");
    }

    JS_FreeCString(ctx, mime_type);
    return result;
}
