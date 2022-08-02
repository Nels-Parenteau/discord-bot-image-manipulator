#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <jpeglib.h>
#include <jerror.h>

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  
  longjmp(myerr->setjmp_buffer, 1);
}

// Raw iamge struct, holds data about the RGB formating eg RGB888 or grayscale
struct rawImage {
  unsigned int numComponents;
  unsigned long int width, height;
  unsigned char* data;
};

int getSubPixel(struct rawImage* rawImageData, int xCoord, int yCoord) {
  return rawImageData->data[rawImageData->width * rawImageData->numComponents
                            * yCoord + xCoord];
}

void setSubPixel(struct rawImage* rawImageData, int xCoord, 
                 int yCoord, unsigned char data) {
  rawImageData->data[rawImageData->width * rawImageData->numComponents * yCoord
                     + xCoord] = data;
  return;
}

//TO-DO way more error catching!
struct rawImage* jpegToRaw(char* imgFile) {
  struct jpeg_decompress_struct info;
  struct my_error_mgr err;

  struct rawImage* newImage;
  
  unsigned char* data;
  JSAMPROW rowBuffer[1];

  FILE* fp = fopen(imgFile, "rb");
  if(fp == NULL) {
    return NULL;
  }

  info.err = jpeg_std_error(&err);

  err.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(err.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress(&info);
    fclose(fp);
    return NULL;
  }
  
  jpeg_create_decompress(&info);
  jpeg_stdio_src(&info, fp);
  jpeg_read_header(&info, TRUE);
  jpeg_start_decompress(&info);

  data = malloc(info.output_width * info.output_height * 3);

  newImage = malloc(sizeof(struct rawImage));
  newImage->numComponents = info.num_components;
  newImage->width = info.output_width;
  newImage->height = info.output_height;
  newImage->data = data;

  while(info.output_scanline < info.output_height) {
    rowBuffer[0] = &data[3 * info.output_width * info.output_scanline];
    jpeg_read_scanlines(&info, rowBuffer, 1);
  }

  jpeg_finish_decompress(&info);
  jpeg_destroy_decompress(&info);
  fclose(fp);

  return newImage;
}

// Reference libjpeg.doc for information about the ordering of the steps. We
// will be using the numbering described from "Compression details" section.
int rawToJpeg(struct rawImage* newImage, char* saveTo, int quality) {
  // 1. Allocate and initialize a JPEG compression object.
  struct jpeg_compress_struct info;
  struct my_error_mgr err;

  info.err = jpeg_std_error(&err);
  FILE* fp = fopen(saveTo, "wb");

  err.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(err.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_compress(&info);
    fclose(fp);
    return 1;
  }

  jpeg_create_compress(&info);

  // 2. Specify the destination for the compressed data.
  if(fp == NULL) {
    return 1;
  }

  jpeg_stdio_dest(&info, fp);

  // 3. Set parameters for compression, including image size & colorspace.
  info.image_width = newImage->width;
  info.image_height = newImage->height;
  info.input_components = newImage->numComponents; 
  info.in_color_space = JCS_RGB; //fix !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  jpeg_set_defaults(&info);
  jpeg_set_quality(&info, quality, TRUE);

  // 4. jpeg_start_compress
  jpeg_start_compress(&info, TRUE);

  // 5. while (scan lines remain to be written)
  JSAMPROW row_pointer[1];

  int row_stride = info.image_width * info.input_components;  

  while(info.next_scanline < info.image_height) {
    row_pointer[0] = &(newImage->data[info.next_scanline * row_stride]);
    jpeg_write_scanlines(&info, row_pointer, 1);
  }

  // 6. jpeg_finish_compress
  jpeg_finish_compress(&info);

  // 7. Release the JPEG compression object.
  jpeg_destroy_compress(&info);

  fclose(fp);
  return 0;
}

int jpegCompressor (char* inputJpeg, char* outputJpeg) {
  struct rawImage* rawImageData = jpegToRaw(inputJpeg);

  if (rawImageData == NULL || rawImageData->data == NULL) {
    return 1;
  }

  int rawToJpegReturn = rawToJpeg(rawImageData, outputJpeg, 0);

  // No chance we free non-malloc'd data as we check above
  free(rawImageData->data);
  free(rawImageData);

  return rawToJpegReturn;
}

int jpegSharpen (char* inputJpeg, char* outputJpeg) {
  struct rawImage* rawImageData = jpegToRaw(inputJpeg);

  if (rawImageData == NULL || rawImageData->data == NULL) {
    return 1;
  }

  struct rawImage* newImage;

  JSAMPROW data;

  data = malloc(rawImageData->width * rawImageData->height 
               * rawImageData->numComponents);

  newImage = malloc(sizeof(struct rawImage));
  newImage->numComponents = rawImageData->numComponents;
  newImage->width = rawImageData->width;
  newImage->height = rawImageData->height;
  newImage->data = data;

  for (int i = 0; i < rawImageData->height; i++) {
    for (int j = 0; j < rawImageData->width * 3; j++) {
      if (i < 3 || j < 3) {
        setSubPixel(newImage, j, i, 0);
      }
      if (i > rawImageData->height - 3 || j > rawImageData->width - 3) {
        setSubPixel(newImage, j, i, 0);
      }
      int border = 9 * getSubPixel(rawImageData, j, i)
                   - 2 * getSubPixel(rawImageData, j + 3, i)
                   - 2 * getSubPixel(rawImageData, j, i + 3)
                   - 2 * getSubPixel(rawImageData, j - 3, i)
                   - 2 * getSubPixel(rawImageData, j, i - 3);
      if (border > 255) {
        border = 255;
      }
      if (border < 0) {
        border = 0;
      }
      setSubPixel(newImage, j, i, border);
    }
  }

  int rawToJpegReturn = rawToJpeg(newImage, outputJpeg, 90);

  // No chance we free non-malloc'd data as we check above
  free(rawImageData->data);
  free(rawImageData);
  free(newImage->data);
  free(newImage);


  return rawToJpegReturn;
}

