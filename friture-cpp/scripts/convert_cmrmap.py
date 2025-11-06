#!/usr/bin/env python3
"""
Convert Python CMRMAP JSON to C++ array initialization

This script reads the CMRMAP colormap data from Friture's Python implementation
and outputs C++ array initialization code suitable for embedding in the C++ port.

Usage:
    python3 convert_cmrmap.py > /path/to/output.txt

The output is formatted as a C++ static array initialization that can be
directly copied into color_transform.cpp.

Source: friture/plotting/generated_cmrmap.py
Target: friture-cpp/src/processing/color_transform.cpp
"""

import json
import sys
import os

def main():
    # Path to Python CMRMAP data (relative to this script's parent directory)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    parent_dir = os.path.dirname(script_dir)
    cmrmap_path = os.path.join(
        parent_dir, '..', 'friture', 'plotting', 'generated_cmrmap.py'
    )

    try:
        with open(cmrmap_path, 'r') as f:
            content = f.read()
    except FileNotFoundError:
        print(f"Error: Could not find {cmrmap_path}", file=sys.stderr)
        print("Make sure you're running this from the friture-cpp directory", file=sys.stderr)
        sys.exit(1)

    # Extract JSON string
    json_start = content.find('"""') + 3
    json_end = content.rfind('"""')
    json_str = content[json_start:json_end]

    # Parse JSON
    try:
        cmap_data = json.loads(json_str)
    except json.JSONDecodeError as e:
        print(f"Error parsing JSON: {e}", file=sys.stderr)
        sys.exit(1)

    # Output C++ array
    print(f"// CMRMAP data - {len(cmap_data)} RGB triplets")
    print("// Generated from Friture's plotting/generated_cmrmap.py")
    print("// Black → Purple → Red → Yellow → White")
    print("static const float CMRMAP_DATA[256][3] = {")

    for i, rgb in enumerate(cmap_data):
        r, g, b = rgb
        if i < len(cmap_data) - 1:
            print(f"    {{{r:.18f}f, {g:.18f}f, {b:.18f}f}},")
        else:
            print(f"    {{{r:.18f}f, {g:.18f}f, {b:.18f}f}}")

    print("};")

    print(f"\n// Successfully generated {len(cmap_data)} color entries", file=sys.stderr)

if __name__ == '__main__':
    main()
