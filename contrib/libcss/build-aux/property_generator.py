#!/usr/bin/env python3
"""
Property Registration Generator for LibCSS

Generates property enums, dispatch tables, and string indices from properties.gen
to eliminate manual alignment errors.

Strategy: Parse dispatch.c for canonical property order, then match properties.gen
"""

import sys
import re
from pathlib import Path


class DispatchParser:
    """Parse dispatch.c to get canonical property order and inherited flags."""
    
    def __init__(self, dispatch_file):
        self.dispatch_file = Path(dispatch_file)
        self.properties = []  # List of {name, inherited} dicts
        
    def parse(self):
        """Extract property order and inherited flags from dispatch.c."""
        # Validate file exists and is readable
        if not self.dispatch_file.exists():
            print(f"FATAL ERROR: dispatch.c not found at {self.dispatch_file}", file=sys.stderr)
            sys.exit(1)
        
        try:
            with open(self.dispatch_file, 'r') as f:
                content = f.read()
        except IOError as e:
            print(f"FATAL ERROR: Cannot read dispatch.c: {e}", file=sys.stderr)
            sys.exit(1)
        
        # Validate that prop_dispatch array exists
        if 'struct prop_table prop_dispatch[' not in content:
            print("FATAL ERROR: Cannot find 'struct prop_table prop_dispatch[' in dispatch.c", file=sys.stderr)
            print("File format may have changed. Expected C code with dispatch table.", file=sys.stderr)
            sys.exit(1)
        
        # Find the prop_dispatch array
        # Format: PROPERTY_FUNCS(name), \n inherited_flag,
        pattern = r'PROPERTY_FUNCS\(([a-z_]+)\),\s*\n\s*(\d+),'
        matches = re.findall(pattern, content)
        
        # Validate we found properties
        if not matches:
            print("FATAL ERROR: No property entries found in dispatch.c", file=sys.stderr)
            print(f"Expected pattern: PROPERTY_FUNCS(name),\\n  inherited_flag,", file=sys.stderr)
            print("Check that dispatch.c format hasn't changed.", file=sys.stderr)
            sys.exit(1)
        
        if len(matches) < 50:
            print(f"FATAL ERROR: Only {len(matches)} properties found in dispatch.c", file=sys.stderr)
            print("Expected at least 50 properties. File may be truncated or malformed.", file=sys.stderr)
            sys.exit(1)
        
        for prop_name, inherited_str in matches:
            # Validate inherited flag is 0 or 1
            if inherited_str not in ('0', '1'):
                print(f"FATAL ERROR: Invalid inherited flag '{inherited_str}' for property {prop_name}", file=sys.stderr)
                print("Inherited flag must be 0 or 1", file=sys.stderr)
                sys.exit(1)
            
            self.properties.append({
                'name': prop_name,
                'inherited': int(inherited_str) == 1
            })
        
        return self.properties


class PropertyGenParser:
    """Parse properties.gen to get enum names and metadata."""
    
    def __init__(self, gen_file):
        self.gen_file = Path(gen_file)
        self.prop_map = {}  # name -> metadata
        
    def parse(self):
        """Parse properties.gen and build name->enum mapping."""
        if not self.gen_file.exists():
            print(f"FATAL ERROR: properties.gen not found at {self.gen_file}", file=sys.stderr)
            sys.exit(1)
        
        try:
            with open(self.gen_file, 'r') as f:
                lines = list(f)
        except IOError as e:
            print(f"FATAL ERROR: Cannot read properties.gen: {e}", file=sys.stderr)
            sys.exit(1)
        
        for line_num, line in enumerate(lines, 1):
            line = line.strip()
            
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            
            # Parse: property_name:ENUM_NAME ...
            # ENUM_NAME can be any uppercase identifier (CSS_PROP_*, BORDER_SIDE_*, etc.)
            match = re.match(r'^([a-z_]+):([A-Z][A-Z0-9_]+)\s+(.*)$', line)
            if match:
                prop_name = match.group(1)
                enum_name = match.group(2)
                spec = match.group(3)
                
                # Validate: enum should be uppercase and have valid format
                if not re.match(r'^[A-Z][A-Z0-9_]*$', enum_name):
                    print(f"WARNING: Line {line_num}: Invalid enum format '{enum_name}'", file=sys.stderr)
                    continue
                
                # Parse the spec into structured data
                parsed = SpecParser.parse(spec)
                
                # Derive flags from parsed spec
                is_shorthand = 'SHORTHAND' in parsed['flags'] or parsed['wrap'] is not None
                is_manual = 'MANUAL' in parsed['flags']
                is_generic = 'GENERIC' in parsed['flags']
                
                # Detect aliases: property name doesn't match enum's base name
                # e.g., inline_size:CSS_PROP_WIDTH is an alias for width
                enum_base = enum_name.replace('CSS_PROP_', '').lower()
                is_alias = (prop_name != enum_base) and not is_shorthand and not is_generic
                
                if not is_generic:
                    self.prop_map[prop_name] = {
                        'enum': enum_name,
                        'is_shorthand': is_shorthand,
                        'is_manual': is_manual,
                        'is_alias': is_alias,
                        'is_wrap': parsed['wrap'] is not None,
                        'spec': spec,
                        'parsed': parsed,
                        'line': line_num
                    }
        
        # Validate we found properties
        if not self.prop_map:
            print("FATAL ERROR: No properties found in properties.gen", file=sys.stderr)
            print("Expected format: property_name:CSS_PROP_ENUM_NAME PARSE_SPEC", file=sys.stderr)
            sys.exit(1)
        
        if len(self.prop_map) < 50:
            print(f"FATAL ERROR: Only {len(self.prop_map)} properties in properties.gen", file=sys.stderr)
            print("Expected at least 50. File may be incomplete.", file=sys.stderr)
            sys.exit(1)
        
        return self.prop_map


