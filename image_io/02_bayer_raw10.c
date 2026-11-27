#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//01_bayer_raw8.c

const uint16_t width  = 640;
const uint16_t height = 480;
const uint16_t stride = 640;

//const uint16_t width  = 8;
//const uint16_t height = 8;
//const uint16_t stride = 8;


#define BLACK_LEVEL 64

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGBPixel;

static void unpack_raw10(uint8_t* packed, uint16_t* unpacked, int pixelCount)
{
    int inIndex = 0;
    int outIndex = 0;

    while ((outIndex + 3) < pixelCount)
    {
        uint8_t b0 = packed[inIndex + 0];
        uint8_t b1 = packed[inIndex + 1];
        uint8_t b2 = packed[inIndex + 2];
        uint8_t b3 = packed[inIndex + 3];
        uint8_t b4 = packed[inIndex + 4];

        unpacked[outIndex + 0] =
            ((uint16_t)b0 << 2) | ((b4 >> 0) & 0x03);

        unpacked[outIndex + 1] =
            ((uint16_t)b1 << 2) | ((b4 >> 2) & 0x03);

        unpacked[outIndex + 2] =
            ((uint16_t)b2 << 2) | ((b4 >> 4) & 0x03);

        unpacked[outIndex + 3] =
            ((uint16_t)b3 << 2) | ((b4 >> 6) & 0x03);

        inIndex += 5;
        outIndex += 4;
    }
}

static uint16_t clamp_u10(int value)
{
    if (value < 0)
        return 0;

    if (value > 1023)
        return 1023;

    return (uint16_t)value;
}

static uint16_t get_raw_pixel(
                    uint16_t* raw16,
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

    return raw16[y * stride + x];
}

static void apply_black_level(uint16_t* raw16,
                                int width, int height, int stride,
                                                    int blackLevel)
{
    int x;
    int y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            int value = raw16[y * stride + x];

            value -= blackLevel;

            if (value < 0)
                value = 0;

            raw16[y * stride + x] = (uint16_t)value;
        }
    }
}

static RGBPixel demosaic_pixel(uint16_t* raw16,
                                        int x,    int y,
                                        int width, int height, int stride)
{
    RGBPixel pixel;

    uint16_t r = 0;
    uint16_t g = 0;
    uint16_t b = 0;

    uint16_t current = get_raw_pixel(raw16, x, y, width, height, stride);
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
                get_raw_pixel(raw16, x - 1, y, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y, width, height, stride) +
                get_raw_pixel(raw16, x, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x, y + 1, width, height, stride)
                ) / 4;
        b =
            (
                get_raw_pixel(raw16, x - 1, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x - 1, y + 1, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y + 1, width, height, stride)
                ) / 4;
    }
    else if (!evenRow && !evenCol)
    {
        /*            BLUE        */

        b = current;
        g =
            (
                get_raw_pixel(raw16, x - 1, y, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y, width, height, stride) +
                get_raw_pixel(raw16, x, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x, y + 1, width, height, stride)
                ) / 4;

        r =
            (
                get_raw_pixel(raw16, x - 1, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x - 1, y + 1, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y + 1, width, height, stride)
                ) / 4;
    }
    else if (evenRow && !evenCol)
    {
        /*            GREEN (ON RED ROW)        */

        g = current;
        r =
            (
                get_raw_pixel(raw16, x - 1, y, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y, width, height, stride)
                ) / 2;

        b =
            (
                get_raw_pixel(raw16, x, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x, y + 1, width, height, stride)
                ) / 2;
    }
    else
    {
        /*            GREEN (ON BLUE ROW)        */

        g = current;
        r =
            (
                get_raw_pixel(raw16, x, y - 1, width, height, stride) +
                get_raw_pixel(raw16, x, y + 1, width, height, stride)
                ) / 2;

        b =
            (
                get_raw_pixel(raw16, x - 1, y, width, height, stride) +
                get_raw_pixel(raw16, x + 1, y, width, height, stride)
                ) / 2;
    }

    r = clamp_u10(r);
    g = clamp_u10(g);
    b = clamp_u10(b);

    //scale down the r/g/b values to fit the range 0-255
    pixel.r = (uint8_t)(r >> 2);
    pixel.g = (uint8_t)(g >> 2);
    pixel.b = (uint8_t)(b >> 2);

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
    uint8_t* packedRaw10;
    uint16_t* raw16;
    RGBPixel* rgb;
    FILE* fp;
    int x;
    int y;
    size_t packedSize = ((width * height) * 10) / 8;

    packedRaw10 = (uint8_t*)malloc(packedSize);

    if (!packedRaw10)
    {
        printf("malloc failed\n");
        return -1;
    }

    fp = fopen("input.packedRaw10", "rb");

    if (!fp)
    {
        printf("cannot open input.packedRaw10\n");
        free(packedRaw10);
        return -1;
    }

    fread(packedRaw10, 1, packedSize, fp);
    fclose(fp);

    raw16 = malloc(width *height * sizeof(uint16_t));
    unpack_raw10(packedRaw10, raw16, width * height);
    printf("First 4 unpacked pixels : %u %u %u %u\n", raw16[0], raw16[1], raw16[2], raw16[3]);
    printf("Last pixel : %u\n", raw16[width * height - 1]);

    apply_black_level(raw16, width, height,stride,BLACK_LEVEL);

    rgb = (RGBPixel*)malloc(width * height * sizeof(RGBPixel));

    if (!rgb)
    {
        free(packedRaw10);
        return -1;
    }

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            rgb[y * width + x] = demosaic_pixel(raw16, x, y, width, height, stride);
        }
    }

    write_ppm("output.ppm", rgb, width, height);

    free(packedRaw10);
    free(raw16);
    free(rgb);

    printf("Done\n");

    return 0;
}
