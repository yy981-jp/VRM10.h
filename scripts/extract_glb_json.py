#!/usr/bin/env python3
"""Extract the JSON chunk from a GLB v2 file for parser integration tests.

This is a test utility, not a GLB loader. It intentionally does not inspect,
decode, or load BIN chunks, images, buffers, or external resources.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

GLB_MAGIC = b"glTF"
GLB_VERSION = 2
JSON_CHUNK_TYPE = 0x4E4F534A
CHUNK_HEADER_SIZE = 8
GLB_HEADER_SIZE = 12


def extract_json_chunk(source: Path) -> bytes:
    data = source.read_bytes()
    if len(data) < GLB_HEADER_SIZE:
        raise ValueError("File is shorter than the GLB header.")

    magic, version, declared_length = struct.unpack_from("<4sII", data, 0)
    if magic != GLB_MAGIC:
        raise ValueError("File does not have the GLB magic identifier.")
    if version != GLB_VERSION:
        raise ValueError(f"Unsupported GLB version: {version}.")
    if declared_length != len(data):
        raise ValueError(
            f"GLB length mismatch: header={declared_length}, actual={len(data)}."
        )

    offset = GLB_HEADER_SIZE
    json_chunk: bytes | None = None
    chunk_index = 0

    while offset < len(data):
        if offset + CHUNK_HEADER_SIZE > len(data):
            raise ValueError("Truncated GLB chunk header.")

        chunk_length, chunk_type = struct.unpack_from("<II", data, offset)
        offset += CHUNK_HEADER_SIZE
        chunk_end = offset + chunk_length
        if chunk_end > len(data):
            raise ValueError("GLB chunk exceeds the declared file length.")

        if chunk_index == 0 and chunk_type != JSON_CHUNK_TYPE:
            raise ValueError("The first GLB chunk must be the JSON chunk.")
        if chunk_type == JSON_CHUNK_TYPE:
            if json_chunk is not None:
                raise ValueError("GLB must not contain multiple JSON chunks.")
            json_chunk = data[offset:chunk_end]

        offset = chunk_end
        chunk_index += 1

    if json_chunk is None:
        raise ValueError("GLB does not contain a JSON chunk.")

    # GLB JSON chunks are padded with spaces to a four-byte boundary.
    payload = json_chunk.rstrip(b" \t\r\n\0")
    json.loads(payload)
    return payload


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    try:
        payload = extract_json_chunk(args.input)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(payload + b"\n")
    except (OSError, ValueError, json.JSONDecodeError) as error:
        parser.error(str(error))

    print(f"Extracted {len(payload)} JSON bytes: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
