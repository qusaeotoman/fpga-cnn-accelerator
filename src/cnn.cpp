#include "cnn.h"
#include "cnn_weights_export.h"

static data_t relu_clip(acc_t x) {
    if (x > 127) {
        x = 127;
    } else if (x < -128) {
        x = -128;
    }

    if (x > 0) {
        return (data_t)x;
    } else {
        return 0;
    }
}

/*
 * Proposed convolution MAC core.
 *
 * Main ideas included in this version:
 * 1) The convolution core is separated from conv_layer.
 * 2) The input pixels are read from a local line buffer, not directly
 *    from the external img array for every kernel access.
 * 3) The 5x5 MAC is organized through row partial sums. This is a
 *    simple systolic-like / PE-like structure at the HLS level:
 *    each kernel row produces a partial sum, then the partial sums are
 *    accumulated together with the bias.
 * 4) Moderate unrolling is applied inside the kernel row. This is a
 *    balanced optimization: it exposes parallelism without trying to
 *    pipeline the whole convolution layer aggressively.
 */
static acc_t conv5x5_linebuf_mac(
    data_t linebuf[KERNEL_SIZE][IMG_COLS],
    int top_row,
    int col,
    int f
) {
#pragma HLS INLINE

    acc_t row_psum[KERNEL_SIZE];
#pragma HLS ARRAY_PARTITION variable=row_psum complete dim=1

    for (int ki = 0; ki < KERNEL_SIZE; ki++) {
#pragma HLS UNROLL
        acc_t row_sum = 0;
        int buf_row = (top_row + ki) % KERNEL_SIZE;

        for (int kj = 0; kj < KERNEL_SIZE; kj++) {
#pragma HLS UNROLL factor=2
            row_sum += linebuf[buf_row][col + kj] * conv1_weights[f][ki][kj];
        }

        row_psum[ki] = row_sum;
    }

    acc_t sum = conv1_bias[f];

    for (int ki = 0; ki < KERNEL_SIZE; ki++) {
#pragma HLS UNROLL
        sum += row_psum[ki];
    }

    return sum;
}

/*
 * Proposed convolution layer.
 *
 * Compared with the original direct implementation, this version uses
 * a real local line buffer. Each input row is loaded once into the
 * line buffer. After the first KERNEL_SIZE rows have been loaded, the
 * convolution outputs are computed using only the local line buffer.
 *
 * The external interface is unchanged:
 *     img      -> 28x28 int8 input image
 *     conv_out -> 24x24x4 int8 output feature map
 *
 * Therefore, the UDM integration and the top-level cnn_accel interface
 * can remain unchanged after HLS synthesis.
 */
static void conv_layer(
    data_t img[IMG_ROWS][IMG_COLS],
    data_t conv_out[CONV_OUT_DIM][CONV_OUT_DIM][FILTERS]
) {
    data_t linebuf[KERNEL_SIZE][IMG_COLS];
#pragma HLS ARRAY_PARTITION variable=linebuf complete dim=1

    // Stream image rows into the local line buffer.
    for (int r = 0; r < IMG_ROWS; r++) {
        int write_row = r % KERNEL_SIZE;

        for (int c = 0; c < IMG_COLS; c++) {
#pragma HLS PIPELINE II=1
            linebuf[write_row][c] = img[r][c];
        }

        // Once five rows are available, produce one output row.
        if (r >= KERNEL_SIZE - 1) {
            int out_i = r - (KERNEL_SIZE - 1);

            if (out_i < CONV_OUT_DIM) {
                for (int j = 0; j < CONV_OUT_DIM; j++) {
                    for (int f = 0; f < FILTERS; f++) {
                        acc_t sum = conv5x5_linebuf_mac(linebuf, out_i, j, f);
                        conv_out[out_i][j][f] = relu_clip(sum);
                    }
                }
            }
        }
    }
}

static void maxpool_layer(
    data_t conv_out[CONV_OUT_DIM][CONV_OUT_DIM][FILTERS],
    data_t pool_out[POOL_DIM][POOL_DIM][FILTERS]
) {
    for (int f = 0; f < FILTERS; f++) {
        for (int i = 0; i < POOL_DIM; i++) {
            for (int j = 0; j < POOL_DIM; j++) {
                int r = i * 2;
                int c = j * 2;

                data_t max_val = conv_out[r][c][f];

                if (conv_out[r][c + 1][f] > max_val) {
                    max_val = conv_out[r][c + 1][f];
                }

                if (conv_out[r + 1][c][f] > max_val) {
                    max_val = conv_out[r + 1][c][f];
                }

                if (conv_out[r + 1][c + 1][f] > max_val) {
                    max_val = conv_out[r + 1][c + 1][f];
                }

                pool_out[i][j][f] = max_val;
            }
        }
    }
}

static void flatten_layer(
    data_t pool_out[POOL_DIM][POOL_DIM][FILTERS],
    data_t flat[FLAT_SIZE]
) {
    int idx = 0;

    for (int i = 0; i < POOL_DIM; i++) {
        for (int j = 0; j < POOL_DIM; j++) {
            for (int f = 0; f < FILTERS; f++) {
                flat[idx] = pool_out[i][j][f];
                idx++;
            }
        }
    }
}

static void fc_layer(
    data_t flat[FLAT_SIZE],
    acc_t out[NUM_CLASSES]
) {
    for (int o = 0; o < NUM_CLASSES; o++) {
        acc_t sum = fc_bias[o];

        for (int i = 0; i < FLAT_SIZE; i++) {
#pragma HLS PIPELINE II=1
            sum += flat[i] * fc_weights[o][i];
        }

        out[o] = sum;
    }
}

void cnn_accel(
    data_t img[IMG_ROWS][IMG_COLS],
    acc_t out[NUM_CLASSES]
) {
#pragma HLS INTERFACE ap_memory port=img
#pragma HLS INTERFACE ap_memory port=out
#pragma HLS INTERFACE ap_ctrl_hs port=return

    data_t conv_out[CONV_OUT_DIM][CONV_OUT_DIM][FILTERS];
    data_t pool_out[POOL_DIM][POOL_DIM][FILTERS];
    data_t flat[FLAT_SIZE];

    conv_layer(img, conv_out);
    maxpool_layer(conv_out, pool_out);
    flatten_layer(pool_out, flat);
    fc_layer(flat, out);
}
