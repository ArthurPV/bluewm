#define _GNU_SOURCE

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

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

static Display *display = NULL;
static Window window = {0};
static unsigned int window_bg_color = BLUE_RGB(66, 85, 148);
static unsigned int window_width = 0;
static unsigned int window_height = 0;
static GC window_gc = {0};
static XFontStruct *font = NULL;
static Pixmap window_pixels = {0};
static Window current_window = None;
static char *current_window_title = NULL;
static size_t current_window_title_len = 0;
static Atom active_window_atom = {0};
static Atom wm_name_atom = {0};

static void draw_bg__BlueWMBar(void);

static const char *get_current_keyboard_layout__BlueWMBar(void);

static void draw_keyboard_layout__BlueWMBar(void);

static void draw_date__BlueWMBar(void);

static void draw_window_title__BlueWMBar(void);

static void draw__BlueWMBar(void);

static void set_font__BlueWMBar(void);

static void handle_active_window_notify__BlueWMBar(const XEvent *event);

static void fetch_window_title__BlueWMBar(void);

static inline void handle_wm_name_notify__BlueWMBar(void);

static void handle_events__BlueWMBar(void);

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

void draw_window_title__BlueWMBar(void)
{
	if (!current_window_title) {
		return;
	}

	XDrawString(display, window_pixels, window_gc, (window_width / 2) - current_window_title_len, WINDOW_MIDDLE(font), current_window_title, current_window_title_len);
}

void draw__BlueWMBar(void)
{	
	draw_bg__BlueWMBar();
	draw_keyboard_layout__BlueWMBar();
	draw_date__BlueWMBar();
	draw_window_title__BlueWMBar();
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

void handle_active_window_notify__BlueWMBar(const XEvent *event)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, event->xproperty.window, active_window_atom, 0, 1, false, XA_WINDOW, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_WINDOW && actual_format == 32 && nitems_return == 1 && prop_return) {
		if (current_window != None) {
			// Stop to receive event from this window
			XSelectInput(display, current_window, 0);
		}

		current_window = *(Window *)prop_return;

		if (current_window != None) {
			XSelectInput(display, current_window, PropertyChangeMask | StructureNotifyMask);
		}

		fetch_window_title__BlueWMBar();
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void fetch_window_title__BlueWMBar(void)
{
	if (current_window_title) {
		XFree(current_window_title);
		current_window_title = NULL;
		current_window_title_len = 0;
	}

	if (current_window == None) {
		return;
	}

	XFetchName(display, current_window, &current_window_title);

	if (current_window_title) {
		current_window_title_len = strlen(current_window_title);
	}
}

void handle_wm_name_notify__BlueWMBar(void)
{
	fetch_window_title__BlueWMBar();
}

void handle_events__BlueWMBar(void)
{
	XEvent event;
	time_t start;

	while (true) {
		while (XPending(display) > 0) {
			XNextEvent(display, &event);

			switch (event.type) {
				case Expose:
					draw__BlueWMBar();

					break;
				case PropertyNotify:
					if (event.xproperty.atom == active_window_atom) {
						handle_active_window_notify__BlueWMBar(&event);
					} else if (event.xproperty.atom == wm_name_atom) {
						handle_wm_name_notify__BlueWMBar();
					}

					break;
				case UnmapNotify:
					if (event.xunmap.window == current_window) {
						current_window = None;
					}

					break;
				default:
					break;
			}
		}

		draw__BlueWMBar();
		time(&start);

		time_t current;

		do {
			time(&current);
			usleep(100000);
		} while (difftime(start, current) && XPending(display) == 0);
	}
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

	active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", false);
	wm_name_atom = XInternAtom(display, "_NET_WM_NAME", false);

	set_font__BlueWMBar();
	XSelectInput(display, window, ExposureMask | FocusChangeMask);
	XSelectInput(display, window_root, PropertyChangeMask);
	XMapWindow(display, window);
	handle_events__BlueWMBar();
	close__BlueWMBar();
}
