#include <X11/Xlib.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <bluewm.h>

Display *display = NULL;
Window window = {0};

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display");
	}

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_root_attr.width, 20, 0, XBlackPixel(display, 0), BLUE_RGB(66, 85, 148));

	XMapWindow(display, window);
	XFlush(display);

	while (true);

	XCloseDisplay(display);
}
