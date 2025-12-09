from PIL import Image
import sys
from pathlib import Path

def rgb888_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def parse_indices(s):
    return [int(x.strip()) for x in s.split(",") if x.strip() != ""]

def main():
    if len(sys.argv) < 4:
        print("Usage: python bmp_to_index_layers.py input.bmp output.h 3")
        print("   or: python bmp_to_index_layers.py input.bmp output.h 1,2,5")
        sys.exit(1)

    in_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    idx_list = parse_indices(sys.argv[3])

    img = Image.open(in_path)

    # Ensure paletted mode
    if img.mode != "P":
        img = img.convert("P", palette=Image.Palette.ADAPTIVE, colors=256)

    w, h = img.size
    pal = img.getpalette()
    px = img.load()

    # Collect coords per target index
    coords_by_idx = {idx: [] for idx in idx_list}

    for y in range(h):
        for x in range(w):
            idx = px[x, y]
            if idx in coords_by_idx:
                packed = (y << 7) | x  # x: 0-127, y: 0-159 expected for your display
                coords_by_idx[idx].append(packed)

    base_name = out_path.stem

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <Arduino.h>\n\n")
        f.write("// Packed coordinate format for 128x160 displays:\n")
        f.write("// packed = (y << 7) | x\n")
        f.write("// x = packed & 0x7F\n")
        f.write("// y = packed >> 7\n\n")

        f.write(f"const uint8_t {base_name}_w = {w};\n")
        f.write(f"const uint8_t {base_name}_h = {h};\n\n")

        for idx in idx_list:
            base = idx * 3
            r, g, b = pal[base], pal[base + 1], pal[base + 2]
            c565 = rgb888_to_565(r, g, b)

            coords = coords_by_idx[idx]
            arr_name = f"{base_name}_idx{idx}"

            f.write(f"const uint16_t {arr_name}_color = 0x{c565:04X};\n")
            f.write(f"const uint16_t {arr_name}_count = {len(coords)};\n")
            f.write(f"const uint16_t {arr_name}_coords[] PROGMEM = {{\n")

            # write in rows for readability
            for i, p in enumerate(coords):
                f.write(f"  0x{p:04X},")
                if (i + 1) % 12 == 0:
                    f.write("\n")
            if len(coords) % 12 != 0:
                f.write("\n")

            f.write("};\n\n")

    print(f"Wrote {out_path}")
    for idx in idx_list:
        print(f" index {idx}: {len(coords_by_idx[idx])} pixels")

if __name__ == "__main__":
    main()
