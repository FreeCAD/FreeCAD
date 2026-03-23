/*
  This utility is just a hack to extract a monospaced bitmap font from an
  image file and outputting it as C code.

  Windows building:
    coin-config --build extractfont extractfont.cpp -lsimage1
 */

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>

#define SIMAGE_DLL

#include <simage.h>

#include <Inventor/system/inttypes.h>

#ifndef FALSE
#define FALSE (0)
#define TRUE (!FALSE)
#endif

int
main(int argc, char ** argv)
{
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);

  static unsigned char charset[256];
  int c;
  for ( c = 0; c < 256; c++ ) { charset[c] = c; }

  int options = 0;
  int output_c = TRUE;
  unsigned char * output = charset;
  int outputlen = 256;

  while ( argv[1+options] && argv[1+options][0] == '-' ) {
    // check options
    if ( strcmp(argv[1+options], "--test") == 0 ) {
      options++;
      output = (unsigned char *) argv[1+options];
      outputlen = strlen((const char *) output);
      output_c = FALSE;
    }
    options++;
  }

  if ( argc != (9 + options) ) {
    printf("usage:\n");
    printf("  extractfont [options] <cw> <ch> <xsp> <ysp> <xoff> <yoff> <cpl> <imagefile>\n");
    printf("arguments:\n");
    printf("  cw - character width\n");
    printf("  ch - character height\n");
    printf("  xsp - x spacing\n");
    printf("  ysp - y spacing\n");
    printf("  xoff - x offset\n");
    printf("  yoff - y offset\n");
    printf("  cpl - characters per line\n");
    printf("options:\n");
    printf("  --test <string>\n");
    return 0;
  }

  int charwidth    = atol(argv[1+options]);
  int charheight   = atol(argv[2+options]);
  int xspacing     = atol(argv[3+options]);
  int yspacing     = atol(argv[4+options]);
  int xoffset      = atol(argv[5+options]);
  int yoffset      = atol(argv[6+options]);
  int charsperline = atol(argv[7+options]);
  const char * imgfile  = argv[8+options];
  assert(imgfile);

  fprintf(stderr, "char width:  %d\n", charwidth);
  fprintf(stderr, "char height: %d\n", charheight);
  fprintf(stderr, "x offset:    %d\n", xoffset);
  fprintf(stderr, "y offset:    %d\n", yoffset);
  fprintf(stderr, "x spacing:   %d\n", xspacing);
  fprintf(stderr, "y spacing:   %d\n", yspacing);
  fprintf(stderr, "chars/line:  %d\n", charsperline);


  s_image * image = s_image_load(imgfile, NULL);

  if ( !image ) {
    printf("error: could not open file %s\n", imgfile);
    return 1;
  }

  int width = s_image_width(image);
  int height = s_image_height(image);
  int components = s_image_components(image);
  unsigned char * data = s_image_data(image);
  assert(data);

  fprintf(stderr, ">image:\n");
  fprintf(stderr, "width:  %d\n", width);
  fprintf(stderr, "height: %d\n", height);
  fprintf(stderr, "comp:   %d\n", components);
  fflush(stderr);

#define THRESHOLD 20

  if ( output_c ) {
    // printf("static uint32_t font_data[][%d] = {\n", charheight);
  }

  for ( c = 0; c < outputlen; c++ ) {
    int letter = output[c];
    int row = letter / charsperline;
    int column = letter % charsperline;

    if ( !output_c ) {
      // output visual text for debugging
      printf("letter: %c (%d, %d)\n", letter, column, row);
      int x, y;
      printf("+");
      for ( x = 0; x < charwidth; x++ ) { printf("-"); }
      printf("+\n");
      for ( y = 0; y < charheight; y++ ) {
        printf("|");
        for ( x = 0; x < charwidth; x++ ) {
          int xpos = xoffset + (column * (charwidth + xspacing)) + x;
          int ypos = yoffset + (row * (charheight + yspacing)) + y;
          
          ypos = height - (ypos + 1);
          
          int offset = ((ypos * width) + xpos) * components;
          int hit = FALSE;
          int i;
          if ( xpos < 0 || xpos >= width || ypos < 0 || ypos >= height ) {
          } else {
            for ( i = 0; i < components; i++ ) {
              if ( (components == 2 && i == 1) || (components == 4 && i == 3) ) {
              } else if ( data[offset+i] > THRESHOLD ) {
                // dark/light scan, assuming no alpha
                hit = TRUE;
              }
            }
          }
          if ( hit ) {
            printf("#");
          } else {
            printf(" ");
          }
        }
        printf("|\n");
      }
      printf("+");
      for ( x = 0; x < charwidth; x++ ) { printf("-"); }
      printf("+\n");
    } else {
      // output C/C++ code
      printf("  { ");
      int x, y;
      for ( y = charheight - 1; y >= 0; y-- ) {
        uint32_t bits = 0;
        for ( x = 0; x < charwidth; x++ ) {
          int xpos = xoffset + (column * (charwidth + xspacing)) + x;
          int ypos = yoffset + (row * (charheight + yspacing)) + y;
          
          ypos = height - (ypos + 1);

          int offset = ((ypos * width) + xpos) * components;
          int hit = FALSE;
          int i;
          if ( xpos < 0 || xpos >= width || ypos < 0 || ypos >= height ) {
          } else {
            for ( i = 0; i < components; i++ ) {
              if ( (components == 2 && i == 1) || (components == 4 && i == 3) ) {
              } else if ( data[offset+i] > THRESHOLD ) {
                // dark/light scan, assuming no alpha
                hit = TRUE;
              }
            }
          }
          bits = bits << 1;
          if ( hit ) {
            bits |= 1;
          }
        }
        bits = bits << (32 - charwidth - 1);
        if ( bits == 0 ) {
          printf("0,0,0,0");
        } else {
          unsigned int part = (bits >> 24) & 0xff;
          printf("0x%x", part);
          part = (bits >> 16) & 0xff;
          printf(",0x%x", part);
          part = (bits >> 8) & 0xff;
          printf(",0x%x", part);
          part = bits & 0xff;
          printf(",0x%x", part);
        }
        if ( y > 0 ) {
          printf(",");
        }
      }
      printf(" }");
      if ( c < (outputlen - 1) ) { printf(","); }
      if ( isprint(letter) ) {
        printf(" /* letter 0x%02x '%c' */", letter, letter);
      } else {
        printf(" /* letter 0x%02x */", letter);
      }
      printf("\n");
    }
  }

  if ( output_c ) {
    // printf("};\n");
  }

  s_image_destroy(image);

  return 0;
}
