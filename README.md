# Optimized CNN accelerator — Vivado HLS

This snapshot contains a CNN HLS component with a five-row circular line buffer, row partial sums, and partial MAC unrolling. It was prepared from the local `cnn_accelerat_Master` folder. The source, model weights, and demonstration testbench are unchanged copies.

## Architecture

- Input: 28×28 single-channel signed 8-bit image.
- Valid convolution: four 5×5 filters → 24×24×4.
- Clip accumulators to [-128, 127], then apply ReLU.
- 2×2 max pooling → 12×12×4; flatten in row, column, filter order → 576 values.
- Dense layer → ten signed 32-bit scores. The testbench selects the largest score.
- Signed 8-bit weights, signed 32-bit accumulators.
- Top function: `cnn_accel`; input/output `ap_memory` interfaces and `ap_ctrl_hs` control.

`conv5x5_linebuf_mac` is inlined. Kernel rows and their sum are fully unrolled, kernel columns use unroll factor 2, and row partial sums and line-buffer rows are partitioned. Loading input rows and the dense-layer accumulation loop request `PIPELINE II=1`. These are source directives; their achieved hardware behavior requires synthesis evidence.

## Repository contents

```text
src/
  cnn.cpp                  optimized implementation
  cnn.h                    dimensions, types, and top-level declaration
  cnn_weights_export.h     quantized model weights and biases
tb/
  cnn_tb.cpp               hand-drawn digit demonstrations: 1, 7, 0, 4
scripts/
  build_hls.tcl            portable HLS project reconstruction
docs/
  inspection-ar.md         Arabic inspection findings and limitations
  reports/
    cnn_accel_csynth.rpt   historical synthesis report
    historical_csim.txt    historical C simulation output
```

The weights header was generated during Python training but is essential model data. The inspected folder did not contain the training code, dataset, original model, or quantization procedure.

## Run the C++ demonstration

From the repository root with a C++ compiler:

```powershell
New-Item -ItemType Directory -Path build -Force | Out-Null
g++ -std=c++11 -O2 -Wall -Wextra -Wno-unknown-pragmas src/cnn.cpp tb/cnn_tb.cpp -o build/cnn_tb.exe
./build/cnn_tb.exe
```

The testbench prints scores and always returns zero; it contains no correctness assertions. Four hand-drawn examples do not establish dataset accuracy.

## Rebuild the HLS project

The inspected project uses Vivado HLS **2019.1**, device **xc7a100t-csg324-1**, and a **10 ns** target clock. From a Vivado HLS command environment, run:

```powershell
vivado_hls -f scripts/build_hls.tcl
```

The script resolves source paths relative to itself, explicitly adds the weights header, and runs C simulation and synthesis into `build/cnn_accel/solution1`. The original `directives.tcl` had no active directives; optimization pragmas are already in the source.

To additionally request RTL co-simulation and IP export:

```powershell
$env:CNN_HLS_COSIM = '1'
$env:CNN_HLS_EXPORT = '1'
vivado_hls -f scripts/build_hls.tcl
```

Build outputs are excluded by `.gitignore`. The new HLS script has not been run in Vivado HLS during packaging. No fresh synthesis, co-simulation, or IP export is claimed.

## Validation and historical evidence

On October 13, 2026, native compilation and execution with MinGW `g++` succeeded. All 40 class scores and four predictions/best scores matched the archived C simulation: predicted classes 1, 7, 0, and 4 with best scores 1784, 1925, 506, and 1319.

A separate native comparison against the direct-convolution source from the previously inspected `cnn_accelerator` folder matched all 1,040 class scores across 104 images: zero, maximum positive, minimum negative, alternating extremes, and 100 deterministic random signed 8-bit images. This is finite functional comparison, not an RTL equivalence proof. The headers, weights, and original testbench are identical between the two folders; only `cnn.cpp` differs. SHA-256 checks confirmed all four packaged files match this snapshot's originals.

The historical synthesis report dated June 5, 2026 estimates a latency range of **12,893–199,933 cycles**, estimated clock **7.777 ns**, **6 BRAM_18K**, **11 DSP48E**, **1,765 FF**, and **3,320 LUT**. Its preprocessed source contains the line buffer and row partial sums. These are archived HLS estimates, not fresh synthesis results or board measurements. The latency range must not be presented as a fixed measured execution time.

## Board integration and publication

The original `design/` directory was empty. No board integration sources, `.xdc` constraints, `.bd` block design, `.xpr` Vivado project, or bitstream were found. This package contains the optimized HLS component; a complete board project must be added from its actual location.

To publish, extract this package and use its inner folder as the repository root. In GitHub Desktop, select **File → Add local repository**, create a repository there if prompted, review the ten prepared files, commit, and publish to your account. No GitHub upload was performed during preparation.

No license was present in the inspected folder. Select a license consistent with ownership of the code and trained weights if you want to grant reuse rights.
