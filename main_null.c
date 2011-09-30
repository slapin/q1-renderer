#define PNG_DEBUG 3
#include <png.h>
#include "control.h"

void updatescr(unsigned char *ptr)
{
}

int initgfx(void)
{
}

void stopgfx(void)
{
}



void do_key_loop(void (*loopfunc)(void *data), void *data)
{
	loopfunc(data);
}

void write_png_file(char* file_name, int width,
		int height,
		const unsigned char *buffer)
{
	png_structp png_ptr;
	png_infop info_ptr;
	unsigned char **row_pointers = malloc(height * sizeof(unsigned char *));
	int i;
        /* create file */
        FILE *fp = fopen(file_name, "wb");
        if (!fp)
		return;
	for (i = 0; i < height; i++)
		row_pointers[i] = buffer + width * i;


        /* initialize stuff */
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
		return;

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
		return;

        if (setjmp(png_jmpbuf(png_ptr)))
		return;

        png_init_io(png_ptr, fp);


        /* write header */
        if (setjmp(png_jmpbuf(png_ptr)))
		return;

        png_set_IHDR(png_ptr, info_ptr, width, height,
                     8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

        png_write_info(png_ptr, info_ptr);


        /* write bytes */
        if (setjmp(png_jmpbuf(png_ptr)))
		return;

        png_write_image(png_ptr, row_pointers);


        /* end write */
        if (setjmp(png_jmpbuf(png_ptr)))
		return;

        png_write_end(png_ptr, NULL);

        fclose(fp);
}

