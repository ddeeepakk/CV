#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define INPUT_FILE  "input.pgm"
#define OUTPUT_FILE "output.pgm"

int main(void)
{
    FILE* fp = fopen(INPUT_FILE, "rb");
    if (fp == NULL)
    {
        printf("Failed to open input file\n");
        return -1;
    }

    char imgFormat[3];
    int width;
    int height;
    int maxval;

    fscanf(fp, "%2s", imgFormat);

    if (strcmp(imgFormat, "P5") != 0)
    {
        printf("Only binary PGM (P5) supported\n");
        fclose(fp);
        return -1;
    }

    fscanf(fp, "%d %d", &width, &height);
    fscanf(fp, "%d", &maxval);

    fgetc(fp);

    printf("Width  = %d\n", width);
    printf("Height = %d\n", height);

    uint8_t* input = (uint8_t*)malloc(width * height);
    uint8_t* output = (uint8_t*)malloc(width * height);

    if (!input || !output)
    {
        printf("Memory allocation failed\n");
        fclose(fp);
        return -1;
    }

    fread(input, 1, width * height, fp);
    fclose(fp);

    memcpy(output, input, width * height);

    const int kernel[3][3] =
    {
        { 1, 2, 1 },
        { 2, 4, 2 },
        { 1, 2, 1 }
    };

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sum = 0;
            for (int ky = -1; ky <= 1; ky++)
            {
                for (int kx = -1; kx <= 1; kx++)
                {
                    int pixel = input[(y + ky) * width + (x + kx)];
                    int weight = kernel[ky + 1][kx + 1];
                    sum += pixel * weight;
                }
            }

            output[y * width + x] = (uint8_t)(sum / 16);
        }
    }

    fp = fopen(OUTPUT_FILE, "wb");
    if (fp == NULL)
    {
        printf("Failed to create output file\n");
        free(input);
        free(output);
        return -1;
    }

    fprintf(fp, "P5\n%d %d\n255\n", width, height);
    fwrite(output, 1, width * height, fp);

    fclose(fp);
    free(input);
    free(output);

    printf("Gaussian blur completed\n");
    printf("Output saved to %s\n", OUTPUT_FILE);

    return 0;
}
