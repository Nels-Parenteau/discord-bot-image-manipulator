#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <png.h>
#include <zlib.h>
#include <jpeglib.h>
#include <jerror.h>

// Replacement struct & error_exit as specified in "example.c"
struct my_error_mgr {
  struct jpeg_error_mgr pub;

  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

void my_error_exit (j_common_ptr cinfo)
{
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  (*cinfo->err->output_message) (cinfo);
  
  longjmp(myerr->setjmp_buffer, 1);
}

// This comes directly from the example.c file from the libpng docs.
int check_if_png(char *file_name) {
  char buf[4];

  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    return 0;
  }

  // Read in some of the signature bytes.
  if (fread(buf, 1, 4, fp) != 4) {
    return 0;
  }

  // Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
  return(!png_sig_cmp(buf, 0, 4));
}

// Raw iamge struct, holds data about the RGB formating eg RGB888 or grayscale
struct rawImage {
  unsigned int numComponents;
  JDIMENSION width, height;
  JSAMPROW data;
  J_COLOR_SPACE colorSpace;
};

// This will return the subpixel
int getSubPixel(struct rawImage* rawImageData, int xCoord, int yCoord) {
  return rawImageData->data[rawImageData->width * rawImageData->numComponents
                            * yCoord + xCoord];
}

// This will set the subpixel
void setSubPixel(struct rawImage* rawImageData, int xCoord, 
                 int yCoord, unsigned char data) {
  rawImageData->data[rawImageData->width * rawImageData->numComponents * yCoord
                     + xCoord] = data;
  return;
}

// This function assumes that the input png is of exactly the format RGB888.
// This is because png can be very complicated and it's output bytestream
// can be very incompatible with jpeg.
int toJpeg(){
  png_image image;
  memset(&image, 0, (sizeof image));
  image.version = PNG_IMAGE_VERSION;

  if (png_image_begin_read_from_file(&image, "output.png")) {
      png_bytep buffer;

      image.format = PNG_FORMAT_RGB;

      buffer = malloc(PNG_IMAGE_SIZE(image));

      if (buffer != NULL 
          && png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
        // setting up the jpeg part
        struct jpeg_compress_struct info;
        struct jpeg_error_mgr err;

        info.err = jpeg_std_error(&err);
        FILE* fp = fopen("input.photo", "wb");

        jpeg_create_compress(&info);
        if(fp == NULL) {
          return 1;
        }

        jpeg_stdio_dest(&info, fp);

        // RGB888 format
        info.in_color_space = JCS_RGB;
        info.image_width = image.width;
        info.image_height = image.height;
        info.input_components = 3;

        jpeg_set_defaults(&info);
        jpeg_set_quality(&info, 90, TRUE);
        jpeg_start_compress(&info, TRUE);

        JSAMPROW row_pointer[1];

        int row_stride = info.image_width * info.input_components;  

        while(info.next_scanline < info.image_height) {
          row_pointer[0] = &(buffer[info.next_scanline * row_stride]);
          jpeg_write_scanlines(&info, row_pointer, 1);
        }

        jpeg_finish_compress(&info);
        jpeg_destroy_compress(&info);
        fclose(fp);
        return 0;
      }
    }
}

// This comes directly from the example.c file from the libpng docs.
int pngToJpeg(char* imgFile) {
  png_image image;
  memset(&image, 0, (sizeof image));
  image.version = PNG_IMAGE_VERSION;

  if (png_image_begin_read_from_file(&image, imgFile) != 0) {
    png_bytep buffer;

    image.format = PNG_FORMAT_RGB;
    buffer = malloc(PNG_IMAGE_SIZE(image));

    if (buffer != NULL 
        && png_image_finish_read(&image, NULL, buffer, 0, NULL)) {

      if (png_image_write_to_file(&image, "output.png", 0, buffer, 0, NULL)) {
        return toJpeg();
      }
    }
    else {
      if (buffer == NULL) {
        png_image_free(&image);
      }
      else {
        free(buffer);
      }
    }
  }
  return 1;
}

// Reference libjpeg.doc for information about the ordering of the steps. We
// will be using the numbering described from "Decompression details" section.
struct rawImage* jpegToRaw(char* imgFile) {
  // 1. Allocate and initialize a JPEG decompression object.
  struct jpeg_decompress_struct info;
  struct my_error_mgr err;

