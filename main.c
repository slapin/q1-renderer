#define PNG_DEBUG 3
#include <png.h>
#include <allegro.h>
#define WIDTH	640
#define HEIGHT	480

BITMAP *localscreen;
void update_scr(unsigned char *ptr)
{
	for (i = 0; i < HEIGHT; i++)
		memcpy(localscreen->line[i], ptr + WIDTH * i, WIDTH);
	blit(localscreen, screen, 0, 0, 0, 0, WIDTH, HEIGHT);
}

int initgfx(void)
{
	int ret;
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

struct keycb {
	int key;
	void (*func)();
	void *data;
}
static int keycount = 0;
struct keycb keylist[100];

void register_key_cb(int key, void (*func)(int key, void *data), void *data)
{
	keylist[keycount].key = key;
	keylist[keycount].func = func;
	keylist[keycount].data = data;
	keycount++;
}

void do_key_loop(void (*loopfunc)(void *data), void *data)
{
	int i;
	while(1) {
		if (key[KEY_ESC])
			break;
		for (i = 0; i < sizeof(keylist)/sizeof(keylist[0]); i++)
			if (keylist[i].key) {
				if (key[keylist[i].key])
					keylist[i].func(keylist[i].key, keylist[i].data)
			}
			loopfunc(data)
	}
}

