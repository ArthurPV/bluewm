#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>

#include <bluewm.h>
#include <config/bluewm.h>

#define BLUE_WM_CLIENT_STATE_FOCUSED 1 << 0
#define BLUE_WM_CLIENT_STATE_FULLSCREEN 1 << 1

enum BlueWMClientRole {
	BLUE_WM_CLIENT_ROLE_NONE,
	BLUE_WM_CLIENT_ROLE_DOCK,
	BLUE_WM_CLIENT_ROLE_SPLASH,
};

struct BlueWMClient {
	enum BlueWMClientRole role;
	int client_state_mask;
	Window window;
	struct BlueWMClient *next;
};

struct BlueWMScreen {
	long screen_number;
	struct BlueWMScreen *next;
	// Visible workspace
	int workspace;
	struct BlueWMClient *dock;
	struct BlueWMClient *splash;
};

enum BlueWMLayoutKind {
	BLUE_WM_LAYOUT_KIND_TAB,
	BLUE_WM_LAYOUT_KIND_HORIZONTAL_SPLIT,
	BLUE_WM_LAYOUT_KIND_VERTICAL_SPLIT
};

struct BlueWMLayout {
	enum BlueWMLayoutKind kind;
};

#define BLUE_WM_WORKSPACE_NUMBER 10

struct BlueWMWorkspace {
	struct BlueWMClient *clients;
	struct BlueWMClient *active;
};

// https://specifications.freedesktop.org/wm/latest/ar01s05.html
enum BlueWMAtom {
	BLUE_WM_ATOM_DOCK,
	BLUE_WM_ATOM_SPLASH,
	BLUE_WM_ATOM_ACTIVE_WINDOW,

	BLUE_WM_ATOM_MAX
};

static void
launch_builtin_program__BlueWM(const char *cmd, ...);

static void set_atoms__BlueWM(void);

static inline void set_cursor__BlueWM(void);

static inline void
unset_cursor__BlueWM(void);

static void
free_screen__BlueWM(struct BlueWMScreen *screen);

static inline void
set_screens__BlueWM(void);

static inline void
unset_screens__BlueWM(void);

static struct BlueWMClient *
init_client__BlueWM(enum BlueWMClientRole role, Window window);

static void
deinit_client__BlueWM(struct BlueWMClient *client);

static void
launch_startup_program__BlueWM(void);

static int
find_atom__BlueWM(const Atom *atom);

static struct BlueWMScreen *
find_screen__BlueWM(long screen_number);

static struct BlueWMScreen *
get_screen_from_window__BlueWM(Window window);

static enum BlueWMClientRole
get_role_of_atom__BlueWM(const Atom *atom);

static void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_none__BlueWM(Window window, struct BlueWMScreen *screen);

static void new_client__BlueWM(Window window, enum BlueWMClientRole role);

static void handle_key_press_event__BlueWM(const XEvent *event);

static void handle_key_release_event__BlueWM(const XEvent *event);

static void handle_button_press_event__BlueWM(const XEvent *event);

static void handle_map_notify_event__BlueWM(const XEvent *event);

static void update_active_window_from_workspaces__BlueWM(const struct BlueWMScreen *screen, Window window);

static void remove_client__BlueWM(const struct BlueWMScreen *screen, Window window);

static void handle_unmap_notify_event__BlueWM(const XEvent *event);

static void handle_events__BlueWM(void);

static void (*const handle_event_functions[])(const XEvent *) = {
	[KeyPress] = &handle_key_press_event__BlueWM,
	[KeyRelease] = &handle_key_release_event__BlueWM,
	[ButtonPress] = &handle_button_press_event__BlueWM,
	[MapNotify] = &handle_map_notify_event__BlueWM, 
	[UnmapNotify] = &handle_unmap_notify_event__BlueWM
};
static Atom atoms[BLUE_WM_ATOM_MAX] = {0};
static struct BlueWMWorkspace workspaces[BLUE_WM_WORKSPACE_NUMBER] = {0};
static Cursor cursor = {0};
static Display *display = NULL;
static struct BlueWMScreen *screens = NULL;
static bool is_running = true;

