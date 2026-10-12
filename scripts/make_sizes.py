"""
Creates the medium (2x) and large (4x) versions of the dataset from
data/small (512x512).

Blur radius scales together with the image (R=3..7 -> 6..14 -> 12..28),
and the program estimates the radius automatically, so nothing else
needs to change.

Run from the project root:   python scripts/make_sizes.py
"""
import os
import cv2

SRC = "data/small"
TARGETS = {"medium": 2, "large": 4}

files = sorted(f for f in os.listdir(SRC) if f.lower().endswith((".png", ".jpg", ".jpeg", ".bmp")))
if not files:
    raise SystemExit(f"No images found in {SRC}")

for name, factor in TARGETS.items():
    out_dir = os.path.join("data", name)
    os.makedirs(out_dir, exist_ok=True)
    for f in files:
        img = cv2.imread(os.path.join(SRC, f), cv2.IMREAD_COLOR)
        big = cv2.resize(img, None, fx=factor, fy=factor, interpolation=cv2.INTER_CUBIC)
        cv2.imwrite(os.path.join(out_dir, f), big)
    print(f"{name}: {len(files)} images, {big.shape[1]}x{big.shape[0]} -> {out_dir}")