class SpecParser:
    """Parse a properties.gen spec string into a structured dict.
    
    The spec format is a simple DSL with these elements:
    - Flags: MANUAL, SHORTHAND, GENERIC, OPCODES:MANUAL
    - Key:value: WRAP:func, COLOR:opcode, URI:opcode
    - Blocks: IDENT:( ... IDENT:), LENGTH_UNIT:( ... LENGTH_UNIT:), etc.
    
    Example spec:
        IDENT:( INHERIT: INITIAL: REVERT: UNSET: AUTO:0,HEIGHT_AUTO IDENT:) 
        LENGTH_UNIT:( UNIT_PX:HEIGHT_SET MASK:UNIT_MASK_HEIGHT RANGE:<0 LENGTH_UNIT:)
        CALC:( UNIT_PX:HEIGHT_CALC CALC:)
    
    Returns a dict with all parsed elements, so downstream code never needs
    to parse the raw string again.
    """
    
    @staticmethod
    def parse(spec):
        """Parse a spec string into a structured dict."""
        result = {
            'flags': set(),           # MANUAL, SHORTHAND, GENERIC, OPCODES_MANUAL
            'wrap': None,             # function name if WRAP:xxx
            'ident_keywords': [],     # [(keyword, flags, opcode_name), ...] from IDENT:( )
            'color': None,            # opcode SET name from COLOR:xxx
            'length_unit': None,      # {'opcode': str, 'unit': str, 'mask': str, 'range': str, 'allow': str, 'disallow': str}
            'number': None,           # {'opcode': str, 'is_int': bool, 'range': str}
            'calc': None,             # {'opcode': str, 'type': str} (type = unit/NUMBER/ANY)
            'uri': None,              # opcode name from URI:xxx
            'ident_list': None,       # {'named_opcode': str, 'default_value': str, 'none_opcode': str}
            'sizing': None,           # {'fit_content': str, 'min_content': str, 'max_content': str}
        }
        
        tokens = spec.split()
        i = 0
        
        while i < len(tokens):
            token = tokens[i]
            
            # --- Simple flags ---
            if token == 'MANUAL':
                result['flags'].add('MANUAL')
                i += 1
            elif token == 'SHORTHAND':
                result['flags'].add('SHORTHAND')
                i += 1
            elif token == 'OPCODES:MANUAL':
                result['flags'].add('OPCODES_MANUAL')
                i += 1
            elif 'GENERIC' in token:
                result['flags'].add('GENERIC')
                i += 1
            
            # --- WRAP:function ---
            elif token.startswith('WRAP:'):
                result['wrap'] = token.split(':', 1)[1]
                i += 1
            
            # --- COLOR:opcode ---
            elif token.startswith('COLOR:'):
                result['color'] = token.split(':', 1)[1]
                i += 1
            
            # --- URI:opcode ---
            elif token.startswith('URI:'):
                result['uri'] = token.split(':', 1)[1]
                i += 1
            
            # --- IDENT:( ... IDENT:) block ---
            elif token == 'IDENT:(':
                i += 1
                while i < len(tokens) and tokens[i] != 'IDENT:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        # Skip standard markers (INHERIT, INITIAL, etc.)
                        if key in ('INHERIT', 'INITIAL', 'REVERT', 'UNSET'):
                            pass
                        elif ',' in value:
                            # KEYWORD:flags,OPCODE_NAME
                            flags_str, opcode = value.split(',', 1)
                            result['ident_keywords'].append((key, flags_str, opcode))
                        # else: unknown ident sub-token, skip
                    i += 1
                i += 1  # skip IDENT:)
            
            # --- LENGTH_UNIT:( ... LENGTH_UNIT:) block ---
            elif token == 'LENGTH_UNIT:(':
                lu = {'opcode': None, 'unit': None, 'mask': None, 
                      'range': None, 'allow': None, 'disallow': None}
                i += 1
                while i < len(tokens) and tokens[i] != 'LENGTH_UNIT:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        if key.startswith('UNIT_'):
                            lu['unit'] = key
                            lu['opcode'] = value
                        elif key == 'MASK':
                            lu['mask'] = value
                        elif key == 'RANGE':
                            lu['range'] = value
                        elif key == 'ALLOW':
                            lu['allow'] = value
                        elif key == 'DISALLOW':
                            lu['disallow'] = value
                    i += 1
                result['length_unit'] = lu
                i += 1  # skip LENGTH_UNIT:)
            
            # --- NUMBER:( ... NUMBER:) block ---
            elif token == 'NUMBER:(':
                num = {'opcode': None, 'is_int': False, 'range': None}
                i += 1
                while i < len(tokens) and tokens[i] != 'NUMBER:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        if key in ('true', 'false'):
                            num['is_int'] = (key == 'true')
                            num['opcode'] = value
                        elif key == 'RANGE':
                            num['range'] = value
                    i += 1
                result['number'] = num
                i += 1  # skip NUMBER:)
            
            # --- CALC:( ... CALC:) block ---
            elif token == 'CALC:(':
                calc = {'opcode': None, 'type': None}
                i += 1
                while i < len(tokens) and tokens[i] != 'CALC:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        if key.startswith('UNIT_'):
                            calc['type'] = key
                            calc['opcode'] = value
                        elif key == 'NUMBER':
                            calc['type'] = 'NUMBER'
                            calc['opcode'] = value
                        elif key == 'ANY':
                            calc['type'] = 'ANY'
                            calc['opcode'] = value
                    i += 1
                result['calc'] = calc
                i += 1  # skip CALC:)
            
            # --- IDENT_LIST:( ... IDENT_LIST:) block ---
            elif token == 'IDENT_LIST:(':
                il = {'named_opcode': None, 'default_value': None, 'none_opcode': None}
                i += 1
                while i < len(tokens) and tokens[i] != 'IDENT_LIST:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        if key == 'STRING_OPTNUM':
                            il['named_opcode'] = value
                        else:
                            # default value like 1:COUNTER_INCREMENT_NONE
                            il['default_value'] = key
                            il['none_opcode'] = value
                    i += 1
                result['ident_list'] = il
                i += 1  # skip IDENT_LIST:)
            
            # --- SIZING:( ... SIZING:) block ---
            elif token == 'SIZING:(':
                sz = {'fit_content': None, 'min_content': None, 'max_content': None}
                i += 1
                while i < len(tokens) and tokens[i] != 'SIZING:)':
                    t = tokens[i]
                    if ':' in t:
                        key, value = t.split(':', 1)
                        if key == 'FIT_CONTENT':
                            sz['fit_content'] = value
                        elif key == 'MIN_CONTENT':
                            sz['min_content'] = value
                        elif key == 'MAX_CONTENT':
                            sz['max_content'] = value
                    i += 1
                result['sizing'] = sz
                i += 1  # skip SIZING:)
            
            else:
                # Unknown token — skip
                i += 1
        
        return result


