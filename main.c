#define PNG_DEBUG 3
#include <png.h>
#include <allegro.h>
#include "control.h"

BITMAP *localscreen;
void updatescr(unsigned char *ptr)
{
	int i;
	for (i = 0; i < HEIGHT; i++)
		memcpy(localscreen->line[i], ptr + WIDTH * i, WIDTH);
	blit(localscreen, screen, 0, 0, 0, 0, WIDTH, HEIGHT);
}

int initgfx(void)
{
	int ret, i;
	PALETTE pal;
	ret = allegro_init();
	if (ret)
		return -1;
	ret = install_keyboard();
	if (ret)
		return -1;
	ret = install_timer();
	if (ret)
		return -1;
	set_color_depth(8);
	ret = set_gfx_mode(GFX_AUTODETECT_WINDOWED, WIDTH, HEIGHT, 0, 0);
	if (ret)
		return -1;
#if 0
	ret = install_mouse();
	if (ret)
		return -1;
	show_mouse(NULL);
#endif
	for (i = 0; i < 256; i++) {
		pal[i].r = i >> 0;
		pal[i].g = i >> 0;
		pal[i].b = i >> 0;
	}
	set_palette(pal);
	localscreen = create_bitmap(WIDTH, HEIGHT);
	clear_keybuf();
	return 0;
}

void stopgfx(void)
{
	allegro_exit();
}



void register_control_key(struct control_data *cd,
	int key_up, int key_down, float *value,
	float d, float vmin, float vmax)
{
	cd->keys[cd->nkeys].kup = key_up;
	cd->keys[cd->nkeys].kdown = key_down;
	cd->keys[cd->nkeys].value = value;
	cd->keys[cd->nkeys].d = d;
	cd->keys[cd->nkeys].min = vmin;
	cd->keys[cd->nkeys].max = vmax;
	cd->nkeys++;
}

void do_key_loop(void (*loopfunc)(void *data), void *data)
{
	int i;
	struct control_data *cd = data;
	register_control_key(cd, KEY_LEFT, KEY_RIGHT, &cd->origin[0], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_UP, KEY_DOWN, &cd->origin[1], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_Q, KEY_A, &cd->vup[0], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_W, KEY_S, &cd->vup[1], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_E, KEY_D, &cd->vup[2], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_R, KEY_F, &cd->vright[0], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_T, KEY_G, &cd->vright[1], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_Y, KEY_H, &cd->vright[2], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_U, KEY_J, &cd->vpn[0], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_I, KEY_K, &cd->vpn[1], 1.,
	-1000.0, 1000.0);
	register_control_key(cd, KEY_O, KEY_L, &cd->vpn[2], 1.,
	-1000.0, 1000.0);
	while(1) {
		if (key[KEY_ESC])
			break;
		for (i = 0; i < cd->nkeys; i++) {
			if (key[cd->keys[i].kup]) {
				*(cd->keys[i].value) += cd->keys[i].d;
				if (*(cd->keys[i].value) > cd->keys[i].max)
					*(cd->keys[i].value) =
					cd->keys[i].max;
				printf("+val %f\n", *(cd->keys[i].value));
			}
			if (key[cd->keys[i].kdown]) {
				*(cd->keys[i].value) -= cd->keys[i].d;
				if (*(cd->keys[i].value) < cd->keys[i].min)
					*(cd->keys[i].value) =
					cd->keys[i].min;
				printf("+val %f\n", *(cd->keys[i].value));
			}
		}
		loopfunc(data);
	}
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