  struct rawImage* newImage;

  jpeg_create_decompress(&info);
  info.err = jpeg_std_error(&err);

  // 2. Specify the source of the compressed data (eg, a file).
  FILE* fp = fopen(imgFile, "rb");
  if(fp == NULL) {
    return NULL;
  } 
  jpeg_stdio_src(&info, fp);

  // "Hyjacking" the error_exit function pointer with out own that will not 
  // exit().
  err.pub.error_exit = my_error_exit;

  // Establish the setjmp return context for my_error_exit to use.
  if (setjmp(err.setjmp_buffer)) {
    // If we get here, the JPEG code has signaled an error.
    // We need to clean up the JPEG object, close the input file, and return.
    jpeg_destroy_decompress(&info);
    fclose(fp);
    return NULL;
  }
  
  // 3. Call jpeg_read_header() to obtain image info.
  jpeg_read_header(&info, TRUE);

  // 5. jpeg_start_decompress(...);
  jpeg_start_decompress(&info);

  unsigned char* data;
  JSAMPROW rowBuffer[1];
  data = malloc(info.output_width * info.output_height 
                                  * info.num_components);

  newImage = malloc(sizeof(struct rawImage));
  newImage->numComponents = info.num_components;
  newImage->width = info.output_width;
  newImage->height = info.output_height;
  newImage->colorSpace = info.out_color_space;
  newImage->data = data;

  while(info.output_scanline < info.output_height) {
    rowBuffer[0] = &data [newImage->numComponents 
                       * info.output_width * info.output_scanline];
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
  // Establish the setjmp return context for my_error_exit to use.
  if (setjmp(err.setjmp_buffer)) {
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
  info.in_color_space = newImage->colorSpace;
  info.image_width = newImage->width;
  info.image_height = newImage->height;
  info.input_components = newImage->numComponents;

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

// This takes in a jpeg and uses the compression algorithm set to 0
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

// Runs an image kernal that makes any large difference between pixels more
// noticable.
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
  newImage->colorSpace = rawImageData->colorSpace;
  newImage->data = data;

  int comp = newImage->numComponents;

  for (int i = 0; i < rawImageData->height; i++) {
    for (int j = 0; j < rawImageData->width * comp; j++) {
      if (i <= 0 || j <= 0) {
        setSubPixel(newImage, j, i, 0);
        continue;
      }
      if (i >= rawImageData->height - 1 || j >= (rawImageData->width - 1) * comp) {
        setSubPixel(newImage, j, i, 0);
        continue;
      }

      // Our image kernal
      int sharpen =  9 * getSubPixel(rawImageData, j       , i    )
                   - 2 * getSubPixel(rawImageData, j + comp, i    )
                   - 2 * getSubPixel(rawImageData, j       , i + 1)
                   - 2 * getSubPixel(rawImageData, j - comp, i    )
                   - 2 * getSubPixel(rawImageData, j       , i - 1);

      if (sharpen > 255) {
        sharpen = 255;
      }
      if (sharpen < 0) {
        sharpen = 0;
      }

      setSubPixel(newImage, j, i, sharpen);
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

// This really highlights borders
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
  newImage->colorSpace = rawImageData->colorSpace;
  newImage->data = data;

  int comp = newImage->numComponents;

  for (int i = 0; i < rawImageData->height; i++) {
    for (int j = 0; j < rawImageData->width * comp; j++) {
      if (i <= 0 || j <= 0) {
        setSubPixel(newImage, j, i, 0);
        continue;
      }
      if (i >= rawImageData->height - 1 || j >= (rawImageData->width - 1) * comp) {
        setSubPixel(newImage, j, i, 0);
        continue;
      }

      // Our image kernal
      int border = 16  * getSubPixel(rawImageData, j       , i    )
                   - 4 * getSubPixel(rawImageData, j + comp, i    )
                   - 4 * getSubPixel(rawImageData, j       , i + 1)
                   - 4 * getSubPixel(rawImageData, j - comp, i    )
                   - 4 * getSubPixel(rawImageData, j       , i - 1);

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

// Creates a text file such that the pixel's brightness corresponds to ascii 
// characters.
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

  if (check_if_png(argv[2])) {
    pngToJpeg(argv[2]);
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

  return 1;
}