#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define WIDTH   640
#define HEIGHT  480

// nv12_to_rgb.c
static uint8_t clamp_to_u8(int value)
{
    if (value < 0)
        return 0;

    if (value > 255)
        return 255;

    return (uint8_t)value;
}

int main(void)
{
    const char* input_file = "input_nv12.yuv";
    const char* output_file = "output.ppm";

    FILE* fp_in = fopen(input_file, "rb");
    if (fp_in == NULL)
    {
        printf("Failed to open input file\n");
        return 1;
    }

    FILE* fp_out = fopen(output_file, "wb");
    if (fp_out == NULL)
    {
        printf("Failed to open output file\n");
        fclose(fp_in);
        return 1;
    }

    size_t y_plane_size = WIDTH * HEIGHT;
    size_t uv_plane_size = WIDTH * HEIGHT / 2;
    size_t frame_size = y_plane_size + uv_plane_size;

    uint8_t* nv12_buffer = (uint8_t*)malloc(frame_size);

    if (nv12_buffer == NULL)
    {
        printf("Memory allocation failed\n");
        fclose(fp_in);
        fclose(fp_out);
        return 1;
    }

    size_t bytes_read = fread(nv12_buffer, 1, frame_size, fp_in);

    if (bytes_read != frame_size)
    {
        printf("Unexpected file size\n");

        free(nv12_buffer);
        fclose(fp_in);
        fclose(fp_out);
        return 1;
    }

    uint8_t* y_plane = nv12_buffer;
    uint8_t* uv_plane = nv12_buffer + y_plane_size;

    /*
        PPM Header

        P6
        WIDTH HEIGHT
        255
    */
    fprintf(fp_out, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
            uint8_t Y = y_plane[y * WIDTH + x];

            int uv_x = x / 2;
            int uv_y = y / 2;

            int uv_index = uv_y * WIDTH + (uv_x * 2);

            uint8_t U = uv_plane[uv_index];
            uint8_t V = uv_plane[uv_index + 1];

            /*
                BT.601 Limited Range
            */

            int C = (int)Y - 16;
            int D = (int)U - 128;
            int E = (int)V - 128;

            int R = (298 * C + 409 * E + 128) >> 8;
            int G = (298 * C - 100 * D - 208 * E + 128) >> 8;
            int B = (298 * C + 516 * D + 128) >> 8;

            uint8_t rgb[3];

            rgb[0] = clamp_to_u8(R);
            rgb[1] = clamp_to_u8(G);
            rgb[2] = clamp_to_u8(B);

            fwrite(rgb, 1, 3, fp_out);
        }
    }

    free(nv12_buffer);

    fclose(fp_in);
    fclose(fp_out);

    printf("Conversion complete\n");

    return 0;
}