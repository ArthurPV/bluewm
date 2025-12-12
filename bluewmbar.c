#include <X11/Xlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <bluewm.h>

#define WINDOW_HEIGHT 20
#define WINDOW_MIDDLE(font) ((WINDOW_HEIGHT / 2) + (font->ascent - (font->ascent + font->descent) / 2))

Display *display = NULL;
Window window = {0};
unsigned int window_bg_color = BLUE_RGB(66, 85, 148);
unsigned int window_width = 0;
unsigned int window_height = 0;
GC window_gc = {0};
XFontStruct *font = NULL;
Pixmap window_pixels = {0};

static void draw_bg__BlueWMBar(void);

static void draw_date__BlueWMBar(void);

static void draw__BlueWMBar(void);

static void set_font__BlueWMBar(void);

void draw_bg__BlueWMBar(void)
{
	XSetForeground(display, window_gc, window_bg_color);
	XFillRectangle(display, window_pixels, window_gc, 0, 0, window_width, window_height);
}

void draw_date__BlueWMBar(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1) {
		BLUE_LOG_ERROR("unable to get time of day");
	}

	char date[30] = {0};

	strftime(date, 30, "%Y-%m-%d %T", localtime(&tv.tv_sec));

	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, 3, WINDOW_MIDDLE(font), date, strlen(date));
}

void draw__BlueWMBar(void)
{	
	draw_bg__BlueWMBar();
	draw_date__BlueWMBar();
	XCopyArea(display, window_pixels, window, window_gc, 0, 0, window_width, window_height, 0, 0);
	XFlush(display);
}

void set_font__BlueWMBar(void)
{
	if (!(font = XLoadQueryFont(display, "fixed"))) {
		BLUE_LOG_ERROR("unable to load font");
	}

	XSetFont(display, window_gc, font->fid);
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display");
	}

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	window_width = window_root_attr.width;
	window_height = WINDOW_HEIGHT;
	Screen *default_screen = DefaultScreenOfDisplay(display);

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), window_bg_color);
	window_gc = XCreateGC(display, window, 0, NULL);
	window_pixels = XCreatePixmap(display, window, window_width, window_height, DefaultDepthOfScreen(default_screen));

	if (XSetGraphicsExposures(display, window_gc, false) == 0) {
		BLUE_LOG_ERROR("cannot set graphics exposures");
	}

	set_font__BlueWMBar();

	XMapWindow(display, window);
	XFlush(display);

	while (true) {
		draw__BlueWMBar();
		sleep(1);
	}

	XCloseDisplay(display);
}
