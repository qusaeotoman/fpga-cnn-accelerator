#ifndef CNN_H
#define CNN_H

#include <stdint.h>

#define IMG_ROWS 28
#define IMG_COLS 28

#define KERNEL_SIZE 5
#define CONV_OUT_DIM 24

#define POOL_DIM 12
#define FILTERS 4

#define FLAT_SIZE (FILTERS * POOL_DIM * POOL_DIM)
#define NUM_CLASSES 10

typedef int8_t data_t;
typedef int8_t weight_t;
typedef int32_t acc_t;

void cnn_accel(
    data_t input[IMG_ROWS][IMG_COLS],
    acc_t output[NUM_CLASSES]
);

#endif
