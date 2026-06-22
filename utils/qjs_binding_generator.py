#!/usr/bin/env python3
"""
QuickJS-ng Binding Generator for Wisp browser.
Generates an interstitial binding layer between QuickJS and Wisp/LibDOM.
Outputs one file per interface for better parallel build performance.
"""

import os
import sys
import argparse
import re
from datetime import datetime

try:
    import widlparser
except ImportError:
    print("Error: widlparser not installed. Install via: pip install widlparser")
    sys.exit(1)

# C keywords to avoid as variable names
C_KEYWORDS = {
    "default", "case", "switch", "if", "else", "for", "while", "do", "return",
    "break", "continue", "goto", "typedef", "struct", "union", "enum", "sizeof",
    "static", "extern", "register", "auto", "volatile", "const", "inline",
    "int", "char", "float", "double", "long", "short", "signed", "unsigned", "void"
}

def safe_name(name):
    if name in C_KEYWORDS:
        return f"{name}_val"
    return name

# Map WebIDL basic types to C types
TYPE_MAP = {
    'DOMString': 'const char *',
    'boolean': 'bool',
    'unsigned short': 'uint16_t',
    'unsigned long': 'uint32_t',
    'short': 'int16_t',
    'long': 'int32_t',
    'double': 'double',
    'float': 'float',
    'any': 'JSValue',
}

def idl_to_js_type(idl_type):
    t = str(idl_type).strip().rstrip('?')
    if t == 'DOMString':
        return 'string'
    if t == 'boolean':
        return 'bool'
    if t in ['unsigned short', 'unsigned long', 'short', 'long']:
        return 'int'
    if t in ['double', 'float']:
        return 'float'
    return 'value'

def get_actual_type(arg):
    if hasattr(arg, 'type'):
        return str(arg.type).strip().rstrip('?')
    s = str(arg).strip().rstrip('?')
    match = re.match(r'^([A-Za-z0-9_]+)', s)
    if match:
        return match.group(1)
    return "any"

