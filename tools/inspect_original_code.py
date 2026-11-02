#!/usr/bin/env python3
"""Read-only x86 code inspection using the LLVM disassembler supplied with macOS CLT."""

import argparse
import ctypes
from pathlib import Path


def create_disassembler(library_path, mode):
    library = ctypes.CDLL(str(library_path))
    library.lto_initialize_disassembler.argtypes = []
    library.lto_initialize_disassembler.restype = None
    library.lto_initialize_disassembler()
    library.LLVMCreateDisasmCPUFeatures.argtypes = [
        ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p,
        ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
    library.LLVMCreateDisasmCPUFeatures.restype = ctypes.c_void_p
    context = library.LLVMCreateDisasmCPUFeatures(
        b"i386-pc-none", b"i386", f"+{mode}bit-mode".encode(), None, 0, None, None)
    if not context:
        raise ValueError("The installed LLVM library does not support x86 disassembly")
    library.LLVMDisasmDispose.argtypes = [ctypes.c_void_p]
    library.LLVMDisasmDispose.restype = None
    library.LLVMSetDisasmOptions.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
    library.LLVMSetDisasmOptions.restype = ctypes.c_int
    library.LLVMSetDisasmOptions(context, 4)  # Intel syntax.
    library.LLVMDisasmInstruction.argtypes = [
        ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint64,
        ctypes.c_uint64, ctypes.c_void_p, ctypes.c_size_t]
    library.LLVMDisasmInstruction.restype = ctypes.c_size_t
    return library, context


def disassemble(data, start, end, mode, library_path):
    if not 0 <= start < end <= len(data):
        raise ValueError("Requested file range is outside the executable")
    library, context = create_disassembler(library_path, mode)
    try:
        offset = start
        while offset < end:
            code = ctypes.create_string_buffer(data[offset:min(offset + 16, end)])
            text = ctypes.create_string_buffer(256)
            count = library.LLVMDisasmInstruction(
                context, code, min(16, end - offset), offset, text, len(text))
            if not count:
                count = 1
                text.value = b"<undecoded byte>"
            raw = data[offset:offset + count].hex(" ")
            print(f"{offset:06x} {raw:32} {text.value.decode()}")
            offset += count
    finally:
        library.LLVMDisasmDispose(context)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=Path)
    parser.add_argument("start", type=lambda value: int(value, 0))
    parser.add_argument("end", type=lambda value: int(value, 0))
    parser.add_argument("--mode", choices=(16, 32), type=int, default=32)
    parser.add_argument("--llvm-library", type=Path, default=Path(
        "/Library/Developer/CommandLineTools/usr/lib/libLTO.dylib"))
    args = parser.parse_args()
    try:
        disassemble(args.exe.read_bytes(), args.start, args.end, args.mode, args.llvm_library)
    except (OSError, ValueError, AttributeError) as error:
        parser.exit(1, f"Inspection failed: {error}\n")


if __name__ == "__main__":
    main()
