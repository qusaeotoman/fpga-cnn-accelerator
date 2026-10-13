# FPGA CNN Accelerator — Vivado HLS

This repository contains the current HLS implementation of a compact integer CNN accelerator for FPGA inference. The design targets Vivado HLS and uses a five-row circular line buffer, row partial sums, and partial MAC unrolling in the convolution stage.

## Network architecture

The complete inference path is:

```text
28×28×1 signed INT8 input
        │
        ▼
5×5 valid convolution, 4 filters
        │
        ▼
24×24×4 feature map
        │
        ▼
Clipping to [-128, 127] + ReLU
        │
        ▼
2×2 max pooling
        │
        ▼
12×12×4 feature map
        │
        ▼
Flatten
        │
        ▼
576 values
        │
        ▼
Fully connected layer, 576 → 10
        │
        ▼
10 signed INT32 class scores
```

The testbench selects the class with the largest output score.

### Data representation

- Input activations: signed 8-bit integers (`int8_t`).
- Weights: signed 8-bit integers (`int8_t`).
- Accumulators and output scores: signed 32-bit integers (`int32_t`).
- Input image size: 28×28, single channel.
- Convolution kernel: 5×5.
- Number of convolution filters: 4.
- Convolution output: 24×24×4.
- Max-pooling window: 2×2.
- Flattened feature vector: 576 elements.
- Output classes: 10.

The HLS top-level function is `cnn_accel`. The input and output arrays use `ap_memory` interfaces, while the function control interface uses `ap_ctrl_hs`.

## HLS optimization

The convolution stage is organized around `conv5x5_linebuf_mac` and a local five-row circular line buffer.

The main optimization techniques are:

- **Line buffering:** input rows are cached locally in `linebuf[KERNEL_SIZE][IMG_COLS]`, reducing repeated accesses to the external input array during convolution.
- **Row partial sums:** each 5×5 convolution is decomposed into five row-level partial sums stored in `row_psum`, which are then accumulated with the filter bias.
- **Array partitioning:** the row partial-sum array and line-buffer row dimension are completely partitioned to expose parallel accesses.
- **Loop unrolling:** kernel rows are fully unrolled, while kernel columns use `UNROLL factor=2` to introduce controlled MAC parallelism.
- **Pipelining:** input-row loading and the dense-layer accumulation loop request `PIPELINE II=1`.
- **Inlining:** `conv5x5_linebuf_mac` is inlined into the convolution datapath.

These pragmas describe the intended HLS parallelism. The achieved latency and resource usage depend on synthesis and the selected FPGA target.

## Repository contents

```text
src/
  cnn.cpp                  optimized HLS implementation
  cnn.h                    network dimensions, data types, top-level declaration
  cnn_weights_export.h     quantized model weights and biases

tb/
  cnn_tb.cpp               demonstration testbench for digits 1, 7, 0, and 4

scripts/
  build_hls.tcl            Vivado HLS project reconstruction script

docs/
  reports/
    cnn_accel_csynth.rpt   archived HLS synthesis report
    historical_csim.txt    archived C simulation output
```

The weights header contains the quantized parameters used by the current CNN implementation.

## Run the C++ demonstration

From the repository root with a C++ compiler:

```powershell
New-Item -ItemType Directory -Path build -Force | Out-Null
g++ -std=c++11 -O2 -Wall -Wextra -Wno-unknown-pragmas src/cnn.cpp tb/cnn_tb.cpp -o build/cnn_tb.exe
./build/cnn_tb.exe
```

The demonstration testbench runs four hand-drawn digit inputs and prints all ten class scores together with the predicted class.

## Rebuild the HLS project

The project configuration uses Vivado HLS **2019.1**, target device **xc7a100t-csg324-1**, and a **10 ns** target clock.

From a Vivado HLS command environment:

```powershell
vivado_hls -f scripts/build_hls.tcl
```

The script creates the HLS project, adds the source files and testbench, runs C simulation, and performs HLS synthesis in `build/cnn_accel/solution1`.

To additionally request RTL co-simulation and IP export:

```powershell
$env:CNN_HLS_COSIM = '1'
$env:CNN_HLS_EXPORT = '1'
vivado_hls -f scripts/build_hls.tcl
```

Generated build outputs are excluded by `.gitignore`.

## Validation evidence

The archived C simulation contains successful runs for digits 1, 7, 0, and 4. The archived HLS synthesis report records the synthesis estimates for the current line-buffer implementation.

These files are retained under `docs/reports/` as reproducibility evidence for the current HLS implementation.

## Scope

This repository focuses on the current HLS CNN accelerator implementation. Board-level Vivado integration, pin constraints, and bitstream files are not included in the repository.

No open-source license is currently provided. Reuse rights should therefore be clarified before redistributing the source or model weights.
