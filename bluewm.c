#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#define LOG_ERROR(msg, ...) \
	fprintf(stderr, msg, ##__VA_ARGS__); \
	exit(1);

#define BLUE_ZERO_ALLOC(size) ({ \
	void *_mem = calloc(1, size); \
\
	if (!_mem) { \
		LOG_ERROR("unable to allocate memory"); \
	} \
\
	_mem; \
})

struct BlueWMClient {
};

static Display *display = NULL;
static bool is_running = true;

static void handle_events__BlueWM(void);

void handle_events__BlueWM(void)
{
	XEvent event;

	while (is_running) {
		XNextEvent(display, &event);

		switch (event.type) {
			case KeyPress:
				break;
			case KeyRelease:
				break;
			case ButtonPress:
				break;
			default:
				break;
		}
	}
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		LOG_ERROR("unable to open display");
	}

	handle_events__BlueWM();

	XCloseDisplay(display);
}