int jpegBorder (char* inputJpeg, char* outputJpeg) {
  struct rawImage* rawImageData = jpegToRaw(inputJpeg);

  if (rawImageData == NULL || rawImageData->data == NULL) {
    return 1;
  }

  struct rawImage* newImage;

  JSAMPROW data;

  data = malloc(rawImageData->width * rawImageData->height 
               * rawImageData->numComponents);

  newImage = malloc(sizeof(struct rawImage));
  newImage->numComponents = rawImageData->numComponents;
  newImage->width = rawImageData->width;
  newImage->height = rawImageData->height;
  newImage->data = data;

  for (int i = 0; i < rawImageData->height; i++) {
    for (int j = 0; j < rawImageData->width * 3; j++) {
      if (i < 3 || j < 3) {
        setSubPixel(newImage, j, i, 0);
      }
      if (i > rawImageData->height - 3 || j > rawImageData->width - 3) {
        setSubPixel(newImage, j, i, 0);
      }
      int border = 16 * getSubPixel(rawImageData, j, i)
                   - 4 * getSubPixel(rawImageData, j + 3, i)
                   - 4 * getSubPixel(rawImageData, j, i + 3)
                   - 4 * getSubPixel(rawImageData, j - 3, i)
                   - 4 * getSubPixel(rawImageData, j, i - 3);
      if (border > 255) {
        border = 255;
      }
      if (border < 0) {
        border = 0;
      }
      setSubPixel(newImage, j, i, border);
    }
  }

  int rawToJpegReturn = rawToJpeg(newImage, outputJpeg, 90);

  // No chance we free non-malloc'd data as we check above
  free(rawImageData->data);
  free(rawImageData);
  free(newImage->data);
  free(newImage);


  return rawToJpegReturn;
}

int jpegToText(char* inputJpeg, char* outputText) {
  struct rawImage* rawImageData = jpegToRaw(inputJpeg);

  if (rawImageData == NULL || rawImageData->data == NULL) {
    return 1;
  }

  FILE* file;

	file = fopen(outputText, "w+");

  int jumpBy = rawImageData->width / 40;

  for (int i = 0; i < rawImageData->height; i += 2 * jumpBy) {
    for (int j = 0; j < rawImageData->width; j += jumpBy) {
      int avgData = (getSubPixel(rawImageData, j * 3, i)
                  + getSubPixel(rawImageData, j * 3, i)
                  + getSubPixel(rawImageData, j * 3, i)) / 3;
      
      char out =
      (0x0F >= avgData)                   * ' ' +
      (0x1F >= avgData && avgData > 0x0F) * '.' +
      (0x2F >= avgData && avgData > 0x1F) * ',' +
      (0x3F >= avgData && avgData > 0x2F) * '-' +
      (0x4F >= avgData && avgData > 0x3F) * ':' +
      (0x5F >= avgData && avgData > 0x4F) * ';' +
      (0x6F >= avgData && avgData > 0x5F) * '-' +
      (0x7F >= avgData && avgData > 0x6F) * '+' +
      (0x8F >= avgData && avgData > 0x7F) * '>' +
      (0x9F >= avgData && avgData > 0x8F) * '=' +
      (0xAF >= avgData && avgData > 0x9F) * 'o' +
      (0xBF >= avgData && avgData > 0xAF) * '^' +
      (0xCF >= avgData && avgData > 0xBF) * 'F' +
      (0xDF >= avgData && avgData > 0xCF) * 'O' +
      (0xEF >= avgData && avgData > 0xDF) * '&' +
      (avgData > 0xEF)                    * '@';


      fprintf(file,"%c", out);
    }
    fprintf(file, "\n");
  }

  // No chance we free non-malloc'd data as we check above
  free(rawImageData->data);
  free(rawImageData);

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    return 1;
  }

  if (strcmp(argv[1], "compress") == 0) {
    return jpegCompressor(argv[2], "outputJpeg.jpg");
  }

  if (strcmp(argv[1], "border") == 0) {
    return jpegBorder(argv[2], "outputJpeg.jpg");
  }

  if (strcmp(argv[1], "text") == 0) {
    return jpegToText(argv[2], "jpegToText.txt");
  }

  if (strcmp(argv[1], "sharpen") == 0) {
    return jpegSharpen(argv[2], "outputJpeg.jpg");
  }

  printf("finished!");

  return 1;
}