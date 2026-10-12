# LLM Usage Log (DRAFT - check every row and rewrite in your own words)

Tool used: Claude. The "Prompt used" column holds your real prompts from the chat.
The last column must describe what YOU actually did to check the answer - fill it in
after you have run and tested things yourself.

| Sr. | Purpose | Prompt used | How the response was used | Modification / Verification (fill in) |
|---|---|---|---|---|
| 1 | Debugging / code review | (pasted image_enhancement.cpp, .h, test_enhancement.cpp) "we are trying to implement 2 image enhancement operations deblurring and image sharpening" | Found that noisePower 0.05 was too high, DFT edges cause ringing, defaults were inconsistent | e.g. rebuilt, compared outputs before/after |
| 2 | Algorithm understanding | "did you find which algorithm it is using" | Learned the code uses Wiener deconvolution + high-boost (unsharp) sharpening | e.g. checked against textbook / lecture notes |
| 3 | Test cases | "plz give image on which i can test this like it will work 100% on that image" | Got a synthetic image blurred with a known disk PSF, plus the sharp original for PSNR | e.g. ran program, PSNR improved |
| 4 | Generalisation | "bro i dont want this give me generalized code which will work for every image i dont want to chanfe the code everytime" | Added automatic blur-radius estimation (spectral dip) and a fixed K = 0.02 | e.g. tested on tiger.jpg and the dataset |
| 5 | Test data | "could u plz make dataset of 100 images just like the image u gave for autronoaut image" | Used the generated 100-image dataset (512px, disk blur radius 3-7) | e.g. checked estimated radius vs filename |
| 6 | Parallelization | (project instructions pasted) "...use the dataset you have generated" | OpenMP parallel-for across images, benchmark and plotting scripts | e.g. compared checksums, ran benchmarks |

(Add your own later prompts: optimisation, performance analysis, report writing.)

## Critical evaluation of LLM output (required section) - draft examples

**Example A - wrong parameter suggestion**
- Suggested: for Wiener deconvolution use K between 0.001 and 0.005.
- Why considered: typical values for images scaled to [0,1].
- Result: on the test image PSNR with K = 0.001 was lower (21.5 dB) than the blurred input (27.1 dB); best was K = 0.015 - 0.02 (about 29.9 dB).
- Decision: suggestion rejected, K changed to 0.02, found by measuring PSNR over a range of K values.
- Verification: PSNR comparison against the known sharp image.

**Example B - unsuitable test data**
- Suggested/generated: a dataset that included flat, low-detail images (clock, colour wheel, cell image).
- Problem: the automatic radius estimate was wrong on 17 of 100 images, all from those flat images.
- Decision: removed those images; the estimate then matched the true radius on 100 of 100.
- Verification: compared estimated radius with the radius in the filename.

(Rewrite both in your own words, and only keep what you really verified.)