class QuickJSBindingGenerator:
    def __init__(self, output_dir: str):
        self.output_dir = output_dir
        self.parser = widlparser.Parser()
        self.interfaces = {}
        self.mixins = {}
        self.all_interface_names = []
        self.current_impl_signatures = []
        self.current_weak_stubs = []

    def parse_idl(self, idl_path: str):
        with open(idl_path, 'r') as f:
            self.parser.parse(f.read())
        
        for construct in self.parser.constructs:
            if isinstance(construct, widlparser.constructs.Interface):
                if construct.name not in self.interfaces:
                    self.interfaces[construct.name] = construct
                else:
                    # Merge members from partial interface
                    self.interfaces[construct.name].members.extend(construct.members)

                if construct.name not in self.all_interface_names:
                    self.all_interface_names.append(construct.name)
            elif isinstance(construct, widlparser.constructs.ImplementsStatement):
                if construct.name not in self.mixins:
                    self.mixins[construct.name] = []
                self.mixins[construct.name].append(construct.implements)

    def _get_inheritance(self, interface_name):
        interface = self.interfaces.get(interface_name)
        if not interface or not interface.inheritance:
            return None
        return str(interface.inheritance).strip().lstrip(':').strip()

    def _get_members(self, interface):
        name = interface.name
        members_by_name = {} # name -> member_info

        to_process = [interface]
        if name in self.mixins:
            for mixin_name in self.mixins[name]:
                mixin = self.interfaces.get(mixin_name)
                if mixin: to_process.append(mixin)

        for current in to_process:
            for member in current.members:
                m = getattr(member, 'member', None)
                if not m or not hasattr(member, 'name') or not member.name: continue

                custom = False
                if hasattr(m, 'extended_attributes') and m.extended_attributes:
                    for attr in m.extended_attributes:
                        if attr.name == 'Custom':
                            custom = True
                            break

                if isinstance(m, widlparser.productions.Attribute):
                    if member.name not in members_by_name:
                        attr_str = str(m.attribute)
                        members_by_name[member.name] = {
                            'kind': 'attribute',
                            'name': member.name,
                            'readonly': 'readonly' in attr_str,
                            'type': str(m.idl_type).strip(),
                            'custom': custom
                        }
                elif isinstance(m, widlparser.productions.Operation):
                    args = []
                    for arg in m.arguments:
                        args.append({'name': arg.name, 'type': get_actual_type(arg), 'optional': arg.optional})

                    # Group by name, prefer the one with most arguments for the Marshaller
                    if member.name not in members_by_name or members_by_name[member.name]['kind'] != 'operation' or len(args) > len(members_by_name[member.name]['args']):
                         members_by_name[member.name] = {
                            'kind': 'operation',
                            'name': member.name,
                            'args': args,
                            'custom': custom,
                            'return_type': str(m.return_type).strip()
                        }
                elif isinstance(m, widlparser.constructs.Const):
                     if member.name not in members_by_name:
                        members_by_name[member.name] = {
                            'kind': 'const',
                            'name': member.name,
                            'value': str(m.value).strip(),
                            'type': str(m.idl_type).strip()
                        }

        attributes = sorted([m for m in members_by_name.values() if m['kind'] == 'attribute'], key=lambda x: x['name'])
        operations = sorted([m for m in members_by_name.values() if m['kind'] == 'operation'], key=lambda x: x['name'])
        constants = sorted([m for m in members_by_name.values() if m['kind'] == 'const'], key=lambda x: x['name'])
        return attributes, operations, constants

    def generate_marshaller_body(self, interface_name, op):
        lower_name = interface_name.lower()
        if op['custom']:
            return f"    return js_{lower_name}_{op['name']}_custom(ctx, this_val, argc, argv);\n"

        code = f"    QJSNodePrivate *priv = qjs_get_dom_priv(this_val);\n"
        code += f"    if (!priv) return JS_EXCEPTION;\n"

        impl_args = ["priv"]
        for i, arg in enumerate(op['args']):
            js_type = idl_to_js_type(arg['type'])
            arg_name = safe_name(arg['name'])
            if js_type == 'string':
                code += f"    const char *{arg_name} = (argc > {i}) ? JS_ToCString(ctx, argv[{i}]) : NULL;\n"
                impl_args.append(arg_name)
            elif js_type == 'bool':
                code += f"    bool {arg_name} = (argc > {i}) ? JS_ToBool(ctx, argv[{i}]) : false;\n"
                impl_args.append(arg_name)
            elif js_type == 'int':
                code += f"    int32_t {arg_name} = 0; if (argc > {i}) JS_ToInt32(ctx, &{arg_name}, argv[{i}]);\n"
                impl_args.append(arg_name)
            else:
                # Assume it's another interface
                code += f"    QJSNodePrivate *{arg_name}_priv = (argc > {i}) ? qjs_get_dom_priv(argv[{i}]) : NULL;\n"
                code += f"    void *{arg_name} = {arg_name}_priv ? {arg_name}_priv->node : NULL;\n"
                impl_args.append(arg_name)

        impl_func = f"wisp_{lower_name}_{op['name']}_impl"

        sig_args = ["QJSNodePrivate *priv"]
        for arg in op['args']:
            js_type = idl_to_js_type(arg['type'])
            arg_name = safe_name(arg['name'])
            if js_type in ['string', 'bool', 'int']:
                c_type = TYPE_MAP.get(arg['type'], "JSValue")
            else:
                c_type = "void *"
            sig_args.append(f"{c_type} {arg_name}")

        sig = f"JSValue {impl_func}(JSContext *ctx, {', '.join(sig_args)})"
        self.current_impl_signatures.append(sig)

        self.current_weak_stubs.append(f"__attribute__((weak)) JSValue {impl_func}(JSContext *ctx, {', '.join(sig_args)}) {{\n"
                               f"    NSLOG(wisp, WARNING, \"Unimplemented WebIDL method: {interface_name}.{op['name']}\");\n"
                               f"    return JS_UNDEFINED;\n}}")

        code += f"    JSValue ret = {impl_func}(ctx, {', '.join(impl_args)});\n"

        for i, arg in enumerate(op['args']):
            arg_name = safe_name(arg['name'])
            if idl_to_js_type(arg['type']) == 'string':
                code += f"    if ({arg_name}) JS_FreeCString(ctx, {arg_name});\n"

        code += "    return ret;\n"
        return code

    def generate_attr_marshaller(self, interface_name, attr, is_set):
        lower_name = interface_name.lower()
        if attr['custom']:
            suffix = "set" if is_set else "get"
            return f"    return js_{lower_name}_{attr['name']}_{suffix}_custom(ctx, this_val, val);\n"

        code = f"    QJSNodePrivate *priv = qjs_get_dom_priv(this_val);\n"
        code += f"    if (!priv) return JS_EXCEPTION;\n"

        impl_func = f"wisp_{lower_name}_{attr['name']}_{'set' if is_set else 'get'}_impl"

        if is_set:
            js_type = idl_to_js_type(attr['type'])
            if js_type == 'string':
                code += f"    const char *value = JS_ToCString(ctx, val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                code += f"    JS_FreeCString(ctx, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, const char * value)"
                stub_body = "    return JS_UNDEFINED;"
            elif js_type == 'bool':
                code += f"    bool value = JS_ToBool(ctx, val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, bool value)"
                stub_body = "    return JS_UNDEFINED;"
            elif js_type == 'int':
                code += f"    int32_t value = 0; JS_ToInt32(ctx, &value, val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, int32_t value)"
                stub_body = "    return JS_UNDEFINED;"
            else:
                code += f"    QJSNodePrivate *val_priv = qjs_get_dom_priv(val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, val_priv ? val_priv->node : NULL);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, void * value)"
                stub_body = "    return JS_UNDEFINED;"

            code += "    return ret;\n"
        else:
            code += f"    return {impl_func}(ctx, priv);\n"
            sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv)"
            stub_body = "    return JS_NULL;"

        self.current_impl_signatures.append(sig)
        self.current_weak_stubs.append(f"__attribute__((weak)) {sig} {{\n"
                               f"    NSLOG(wisp, WARNING, \"Unimplemented WebIDL attribute {'set' if is_set else 'get'}: {interface_name}.{attr['name']}\");\n"
                               f"{stub_body}\n}}")
        return code

    def generate_interface_files(self, interface_name: str):
        interface = self.interfaces.get(interface_name)
        name = interface.name
        lower_name = name.lower()
        parent_name = self._get_inheritance(interface_name)
        attributes, operations, constants = self._get_members(interface)
        
        self.current_impl_signatures = []
        self.current_weak_stubs = []

        # Header
        h_code = f"#ifndef WISP_JS_{name.upper()}_GEN_H\n#define WISP_JS_{name.upper()}_GEN_H\n\n"
        h_code += "#include \"quickjs.h\"\n#include <stdbool.h>\n"
        h_code += "#include \"dom_bridge.h\"\n\n"
        h_code += f"extern JSClassID qjs_{lower_name}_class_id;\n"
        h_code += f"int qjs_init_{lower_name}(JSContext *ctx);\n"
        h_code += f"JSValue qjs_new_{lower_name}(JSContext *ctx, void *node, bool is_dom_node);\n\n"

        # Binding Source
        c_code = f"#include <stdlib.h>\n#include <string.h>\n#include <stdbool.h>\n#include <stdint.h>\n"
        c_code += "#include \"quickjs.h\"\n#include \"dom_bridge.h\"\n#include \"qjs_internal.h\"\n"
        c_code += "#include <wisp/utils/log.h>\n#include \"utils/libdom.h\"\n"
        c_code += f"#include \"JS{name}.gen.h\"\n"
        if parent_name:
            c_code += f"#include \"JS{parent_name}.gen.h\"\n"
        c_code += "\n"

        c_code += f"JSClassID qjs_{lower_name}_class_id;\n\n"

        # Marshaller declarations
        c_code += f"static void js_{lower_name}_finalizer(JSRuntime *rt, JSValue val);\n"
        for op in operations:
            c_code += f"static JSValue js_{lower_name}_{op['name']}(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);\n"
        for attr in attributes:
            c_code += f"static JSValue js_{lower_name}_{attr['name']}_get(JSContext *ctx, JSValueConst this_val);\n"
            if not attr['readonly']:
                c_code += f"static JSValue js_{lower_name}_{attr['name']}_set(JSContext *ctx, JSValueConst this_val, JSValueConst val);\n"

        # Prototype funcs
        c_code += f"\nstatic const JSCFunctionListEntry js_{lower_name}_proto_funcs[] = {{\n"
        for op in operations:
            c_code += f"    JS_CFUNC_DEF(\"{op['name']}\", {len(op['args'])}, js_{lower_name}_{op['name']}),\n"
        for attr in attributes:
            if attr['readonly']:
                c_code += f"    JS_CGETSET_DEF(\"{attr['name']}\", js_{lower_name}_{attr['name']}_get, NULL),\n"
            else:
                c_code += f"    JS_CGETSET_DEF(\"{attr['name']}\", js_{lower_name}_{attr['name']}_get, js_{lower_name}_{attr['name']}_set),\n"
        for const in constants:
            c_code += f"    JS_PROP_INT32_DEF(\"{const['name']}\", {const['value']}, JS_PROP_CONFIGURABLE),\n"
        c_code += "};\n\n"
        
        c_code += f"static JSClassDef js_{lower_name}_class = {{\n    \"{name}\",\n    .finalizer = js_{lower_name}_finalizer,\n}};\n\n"

        c_code += f"static void js_{lower_name}_finalizer(JSRuntime *rt, JSValue val)\n{{\n"
        c_code += f"    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_{lower_name}_class_id);\n"
        c_code += f"    if (priv) {{\n"
        c_code += f"        if (priv->magic == QJS_DOM_MAGIC) {{\n"
        c_code += f"            qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);\n"
        c_code += f"            if (priv->is_dom_node && priv->node) dom_node_unref((dom_node *)priv->node);\n"
        c_code += f"        }}\n"
        c_code += f"        free(priv);\n"
        c_code += f"    }}\n}}\n\n"

        for op in operations:
            c_code += f"static JSValue js_{lower_name}_{op['name']}(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)\n{{\n"
            c_code += self.generate_marshaller_body(name, op)
            c_code += f"}}\n\n"

        for attr in attributes:
            c_code += f"static JSValue js_{lower_name}_{attr['name']}_get(JSContext *ctx, JSValueConst this_val)\n{{\n"
            c_code += self.generate_attr_marshaller(name, attr, False)
            c_code += f"}}\n\n"
            if not attr['readonly']:
                c_code += f"static JSValue js_{lower_name}_{attr['name']}_set(JSContext *ctx, JSValueConst this_val, JSValueConst val)\n{{\n"
                c_code += self.generate_attr_marshaller(name, attr, True)
                c_code += f"}}\n\n"

        h_code += f"int qjs_init_{lower_name}_gen(JSContext *ctx);\n"

        c_code += f"int qjs_init_{lower_name}_gen(JSContext *ctx)\n{{\n"
        c_code += f"    JSRuntime *rt = JS_GetRuntime(ctx);\n"
        c_code += f"    if (qjs_{lower_name}_class_id == 0) JS_NewClassID(rt, &qjs_{lower_name}_class_id);\n"
        c_code += f"    if (!JS_IsRegisteredClass(rt, qjs_{lower_name}_class_id)) JS_NewClass(rt, qjs_{lower_name}_class_id, &js_{lower_name}_class);\n"
        c_code += f"    JSValue proto = JS_GetClassProto(ctx, qjs_{lower_name}_class_id);\n"
        c_code += f"    if (JS_IsNull(proto) || JS_IsUndefined(proto)) {{\n"
        c_code += f"        proto = JS_NewObject(ctx);\n"
        if parent_name:
            c_code += f"        JSValue parent_proto = JS_GetClassProto(ctx, qjs_{parent_name.lower()}_class_id);\n"
            c_code += f"        JS_SetPrototype(ctx, proto, parent_proto);\n"
            c_code += f"        JS_FreeValue(ctx, parent_proto);\n"
        c_code += f"        JS_SetPropertyFunctionList(ctx, proto, js_{lower_name}_proto_funcs, sizeof(js_{lower_name}_proto_funcs) / sizeof(js_{lower_name}_proto_funcs[0]));\n"
        c_code += f"        JS_SetClassProto(ctx, qjs_{lower_name}_class_id, proto);\n"
        c_code += f"    }} else {{\n        JS_FreeValue(ctx, proto);\n    }}\n"
        c_code += f"    return 0;\n}}\n\n"

        c_code += f"__attribute__((weak)) int qjs_init_{lower_name}(JSContext *ctx)\n{{\n"
        c_code += f"    return qjs_init_{lower_name}_gen(ctx);\n"
        c_code += f"}}\n\n"

        c_code += f"__attribute__((weak)) JSValue qjs_new_{lower_name}(JSContext *ctx, void *node, bool is_dom_node)\n{{\n"
        c_code += f"    JSValue obj = JS_NewObjectClass(ctx, qjs_{lower_name}_class_id);\n"
        c_code += f"    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));\n    if (!priv) return JS_ThrowOutOfMemory(ctx);\n"
        c_code += f"    priv->magic = QJS_DOM_MAGIC; priv->node = node; priv->is_dom_node = is_dom_node; priv->ctx = ctx;\n    if (is_dom_node && node) dom_node_ref((dom_node *)node);\n"
        c_code += f"    JS_SetOpaque(obj, priv); return obj;\n}}\n"

        # Implementation signatures in header
        for sig in self.current_impl_signatures:
            h_code += f"{sig};\n"
        h_code += "\n#endif\n"

        with open(os.path.join(self.output_dir, f"JS{name}.gen.h"), "w") as f:
            f.write(h_code)
        with open(os.path.join(self.output_dir, f"JS{name}.gen.c"), "w") as f:
            f.write(c_code)
        with open(os.path.join(self.output_dir, f"JS{name}_stubs.gen.c"), "w") as f:
            f.write(f"#include \"JS{name}.gen.h\"\n#include \"qjs_internal.h\"\n#include <wisp/utils/log.h>\n\n")
            for stub in self.current_weak_stubs:
                f.write(stub + "\n\n")

    def generate_registration(self) -> str:
        code = "#include <stdlib.h>\n#include \"quickjs.h\"\n"
        for name in sorted(self.all_interface_names):
            code += f"#include \"JS{name}.gen.h\"\n"

        code += "\nvoid wisp_js_register_all_bindings(JSContext *ctx)\n{\n"

        initialized = set()
        to_init = sorted(self.all_interface_names)

        while to_init:
            progress = False
            for name in list(to_init):
                parent = self._get_inheritance(name)
                if not parent or parent not in self.all_interface_names or parent in initialized:
                    code += f"    qjs_init_{name.lower()}(ctx);\n"
                    initialized.add(name)
                    to_init.remove(name)
                    progress = True
            if (not progress) and to_init:
                for name in sorted(to_init):
                    code += f"    qjs_init_{name.lower()}(ctx);\n"
                    to_init.remove(name)
                break

        code += "}\n"
        return code

