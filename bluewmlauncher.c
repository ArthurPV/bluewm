#define _GNU_SOURCE

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bluewm.h>

#define WINDOW_WIDTH 600
// Number of programs displayed at the same time, under the search bar.
#define VISIBLE_ENTRY_COUNT 10
#define ENTRY_PADDING 4
#define ENTRY_HEIGHT(font) ((font)->ascent + (font)->descent + 2 * ENTRY_PADDING)
#define ENTRY_BASELINE(font) ((font)->ascent + ENTRY_PADDING)
// Maximum length of a search, without the terminating nul.
#define SEARCH_MAX_LEN 255

static Display *display = NULL;
static Window window = {0};
static unsigned int window_bg_color = BLUE_RGB(28, 32, 46);
static unsigned int window_width = WINDOW_WIDTH;
static unsigned int window_height = 0;
static GC window_gc = {0};
static XFontStruct *font = NULL;
static Pixmap window_pixels = {0};
// Every executable of the PATH, sorted and without any duplicate.
static char **programs = NULL;
static size_t programs_len = 0;
// Indexes in `programs` of the entries matching `search`.
static size_t *matches = NULL;
static size_t matches_len = 0;
static size_t selected_match = 0;
// First entry of `matches` to draw, to keep the selected entry visible.
static size_t first_visible_match = 0;
static char search[SEARCH_MAX_LEN + 1] = {0};
static size_t search_len = 0;
static bool is_running = true;

static int compare_programs__BlueWMLauncher(const void *lhs, const void *rhs);

static void push_program__BlueWMLauncher(const char *name);

static void fetch_programs_from_dir__BlueWMLauncher(const char *dir_path);

static void fetch_programs__BlueWMLauncher(void);

static void update_matches__BlueWMLauncher(void);

static void draw_bg__BlueWMLauncher(void);

static void draw_search__BlueWMLauncher(void);

static void draw_matches__BlueWMLauncher(void);

static void draw__BlueWMLauncher(void);

static void set_font__BlueWMLauncher(void);

static void select_match__BlueWMLauncher(size_t match);

static void launch_selected_match__BlueWMLauncher(void);

static void handle_key_press_event__BlueWMLauncher(const XEvent *event);

static void handle_focus_out_event__BlueWMLauncher(const XEvent *event);

static void handle_focus_out_event__BlueWMLauncher(const XEvent *event)
{
	// The keyboard grab makes the server report a focus change of its own, and
	// the pointer never gives the focus by itself here.
	if (event->xfocus.mode == NotifyGrab || event->xfocus.mode == NotifyUngrab) {
		return;
	}

	if (event->xfocus.detail == NotifyInferior || event->xfocus.detail == NotifyPointer || event->xfocus.detail == NotifyPointerRoot) {
		return;
	}

	is_running = false;
}

void handle_events__BlueWMLauncher(void);

static void close__BlueWMLauncher(void);

int compare_programs__BlueWMLauncher(const void *lhs, const void *rhs)
{
	return strcmp(*(const char *const *)lhs, *(const char *const *)rhs);
}

void push_program__BlueWMLauncher(const char *name)
{
#define PROGRAMS_CAPACITY_STEP 256

	if (programs_len % PROGRAMS_CAPACITY_STEP == 0) {
		programs = BLUE_F_ALLOC(realloc, programs, (programs_len + PROGRAMS_CAPACITY_STEP) * sizeof(char *));
	}

#undef PROGRAMS_CAPACITY_STEP

	programs[programs_len++] = BLUE_F_ALLOC(strdup, name);
}

void fetch_programs_from_dir__BlueWMLauncher(const char *dir_path)
{
	DIR *dir = opendir(dir_path);

	if (!dir) {
		return;
	}

	const struct dirent *entry = NULL;

	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.') {
			continue;
		}

		// Only an executable file can be launched, and the check is done on the
		// full path, as the search is not done from `dir_path`.
		char path[PATH_MAX] = {0};

		snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

		if (access(path, X_OK) != 0) {
			continue;
		}

		push_program__BlueWMLauncher(entry->d_name);
	}

	closedir(dir);
}

