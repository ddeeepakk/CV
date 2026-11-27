#include <stdio.h>
#include <stdint.h>

#define WIDTH   640
#define HEIGHT  480

int main(void)
{
    FILE* fp = fopen("input_gray.y", "rb");

    if (fp == NULL)
    {
        printf("Failed to open file\n");
        return -1;
    }

    uint32_t histogram[256] = { 0 };

    uint8_t pixel;

    while (fread(&pixel, 1, 1, fp) == 1)
    {
        histogram[pixel]++;
    }

    fclose(fp);

    for (int i = 0; i < 256; i++)
    {
        printf("%3d : %u\n", i, histogram[i]);
    }

    return 0;
}