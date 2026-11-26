#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//01_bayer_raw8.c

const uint16_t WIDTH  = 640;
const uint16_t HEIGHT = 480;
const uint16_t STRIDE = 640;

//const uint16_t WIDTH  = 8;
//const uint16_t HEIGHT = 8;
//const uint16_t STRIDE = 8;


#define BLACK_LEVEL 16

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBPixel;

static uint8_t clamp_u8(int value)
{
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return (uint8_t)value;
}

static uint8_t get_raw_pixel(
                    uint8_t* raw,
                    int x,    int y,
                    int width,int height, int stride)
{
    if (x < 0)
        x = 0;

    if (y < 0)
        y = 0;

    if (x >= width)
        x = width - 1;

    if (y >= height)
        y = height - 1;

    return raw[y * stride + x];
}

static void apply_black_level(
    uint8_t* raw,
    int width, int height, int stride,
    int blackLevel)
{
    int x;
    int y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int value = raw[y * stride + x];

            value -= blackLevel;

            if (value < 0)
                value = 0;

            raw[y * stride + x] = (uint8_t)value;
        }
    }
}

static RGBPixel demosaic_pixel(
    uint8_t* raw,
    int x,    int y,
    int width, int height, int stride)
{
    RGBPixel pixel;

    int r = 0;
    int g = 0;
    int b = 0;

    int current =
        get_raw_pixel(
            raw,
            x,
            y,
            width,
            height,
            stride);

    int evenRow = ((y & 1) == 0);
    int evenCol = ((x & 1) == 0);

    /*
        RGGB

        R G R G
        G B G B
        R G R G
        G B G B
    */

    if (evenRow && evenCol)
    {
        /*            RED        */

        r = current;
        g =
            (
                get_raw_pixel(raw, x - 1, y, width, height, stride) +
                get_raw_pixel(raw, x + 1, y, width, height, stride) +
                get_raw_pixel(raw, x, y - 1, width, height, stride) +
                get_raw_pixel(raw, x, y + 1, width, height, stride)
                ) / 4;
        b =
            (
                get_raw_pixel(raw, x - 1, y - 1, width, height, stride) +
                get_raw_pixel(raw, x + 1, y - 1, width, height, stride) +
                get_raw_pixel(raw, x - 1, y + 1, width, height, stride) +
                get_raw_pixel(raw, x + 1, y + 1, width, height, stride)
                ) / 4;
    }
    else if (!evenRow && !evenCol)
    {
        /*            BLUE        */

        b = current;
        g =
            (
                get_raw_pixel(raw, x - 1, y, width, height, stride) +
                get_raw_pixel(raw, x + 1, y, width, height, stride) +
                get_raw_pixel(raw, x, y - 1, width, height, stride) +
                get_raw_pixel(raw, x, y + 1, width, height, stride)
                ) / 4;

        r =
            (
                get_raw_pixel(raw, x - 1, y - 1, width, height, stride) +
                get_raw_pixel(raw, x + 1, y - 1, width, height, stride) +
                get_raw_pixel(raw, x - 1, y + 1, width, height, stride) +
                get_raw_pixel(raw, x + 1, y + 1, width, height, stride)
                ) / 4;
    }
    else if (evenRow && !evenCol)
    {
        /*            GREEN (ON RED ROW)        */

        g = current;
        r =
            (
                get_raw_pixel(raw, x - 1, y, width, height, stride) +
                get_raw_pixel(raw, x + 1, y, width, height, stride)
                ) / 2;

        b =
            (
                get_raw_pixel(raw, x, y - 1, width, height, stride) +
                get_raw_pixel(raw, x, y + 1, width, height, stride)
                ) / 2;
    }
    else
    {
        /*            GREEN (ON BLUE ROW)        */

        g = current;
        r =
            (
                get_raw_pixel(raw, x, y - 1, width, height, stride) +
                get_raw_pixel(raw, x, y + 1, width, height, stride)
                ) / 2;

        b =
            (
                get_raw_pixel(raw, x - 1, y, width, height, stride) +
                get_raw_pixel(raw, x + 1, y, width, height, stride)
                ) / 2;
    }

    pixel.r = clamp_u8(r);
    pixel.g = clamp_u8(g);
    pixel.b = clamp_u8(b);

    return pixel;
}

static int write_ppm(
    const char* filename,
    RGBPixel* rgb,
    int width,
    int height)
{
    FILE* fp = fopen(filename, "wb");

    if (!fp)
        return -1;

    fprintf(
        fp,
        "P6\n%d %d\n255\n",
        width,
        height);

    fwrite(
        rgb,
        sizeof(RGBPixel),
        width * height,
        fp);

    fclose(fp);

    return 0;
}

int main(void)
{
    FILE* fp;

    uint8_t* raw;

    RGBPixel* rgb;

    int x;
    int y;

    size_t rawSize = STRIDE * HEIGHT;

    raw = (uint8_t*)malloc(rawSize);

    if (!raw)
    {
        printf("malloc failed\n");
        return -1;
    }

    fp = fopen("640x480_RAW8_RGGB.raw", "rb");

    if (!fp)
    {
        printf("cannot open input.raw\n");
        free(raw);
        return -1;
    }

    fread(raw, 1, rawSize, fp);
    fclose(fp);

    apply_black_level(raw, WIDTH, HEIGHT,STRIDE,BLACK_LEVEL);

    rgb = (RGBPixel*)malloc(WIDTH * HEIGHT * sizeof(RGBPixel));

    if (!rgb)
    {
        free(raw);
        return -1;
    }

    for (y = 0; y < HEIGHT; y++)
    {
        for (x = 0; x < WIDTH; x++)
        {
            rgb[y * WIDTH + x] = demosaic_pixel(raw, x, y, WIDTH, HEIGHT, STRIDE);
        }
    }

    write_ppm("output.ppm", rgb, WIDTH, HEIGHT);

    free(raw);
    free(rgb);

    printf("Done\n");

    return 0;
}
