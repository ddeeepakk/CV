#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int width;
    int height;
    int maxval;
    unsigned char* data;
} PGMImage;

int readPGM(const char* filename, PGMImage* img)
{
    FILE* fp = fopen(filename, "rb");
    if (!fp)
    {
        printf("Cannot open input file\n");
        return -1;
    }

    char format[3];

    fscanf(fp, "%2s", format);

    if (format[0] != 'P' || format[1] != '5')
    {
        printf("Only P5 PGM supported\n");
        fclose(fp);
        return -1;
    }

    fscanf(fp, "%d %d", &img->width, &img->height);

    fscanf(fp, "%d", &img->maxval);

    fgetc(fp);

    img->data = (unsigned char*)malloc(img->width * img->height);

    if (!img->data)
    {
        fclose(fp);
        return -1;
    }

    fread(img->data, 1, img->width * img->height, fp);
    fclose(fp);

    return 0;
}

int writePGM(const char* filename, PGMImage* img)
{
    FILE* fp = fopen(filename, "wb");
    if (!fp)
    {
        printf("Cannot create output file\n");
        return -1;
    }

    fprintf(fp, "P5\n%d %d\n%d\n", img->width, img->height, img->maxval);
    fwrite(img->data, 1, img->width * img->height, fp);
    fclose(fp);

    return 0;
}

void thresholdImage(PGMImage* img, unsigned char threshold)
{
    int size = img->width * img->height;

    for (int i = 0; i < size; i++)
    {
        if (img->data[i] > threshold)
            img->data[i] = 255;
        else
            img->data[i] = 0;
    }
}

int main()
{
    PGMImage img;

    unsigned char threshold;

    if (readPGM("input.pgm", &img) != 0)
    {
        return -1;
    }

    printf("Enter threshold (0-255): ");
    scanf("%hhu", &threshold);

    thresholdImage(&img, threshold);

    writePGM("output_threshold.pgm", &img);

    free(img.data);

    printf("Thresholding complete\n");

    return 0;
}