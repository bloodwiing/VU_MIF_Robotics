from PIL import Image
import sys
from pathlib import Path

def pack_xy(x, y):
    return (y << 7) | x

def parse_ignore(args):
    if "--ignore" not in args:
        return set()
    i = args.index("--ignore")
    if i + 1 >= len(args):
        return set()
    raw = args[i + 1].strip()
    if not raw:
        return set()
    out = set()
    for part in raw.split(","):
        part = part.strip()
        if part == "":
            continue
        v = int(part)
        if 0 <= v <= 255:
            out.add(v)
    return out

def main():
    if len(sys.argv) < 3:
        print("Usage:")
        print("  python bmp_to_index_offsets_blob_nopalette.py input.bmp output.h")
        print("  python bmp_to_index_offsets_blob_nopalette.py input.bmp output.h --ignore 0")
        sys.exit(1)

    in_path = Path(sys.argv[1])
    out_path = Path(sys.argv[2])
    ignore_set = parse_ignore(sys.argv[3:])

    img = Image.open(in_path)

    # Keep authored indices if already paletted.
    if img.mode != "P":
        img = img.convert(
            "P",
            palette=Image.Palette.ADAPTIVE,
            colors=256,
            dither=Image.Dither.NONE
        )

    w, h = img.size
    if w > 128 or h > 160:
        print(f"WARNING: Image is {w}x{h}. Packing assumes x<128, y<160 for 1.8\" TFT.")

    px = img.load()

    # Collect coords per index excluding ignored indices
    coords_by_idx = [[] for _ in range(256)]
    for y in range(h):
        for x in range(w):
            idx = px[x, y] & 0xFF
            if idx in ignore_set:
                continue
            coords_by_idx[idx].append(pack_xy(x, y))

    # Build offsets table (257)
    offsets = [0]
    running = 0
    for i in range(256):
        running += len(coords_by_idx[i])
        offsets.append(running)

    # Flatten coords pool grouped by index
    coords_pool = []
    for i in range(256):
        coords_pool.extend(coords_by_idx[i])

    # Build blob: [w, h, offsets..., coords...]
    blob = bytearray()
    blob.append(w & 0xFF)
    blob.append(h & 0xFF)

    # Offsets little-endian uint16
    for off in offsets:
        blob.append(off & 0xFF)
        blob.append((off >> 8) & 0xFF)

    # Coords little-endian uint16
    for p in coords_pool:
        blob.append(p & 0xFF)
        blob.append((p >> 8) & 0xFF)

    name = out_path.stem

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("#pragma once\n")
        f.write("#include <Arduino.h>\n\n")
        f.write("// Indexed FAST blob format (NO palette, single array):\n")
        f.write("// Byte 0: width\n")
        f.write("// Byte 1: height\n")
        f.write("// Next 514 bytes: offsets table, 257 x uint16 little-endian\n")
        f.write("// Remaining: coords pool, uint16 packed coords grouped by index\n")
        f.write("//\n")
        f.write("// Packed coord for 128x160:\n")
        f.write("// packed = (y << 7) | x\n")
        f.write("// x = packed & 0x7F\n")
        f.write("// y = packed >> 7\n")
        if ignore_set:
            f.write(f"// Ignored indices: {sorted(ignore_set)}\n")
        f.write("\n")

        f.write(f"const uint8_t {name}_data[] PROGMEM = {{\n  ")

        for i, b in enumerate(blob):
            f.write(f"0x{b:02X}, ")
            if (i + 1) % 16 == 0:
                f.write("\n  ")

        f.write("\n};\n")

    total_coords = len(coords_pool)
    print(f"Wrote: {out_path}")
    print(f"Image: {w}x{h} ({w*h} pixels)")
    print(f"Ignoring: {sorted(ignore_set) if ignore_set else 'none'}")
    print(f"Total coords: {total_coords}")
    print(f"Blob bytes: {len(blob)}")
    print("Size estimate:")
    print(f"  header: 2")
    print(f"  offsets: 514")
    print(f"  coords: {total_coords * 2}")
    print(f"  total: {2 + 514 + total_coords * 2}")

if __name__ == "__main__":
    main()