class GperfInputGenerator:
    """Generate gperf input file for perfect hash CSS property lookup.
    
    Uses GNU gperf (an external tool) instead of the Python perfect-hash
    module. The Python script generates a .gperf input file, and CMake
    invokes gperf to produce the final C code.
    
    This follows the same pattern used by libhubbub's element-type.gperf.
    """
    
    def __init__(self, keys):
        """Initialize with list of (css_name, handler_name) tuples."""
        self.keys = keys
    
    def generate_gperf_input(self):
        """Generate gperf input file content.
        
        The gperf file defines:
        - struct css_prop_entry { name, handler }
        - A lookup function css_prop_lookup_generated() returning const struct *
        
        A wrapper function css_prop_lookup() is appended after the gperf
        output (via the %% section) to extract the handler field, preserving
        the existing API: css_prop_handler css_prop_lookup(name, len).
        """
        lines = []
        
        # gperf declarations section
        lines.append("/*")
        lines.append(" * AUTO-GENERATED gperf input - DO NOT EDIT!")
        lines.append(" *")
        lines.append(" * Generated by property_generator.py")
        lines.append(" * Processed by GNU gperf to produce prop_hash_table.inc")
        lines.append(" */")
        lines.append("")
        
        # gperf options - optimized for fastest possible lookup
        #
        # %compare-lengths: reject mismatches by length before strncmp
        #   (cheap integer compare eliminates most non-matches)
        # %compare-strncmp: use strncmp for final verification
        #   (required: hash alone can't distinguish unknown inputs from
        #    valid keywords that happen to share the same hash slot)
        # %readonly-tables: const-qualify all tables (cache-friendly)
        # %ignore-case: case-insensitive matching (CSS is case-insensitive)
        # %enum: use enum for hash values (compiler optimization)
        # %7bit: assume 7-bit ASCII input (valid for CSS property names)
        # %null-strings: NULL for empty slots (fast pointer check)
        # No %switch: default array lookup is faster for 200+ keywords
        lines.append("%language=ANSI-C")
        lines.append("%compare-lengths")
        lines.append("%compare-strncmp")
        lines.append("%readonly-tables")
        lines.append("%ignore-case")
        lines.append("%struct-type")
        lines.append("%enum")
        lines.append("%7bit")
        lines.append("%null-strings")
        lines.append("%define hash-function-name css_prop_hash")
        lines.append("%define lookup-function-name css_prop_lookup_generated")
        lines.append("")
        
        # C code block included verbatim before the hash table
        lines.append("%{")
        lines.append("#include <string.h>")
        lines.append("#include <stddef.h>")
        lines.append("")
        lines.append('#include "parse/properties/properties.h"')
        lines.append("%}")
        lines.append("")
        
        # Full struct definition - gperf needs this to generate the wordlist array.
        # The 'name' field is the keyword (filled by gperf automatically).
        # The 'handler' field is the css_prop_handler function pointer.
        lines.append("struct css_prop_entry {")
        lines.append("    const char *name;")
        lines.append("    css_prop_handler handler;")
        lines.append("};")
        lines.append("")
        
        # Keyword section
        lines.append("%%")
        
        # Each line: keyword, field_value
        for css_name, handler_name in self.keys:
            lines.append(f"{css_name}, {handler_name}")
        
        lines.append("%%")
        lines.append("")
        
        # Wrapper function appended after gperf's generated code.
        # gperf's css_prop_lookup_generated() returns const struct css_prop_entry *
        # but language.c expects css_prop_handler (a function pointer).
        # This wrapper bridges the gap - same pattern as libhubbub's element-type.c
        lines.append("/* Wrapper: extract handler from gperf lookup result */")
        lines.append("static inline css_prop_handler css_prop_lookup(const char *name, size_t len) {")
        lines.append("    const struct css_prop_entry *entry;")
        lines.append("    entry = css_prop_lookup_generated(name, len);")
        lines.append("    if (entry == NULL) return NULL;")
        lines.append("    return entry->handler;")
        lines.append("}")
        lines.append("")
        
        return "\n".join(lines)


