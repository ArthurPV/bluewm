#include <X11/Xlib.h>
#include <png.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <bluewm.h>

Display *display = NULL;
Window window = 0;
unsigned int window_width = 0;
unsigned int window_height = 0;

static void close__BlueWMBg(void);

void close__BlueWMBg(void)
{
	XCloseDisplay(display);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		BLUE_LOG_ERROR("expected to have a path");
	}

	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes\n");
	}

	window_width = window_root_attr.width;
	window_height = window_root_attr.height;

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), BLUE_RGB(0, 0, 0));

	XMapWindow(display, window);
	XFlush(display);

	while (true) {
		sleep(1);
	}

	close__BlueWMBg();
}
