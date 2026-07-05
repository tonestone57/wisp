import os
import re
import sys

def patch_file(filepath, pattern, replacement):
    if not os.path.exists(filepath):
        print(f"Warning: {filepath} not found")
        return
    with open(filepath, 'r') as f:
        content = f.read()

    if replacement in content:
        print(f"Already patched or replacement exists in: {filepath}")
        return

    new_content = re.sub(pattern, replacement, content)
    if new_content == content:
        print(f"Could not find pattern in {filepath}")
    else:
        with open(filepath, 'w') as f:
            f.write(new_content)
        print(f"Successfully patched: {filepath}")

def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    blend2d_dir = os.path.join(base_dir, "blend2d")

    if not os.path.exists(blend2d_dir):
        print(f"Error: Blend2D directory not found at {blend2d_dir}")
        sys.exit(1)

    # 1. Patch runtime.cpp to support disabling auto-initialization
    runtime_cpp = os.path.join(blend2d_dir, "blend2d", "core", "runtime.cpp")
    patch_file(runtime_cpp,
               r"static BL_RUNTIME_INITIALIZER BLRuntimeInitializer bl_runtime_auto_init;",
               r"#ifndef BL_NO_AUTO_INIT\nstatic BL_RUNTIME_INITIALIZER BLRuntimeInitializer bl_runtime_auto_init;\n#endif")

    # 2. Patch object.cpp to set init_priority for static globals
    object_cpp = os.path.join(blend2d_dir, "blend2d", "core", "object.cpp")
    patch_file(object_cpp,
               r"BLObjectCore bl_object_defaults\[BL_OBJECT_TYPE_MAX_VALUE \+ 1\];",
               r"BLObjectCore bl_object_defaults[BL_OBJECT_TYPE_MAX_VALUE + 1] __attribute__((init_priority(101)));")
    patch_file(object_cpp,
               r"const BLObjectImplHeader bl_object_header_with_ref_count_eq_0 = \{ 0, 0 \};",
               r"const BLObjectImplHeader bl_object_header_with_ref_count_eq_0 __attribute__((init_priority(101))) = { 0, 0 };")
    patch_file(object_cpp,
               r"const BLObjectImplHeader bl_object_header_with_ref_count_eq_1 = \{ 1, 0 \};",
               r"const BLObjectImplHeader bl_object_header_with_ref_count_eq_1 __attribute__((init_priority(101))) = { 1, 0 };")

    # 3. Patch imagedecoder.cpp to set init_priority for default_decoder
    imagedecoder_cpp = os.path.join(blend2d_dir, "blend2d", "core", "imagedecoder.cpp")
    patch_file(imagedecoder_cpp,
               r"static BLObjectEternalVirtualImpl<BLImageDecoderImpl, BLImageDecoderVirt> default_decoder;",
               r"static BLObjectEternalVirtualImpl<BLImageDecoderImpl, BLImageDecoderVirt> default_decoder __attribute__((init_priority(101)));")

if __name__ == "__main__":
    main()
