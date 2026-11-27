#include <stdio.h>
#include <stdint.h>

#define NUM_BINS 16

int main(void)
{
    FILE* fp = fopen("input_gray.y", "rb");

    if (fp == NULL)
    {
        printf("Failed to open file\n");
        return -1;
    }

    uint32_t histogram[NUM_BINS] = { 0 };

    uint8_t pixel;

    while (fread(&pixel, 1, 1, fp) == 1)
    {
        int bin = pixel / 16;

        histogram[bin]++;
    }

    fclose(fp);

    for (int i = 0; i < NUM_BINS; i++)
    {
        int start = i * 16;
        int end = start + 15;

        printf("[%3d - %3d] : %u\n",
            start,
            end,
            histogram[i]);
    }

    return 0;
}