void
launch_builtin_program__BlueWM(const char *cmd, ...)
{
	pid_t pid = fork();

	if (pid == 0) {
		va_list vl;
#define ARGV_LEN 32
		char const *argv[ARGV_LEN] = {0};

		argv[0] = cmd;

		va_start(vl, cmd);

		for (int i = 0; i < ARGV_LEN; ++i) {
			const char *current = va_arg(vl, const char*);

			argv[i + 1] = current;

			if (!current) {
				break;
			}
		}

		va_end(vl);
		execvp(cmd, (char *const*)argv);

#undef ARGV_LEN
	} else if (pid == -1) {
		BLUE_LOG_WARNING("failed to fork process");
	}
}

static void set_atoms__BlueWM(void)
{
	static const char *atom_names[BLUE_WM_ATOM_MAX] = {
		"_NET_WM_WINDOW_TYPE_DOCK",
		"_NET_WM_WINDOW_TYPE_SPLASH",
		"_NET_ACTIVE_WINDOW"
	};

	for (enum BlueWMAtom atom = 0; atom < BLUE_WM_ATOM_MAX; ++atom) {
		atoms[atom] = XInternAtom(display, atom_names[atom], false);
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
	int screen_count = ScreenCount(display);

	for (int screen_number = 0; screen_number < screen_count; ++screen_number) {	
		struct BlueWMScreen *bscreen = BLUE_ZERO_ALLOC(sizeof(struct BlueWMScreen));

		bscreen->screen_number = screen_number;
		bscreen->next = screens;
		// In case we have more screen than workspace
		bscreen->workspace = screen_number % BLUE_WM_WORKSPACE_NUMBER;

		XSelectInput(display, XRootWindow(display, screen_number),
				KeyPressMask | KeyReleaseMask | ButtonPressMask |
				SubstructureNotifyMask);

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

struct BlueWMClient *
init_client__BlueWM(enum BlueWMClientRole role, Window window)
{
	struct BlueWMClient *client = BLUE_ZERO_ALLOC(sizeof(struct BlueWMClient));

	client->role = role;
	client->window = window;

	return client;
}

void
deinit_client__BlueWM(struct BlueWMClient *client)
{
	free(client);
}

void
launch_startup_program__BlueWM(void)
{
	// Launch our status bar
	launch_builtin_program__BlueWM("./bluewmbar", NULL);

	// Launch our background
	launch_builtin_program__BlueWM("./bluewmbg", BLUE_CONFIG_BG_PATH, NULL);
}

int
find_atom__BlueWM(const Atom *atom)
{
	for (int i = 0; i < BLUE_WM_ATOM_MAX; ++i) {
		if (atoms[i] == *atom) {
			return i;
		}
	}

	return -1;
}

struct BlueWMScreen *
find_screen__BlueWM(long screen_number)
{
	struct BlueWMScreen *current = screens;

	while (current) {
		if (current->screen_number == screen_number) {
			return current;
		}

		current = current->next;
	}

	BLUE_LOG_UNREACHABLE("unable to find screen\n");
}

struct BlueWMScreen *
get_screen_from_window__BlueWM(Window window)
{
	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, window, &window_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	long screen_number = XScreenNumberOfScreen(window_attr.screen);

	return find_screen__BlueWM(screen_number);
}

enum BlueWMClientRole
get_role_of_atom__BlueWM(const Atom *atom)
{
	static enum BlueWMClientRole roles[BLUE_WM_ATOM_MAX] = {
		[BLUE_WM_ATOM_DOCK] = BLUE_WM_CLIENT_ROLE_DOCK,
		[BLUE_WM_ATOM_SPLASH] = BLUE_WM_CLIENT_ROLE_SPLASH
	};
	int atom_index = find_atom__BlueWM(atom);

	if (atom_index == -1) {
		return BLUE_WM_CLIENT_ROLE_NONE;
	}

	return roles[atom_index];
}

void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen)
{
	screen->splash = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_SPLASH, window);
	XConfigureWindow(display, window, CWStackMode, &(XWindowChanges){ .stack_mode = Below });
}

void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen)
{
	screen->dock = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_DOCK, window);
	XConfigureWindow(display, window, CWStackMode, &(XWindowChanges){ .stack_mode = Above });
}

