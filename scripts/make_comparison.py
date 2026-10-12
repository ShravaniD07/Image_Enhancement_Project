"""
Makes before/after pictures to show the teacher.

Each row = [ blurred input | enhanced result | zoomed blurred | zoomed enhanced ]
(the zoom is a crop from the middle of the picture, so you can see the detail)

Run from the project root:

  python scripts/make_comparison.py
        -> uses data/small, data/medium, data/large  with  output/small ...
  python scripts/make_comparison.py <input_folder> <output_folder> <result.png>
        -> for any other dataset, e.g.
  python scripts/make_comparison.py data/real output/real results/comparison_real.png
"""
import os
import sys

import cv2
import numpy as np

PANEL_H = 360          # height of every panel in the final picture
MAX_ROWS = 4           # images per comparison sheet


def label(img, text):
    out = img.copy()
    cv2.rectangle(out, (0, 0), (out.shape[1], 28), (0, 0, 0), -1)
    cv2.putText(out, text, (8, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.55,
                (255, 255, 255), 1, cv2.LINE_AA)
    return out


def fit_height(img, h=PANEL_H):
    scale = h / img.shape[0]
    interp = cv2.INTER_AREA if scale < 1 else cv2.INTER_CUBIC
    return cv2.resize(img, (max(1, int(img.shape[1] * scale)), h), interpolation=interp)


def center_crop(img):
    h, w = img.shape[:2]
    c = min(h, w) // 4
    y0, x0 = (h - c) // 2, (w - c) // 2
    return img[y0:y0 + c, x0:x0 + c]


def find_original(in_dir, enhanced_name):
    stem = enhanced_name.replace("_enhanced", "")
    stem = os.path.splitext(stem)[0]
    for ext in (".png", ".jpg", ".jpeg", ".bmp"):
        p = os.path.join(in_dir, stem + ext)
        if os.path.exists(p):
            return p
    return None


def make_sheet(in_dir, out_dir, result_png):
    if not os.path.isdir(out_dir):
        print(f"Skipping: {out_dir} not found (run omp with --save first)")
        return
    names = sorted(f for f in os.listdir(out_dir) if f.lower().endswith(".png"))[:MAX_ROWS]
    rows = []
    for n in names:
        orig_path = find_original(in_dir, n)
        if orig_path is None:
            continue
        a = cv2.imread(orig_path)
        b = cv2.imread(os.path.join(out_dir, n))
        if a is None or b is None:
            continue
        panels = [
            label(fit_height(a), "Blurred input"),
            label(fit_height(b), "Enhanced"),
            label(fit_height(center_crop(a)), "Blurred (zoom)"),
            label(fit_height(center_crop(b)), "Enhanced (zoom)"),
        ]
        rows.append(np.hstack(panels))
    if not rows:
        print(f"No matching pairs for {in_dir} / {out_dir}")
        return
    width = max(r.shape[1] for r in rows)
    rows = [cv2.copyMakeBorder(r, 0, 6, 0, width - r.shape[1], cv2.BORDER_CONSTANT,
                               value=(255, 255, 255)) for r in rows]
    os.makedirs(os.path.dirname(result_png) or ".", exist_ok=True)
    cv2.imwrite(result_png, np.vstack(rows))
    print(f"Saved {result_png}  ({len(rows)} images)")


if len(sys.argv) == 4:
    make_sheet(sys.argv[1], sys.argv[2], sys.argv[3])
else:
    for s in ("small", "medium", "large"):
        make_sheet(f"data/{s}", f"output/{s}", f"results/comparison_{s}.png")
