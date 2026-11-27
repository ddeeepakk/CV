/*
    p010_to_rgb.c

    Simple educational P010 reader.

    Assumptions:
      - P010 (4:2:0)
      - Width and Height are known
      - Little-endian platform
      - Output is RGB888 (PPM)

    Memory Layout:

      Y Plane:
      [Y Y Y Y ...]

      UV Plane:
      [U V U V U V ...]

    Each sample is stored as:

      Bits [15:6] = 10-bit pixel value
      Bits [5:0 ] = 0

    Example:

      uint16_t stored = 32768;

      actual_10bit = stored >> 6;
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(void)
{
     //Image dimensions                                     
    const int WIDTH = 640;
    const int HEIGHT = 480;

    const char* inputFileName = "input.p010";
    const char* outputFileName = "output.ppm";

     //Open input file                                      
    FILE* inputFile = fopen(inputFileName, "rb");
    if (inputFile == NULL)
    {
        printf("Failed to open %s\n", inputFileName);
        return 1;
    }

     //Compute expected file size                           

    size_t yPlaneBytes = WIDTH * HEIGHT * sizeof(uint16_t);
    size_t uvPlaneBytes = WIDTH * HEIGHT / 2 * sizeof(uint16_t);
    size_t totalBytes = yPlaneBytes + uvPlaneBytes;

    printf("Expected file size : %zu bytes\n", totalBytes);

     //Allocate memory                                      
    uint8_t* fileBuffer = (uint8_t*)malloc(totalBytes);
    if (fileBuffer == NULL)
    {
        printf("Memory allocation failed\n");

        fclose(inputFile);
        return 1;
    }

     //Read entire file                                     
    size_t bytesRead = fread(fileBuffer, 1, totalBytes, inputFile);
    fclose(inputFile);

    if (bytesRead != totalBytes)
    {
        printf("File size mismatch\n");
        printf("Expected %zu bytes\n", totalBytes);
        printf("Read %zu bytes\n", bytesRead);

        free(fileBuffer);
        return 1;
    }

    printf("Input file read successfully\n");

     //Locate planes                                        
    uint16_t* yPlane = (uint16_t*)fileBuffer;
    uint16_t* uvPlane = (uint16_t*)(fileBuffer + yPlaneBytes);

     //Create output ppm                                    
    FILE* outputFile = fopen(outputFileName, "wb");
    if (outputFile == NULL)
    {
        printf("Failed to create output file\n");

        free(fileBuffer);
        return 1;
    }

     //PPM Header                                           
    fprintf(outputFile, "P6\n%d %d\n255\n", WIDTH, HEIGHT);

     //Process every pixel                                  
    for (int y = 0; y < HEIGHT; y++)
    {
        for (int x = 0; x < WIDTH; x++)
        {
             //Read Y                                   
            int yIndex = y * WIDTH + x;
            uint16_t yStored = yPlane[yIndex];
            uint16_t y10 = yStored >> 6;

             //Locate UV                                
            int uvX = x / 2;
            int uvY = y / 2;
            int uvIndex = (uvY * WIDTH) + (uvX * 2);

            uint16_t uStored = uvPlane[uvIndex + 0];
            uint16_t vStored = uvPlane[uvIndex + 1];

            uint16_t u10 = uStored >> 6;
            uint16_t v10 = vStored >> 6;

             //Convert to float                         
            float Yf = (float)y10;
            float Uf = (float)u10 - 512.0f;
            float Vf = (float)v10 - 512.0f;

             //YUV -> RGB                               
            float r10 = Yf + 1.402f * Vf;
            float g10 = Yf - 0.344136f * Uf - 0.714136f * Vf;
            float b10 = Yf + 1.772f * Uf;

             //Clamp 10-bit RGB                         
            if (r10 < 0.0f)
                r10 = 0.0f;

            if (r10 > 1023.0f)
                r10 = 1023.0f;

            if (g10 < 0.0f)
                g10 = 0.0f;

            if (g10 > 1023.0f)
                g10 = 1023.0f;

            if (b10 < 0.0f)
                b10 = 0.0f;

            if (b10 > 1023.0f)
                b10 = 1023.0f;

             //Convert 10-bit -> 8-bit                  
            uint8_t r8 = (uint8_t)((r10 * 255.0f) / 1023.0f);
            uint8_t g8 = (uint8_t)((g10 * 255.0f) / 1023.0f);
            uint8_t b8 = (uint8_t)((b10 * 255.0f) / 1023.0f);

            fwrite(&r8, 1, 1, outputFile);
            fwrite(&g8, 1, 1, outputFile);
            fwrite(&b8, 1, 1, outputFile);
        }
    }

    fclose(outputFile);
    free(fileBuffer);

    printf("Output written to %s\n", outputFileName);

    return 0;
}