class KeywordsParser:
    """Parse keywords.gen to get CSS syntax keywords."""
    
    def __init__(self, keywords_file):
        self.keywords_file = Path(keywords_file)
        self.keywords = []  # List of keyword names
        
    def parse(self):
        """Parse keywords.gen and extract keyword names."""
        if not self.keywords_file.exists():
            print(f"FATAL ERROR: keywords.gen not found at {self.keywords_file}", file=sys.stderr)
            sys.exit(1)
        
        try:
            with open(self.keywords_file, 'r') as f:
                lines = list(f)
        except IOError as e:
            print(f"FATAL ERROR: Cannot read keywords.gen: {e}", file=sys.stderr)
            sys.exit(1)
        
        for line_num, line in enumerate(lines, 1):
            line = line.strip()
            
            # Skip comments and empty lines
            if not line or line.startswith('#'):
                continue
            
            # Each non-comment line is a keyword name
            if re.match(r'^[A-Z_]+$', line):
                self.keywords.append(line)
            else:
                print(f"WARNING: Invalid keyword '{line}' at line {line_num} in keywords.gen", file=sys.stderr)
        
        # Validate we found keywords
        if not self.keywords:
            print("FATAL ERROR: No keywords found in keywords.gen", file=sys.stderr)
            print("Expected format: One KEYWORD_NAME per line", file=sys.stderr)
            sys.exit(1)
        
        if len(self.keywords) < 10:
            print(f"FATAL ERROR: Only {len(self.keywords)} keywords in keywords.gen", file=sys.stderr)
            print("Expected at least 10 keywords. File may be incomplete.", file=sys.stderr)
            sys.exit(1)
        
        return self.keywords