void fetch_programs__BlueWMLauncher(void)
{
	const char *path = getenv("PATH");

	if (!path) {
		BLUE_LOG_WARNING("no PATH in the environment\n");

		return;
	}

	// `strtok` modifies the string it parses, and the environment must not be
	// modified.
	char *paths = BLUE_F_ALLOC(strdup, path);
	const char *dir_path = strtok(paths, ":");

	while (dir_path) {
		fetch_programs_from_dir__BlueWMLauncher(dir_path);
		dir_path = strtok(NULL, ":");
	}

	free(paths);

	if (programs_len == 0) {
		return;
	}

	qsort(programs, programs_len, sizeof(char *), &compare_programs__BlueWMLauncher);

	// The same program is often in more than one directory of the PATH, and
	// only the first one of a sorted run is kept.
	size_t unique_len = 1;

	for (size_t i = 1; i < programs_len; ++i) {
		if (strcmp(programs[i], programs[unique_len - 1]) == 0) {
			free(programs[i]);

			continue;
		}

		programs[unique_len++] = programs[i];
	}

	programs_len = unique_len;
	matches = BLUE_ALLOC(programs_len * sizeof(size_t));
}

void update_matches__BlueWMLauncher(void)
{
	matches_len = 0;

	for (size_t i = 0; i < programs_len; ++i) {
		if (search_len == 0 || strcasestr(programs[i], search)) {
			matches[matches_len++] = i;
		}
	}

	select_match__BlueWMLauncher(0);
}

void draw_bg__BlueWMLauncher(void)
{
	XSetForeground(display, window_gc, window_bg_color);
	XFillRectangle(display, window_pixels, window_gc, 0, 0, window_width, window_height);
}

void draw_search__BlueWMLauncher(void)
{
	XSetForeground(display, window_gc, BLUE_RGB(66, 85, 148));
	XFillRectangle(display, window_pixels, window_gc, 0, 0, window_width, ENTRY_HEIGHT(font));
	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, ENTRY_PADDING, ENTRY_BASELINE(font), search, search_len);

	// The cursor is drawn at the end of the search, as the search is only
	// edited from its end.
	int cursor_x = ENTRY_PADDING + XTextWidth(font, search, search_len);

	XDrawLine(display, window_pixels, window_gc, cursor_x, ENTRY_PADDING, cursor_x, ENTRY_PADDING + font->ascent + font->descent);
}

void draw_matches__BlueWMLauncher(void)
{
	for (size_t i = 0; i < VISIBLE_ENTRY_COUNT; ++i) {
		size_t match = first_visible_match + i;

		if (match >= matches_len) {
			break;
		}

		const char *name = programs[matches[match]];
		int entry_y = (i + 1) * ENTRY_HEIGHT(font);

		if (match == selected_match) {
			XSetForeground(display, window_gc, BLUE_RGB(252, 184, 2));
			XFillRectangle(display, window_pixels, window_gc, 0, entry_y, window_width, ENTRY_HEIGHT(font));
			XSetForeground(display, window_gc, BLUE_RGB(0, 0, 0));
		} else {
			XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
		}

		XDrawString(display, window_pixels, window_gc, ENTRY_PADDING, entry_y + ENTRY_BASELINE(font), name, strlen(name));
	}
}

void draw__BlueWMLauncher(void)
{
	draw_bg__BlueWMLauncher();
	draw_search__BlueWMLauncher();
	draw_matches__BlueWMLauncher();
	XCopyArea(display, window_pixels, window, window_gc, 0, 0, window_width, window_height, 0, 0);
	XFlush(display);
}

void set_font__BlueWMLauncher(void)
{
	if (!(font = XLoadQueryFont(display, "fixed"))) {
		BLUE_LOG_ERROR("unable to load font\n");
	}
}

void select_match__BlueWMLauncher(size_t match)
{
	if (matches_len == 0) {
		selected_match = 0;
		first_visible_match = 0;

		return;
	}

	selected_match = match >= matches_len ? matches_len - 1 : match;

	// The list is scrolled by the minimum needed to keep the selected entry in
	// the visible entries.
	if (selected_match < first_visible_match) {
		first_visible_match = selected_match;
	} else if (selected_match >= first_visible_match + VISIBLE_ENTRY_COUNT) {
		first_visible_match = selected_match - VISIBLE_ENTRY_COUNT + 1;
	}
}

void launch_selected_match__BlueWMLauncher(void)
{
	if (matches_len == 0) {
		return;
	}

	const char *name = programs[matches[selected_match]];
	pid_t pid = fork();

	if (pid == 0) {
		// The launcher exits right after the launch, so the program is detached
		// from it to not be killed with it.
		setsid();
		execlp(name, name, NULL);
		BLUE_LOG_ERROR("unable to launch %s\n", name);
	} else if (pid == -1) {
		BLUE_LOG_WARNING("failed to fork process\n");

		return;
	}

	is_running = false;
}