def main():
    parser = argparse.ArgumentParser(description='Generate QuickJS-ng C bindings from WebIDL files')
    parser.add_argument('idl_files', nargs='+', help='Path to the IDL files')
    parser.add_argument('-o', '--output', help='Output directory')
    parser.add_argument('--list-interfaces', action='store_true', help='List interfaces and exit')
    args = parser.parse_args()

    if args.list_interfaces:
        generator = QuickJSBindingGenerator("")
        for idl in args.idl_files:
            generator.parse_idl(idl)
        for name in sorted(generator.all_interface_names):
            print(name)
        return

    if not args.output:
        print("Error: -o/--output required for generation")
        sys.exit(1)

    os.makedirs(args.output, exist_ok=True)
    generator = QuickJSBindingGenerator(args.output)
    for idl in args.idl_files:
        generator.parse_idl(idl)

    for name in generator.all_interface_names:
        generator.generate_interface_files(name)
        print(f"Generated: {name}")

    with open(os.path.join(args.output, "wisp_binding_reg.gen.c"), "w") as f:
        f.write(generator.generate_registration())

    # Generated header that includes all interface headers
    with open(os.path.join(args.output, "generated_bindings.h"), "w") as f:
        f.write("#ifndef WISP_GENERATED_BINDINGS_H\n#define WISP_GENERATED_BINDINGS_H\n\n")
        for name in sorted(generator.all_interface_names):
            f.write(f"#include \"JS{name}.gen.h\"\n")
        f.write("\n#endif\n")

    print("Generated centralized registration file")

if __name__ == "__main__":
    main()
