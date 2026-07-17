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
    'USVString': 'const char *',
    'ByteString': 'const char *',
    'boolean': 'bool',
    'byte': 'int8_t',
    'octet': 'uint8_t',
    'unsigned short': 'uint16_t',
    'unsigned long': 'uint32_t',
    'unsigned long long': 'uint64_t',
    'short': 'int16_t',
    'long': 'int32_t',
    'long long': 'int64_t',
    'double': 'double',
    'float': 'float',
    'any': 'JSValue',
    'DOMTimeStamp': 'uint64_t',
}

def idl_to_js_type(idl_type):
    t = str(idl_type).strip().rstrip('?')
    if t in ['DOMString', 'USVString', 'ByteString']:
        return 'string'
    if t == 'boolean':
        return 'bool'
    if t in ['byte', 'octet', 'unsigned short', 'unsigned long', 'short', 'long']:
        return 'int'
    if t in ['unsigned long long', 'long long', 'DOMTimeStamp']:
        return 'int64'
    if t in ['double', 'float'] or 'double' in t or 'float' in t:
        return 'float'
    return 'value'

def normalize_primitive_type(idl_type):
    type_str = str(idl_type).strip()
    if type_str.startswith('unrestricted '):
        type_str = type_str.replace('unrestricted ', '').strip()
    type_mapping = {
        'double': 'double',
        'float': 'float',
        'long': 'int32_t',
        'unsigned long': 'uint32_t',
        'long long': 'int64_t',
        'unsigned long long': 'uint64_t',
        'boolean': 'bool',
        'DOMTimeStamp': 'uint64_t'
    }
    return type_mapping.get(type_str, None)

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
        self.dictionaries = set()
        self.mixins = {}
        self.all_interface_names = []
        self.current_impl_signatures = []
        self.current_weak_stubs = []

    def parse_idl(self, idl_path: str):
        with open(idl_path, 'r') as f:
            self.parser.parse(f.read())

    def prepare(self):
        for construct in self.parser.constructs:
            if isinstance(construct, (widlparser.constructs.Interface, widlparser.constructs.Mixin)):
                if construct.name not in self.interfaces:
                    self.interfaces[construct.name] = construct
                else:
                    # Merge members from partial interface
                    self.interfaces[construct.name].members.extend(construct.members)

                if isinstance(construct, widlparser.constructs.Interface):
                    if construct.name not in self.all_interface_names:
                        self.all_interface_names.append(construct.name)
            elif isinstance(construct, widlparser.constructs.Dictionary):
                self.dictionaries.add(construct.name)
            elif isinstance(construct, widlparser.constructs.ImplementsStatement):
                target = str(construct.name).strip()
                if target not in self.mixins:
                    self.mixins[target] = []
                self.mixins[target].append(str(construct.implements).strip())
            elif isinstance(construct, widlparser.constructs.IncludesStatement):
                target = str(construct.name).strip()
                if target not in self.mixins:
                    self.mixins[target] = []
                self.mixins[target].append(str(construct.includes).strip())

    def _get_inheritance(self, interface_name):
        interface = self.interfaces.get(interface_name)
        if not interface or not interface.inheritance:
            return None
        # Handle widlparser 1.5.0 Inheritance object vs older string-like behavior
        base = getattr(interface.inheritance, 'base', None)
        if base:
            return str(base).strip()
        return str(interface.inheritance).strip().lstrip(':').strip()

    def _is_event_type(self, interface_name):
        if interface_name == 'Event':
            return True
        parent = self._get_inheritance(interface_name)
        if parent:
            return self._is_event_type(parent)
        return False

    def _get_members(self, interface):
        name = interface.name
        members_by_name = {} # name -> member_info

        processed_mixins = set()
        to_process = [interface]

        mixins_to_check = []
        if name in self.mixins:
            mixins_to_check.extend(self.mixins[name])

        while mixins_to_check:
            mixin_name = mixins_to_check.pop(0)
            if mixin_name in processed_mixins:
                continue
            processed_mixins.add(mixin_name)

            mixin = self.interfaces.get(mixin_name)
            if mixin:
                if mixin not in to_process:
                    to_process.append(mixin)
                if mixin_name in self.mixins:
                    mixins_to_check.extend(self.mixins[mixin_name])

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
                        # Extract type from m.attribute.type for more accurate results
                        if hasattr(m, 'attribute') and hasattr(m.attribute, 'type'):
                            idl_type_val = str(m.attribute.type).strip()
                        else:
                            idl_type_val = str(m.idl_type).strip()

                        if idl_type_val.startswith('unrestricted '):
                            idl_type_val = idl_type_val.replace('unrestricted ', '').strip()

                        members_by_name[member.name] = {
                            'kind': 'attribute',
                            'name': member.name,
                            'readonly': m.readonly if hasattr(m, 'readonly') else ('readonly' in str(m)),
                            'type': idl_type_val,
                            'custom': custom
                        }
                elif isinstance(m, (widlparser.productions.Operation, widlparser.productions.SpecialOperation)):
                    if isinstance(m, widlparser.productions.SpecialOperation):
                        op = m.operation
                    else:
                        op = m

                    args = []
                    for arg in op.arguments:
                        actual_type = get_actual_type(arg)
                        if actual_type.startswith('unrestricted '):
                            actual_type = actual_type.replace('unrestricted ', '').strip()
                        variadic = False
                        if hasattr(arg, 'variadic'):
                            variadic = arg.variadic
                        elif '...' in str(arg):
                            variadic = True
                        args.append({'name': arg.name, 'type': actual_type, 'optional': arg.optional, 'variadic': variadic})

                    # Group by name to handle overloads
                    if member.name not in members_by_name:
                         members_by_name[member.name] = {
                            'kind': 'operation',
                            'name': member.name,
                            'overloads': []
                        }

                    if members_by_name[member.name]['kind'] == 'operation':
                        impl_name = f"{member.name}_{len(members_by_name[member.name]['overloads'])}"
                        members_by_name[member.name]['overloads'].append({
                            'name': member.name,
                            'impl_name': impl_name,
                            'args': args,
                            'custom': custom,
                            'return_type': str(m.return_type).strip()
                        })
                elif isinstance(m, widlparser.productions.Stringifier):
                    name = member.name if (member.name and member.name != "__stringifier__") else "toString"
                    if name not in members_by_name:
                         members_by_name[name] = {
                            'kind': 'operation',
                            'name': name,
                            'overloads': []
                        }

                    if members_by_name[name]['kind'] == 'operation':
                        impl_name = f"{name}_{len(members_by_name[name]['overloads'])}"
                        members_by_name[name]['overloads'].append({
                            'name': name,
                            'impl_name': impl_name,
                            'args': [],
                            'custom': False,
                            'return_type': 'DOMString'
                        })
                elif isinstance(m, widlparser.constructs.Const):
                     if member.name not in members_by_name:
                        members_by_name[member.name] = {
                            'kind': 'const',
                            'name': member.name,
                            'value': str(m.value).strip(),
                            'type': str(m.type).strip()
                        }

        attributes = sorted([m for m in members_by_name.values() if m['kind'] == 'attribute'], key=lambda x: x['name'])
        operations = sorted([m for m in members_by_name.values() if m['kind'] == 'operation'], key=lambda x: x['name'])
        # Flatten operations for individual marshaller generation
        flat_ops = []
        for op_group in operations:
            if len(op_group['overloads']) == 1:
                # Single overload: use the original name for implementation
                op_group['overloads'][0]['impl_name'] = op_group['name']

            for op in op_group['overloads']:
                flat_ops.append(op)

        constants = sorted([m for m in members_by_name.values() if m['kind'] == 'const'], key=lambda x: x['name'])

        constructors = []
        if hasattr(interface, 'constructors'):
            for idx, ctor in enumerate(interface.constructors):
                ctor_str = str(ctor)
                if ctor_str.startswith('Constructor') or 'NamedConstructor' in ctor_str:
                    name = "constructor"
                    if 'NamedConstructor=' in ctor_str:
                        match = re.search(r'NamedConstructor=([A-Za-z0-9_]+)', ctor_str)
                        if match: name = match.group(1)

                    # Handle overloaded constructors by appending index if more than one
                    if name == "constructor" and len([c for c in interface.constructors if str(c).startswith('Constructor')]) > 1:
                        impl_name = f"{name}_{idx}"
                    elif 'NamedConstructor' in ctor_str and len([c for c in interface.constructors if 'NamedConstructor' in str(c)]) > 1:
                        impl_name = f"{name}_{idx}"
                    else:
                        impl_name = name

                    args = []
                    ctor_args = []
                    if hasattr(ctor, 'attribute') and hasattr(ctor.attribute, '_arguments') and ctor.attribute._arguments:
                        ctor_args = ctor.attribute._arguments

                    for arg in ctor_args:
                        actual_type = get_actual_type(arg)
                        if actual_type.startswith('unrestricted '):
                            actual_type = actual_type.replace('unrestricted ', '').strip()
                        args.append({'name': arg.name, 'type': actual_type, 'optional': arg.optional})

                    constructors.append({'name': name, 'impl_name': impl_name, 'args': args})

        return attributes, operations, constants, constructors, flat_ops

    def _get_type_check(self, arg_idx, idl_type):
        js_type = idl_to_js_type(idl_type)
        val = f"argv[{arg_idx}]"

        if js_type == 'string':
            return f"JS_IsString({val})"
        if js_type == 'bool':
            return f"JS_IsBool({val})"
        if js_type in ['int', 'int64', 'float']:
            return f"JS_IsNumber({val})"

        actual_type = str(idl_type).strip().rstrip('?')
        if actual_type.startswith('unrestricted '):
            actual_type = actual_type.replace('unrestricted ', '').strip()

        if actual_type in self.all_interface_names:
            return f"qjs_get_dom_priv(ctx, {val}) != NULL"

        if actual_type == "ArrayBuffer":
             return f"JS_IsArrayBuffer({val})"

        if actual_type == "ArrayBufferView" or "Array" in actual_type or "sequence" in actual_type:
            return f"JS_IsArray({val})"

        return "true"

    def generate_marshaller_body(self, interface_name, op):
        lower_name = interface_name.lower()
        if op['custom']:
            return f"    return js_{lower_name}_{op['name']}_custom(ctx, this_val, argc, argv);\n"

        code = f"    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);\n"
        code += f"    if (!priv) return JS_EXCEPTION;\n"

        impl_args = ["priv"]
        for i, arg in enumerate(op['args']):
            js_type = idl_to_js_type(arg['type'])
            arg_name = safe_name(arg['name'])
            if arg.get('variadic'):
                code += f"    JSValue {arg_name} = JS_NewArray(ctx);\n"
                code += f"    for (int j = {i}; j < argc; j++) JS_SetPropertyUint32(ctx, {arg_name}, j - {i}, JS_DupValue(ctx, argv[j]));\n"
                impl_args.append(arg_name)
            elif js_type == 'string':
                code += f"    const char *{arg_name} = (argc > {i}) ? JS_ToCString(ctx, argv[{i}]) : NULL;\n"
                code += f"    if (argc > {i} && !{arg_name}) {{\n"
                for prev_idx, prev_arg in enumerate(op['args'][:i]):
                    prev_name = safe_name(prev_arg['name'])
                    prev_js_type = idl_to_js_type(prev_arg['type'])
                    if prev_arg.get('variadic'):
                         code += f"        JS_FreeValue(ctx, {prev_name});\n"
                    elif prev_js_type == 'string':
                        code += f"        if ({prev_name}) JS_FreeCString(ctx, {prev_name});\n"
                    elif prev_js_type not in ['bool', 'int', 'int64', 'float']:
                         actual_type = str(prev_arg['type']).strip().rstrip('?')
                         if actual_type not in self.all_interface_names or actual_type in self.dictionaries:
                            code += f"        JS_FreeValue(ctx, {prev_name});\n"
                code += f"        return JS_EXCEPTION;\n"
                code += f"    }}\n"
                impl_args.append(arg_name)
            elif js_type == 'bool':
                code += f"    bool {arg_name} = (argc > {i}) ? JS_ToBool(ctx, argv[{i}]) : false;\n"
                impl_args.append(arg_name)
            elif js_type == 'int':
                if 'unsigned' in str(arg['type']):
                    code += f"    uint32_t {arg_name} = 0; if (argc > {i}) JS_ToUint32(ctx, &{arg_name}, argv[{i}]);\n"
                else:
                    code += f"    int32_t {arg_name} = 0; if (argc > {i}) JS_ToInt32(ctx, &{arg_name}, argv[{i}]);\n"
                impl_args.append(arg_name)
            elif js_type == 'int64':
                if 'unsigned' in str(arg['type']):
                    code += f"    uint64_t {arg_name} = 0; if (argc > {i}) {{ int64_t tmp; JS_ToInt64(ctx, &tmp, argv[{i}]); {arg_name} = (uint64_t)tmp; }}\n"
                else:
                    code += f"    int64_t {arg_name} = 0; if (argc > {i}) JS_ToInt64(ctx, &{arg_name}, argv[{i}]);\n"
                impl_args.append(arg_name)
            elif js_type == 'float':
                code += f"    double {arg_name} = 0; if (argc > {i}) JS_ToFloat64(ctx, &{arg_name}, argv[{i}]);\n"
                impl_args.append(arg_name)
            else:
                # Differentiate between another IDL interface and other types (any, dictionary)
                actual_type = str(arg['type']).strip().rstrip('?')
                if actual_type.startswith('unrestricted '):
                    actual_type = actual_type.replace('unrestricted ', '').strip()

                if actual_type in self.all_interface_names and actual_type not in self.dictionaries:
                    code += f"    QJSNodePrivate *{arg_name}_priv = (argc > {i}) ? qjs_get_dom_priv(ctx, argv[{i}]) : NULL;\n"
                    code += f"    void *{arg_name} = {arg_name}_priv ? {arg_name}_priv->node : NULL;\n"
                    impl_args.append(arg_name)
                else:
                    code += f"    JSValue {arg_name} = (argc > {i}) ? JS_DupValue(ctx, argv[{i}]) : JS_UNDEFINED;\n"
                    impl_args.append(arg_name)

        impl_func = f"wisp_{lower_name}_{op['impl_name']}_impl"

        sig_args = ["QJSNodePrivate *priv"]
        for arg in op['args']:
            js_type = idl_to_js_type(arg['type'])
            arg_name = safe_name(arg['name'])
            if arg.get('variadic'):
                c_type = "JSValue"
            elif js_type in ['string', 'bool', 'int', 'float']:
                c_type = TYPE_MAP.get(arg['type'], "JSValue")
                if js_type == 'float':
                    c_type = "double"
            elif js_type == 'int64':
                if 'unsigned' in str(arg['type']):
                    c_type = "uint64_t"
                else:
                    c_type = "int64_t"
            else:
                actual_type = str(arg['type']).strip().rstrip('?')
                if actual_type.startswith('unrestricted '):
                    actual_type = actual_type.replace('unrestricted ', '').strip()

                if actual_type in self.all_interface_names and actual_type not in self.dictionaries:
                    c_type = "void *"
                else:
                    c_type = "JSValue"
            sig_args.append(f"{c_type} {arg_name}")

        sig = f"JSValue {impl_func}(JSContext *ctx, {', '.join(sig_args)})"
        self.current_impl_signatures.append(sig)

        self.current_weak_stubs.append(f"__attribute__((weak)) JSValue {impl_func}(JSContext *ctx, {', '.join(sig_args)}) {{\n"
                               f"    NSLOG(wisp, WARNING, \"Unimplemented WebIDL method: {interface_name}.{op['name']}\");\n"
                               f"    return JS_UNDEFINED;\n}}")

        code += f"    JSValue ret = {impl_func}(ctx, {', '.join(impl_args)});\n"

        for i, arg in enumerate(op['args']):
            arg_name = safe_name(arg['name'])
            js_type = idl_to_js_type(arg['type'])
            if arg.get('variadic'):
                code += f"    JS_FreeValue(ctx, {arg_name});\n"
            elif js_type == 'string':
                code += f"    if ({arg_name}) JS_FreeCString(ctx, {arg_name});\n"
            else:
                actual_type = str(arg['type']).strip().rstrip('?')
                if actual_type.startswith('unrestricted '):
                    actual_type = actual_type.replace('unrestricted ', '').strip()
                if not (actual_type in self.all_interface_names and actual_type not in self.dictionaries) and js_type not in ['bool', 'int', 'int64', 'float']:
                    code += f"    JS_FreeValue(ctx, {arg_name});\n"

        code += "    if (JS_IsException(ret)) return ret;\n"
        code += "    return ret;\n"
        return code

    def generate_attr_marshaller(self, interface_name, attr, is_set):
        lower_name = interface_name.lower()
        if attr['custom']:
            suffix = "set" if is_set else "get"
            return f"    return js_{lower_name}_{attr['name']}_{suffix}_custom(ctx, this_val, val);\n"

        code = f"    QJSNodePrivate *priv = qjs_get_dom_priv(ctx, this_val);\n"
        code += f"    if (!priv) return JS_EXCEPTION;\n"

        impl_func = f"wisp_{lower_name}_{attr['name']}_{'set' if is_set else 'get'}_impl"

        if is_set:
            primitive_type = normalize_primitive_type(attr['type'])
            if primitive_type in ['double', 'float']:
                code += f"    double value = 0; JS_ToFloat64(ctx, &value, val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, double value)"
                stub_body = "    return JS_UNDEFINED;"
            elif idl_to_js_type(attr['type']) == 'string':
                code += f"    const char *value = JS_ToCString(ctx, val);\n"
                code += f"    if (!value) return JS_EXCEPTION;\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                code += f"    JS_FreeCString(ctx, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, const char * value)"
                stub_body = "    return JS_UNDEFINED;"
            elif idl_to_js_type(attr['type']) == 'bool':
                code += f"    bool value = JS_ToBool(ctx, val);\n"
                code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, bool value)"
                stub_body = "    return JS_UNDEFINED;"
            elif idl_to_js_type(attr['type']) == 'int':
                if 'unsigned' in str(attr['type']):
                    code += f"    uint32_t value = 0; JS_ToUint32(ctx, &value, val);\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, uint32_t value)"
                else:
                    code += f"    int32_t value = 0; JS_ToInt32(ctx, &value, val);\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, int32_t value)"
                stub_body = "    return JS_UNDEFINED;"
            elif idl_to_js_type(attr['type']) == 'int64':
                if 'unsigned' in str(attr['type']):
                    code += f"    uint64_t value = 0; {{ int64_t tmp; JS_ToInt64(ctx, &tmp, val); value = (uint64_t)tmp; }}\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, uint64_t value)"
                else:
                    code += f"    int64_t value = 0; JS_ToInt64(ctx, &value, val);\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, value);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, int64_t value)"
                stub_body = "    return JS_UNDEFINED;"
            else:
                actual_type = str(attr['type']).strip().rstrip('?')
                if actual_type.startswith('unrestricted '):
                    actual_type = actual_type.replace('unrestricted ', '').strip()

                if actual_type in self.all_interface_names and actual_type not in self.dictionaries:
                    code += f"    QJSNodePrivate *val_priv = qjs_get_dom_priv(ctx, val);\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, val_priv ? val_priv->node : NULL);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, void * value)"
                    stub_body = "    return JS_UNDEFINED;"
                else:
                    code += f"    JSValue val_dup = JS_DupValue(ctx, val);\n"
                    code += f"    JSValue ret = {impl_func}(ctx, priv, val_dup);\n"
                    code += f"    JS_FreeValue(ctx, val_dup);\n"
                    sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv, JSValue value)"
                    stub_body = "    return JS_UNDEFINED;"

            code += "    if (JS_IsException(ret)) return ret;\n"
            code += "    return ret;\n"
        else:
            code += f"    return {impl_func}(ctx, priv);\n"
            sig = f"JSValue {impl_func}(JSContext *ctx, QJSNodePrivate *priv)"
            stub_body = "    return JS_NULL;"

        self.current_impl_signatures.append(sig)
        self.current_weak_stubs.append(f"__attribute__((weak)) {sig} {{\n"
                               f"    NSLOG(wisp, WARNING, \"Unimplemented WebIDL attribute {'set' if is_set else 'get'}: {interface_name}.{attr['name']}\");\n"
                               f"    {stub_body}\n}}")
        return code

    def generate_interface_files(self, interface_name: str):
        interface = self.interfaces.get(interface_name)
        name = interface.name
        lower_name = name.lower()
        parent_name = self._get_inheritance(interface_name)
        attributes, operations, constants, constructors, flat_ops = self._get_members(interface)
        
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

        c_code += f"__attribute__((weak)) JSClassID qjs_{lower_name}_class_id;\n\n"

        # Marshaller declarations
        c_code += f"static void js_{lower_name}_finalizer(JSRuntime *rt, JSValue val);\n"
        for ctor in constructors:
            if ctor.get('is_dummy'):
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);\n"
            elif ctor['name'] == 'constructor':
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);\n"
            else:
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_ctor_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);\n"
        for op in flat_ops:
            c_code += f"static JSValue js_{lower_name}_{op['impl_name']}_marshaller(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);\n"

        # Dispatchers for overloaded operations
        op_names = sorted(list(set(op['name'] for op in flat_ops)))
        for op_name in op_names:
            c_code += f"static JSValue js_{lower_name}_{op_name}(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);\n"
        for attr in attributes:
            c_code += f"static JSValue js_{lower_name}_{attr['name']}_get(JSContext *ctx, JSValueConst this_val);\n"
            if not attr['readonly']:
                c_code += f"static JSValue js_{lower_name}_{attr['name']}_set(JSContext *ctx, JSValueConst this_val, JSValueConst val);\n"

        # Prototype funcs
        c_code += f"\nstatic const JSCFunctionListEntry js_{lower_name}_proto_funcs[] = {{\n"
        for op_name in op_names:
            # Find the max args for this operation name
            max_args = max(len(op['args']) for op in flat_ops if op['name'] == op_name)
            c_code += f"    JS_CFUNC_DEF(\"{op_name}\", {max_args}, js_{lower_name}_{op_name}),\n"
        for attr in attributes:
            if attr['readonly']:
                c_code += f"    JS_CGETSET_DEF(\"{attr['name']}\", js_{lower_name}_{attr['name']}_get, NULL),\n"
            else:
                c_code += f"    JS_CGETSET_DEF(\"{attr['name']}\", js_{lower_name}_{attr['name']}_get, js_{lower_name}_{attr['name']}_set),\n"
        for const in constants:
            c_code += f"    JS_PROP_INT32_DEF(\"{const['name']}\", {const['value']}, JS_PROP_CONFIGURABLE),\n"
        c_code += "};\n\n"
        
        c_code += f"static JSClassDef js_{lower_name}_class = {{\n    \"{name}\",\n    .finalizer = js_{lower_name}_finalizer,\n}};\n\n"

        is_event = self._is_event_type(name)
        c_code += f"static void js_{lower_name}_finalizer(JSRuntime *rt, JSValue val)\n{{\n"
        c_code += f"    QJSNodePrivate *priv = JS_GetOpaque(val, qjs_{lower_name}_class_id);\n"
        c_code += f"    if (priv) {{\n"
        c_code += f"        if (priv->magic == QJS_DOM_MAGIC && priv->node) {{\n"
        if is_event:
            c_code += f"            dom_event_unref((dom_event *)priv->node);\n"
        else:
            c_code += f"            if (priv->is_dom_node) qjs_bridge_remove_node(rt, (dom_node *)priv->node, priv->ctx);\n"
            c_code += f"            if (priv->is_dom_node) dom_node_unref((dom_node *)priv->node);\n"
            if name == "NodeList" or name == "RadioNodeList":
                c_code += f"            else dom_nodelist_unref((dom_nodelist *)priv->node);\n"
            elif name == "HTMLCollection":
                c_code += f"            else dom_html_collection_unref((dom_html_collection *)priv->node);\n"
            elif name == "HTMLOptionsCollection":
                c_code += f"            else dom_html_options_collection_unref((dom_html_options_collection *)priv->node);\n"
            elif name == "NamedNodeMap":
                c_code += f"            else dom_namednodemap_unref((dom_namednodemap *)priv->node);\n"
        c_code += f"        }}\n"
        c_code += f"        free(priv);\n"
        c_code += f"    }}\n}}\n\n"

        # Ensure a standard constructor exists on the global object for every interface
        # so that window.HTMLElement, window.Element, window.EventTarget etc are defined
        # and throw a TypeError on direct instantiation but can be subclassed / extended.
        has_constructor = any(c['name'] == 'constructor' for c in constructors)
        if not has_constructor:
            constructors.append({'name': 'constructor', 'impl_name': 'dummy_constructor', 'args': [], 'is_dummy': True})

        # Helper to group constructors by name
        ctors_by_name = {}
        for ctor in constructors:
            if ctor['name'] not in ctors_by_name:
                ctors_by_name[ctor['name']] = []
            ctors_by_name[ctor['name']].append(ctor)

        for ctor in constructors:
            if ctor.get('is_dummy'):
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)\n{{\n"
                c_code += f"    return JS_ThrowTypeError(ctx, \"Illegal constructor\");\n"
                c_code += f"}}\n\n"
                continue

            if ctor['name'] == 'constructor':
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)\n{{\n"
            else:
                c_code += f"static JSValue js_{lower_name}_{ctor['impl_name']}_ctor_marshaller(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)\n{{\n"

            impl_args = []
            for i, arg in enumerate(ctor['args']):
                js_type = idl_to_js_type(arg['type'])
                arg_name = safe_name(arg['name'])
                if js_type == 'string':
                    c_code += f"    const char *{arg_name} = (argc > {i}) ? JS_ToCString(ctx, argv[{i}]) : NULL;\n"
                    c_code += f"    if (argc > {i} && !{arg_name}) return JS_EXCEPTION;\n"
                    impl_args.append(arg_name)
                elif js_type == 'bool':
                    c_code += f"    bool {arg_name} = (argc > {i}) ? JS_ToBool(ctx, argv[{i}]) : false;\n"
                    impl_args.append(arg_name)
                elif js_type == 'int':
                    if 'unsigned' in str(arg['type']):
                        c_code += f"    uint32_t {arg_name} = 0; if (argc > {i}) JS_ToUint32(ctx, &{arg_name}, argv[{i}]);\n"
                    else:
                        c_code += f"    int32_t {arg_name} = 0; if (argc > {i}) JS_ToInt32(ctx, &{arg_name}, argv[{i}]);\n"
                    impl_args.append(arg_name)
                elif js_type == 'int64':
                    if 'unsigned' in str(arg['type']):
                        c_code += f"    uint64_t {arg_name} = 0; if (argc > {i}) {{ int64_t tmp; JS_ToInt64(ctx, &tmp, argv[{i}]); {arg_name} = (uint64_t)tmp; }}\n"
                    else:
                        c_code += f"    int64_t {arg_name} = 0; if (argc > {i}) JS_ToInt64(ctx, &{arg_name}, argv[{i}]);\n"
                    impl_args.append(arg_name)
                elif js_type == 'float':
                    c_code += f"    double {arg_name} = 0; if (argc > {i}) JS_ToFloat64(ctx, &{arg_name}, argv[{i}]);\n"
                    impl_args.append(arg_name)
                else:
                    actual_type = str(arg['type']).strip().rstrip('?')
                    if actual_type.startswith('unrestricted '):
                        actual_type = actual_type.replace('unrestricted ', '').strip()

                    if actual_type in self.all_interface_names and actual_type not in self.dictionaries:
                        c_code += f"    QJSNodePrivate *{arg_name}_priv = (argc > {i}) ? qjs_get_dom_priv(ctx, argv[{i}]) : NULL;\n"
                        c_code += f"    void *{arg_name} = {arg_name}_priv ? {arg_name}_priv->node : NULL;\n"
                        impl_args.append(arg_name)
                    else:
                        c_code += f"    JSValue {arg_name} = (argc > {i}) ? JS_DupValue(ctx, argv[{i}]) : JS_UNDEFINED;\n"
                        impl_args.append(arg_name)

            impl_func = f"wisp_{lower_name}_{ctor['impl_name']}_impl"
            sig_args = ["JSContext *ctx"]
            for arg in ctor['args']:
                js_type = idl_to_js_type(arg['type'])
                arg_name = safe_name(arg['name'])
                if js_type in ['string', 'bool', 'int', 'float']:
                    c_type = TYPE_MAP.get(arg['type'], "JSValue")
                    if js_type == 'float': c_type = "double"
                elif js_type == 'int64':
                    if 'unsigned' in str(arg['type']):
                        c_type = "uint64_t"
                    else:
                        c_type = "int64_t"
                else:
                    actual_type = str(arg['type']).strip().rstrip('?')
                    if actual_type.startswith('unrestricted '):
                         actual_type = actual_type.replace('unrestricted ', '').strip()
                    if actual_type in self.all_interface_names and actual_type not in self.dictionaries:
                        c_type = "void *"
                    else:
                        c_type = "JSValue"
                sig_args.append(f"{c_type} {arg_name}")

            sig = f"JSValue {impl_func}({', '.join(sig_args)})"
            self.current_impl_signatures.append(sig)
            self.current_weak_stubs.append(f"__attribute__((weak)) {sig} {{\n"
                                f"    NSLOG(wisp, WARNING, \"Unimplemented WebIDL constructor: {name}.{ctor['impl_name']}\");\n"
                                f"    return JS_UNDEFINED;\n}}")

            call_args = ["ctx"] + impl_args
            c_code += f"    JSValue ret = {impl_func}({', '.join(call_args)});\n"
            for i, arg in enumerate(ctor['args']):
                arg_name = safe_name(arg['name'])
                js_type = idl_to_js_type(arg['type'])
                if js_type == 'string':
                    c_code += f"    if ({arg_name}) JS_FreeCString(ctx, {arg_name});\n"
                elif js_type not in ['bool', 'int', 'int64', 'float']:
                     actual_type = str(arg['type']).strip().rstrip('?')
                     if not (actual_type in self.all_interface_names and actual_type not in self.dictionaries):
                        c_code += f"    JS_FreeValue(ctx, {arg_name});\n"
            c_code += "    if (JS_IsException(ret)) return ret;\n"
            c_code += "    return ret;\n"
            c_code += f"}}\n\n"

        for ctor_name, group in ctors_by_name.items():
            if ctor_name == 'constructor':
                c_code += f"static JSValue js_{lower_name}_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)\n{{\n"
            else:
                c_code += f"static JSValue js_{lower_name}_{ctor_name}_ctor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv)\n{{\n"

            if len(group) == 1:
                ctor = group[0]
                suffix = "" if ctor_name == 'constructor' else "_ctor"
                c_code += f"    return js_{lower_name}_{ctor['impl_name']}{suffix}_marshaller(ctx, new_target, argc, argv);\n"
            else:
                # Dispatch by argc and type
                # Sort by decreasing argc, then by specificity (generic 'true' checks last)
                def ctor_sort_key(c):
                    # Higher argc first.
                    # Then count number of specific checks (not 'true')
                    specific_checks = sum(1 for idx, a in enumerate(c['args']) if self._get_type_check(idx, a['type']) != "true")
                    return (-len(c['args']), -specific_checks)

                sorted_group = sorted(group, key=ctor_sort_key)
                for i, ctor in enumerate(sorted_group):
                    checks = [f"argc >= {len(ctor['args'])}"]
                    for idx, arg in enumerate(ctor['args']):
                        check = self._get_type_check(idx, arg['type'])
                        if check != "true":
                            checks.append(check)

                    c_code += f"    {'else ' if i > 0 else ''}if ({' && '.join(checks)}) "

                    suffix = "" if ctor_name == 'constructor' else "_ctor"
                    c_code += f"return js_{lower_name}_{ctor['impl_name']}{suffix}_marshaller(ctx, new_target, argc, argv);\n"

                c_code += f"    else return JS_ThrowTypeError(ctx, \"No matching constructor for {name}\");\n"
            c_code += f"}}\n\n"

        for op in flat_ops:
            c_code += f"static JSValue js_{lower_name}_{op['impl_name']}_marshaller(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)\n{{\n"
            c_code += self.generate_marshaller_body(name, op)
            c_code += f"}}\n\n"

        # Operation Dispatchers
        for op_name in op_names:
            overloads = [op for op in flat_ops if op['name'] == op_name]
            c_code += f"static JSValue js_{lower_name}_{op_name}(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv)\n{{\n"
            if len(overloads) == 1:
                c_code += f"    return js_{lower_name}_{overloads[0]['impl_name']}_marshaller(ctx, this_val, argc, argv);\n"
            else:
                # Dispatch by argc and type
                def op_sort_key(o):
                    # Higher argc first.
                    # Then count number of specific checks (not 'true')
                    specific_checks = sum(1 for idx, a in enumerate(o['args']) if self._get_type_check(idx, a['type']) != "true")
                    return (-len(o['args']), -specific_checks)

                sorted_overloads = sorted(overloads, key=op_sort_key)
                for i, op in enumerate(sorted_overloads):
                    # For variadic, minimum argc is the number of args before the variadic one
                    min_argc = len([a for a in op['args'] if not a.get('variadic')])
                    checks = [f"argc >= {min_argc}"]
                    for idx, arg in enumerate(op['args']):
                        if arg.get('variadic'): continue
                        check = self._get_type_check(idx, arg['type'])
                        if check != "true":
                            checks.append(check)

                    c_code += f"    {'else ' if i > 0 else ''}if ({' && '.join(checks)}) "
                    c_code += f"return js_{lower_name}_{op['impl_name']}_marshaller(ctx, this_val, argc, argv);\n"

                c_code += f"    else return JS_ThrowTypeError(ctx, \"No matching overload for {name}.{op_name}\");\n"
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
        c_code += f"    if (!JS_IsObject(proto)) {{\n"
        c_code += f"        JS_FreeValue(ctx, proto);\n"
        c_code += f"        proto = JS_NewObject(ctx);\n"
        if parent_name:
            c_code += f"        JSValue parent_proto = JS_GetClassProto(ctx, qjs_{parent_name.lower()}_class_id);\n"
            c_code += f"        JS_SetPrototype(ctx, proto, parent_proto);\n"
            c_code += f"        JS_FreeValue(ctx, parent_proto);\n"
        c_code += f"        JS_SetPropertyFunctionList(ctx, proto, js_{lower_name}_proto_funcs, sizeof(js_{lower_name}_proto_funcs) / sizeof(js_{lower_name}_proto_funcs[0]));\n"
        c_code += f"        JS_SetClassProto(ctx, qjs_{lower_name}_class_id, JS_DupValue(ctx, proto));\n"

        for ctor_name, group in ctors_by_name.items():
            max_args = max(len(c['args']) for c in group)
            if ctor_name == 'constructor':
                c_code += f"        {{\n"
                c_code += f"            JSValue ctor = JS_NewCFunction2(ctx, js_{lower_name}_constructor, \"{name}\", {max_args}, JS_CFUNC_constructor, 0);\n"
                c_code += f"            JS_SetConstructor(ctx, ctor, proto);\n"
                c_code += f"            JSValue global_obj = JS_GetGlobalObject(ctx);\n"
                c_code += f"            JS_SetPropertyStr(ctx, global_obj, \"{name}\", ctor);\n"
                c_code += f"            JS_FreeValue(ctx, global_obj);\n"
                c_code += f"        }}\n"
            else:
                c_code += f"        {{\n"
                c_code += f"            JSValue {ctor_name}_ctor_val = JS_NewCFunction2(ctx, (JSCFunction *)js_{lower_name}_{ctor_name}_ctor, \"{ctor_name}\", {max_args}, JS_CFUNC_constructor, 0);\n"
                c_code += f"            JS_SetConstructor(ctx, {ctor_name}_ctor_val, proto);\n"
                c_code += f"            JSValue global_obj = JS_GetGlobalObject(ctx);\n"
                c_code += f"            JS_SetPropertyStr(ctx, global_obj, \"{ctor_name}\", {ctor_name}_ctor_val);\n"
                c_code += f"            JS_FreeValue(ctx, global_obj);\n"
                c_code += f"        }}\n"

        c_code += f"    }}\n    JS_FreeValue(ctx, proto);\n"
        c_code += f"    return 0;\n}}\n\n"

        c_code += f"__attribute__((weak)) int qjs_init_{lower_name}(JSContext *ctx)\n{{\n"
        c_code += f"    return qjs_init_{lower_name}_gen(ctx);\n"
        c_code += f"}}\n\n"

        c_code += f"__attribute__((weak)) JSValue qjs_new_{lower_name}(JSContext *ctx, void *node, bool is_dom_node)\n{{\n"
        c_code += f"    JSValue obj = JS_NewObjectClass(ctx, qjs_{lower_name}_class_id); if (JS_IsException(obj)) return obj;\n"
        c_code += f"    QJSNodePrivate *priv = calloc(1, sizeof(QJSNodePrivate));\n    if (!priv) {{ JS_FreeValue(ctx, obj); return JS_ThrowOutOfMemory(ctx); }}\n"
        c_code += f"    priv->magic = QJS_DOM_MAGIC; priv->node = node; priv->is_dom_node = is_dom_node; priv->ctx = ctx;\n"
        if is_event:
            c_code += f"    if (node) dom_event_ref((dom_event *)node);\n"
        else:
            c_code += f"    if (is_dom_node && node) dom_node_ref((dom_node *)node);\n"
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

        # Correct registration order: Base classes first
        # 1. Start with core DOM classes in fixed order
        core_order = ["Event", "EventTarget", "Node", "Document", "Element", "CharacterData", "HTMLElement"]
        initialized = set()

        for name in core_order:
            if name in self.all_interface_names:
                code += f"    qjs_init_{name.lower()}(ctx);\n"
                initialized.add(name)

        to_init = sorted(self.all_interface_names)

        while to_init:
            progress = False
            for name in list(to_init):
                if name in initialized:
                    to_init.remove(name)
                    continue

                parent = self._get_inheritance(name)
                if not parent or parent not in self.all_interface_names or parent in initialized:
                    code += f"    qjs_init_{name.lower()}(ctx);\n"
                    initialized.add(name)
                    to_init.remove(name)
                    progress = True
            if (not progress) and to_init:
                # Force remaining in alphabetical order if dependencies are circular or missing
                for name in sorted(to_init):
                    code += f"    qjs_init_{name.lower()}(ctx);\n"
                    initialized.add(name)
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
        generator.prepare()
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
    generator.prepare()

    for name in generator.all_interface_names:
        if name in generator.dictionaries:
            continue
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