void handle_key_press_event__BlueWMLauncher(const XEvent *event)
{
#define KEY_BUFFER_LEN 32

	char buffer[KEY_BUFFER_LEN] = {0};
	KeySym sym = NoSymbol;
	int len = XLookupString((XKeyEvent *)&event->xkey, buffer, KEY_BUFFER_LEN, &sym, NULL);

#undef KEY_BUFFER_LEN

	switch (sym) {
		case XK_Escape:
			is_running = false;

			return;
		case XK_Return:
		case XK_KP_Enter:
			launch_selected_match__BlueWMLauncher();

			return;
		case XK_BackSpace:
			if (search_len > 0) {
				search[--search_len] = '\0';
				update_matches__BlueWMLauncher();
			}

			return;
		case XK_Up:
			select_match__BlueWMLauncher(selected_match > 0 ? selected_match - 1 : 0);

			return;
		case XK_Down:
		case XK_Tab:
			select_match__BlueWMLauncher(selected_match + 1);

			return;
		default:
			break;
	}

	// A control character is not part of a search, and only the keys handled
	// above are expected to be pressed with a modifier.
	if (len <= 0 || buffer[0] < ' ' || buffer[0] == 0x7f) {
		return;
	}

	for (int i = 0; i < len && search_len < SEARCH_MAX_LEN; ++i) {
		search[search_len++] = buffer[i];
	}

	search[search_len] = '\0';
	update_matches__BlueWMLauncher();
}

void handle_events__BlueWMLauncher(void)
{
	XEvent event;

	while (is_running) {
		XNextEvent(display, &event);

		switch (event.type) {
			case Expose:
				draw__BlueWMLauncher();

				break;
			case KeyPress:
				handle_key_press_event__BlueWMLauncher(&event);
				draw__BlueWMLauncher();

				break;
			case FocusOut:
				handle_focus_out_event__BlueWMLauncher(&event);

				break;
			default:
				break;
		}
	}
}

void close__BlueWMLauncher(void)
{
	for (size_t i = 0; i < programs_len; ++i) {
		free(programs[i]);
	}

	free(programs);
	free(matches);
	XUngrabKeyboard(display, CurrentTime);
	XFreeGC(display, window_gc);
	XFreePixmap(display, window_pixels);
	XFreeFont(display, font);
	XDestroyWindow(display, window);
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

	// The font metrics give the height of the window, so it is loaded before
	// the creation of the window.
	set_font__BlueWMLauncher();

	window_height = (VISIBLE_ENTRY_COUNT + 1) * ENTRY_HEIGHT(font);

	int window_x = (window_root_attr.width - (int)window_width) / 2;
	int window_y = (window_root_attr.height - (int)window_height) / 4;
	Screen *default_screen = DefaultScreenOfDisplay(display);

	window = XCreateSimpleWindow(display, window_root, window_x, window_y, window_width, window_height, 0, BLUE_RGB(0, 0, 0), window_bg_color);
	window_gc = XCreateGC(display, window, 0, NULL);
	window_pixels = XCreatePixmap(display, window, window_width, window_height, DefaultDepthOfScreen(default_screen));

	if (XSetGraphicsExposures(display, window_gc, false) == 0) {
		BLUE_LOG_ERROR("cannot set graphics exposures\n");
	}

	XSetFont(display, window_gc, font->fid);
	fetch_programs__BlueWMLauncher();
	update_matches__BlueWMLauncher();
	XSelectInput(display, window, ExposureMask | KeyPressMask | StructureNotifyMask | FocusChangeMask);
	XMapRaised(display, window);

	// The window is mapped by the WM, and it can only be grabbed once it is
	// viewable, so its mapping is waited for.
	XEvent map_event;

	do {
		XMaskEvent(display, StructureNotifyMask, &map_event);
	} while (map_event.type != MapNotify);

	// The keys are grabbed to not depend on the focus policy of the WM: the
	// pointer stays free to move over the other windows.
	if (XGrabKeyboard(display, window, true, GrabModeAsync, GrabModeAsync, CurrentTime) != GrabSuccess) {
		BLUE_LOG_WARNING("unable to grab the keyboard\n");
	}

	handle_events__BlueWMLauncher();
	close__BlueWMLauncher();
}
