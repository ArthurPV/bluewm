#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#include <bluewm.h>

struct BlueWMClient {
	Window window;
	bool is_focused;
};

struct BlueWMScreen {
	int screen_number;
	struct BlueWMScreen *next;
};

static Cursor cursor = {0};
static Display *display = NULL;
static struct BlueWMScreen *screens = NULL;
static bool is_running = true;

static inline void set_cursor__BlueWM(void);

static inline void
unset_cursor__BlueWM(void);

static void
free_screen__BlueWM(struct BlueWMScreen *screen);

static inline void
set_screens__BlueWM(void);

static inline void
unset_screens__BlueWM(void);

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

void set_cursor__BlueWM(void)
{
	cursor = XCreateFontCursor(display, XC_arrow);

	XDefineCursor(display, DefaultRootWindow(display), cursor);
}

void
unset_cursor__BlueWM(void)
{
	XUndefineCursor(display, DefaultRootWindow(display));
	XFreeCursor(display, cursor);
}

void
free_screen__BlueWM(struct BlueWMScreen *screen)
{
	free(screen);
}

void
set_screens__BlueWM(void)
{
	for (int screen_number = 0; screen_number < ScreenCount(display); ++screen_number) {	
		struct BlueWMScreen *bscreen = BLUE_ZERO_ALLOC(sizeof(struct BlueWMScreen));

		bscreen->screen_number = screen_number;
		bscreen->next = screens;

		XSelectInput(display, XRootWindow(display, screen_number), KeyPress | KeyRelease | ButtonPress);

		screens = bscreen;
	}
}

void
unset_screens__BlueWM(void)
{
	struct BlueWMScreen *current = screens;

	while (current) {
		struct BlueWMScreen *previous = current;

		current = current->next;

		free_screen__BlueWM(previous);
	}
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	// Launch or status bar
	execl("/bin/sh", "/bin/sh", "-c", "./bluewmbar", NULL);

	set_screens__BlueWM();
	set_cursor__BlueWM();
	handle_events__BlueWM();
	unset_screens__BlueWM();
	unset_cursor__BlueWM();

	XCloseDisplay(display);
}
