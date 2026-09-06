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
static Atom resize_window_atom = {0};
static Atom active_workspace_atom = {0};
static bool resizing_window = false;
static int active_workspace = -1;

static void draw_bg__BlueWMBar(void);

static const char *get_current_keyboard_layout__BlueWMBar(void);

static void draw_keyboard_layout__BlueWMBar(void);

static void draw_date__BlueWMBar(void);

static void draw_window_title__BlueWMBar(void);

static void draw_workspaces_number__BlueWMBar(void);

static void draw__BlueWMBar(void);

static void set_font__BlueWMBar(void);

static void handle_active_window_notify__BlueWMBar(const XEvent *event);

static void fetch_window_title__BlueWMBar(void);

static inline void handle_wm_name_notify__BlueWMBar(void);

static void handle_resize_window_notify__BlueWMBar(const XEvent *event);

static void fetch_active_workspace__BlueWMBar(Window window);

static void handle_active_workspace_notify__BlueWMBar(const XEvent *event);

static void handle_events__BlueWMBar(void);

static void close__BlueWMBar(void);

static int handle_error__BlueWMBar(Display *error_display, XErrorEvent *error);

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

	strftime(date, sizeof(date) - 1, "%Y-%m-%d %T", localtime(&tv.tv_sec));

	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, 3, WINDOW_MIDDLE(font), date, strlen(date));
}

void draw_window_title__BlueWMBar(void)
{
	if (!current_window_title) {
		return;
	}

	if (resizing_window) {
		XSetForeground(display, window_gc, BLUE_RGB(255, 0, 0));
	} else {
		XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	}

	// The title is centered on its width in pixels, not on its number of
	// characters.
	int title_width = XTextWidth(font, current_window_title, current_window_title_len);

	XDrawString(display, window_pixels, window_gc, (window_width - title_width) / 2, WINDOW_MIDDLE(font), current_window_title, current_window_title_len);
}

void draw_workspaces_number__BlueWMBar(void)
{
	for (int i = 0; i < BLUE_WM_WORKSPACE_NUMBER; ++i) {
		// The workspaces are laid out as on the keyboard row: 1, 2, ..., 9, 0.
		int workspace_number = (i + 1) % BLUE_WM_WORKSPACE_NUMBER;
		char number[10] = {0};

		snprintf(number, sizeof(number) - 1, "%d", workspace_number);

		if (active_workspace == workspace_number) {
			XSetForeground(display, window_gc, BLUE_RGB(252, 184, 2));
		} else {
			XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
		}

#define SPACE_FACTOR 2

		XDrawString(display, window_pixels, window_gc, window_width - SPACE_FACTOR * font->ascent * (BLUE_WM_WORKSPACE_NUMBER - i), WINDOW_MIDDLE(font), number, strlen(number));

#undef SPACE_FACTOR
	}
}

void draw__BlueWMBar(void)
{	
	draw_bg__BlueWMBar();
	draw_keyboard_layout__BlueWMBar();
	draw_date__BlueWMBar();
	draw_window_title__BlueWMBar();
	draw_workspaces_number__BlueWMBar();
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

void handle_resize_window_notify__BlueWMBar(const XEvent *event)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, event->xproperty.window, resize_window_atom, 0, 1, false, XA_WINDOW, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_WINDOW && actual_format == 32 && nitems_return == 1 && prop_return) {
		Window resized_window = *(Window*)prop_return;

		if (resized_window == None) {
			resizing_window = false;
		} else {
			resizing_window = true;
		}
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void fetch_active_workspace__BlueWMBar(Window window)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, window, active_workspace_atom, 0, 1, false, XA_INTEGER, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_INTEGER && actual_format == 32 && nitems_return == 1 && prop_return) {
		active_workspace = *(int*)prop_return;
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void handle_active_workspace_notify__BlueWMBar(const XEvent *event)
{
	fetch_active_workspace__BlueWMBar(event->xproperty.window);
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
					} else if (event.xproperty.atom == resize_window_atom) {
						handle_resize_window_notify__BlueWMBar(&event);
					} else if (event.xproperty.atom == active_workspace_atom) {
						handle_active_workspace_notify__BlueWMBar(&event);
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

int handle_error__BlueWMBar(Display *error_display, XErrorEvent *error)
{
	// A window can be destroyed between the moment the WM notifies it and the
	// moment it is used here, and the bar must not die with it.
	if (error->error_code == BadWindow) {
		return 0;
	}

	char message[256] = {0};

	XGetErrorText(error_display, error->error_code, message, sizeof(message));
	BLUE_LOG_WARNING("%s\n", message);

	return 0;
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

	XSetErrorHandler(&handle_error__BlueWMBar);

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
	Atom window_type_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
	Atom dock_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", false);

	// The role of a window is advertised through _NET_WM_WINDOW_TYPE, and not
	// through WM_PROTOCOLS, which only carries the protocols the client
	// understands.
	XChangeProperty(display, window, window_type_atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_atom, 1);

	active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", false);
	wm_name_atom = XInternAtom(display, "_NET_WM_NAME", false);
	resize_window_atom = XInternAtom(display, "_NET_WM_ACTION_RESIZE", false);
	active_workspace_atom = XInternAtom(display, "_BLUE_WM_ACTIVE_WORKSPACE", false);

	fetch_active_workspace__BlueWMBar(window_root);

	set_font__BlueWMBar();
	XSelectInput(display, window, ExposureMask | FocusChangeMask);
	XSelectInput(display, window_root, PropertyChangeMask);
	XMapWindow(display, window);
	handle_events__BlueWMBar();
	close__BlueWMBar();
}
