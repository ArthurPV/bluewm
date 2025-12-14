#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <bluewm.h>

#define WINDOW_HEIGHT 20
#define WINDOW_MIDDLE(font) ((WINDOW_HEIGHT / 2) + ((font)->ascent - ((font)->ascent + (font)->descent) / 2))

Display *display = NULL;
Window window = {0};
unsigned int window_bg_color = BLUE_RGB(66, 85, 148);
unsigned int window_width = 0;
unsigned int window_height = 0;
GC window_gc = {0};
XFontStruct *font = NULL;
Pixmap window_pixels = {0};

static void draw_bg__BlueWMBar(void);

static const char *get_current_keyboard_layout__BlueWMBar(void);

static void draw_keyboard_layout__BlueWMBar(void);

static void draw_date__BlueWMBar(void);

static void draw__BlueWMBar(void);

static void set_font__BlueWMBar(void);

static void close__BlueWMBar(void);

void draw_bg__BlueWMBar(void)
{
	XSetForeground(display, window_gc, window_bg_color);
	XFillRectangle(display, window_pixels, window_gc, 0, 0, window_width, window_height);
}

const char *get_current_keyboard_layout__BlueWMBar(void)
{
	return NULL;
}

void draw_keyboard_layout__BlueWMBar(void)
{
	get_current_keyboard_layout__BlueWMBar();
}

void draw_date__BlueWMBar(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1) {
		BLUE_LOG_ERROR("unable to get time of day\n");
	}

	char date[30] = {0};

	strftime(date, 30, "%Y-%m-%d %T", localtime(&tv.tv_sec));

	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, 3, WINDOW_MIDDLE(font), date, strlen(date));
}

void draw__BlueWMBar(void)
{	
	draw_bg__BlueWMBar();
	draw_keyboard_layout__BlueWMBar();
	draw_date__BlueWMBar();
	XCopyArea(display, window_pixels, window, window_gc, 0, 0, window_width, window_height, 0, 0);
	XFlush(display);
}

void set_font__BlueWMBar(void)
{
	if (!(font = XLoadQueryFont(display, "fixed"))) {
		BLUE_LOG_ERROR("unable to load font\n");
	}

	XSetFont(display, window_gc, font->fid);
}

void close__BlueWMBar(void)
{
	XFreeGC(display, window_gc);
	XFreePixmap(display, window_pixels);
	XFreeFont(display, font);
	XCloseDisplay(display);
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes\n");
	}

	window_width = window_root_attr.width;
	window_height = WINDOW_HEIGHT;
	Screen *default_screen = DefaultScreenOfDisplay(display);

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), window_bg_color);
	window_gc = XCreateGC(display, window, 0, NULL);
	window_pixels = XCreatePixmap(display, window, window_width, window_height, DefaultDepthOfScreen(default_screen));

	if (XSetGraphicsExposures(display, window_gc, false) == 0) {
		BLUE_LOG_ERROR("cannot set graphics exposures\n");
	}

	// https://specifications.freedesktop.org/wm/latest/ar01s05.html
	Atom dock_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", false);

	if (XSetWMProtocols(display, window, &dock_atom, 1) == 0) {
		BLUE_LOG_ERROR("cannot set WM_PROTOCOLS\n");
	}

	set_font__BlueWMBar();
	XMapWindow(display, window);
	XFlush(display);

	while (true) {
		draw__BlueWMBar();
		sleep(1);
	}

	close__BlueWMBar();
}