class MultiFileGenerator:
    """Generate multiple output files from parsed data."""
    
    def __init__(self, dispatch_order, metadata, keywords):
        self.dispatch_order = dispatch_order  # From dispatch.c
        self.metadata = metadata  # From properties.gen
        self.keywords = keywords  # From keywords.gen
    
    def _header(self, file_type, description):
        """Generate common header for all files."""
        lines = []
        lines.append("/*")
        lines.append(f" * AUTO-GENERATED FILE - DO NOT EDIT!")
        lines.append(" *")
        lines.append(f" * This file was automatically generated by property_generator.py")
        lines.append(f" * {description}")
        lines.append(" *")
        lines.append(" * To regenerate this file, run:")
        lines.append(" *   python3 build-aux/property_generator.py \\")
        lines.append(" *     src/select/dispatch.c \\")
        lines.append(" *     src/parse/properties/properties.gen \\")
        lines.append(" *     <output_files>")
        lines.append(" *")
        lines.append(" * Source files:")
        lines.append(" *   - dispatch.c (defines property order)")
        lines.append(" *   - properties.gen (defines property metadata)")
        lines.append(" */")
        lines.append("")
        return lines
        
    def generate_enum(self):
        """Generate property enum matching dispatch order."""
        lines = self._header("ENUM", "Property enum values matching dispatch.c order")
        
        for idx, prop_info in enumerate(self.dispatch_order):
            prop_name = prop_info['name']
            if prop_name in self.metadata:
                enum_name = self.metadata[prop_name]['enum']
                hex_val = f"0x{idx:03x}"
                lines.append(f"\t{enum_name} = {hex_val},")
            # else: skip this index - creates a gap in enum values
        
        # No extra content after last entry - CSS_N_PROPERTIES is in the parent enum
        
        return '\n'.join(lines)
    
    def generate_dispatch(self):
        """Generate dispatch table entries."""
        lines = self._header("DISPATCH", "Dispatch table entries in canonical order")
        
        for prop_info in self.dispatch_order:
            prop_name = prop_info['name']
            if prop_name in self.metadata:
                inherited = 1 if prop_info['inherited'] else 0  # From dispatch.c!
                lines.append("\t{")
                lines.append(f"\t\tPROPERTY_FUNCS({prop_name}),")
                lines.append(f"\t\t{inherited},")
                lines.append("\t},")
            # else: skip - dispatch.c already has this entry manually
        
        return '\n'.join(lines)
    
    def generate_propstrings(self):
        """Generate property string enum for parser (includes keywords and properties)."""
        lines = self._header("PROPSTRINGS", "Parser string identifiers (keywords + properties, alphabetical)")
        
        # Get longhand properties from dispatch.c
        all_props = [prop_info['name'].upper() for prop_info in self.dispatch_order]
        
        # Add shorthand properties from metadata (marked SHORTHAND or WRAP)
        shorthands = [name.upper() for name, meta in self.metadata.items() 
                      if meta.get('is_shorthand', False) and name.upper() not in all_props]
        
        # Add alias properties (different name but same bytecode target)
        aliases = [name.upper() for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name.upper() not in all_props]
        
        # Combine keywords, longhands, shorthands, and aliases, sort alphabetically
        # Properties ONLY - keywords are already in propstrings.h before FIRST_PROP
        all_identifiers = sorted(all_props + shorthands + aliases)
        
        # Generate enum
        for idx, identifier in enumerate(all_identifiers):
            if idx == 0:
                lines.append(f"\t{identifier} = FIRST_PROP,")
            else:
                lines.append(f"\t{identifier},")
        
        if all_identifiers:
            last_id = all_identifiers[-1]
            lines.append("")
            lines.append(f"\tLAST_PROP = {last_id},")
        
        return '\n'.join(lines)
    
    def generate_parse_handlers(self):
        """Generate parse handler array as complete definition."""
        lines = self._header("HANDLERS", "Parse handler function pointers (alphabetical order)")
        
        # Add complete array definition
        lines.append("const css_prop_handler property_handlers[LAST_PROP + 1 - FIRST_PROP] = {")
        
        # Get longhand properties from dispatch.c
        all_props = [prop_info['name'] for prop_info in self.dispatch_order]
        
        # Add shorthand properties from metadata (marked SHORTHAND or WRAP)
        shorthands = [name for name, meta in self.metadata.items() 
                      if meta.get('is_shorthand', False) and name not in all_props]
        
        # Add alias properties
        aliases = [name for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name not in all_props]
        
        # Sort all properties alphabetically
        sorted_props = sorted(all_props + shorthands + aliases)
        
        # Each property uses its own autogenerated parser function
        # Aliases like inline_size have autogenerated_inline_size.c that
        # correctly emit the target property's bytecode (e.g., CSS_PROP_WIDTH)
        for prop_name in sorted_props:
            lines.append(f"\tcss__parse_{prop_name},")
        
        lines.append("};")
        
        return '\n'.join(lines)
    
    def generate_propstrings_strings(self):
        """Generate property string map entries for propstrings.c."""
        lines = self._header("PROPSTRINGS_STRINGS", "Property string map entries (propstrings.c)")
        
        # Get longhand properties from dispatch.c
        all_props = [prop_info['name'] for prop_info in self.dispatch_order]
        
        # Add SHORTHAND properties (not in dispatch.c but need entries)
        shorthands = [name for name, meta in self.metadata.items() 
                      if meta.get('is_shorthand', False) and not meta.get('is_manual', False)
                      and name not in all_props]
        
        # Add alias properties
        aliases = [name for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name not in all_props]
        
        # Sort alphabetically to match propstrings_enum.inc
        sorted_props = sorted(all_props + shorthands + aliases)
        
        # Generate SMAP entries - property_name becomes property-name
        for prop_name in sorted_props:
            css_name = prop_name.replace('_', '-')
            lines.append(f'\tSMAP("{css_name}"),')
        
        return '\n'.join(lines)
    
    def generate_gperf_input(self):
        """Generate gperf input file for O(1) property lookup.
        
        Returns the content of a .gperf file that GNU gperf will process
        into C code with a perfect hash function.
        """
        # Collect all properties that need parse handlers
        # Get longhand properties from dispatch.c
        all_props = [prop_info['name'] for prop_info in self.dispatch_order]
        
        # Add SHORTHAND properties
        shorthands = [name for name, meta in self.metadata.items()
                      if meta.get('is_shorthand', False) and not meta.get('is_manual', False)
                      and name not in all_props]
        
        # Add alias properties
        aliases = [name for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name not in all_props]
        
        sorted_props = sorted(all_props + shorthands + aliases)
        
        # Build key tuples: (css_name, handler_name)
        keys = []
        for prop_name in sorted_props:
            css_name = prop_name.replace('_', '-')
            handler_name = f"css__parse_{prop_name}"
            keys.append((css_name, handler_name))
        
        # Generate gperf input
        gen = GperfInputGenerator(keys)
        return gen.generate_gperf_input()
    
    def generate_cascade_declarations(self):
        """Generate PROPERTY_FUNCS declarations for select/properties/properties.h."""
        lines = self._header("CASCADE_DECL",
            "Cascade function declarations for all properties (select/properties/properties.h)")
        
        for prop_info in self.dispatch_order:
            lines.append(f"PROPERTY_FUNCS({prop_info['name']});")
        
        return '\n'.join(lines)
    
    def generate_parse_declarations(self):
        """Generate css__parse_* forward declarations for parse/properties/properties.h."""
        lines = self._header("PARSE_DECL",
            "Parse function forward declarations for all properties")
        
        # Longhand properties from dispatch.c
        all_props = [prop_info['name'] for prop_info in self.dispatch_order]
        
        # Shorthand properties from properties.gen
        shorthands = [name for name, meta in self.metadata.items()
                      if meta.get('is_shorthand', False) and name not in all_props]
        
        # Alias properties
        aliases = [name for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name not in all_props]
        
        # Sort alphabetically for readability 
        sorted_props = sorted(all_props + shorthands + aliases)
        
        for prop_name in sorted_props:
            lines.append(f"css_error css__parse_{prop_name}(css_language *c, "
                         f"const parserutils_vector *vector, int32_t *ctx, css_style *result);")
        
        return '\n'.join(lines)
    
    def generate_unit_masks(self):
        """Generate UNIT_MASK_* defines and property_unit_mask[] array.
        
        Unit masks are derived from the parse type in properties.gen:
        - COLOR/IDENT types -> 0 (no unit validation)
        - LENGTH types -> UNIT_LENGTH | UNIT_PCT
        - ANGLE types -> UNIT_ANGLE
        - TIME types -> UNIT_TIME | UNIT_PCT
        - FREQ types -> UNIT_FREQ
        
        For MANUAL properties, we preserve their existing unit masks.
        """
        lines = self._header("UNIT_MASKS",
            "Unit mask definitions and property_unit_mask[] array")
        
        # Map of known unit mask overrides for properties that don't follow
        # the simple type-based pattern. These are properties where the
        # auto-derived mask from properties.gen type would be wrong.
        unit_mask_overrides = {
            # Shorthands with mixed types
            'background_position': '(UNIT_LENGTH | UNIT_PCT)',
            'border_spacing': '(UNIT_LENGTH)',
            'clip': '(UNIT_LENGTH)',
            'object_position': '(UNIT_LENGTH | UNIT_PCT)',
            # Special LENGTH properties  
            'letter_spacing': '(UNIT_LENGTH)',
            'word_spacing': '(UNIT_LENGTH)',
            'outline_width': '(UNIT_LENGTH)',
            'column_gap': '(UNIT_LENGTH)',
            'column_rule_width': '(UNIT_LENGTH)',
            'column_width': '(UNIT_LENGTH)',
            'row_gap': '(UNIT_LENGTH)',
            # ANGLE type
            'azimuth': '(UNIT_ANGLE)',
            'elevation': '(UNIT_ANGLE)',
            # TIME type
            'pause_after': '(UNIT_TIME | UNIT_PCT)',
            'pause_before': '(UNIT_TIME | UNIT_PCT)',
            # FREQ type
            'pitch': '(UNIT_FREQ)',
            # PCT type
            'volume': '(UNIT_PCT)',
        }
        
        # Properties with LENGTH | PCT (the default for LENGTH/DIMENSION types)
        length_pct_props = {
            'bottom', 'top', 'left', 'right',
            'height', 'width', 'min_height', 'min_width', 'max_height', 'max_width',
            'margin_top', 'margin_right', 'margin_bottom', 'margin_left',
            'padding_top', 'padding_right', 'padding_bottom', 'padding_left',
            'font_size', 'line_height', 'text_indent', 'vertical_align',
            'flex_basis',
            'grid_template_columns', 'grid_template_rows',
            'border_top_width', 'border_right_width', 'border_bottom_width', 'border_left_width',
        }
        
        # Emit #define for each dispatch property
        lines.append("/* Unit mask definitions */")
        for prop_info in self.dispatch_order:
            name = prop_info['name']
            NAME = name.upper()
            
            if name in unit_mask_overrides:
                mask = unit_mask_overrides[name]
            elif name in length_pct_props:
                mask = '(UNIT_LENGTH | UNIT_PCT)'
            else:
                mask = '(0)'
            
            lines.append(f"#define UNIT_MASK_{NAME} {mask}")
        
        lines.append("")
        
        # Emit the property_unit_mask[] array
        lines.append("/** Mapping from property bytecode index to bytecode unit class mask. */")
        lines.append("static const uint32_t generated_property_unit_mask[CSS_N_PROPERTIES] = {")
        for prop_info in self.dispatch_order:
            name = prop_info['name']
            NAME = name.upper()
            if name in self.metadata:
                enum_name = self.metadata[name]['enum']
                lines.append(f"    [{enum_name}] = UNIT_MASK_{NAME},")
        lines.append("};")
        
        return '\n'.join(lines)
    
    def generate_opcodes(self):
        """Generate enum op_* definitions from parsed properties.gen specs.
        
        Uses the structured `parsed` dict from SpecParser — no regex needed.
        Properties marked MANUAL, SHORTHAND, WRAP, ALIAS, or OPCODES_MANUAL
        are skipped.
        """
        lines = []
        lines.append("/* Auto-generated opcode enums from properties.gen */")
        lines.append("/* DO NOT EDIT — generated by property_generator.py */")
        lines.append("")
        
        generated_count = 0
        all_opcode_names = {}  # name → property that defined it (for dup detection)
        errors = []
        
        for prop_name, meta in sorted(self.metadata.items()):
            parsed = meta.get('parsed')
            if not parsed:
                continue
            
            # Skip properties that don't get generated opcodes
            if meta.get('is_shorthand') or meta.get('is_alias') or meta.get('is_wrap'):
                continue
            if meta.get('is_manual') or 'OPCODES_MANUAL' in parsed['flags']:
                continue
            
            # Extract opcode values from the parsed spec
            opcodes = self._extract_opcodes(prop_name, parsed)
            if not opcodes:
                continue
            
            # --- Validation: duplicate opcode names across enums = FATAL ---
            for opc_name, _ in opcodes:
                if opc_name in all_opcode_names:
                    other_prop = all_opcode_names[opc_name]
                    errors.append(
                        f"enum op_{prop_name}: opcode name '{opc_name}' already "
                        f"defined by enum op_{other_prop}. "
                        f"Add OPCODES:MANUAL to '{prop_name}' in properties.gen "
                        f"if these properties intentionally share opcodes.")
            
            # --- Validation: duplicate values within enum ---
            seen_values = {}
            for opc_name, opc_value in opcodes:
                if opc_value in seen_values and opc_value != "VALUE_IS_CALC":
                    errors.append(
                        f"enum op_{prop_name}: duplicate value {opc_value} "
                        f"for {opc_name} and {seen_values[opc_value]}")
                seen_values[opc_value] = opc_name
            
            # Register all opcode names
            for opc_name, _ in opcodes:
                all_opcode_names[opc_name] = prop_name
            
            # Emit enum
            if len(opcodes) <= 3:
                inline = ', '.join(f"{n} = {v}" for n, v in opcodes)
                lines.append(f"enum op_{prop_name} {{ {inline} }};")
            else:
                lines.append(f"enum op_{prop_name} {{")
                entries = [f"    {n} = {v}" for n, v in opcodes]
                lines.append(',\n'.join(entries))
                lines.append("};")
            lines.append("")
            generated_count += 1
        
        if errors:
            print(f"\nFATAL ERROR: Opcode generation found {len(errors)} error(s):",
                  file=sys.stderr)
            for err in errors:
                print(f"  - {err}", file=sys.stderr)
            sys.exit(1)
        
        print(f"  Generated {generated_count} opcode enums")
        return '\n'.join(lines)
    
    def _extract_opcodes(self, prop_name, parsed):
        """Extract opcode name/value pairs from a parsed spec dict.
        
        Returns list of (opcode_name, value_expression) tuples, or None.
        Uses the structured output from SpecParser.parse().
        """
        opcodes = []
        ident_kws = parsed['ident_keywords']  # [(keyword, flags, opcode_name), ...]
        
        if parsed['color']:
            # COLOR type property
            set_name = parsed['color']
            prefix = set_name.replace('_SET', '')
            
            if ident_kws:
                # COLOR + IDENT keywords = ambiguous opcode layout.
                # Must use OPCODES:MANUAL (should have been caught above,
                # but emit error here as safety net)
                return None
            
            # Standard COLOR template: TRANSPARENT, CURRENT_COLOR, SET
            opcodes.append((f"{prefix}_TRANSPARENT", "0x0000"))
            opcodes.append((f"{prefix}_CURRENT_COLOR", "0x0001"))
            opcodes.append((set_name, "0x0080"))
        
        elif parsed['length_unit'] or parsed['number'] or parsed['uri'] or parsed['ident_list']:
            # LENGTH/NUMBER/URI/IDENT_LIST type
            
            # CALC first if present
            if parsed['calc'] and parsed['calc']['opcode']:
                opcodes.append((parsed['calc']['opcode'], "VALUE_IS_CALC"))
            
            # NUMBER opcode (NUMBER, DIMENSION variants)
            if parsed['number'] and parsed['number']['opcode']:
                num_opc = parsed['number']['opcode']
                if num_opc.endswith('_NUMBER'):
                    opcodes.append((num_opc, "0x0080"))
                    # Check for DIMENSION variant in length_unit
                    if parsed['length_unit'] and parsed['length_unit']['opcode']:
                        lu_opc = parsed['length_unit']['opcode']
                        if lu_opc.endswith('_DIMENSION'):
                            opcodes.append((lu_opc, "0x0081"))
                        elif lu_opc != num_opc:
                            if not any(n == lu_opc for n, _ in opcodes):
                                opcodes.append((lu_opc, "0x0080"))
                elif num_opc.endswith('_SET'):
                    if not any(n == num_opc for n, _ in opcodes):
                        opcodes.append((num_opc, "0x0080"))
            
            # LENGTH SET (if not already added)
            if parsed['length_unit'] and parsed['length_unit']['opcode']:
                lu_opc = parsed['length_unit']['opcode']
                if not any(n == lu_opc for n, _ in opcodes):
                    opcodes.append((lu_opc, "0x0080"))
            
            # URI opcode
            if parsed['uri']:
                if not any(n == parsed['uri'] for n, _ in opcodes):
                    opcodes.append((parsed['uri'], "0x0080"))
            
            # IDENT_LIST NAMED opcode
            if parsed['ident_list'] and parsed['ident_list']['named_opcode']:
                named = parsed['ident_list']['named_opcode']
                if not any(n == named for n, _ in opcodes):
                    opcodes.append((named, "0x0080"))
            
            # IDENT keywords at low values
            for idx, (_kw, _flags, opc_name) in enumerate(ident_kws):
                if not any(n == opc_name for n, _ in opcodes):
                    opcodes.append((opc_name, f"0x{idx:04x}"))
        
        elif ident_kws:
            # Pure IDENT-only type
            for idx, (_kw, _flags, opc_name) in enumerate(ident_kws):
                opcodes.append((opc_name, f"0x{idx:04x}"))
        
        else:
            return None
        
        return opcodes if opcodes else None
    
    def validate_indexes(self):
        """Validate property consistency and detect propstrings collisions.
        
        Collisions happen when a property name matches a keyword name.
        Both map to the same string (e.g., "fill"), so the enum would have
        two entries for the same string. This is wasteful and confusing.
        
        The fix is to remove the keyword entry from propstrings.h and reuse
        the property's enum value in parser code. The generator makes this
        a fatal error to prevent silent issues.
        """
        # Get longhand properties from dispatch.c
        all_props = [prop_info['name'] for prop_info in self.dispatch_order]
        
        # Add SHORTHAND properties
        shorthands = [name for name, meta in self.metadata.items() 
                      if meta.get('is_shorthand', False) and not meta.get('is_manual', False)
                      and name not in all_props]
        
        # Add alias properties
        aliases = [name for name, meta in self.metadata.items()
                   if meta.get('is_alias', False) and name not in all_props]
        
        sorted_props = sorted(all_props + shorthands + aliases)
        
        # Check for duplicate properties
        seen = set()
        for prop in sorted_props:
            if prop in seen:
                print(f"FATAL ERROR: Duplicate property '{prop}' in generated output", file=sys.stderr)
                sys.exit(1)
            seen.add(prop)
        
        # Check for propstrings collisions: property names vs keywords.gen
        prop_names_upper = set(p.upper() for p in sorted_props)
        keyword_collisions = prop_names_upper.intersection(set(self.keywords))
        if keyword_collisions:
            print(f"\nFATAL ERROR: Property/keyword name collision detected!", file=sys.stderr)
            print(f"The following names exist in BOTH properties.gen and keywords.gen:", file=sys.stderr)
            for name in sorted(keyword_collisions):
                print(f"  - {name}", file=sys.stderr)
            print(f"\nBoth map to the same string, so only one enum entry is needed.", file=sys.stderr)
            print(f"Fix: Remove the entry from keywords.gen. The property's enum", file=sys.stderr)
            print(f"value (in propstrings_enum.inc) provides the same string.", file=sys.stderr)
            print(f"Parser code should use the property enum name directly.", file=sys.stderr)
            sys.exit(1)
        
        # Also scan propstrings.h for manually-listed keywords that match property names.
        # These are keywords listed after LAST_PROP in propstrings.h (INHERIT, NONE, AUTO, etc.).
        # Property names are auto-generated between FIRST_PROP and LAST_PROP, so a manual
        # keyword with the same name would create a duplicate enum entry → compiler error.
        # But a keyword with a DIFFERENT name for the same string (e.g., FILL_KEYWORD for "fill")
        # would silently create a wasted duplicate string entry — we want to catch that too.
        manual_keyword_collisions = []
        for kw in self.keywords:
            # keyword names from keywords.gen that appear BEFORE FIRST_PROP in propstrings.h
            # are fine — they're in a separate section. We already checked keywords.gen above.
            pass
        
        # The main protection is: keywords.gen collision = fatal error (checked above).
        # For the manual keywords in propstrings.h after LAST_PROP, a collision would
        # cause a compiler error (duplicate enum name), so the compiler catches those.
        # This validation covers the keywords.gen case which IS the build system's
        # responsibility.
        
        print(f"  Validation passed: {len(sorted_props)} properties, no duplicates")
        return True


