#include <X11/Xlib.h>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <bluewm.h>

Display *display = NULL;

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}
}
