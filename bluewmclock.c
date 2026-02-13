#include <X11/Xlib.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <bluewm.h>

static Display *display = NULL;
static Window window = 0;
static const unsigned int window_width = 500;
static const unsigned int window_height = 500;

static void handle_events__BlueWMClock(void);

static void close__BlueWMClock(void);

void handle_events__BlueWMClock(void)
{
	XEvent event;

	while (true) {
		XNextEvent(display, &event);

		switch (event.type) {
			case Expose:
				break;
			default:
				break;
		}
	}

}

void close__BlueWMClock(void)
{
	XCloseDisplay(display);
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	Window window_root = XDefaultRootWindow(display);

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), BLUE_RGB(255, 255, 255));

	XSelectInput(display, window, ExposureMask);

	handle_events__BlueWMClock();
	close__BlueWMClock();
}