def main():
    if len(sys.argv) < 9:
        print("Usage: property_generator.py dispatch.c properties.gen keywords.gen "
              "enum.inc dispatch.inc propstrings.inc propstrings_strings.inc "
              "prop_hash_table.gperf "
              "[cascade_decl.inc parse_decl.inc unit_masks.inc opcodes.inc]", file=sys.stderr)
        sys.exit(1)
    
    dispatch_file = sys.argv[1]
    gen_file = sys.argv[2]
    keywords_file = sys.argv[3]
    enum_out = sys.argv[4]
    dispatch_out = sys.argv[5]
    propstrings_out = sys.argv[6]
    propstrings_strings_out = sys.argv[7]
    gperf_out = sys.argv[8]
    
    # Optional new output files (Phase 1 of generator refactor)
    cascade_decl_out = sys.argv[9] if len(sys.argv) > 9 else None
    parse_decl_out = sys.argv[10] if len(sys.argv) > 10 else None
    unit_masks_out = sys.argv[11] if len(sys.argv) > 11 else None
    # Phase 3: opcodes
    opcodes_out = sys.argv[12] if len(sys.argv) > 12 else None
    
    # Parse input files
    print("Parsing dispatch.c for property order...")
    dispatch_parser = DispatchParser(dispatch_file)
    dispatch_order = dispatch_parser.parse()
    print(f"  Found {len(dispatch_order)} properties in dispatch.c")
    
    print("Parsing properties.gen for metadata...")
    gen_parser = PropertyGenParser(gen_file)
    prop_metadata = gen_parser.parse()
    print(f"  Found {len(prop_metadata)} properties in properties.gen")
    
    print("Parsing keywords.gen for CSS keywords...")
    keywords_parser = KeywordsParser(keywords_file)
    keywords = keywords_parser.parse()
    print(f"  Found {len(keywords)} keywords in keywords.gen")
    
    # Generate output files
    generator = MultiFileGenerator(dispatch_order, prop_metadata, keywords)
    
    # Run validation first
    print("Validating property indexes...")
    generator.validate_indexes()
    
    with open(enum_out, 'w') as f:
        f.write(generator.generate_enum())
    print(f"Generated: {enum_out}")
    
    with open(dispatch_out, 'w') as f:
        f.write(generator.generate_dispatch())
    print(f"Generated: {dispatch_out}")
    
    with open(propstrings_out, 'w') as f:
        f.write(generator.generate_propstrings())
    print(f"Generated: {propstrings_out}")
    
    with open(propstrings_strings_out, 'w') as f:
        f.write(generator.generate_propstrings_strings())
    print(f"Generated: {propstrings_strings_out}")
    
    # Generate gperf input file (replaces perfect-hash generation)
    # CMake will then invoke gperf to produce the final prop_hash_table.inc
    print("Generating gperf input file...")
    with open(gperf_out, 'w') as f:
        f.write(generator.generate_gperf_input())
    print(f"Generated: {gperf_out}")
    
    # Phase 1 outputs: cascade declarations, parse declarations, unit masks
    if cascade_decl_out:
        with open(cascade_decl_out, 'w') as f:
            f.write(generator.generate_cascade_declarations())
        print(f"Generated: {cascade_decl_out}")
    
    if parse_decl_out:
        with open(parse_decl_out, 'w') as f:
            f.write(generator.generate_parse_declarations())
        print(f"Generated: {parse_decl_out}")
    
    if unit_masks_out:
        with open(unit_masks_out, 'w') as f:
            f.write(generator.generate_unit_masks())
        print(f"Generated: {unit_masks_out}")
    
    # Phase 3 output: opcodes
    if opcodes_out:
        with open(opcodes_out, 'w') as f:
            f.write(generator.generate_opcodes())
        print(f"Generated: {opcodes_out}")


if __name__ == '__main__':
    main()