void handle_client_role_none__BlueWM(Window window, struct BlueWMScreen *screen)
{
	XSetInputFocus(display, window, RevertToParent, CurrentTime);
	XSelectInput(display, window, KeyPressMask);

	struct BlueWMClient *client = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_NONE, window);
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (workspace->clients) {
		workspace->clients->next = client;
	} else {
		workspace->clients = client;
	}

	XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_ACTIVE_WINDOW], XA_WINDOW, 32, PropModeReplace, (unsigned char*)&window, 1);
	workspace->active = client;
}

void new_client__BlueWM(Window window, enum BlueWMClientRole role)
{
	struct BlueWMScreen *screen = get_screen_from_window__BlueWM(window);

	switch (role) {
		case BLUE_WM_CLIENT_ROLE_SPLASH:
			handle_client_role_splash__BlueWM(window, screen);

			break;
		case BLUE_WM_CLIENT_ROLE_DOCK:
			handle_client_role_dock__BlueWM(window, screen);

			break;
		case BLUE_WM_CLIENT_ROLE_NONE:
			handle_client_role_none__BlueWM(window, screen);

			break;
		default:
			BLUE_LOG_UNREACHABLE("unknown role\n");
	}
}

void handle_key_press_event__BlueWM(const XEvent *event)
{
	KeySym sym = XLookupKeysym((XKeyEvent*)&event->xkey, 0);

	if (event->xkey.state & Mod4Mask && sym == XK_Return) {
		launch_builtin_program__BlueWM("xterm", NULL);
	}
}

void handle_key_release_event__BlueWM(const XEvent *event)
{
}

void handle_button_press_event__BlueWM(const XEvent *event)
{
}

void handle_map_notify_event__BlueWM(const XEvent *event)
{
	Window window = event->xmap.window;
	Atom *window_atoms = NULL;
	int window_atoms_count = 0;
	enum BlueWMClientRole window_role = BLUE_WM_CLIENT_ROLE_NONE;

	XGetWMProtocols(display, window, &window_atoms, &window_atoms_count);

	for (int i = 0; i < window_atoms_count; ++i) {
		window_role = get_role_of_atom__BlueWM(&window_atoms[i]);
	}

	new_client__BlueWM(window, window_role);

	if (window_atoms) {
		XFree(window_atoms);
	}
}

void update_active_window_from_workspaces__BlueWM(const struct BlueWMScreen *screen, Window window)
{
	for (size_t i = 0; i < BLUE_WM_WORKSPACE_NUMBER; ++i) {
		struct BlueWMWorkspace *workspace = &workspaces[i];

		if (workspace->active && workspace->active->window == window) {
			Window new_window = None;

			XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_ACTIVE_WINDOW], XA_WINDOW, 32, PropModeReplace, (unsigned char*)&new_window, 1);
			workspace->active = NULL;

			break;
		}
	}
}

void remove_client__BlueWM(const struct BlueWMScreen *screen, Window window)
{
	const struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	struct BlueWMClient *current = workspace->clients;
	struct BlueWMClient *previous = NULL;

	while (current) {
		if (current->window == window) {
			if (previous) {
				previous->next = current->next;
			}

			deinit_client__BlueWM(current);

			break;
		}

		previous = current;
		current = current->next;
	}
}

void handle_unmap_notify_event__BlueWM(const XEvent *event)
{	
	Window event_window = event->xunmap.event;
	Window unmapped_window = event->xunmap.window;
	// NOTE: In this case, we cannot use unmapped_window as it's
	// already get unmapped
	struct BlueWMScreen *screen = get_screen_from_window__BlueWM(event_window);

	update_active_window_from_workspaces__BlueWM(screen, unmapped_window);
	remove_client__BlueWM(screen, unmapped_window);
}

void handle_events__BlueWM(void)
{
	XEvent event;

	while (is_running) {
		XNextEvent(display, &event);

		switch (event.type) {
			case KeyPress:
			case KeyRelease:
			case ButtonPress:
			case MapNotify:
			case UnmapNotify:
				handle_event_functions[event.type](&event);

				break;
			default:
				break;
		}
	}
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	set_atoms__BlueWM();
	set_screens__BlueWM();
	set_cursor__BlueWM();
	launch_startup_program__BlueWM();
	handle_events__BlueWM();
	unset_screens__BlueWM();
	unset_cursor__BlueWM();

	XCloseDisplay(display);
}
