#include "../src/cnn.h"
#include <stdio.h>

#define PIXEL_VALUE 1

static void clear_image(data_t img[IMG_ROWS][IMG_COLS]) {
    for (int i = 0; i < IMG_ROWS; i++) {
        for (int j = 0; j < IMG_COLS; j++) {
            img[i][j] = 0;
        }
    }
}

static void draw_digit_one(data_t img[IMG_ROWS][IMG_COLS]) {
    for (int i = 6; i < 22; i++) {
        img[i][14] = PIXEL_VALUE;
        img[i][15] = PIXEL_VALUE;
    }

    for (int j = 11; j < 19; j++) {
        img[21][j] = PIXEL_VALUE;
        img[22][j] = PIXEL_VALUE;
    }

    img[6][13] = PIXEL_VALUE;
    img[7][12] = PIXEL_VALUE;
    img[8][11] = PIXEL_VALUE;
}

static void draw_digit_seven(data_t img[IMG_ROWS][IMG_COLS]) {
    for (int j = 8; j < 20; j++) {
        img[6][j] = PIXEL_VALUE;
        img[7][j] = PIXEL_VALUE;
    }

    for (int k = 0; k < 14; k++) {
        int r = 8 + k;
        int c = 19 - k / 2;

        if (r >= 0 && r < IMG_ROWS && c >= 0 && c < IMG_COLS) {
            img[r][c] = PIXEL_VALUE;

            if (c - 1 >= 0) {
                img[r][c - 1] = PIXEL_VALUE;
            }
        }
    }
}

static void draw_digit_zero(data_t img[IMG_ROWS][IMG_COLS]) {
    for (int j = 10; j < 18; j++) {
        img[6][j] = PIXEL_VALUE;
        img[7][j] = PIXEL_VALUE;
        img[20][j] = PIXEL_VALUE;
        img[21][j] = PIXEL_VALUE;
    }

    for (int i = 8; i < 20; i++) {
        img[i][10] = PIXEL_VALUE;
        img[i][11] = PIXEL_VALUE;
        img[i][16] = PIXEL_VALUE;
        img[i][17] = PIXEL_VALUE;
    }
}

static void draw_digit_four(data_t img[IMG_ROWS][IMG_COLS]) {
    for (int i = 6; i < 15; i++) {
        img[i][10] = PIXEL_VALUE;
        img[i][11] = PIXEL_VALUE;
    }

    for (int i = 6; i < 22; i++) {
        img[i][17] = PIXEL_VALUE;
        img[i][18] = PIXEL_VALUE;
    }

    for (int j = 10; j < 19; j++) {
        img[14][j] = PIXEL_VALUE;
        img[15][j] = PIXEL_VALUE;
    }

    img[7][12] = PIXEL_VALUE;
    img[8][13] = PIXEL_VALUE;
    img[9][14] = PIXEL_VALUE;
    img[10][15] = PIXEL_VALUE;
}

static void print_image(data_t img[IMG_ROWS][IMG_COLS]) {
    printf("Input image (28x28):\n");

    for (int i = 0; i < IMG_ROWS; i++) {
        for (int j = 0; j < IMG_COLS; j++) {
            if (img[i][j] > 0) {
                printf("##");
            } else {
                printf("..");
            }
        }
        printf("\n");
    }

    printf("\n");
}

static void print_predictions(acc_t pred[NUM_CLASSES]) {
    int best_class = 0;
    acc_t best_score = pred[0];

    printf("Predictions:\n");

    for (int c = 0; c < NUM_CLASSES; c++) {
        printf("class %d: %d\n", c, (int)pred[c]);

        if (pred[c] > best_score) {
            best_score = pred[c];
            best_class = c;
        }
    }

    printf("Predicted class: %d\n", best_class);
    printf("Best score: %d\n", (int)best_score);
    printf("\n");
}

static void run_test(
    const char *name,
    void (*draw_fn)(data_t img[IMG_ROWS][IMG_COLS])
) {
    data_t img[IMG_ROWS][IMG_COLS];
    acc_t pred[NUM_CLASSES];

    clear_image(img);
    draw_fn(img);

    printf("========================================\n");
    printf("Test image: %s\n", name);
    printf("========================================\n");

    print_image(img);

    cnn_accel(img, pred);

    print_predictions(pred);
}

int main() {
    run_test("digit 1", draw_digit_one);
    run_test("digit 7", draw_digit_seven);
    run_test("digit 0", draw_digit_zero);
    run_test("digit 4", draw_digit_four);

    return 0;
}
