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
	int saved_width;
	int saved_height;
	bool is_fullscreen;
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

struct BlueWMWorkspace {
	struct BlueWMClient *clients;
	struct BlueWMClient *active;
};

// https://specifications.freedesktop.org/wm/latest/ar01s05.html
enum BlueWMAtom {
	BLUE_WM_ATOM_DOCK,
	BLUE_WM_ATOM_SPLASH,
	BLUE_WM_ATOM_ACTIVE_WINDOW,
	BLUE_WM_ATOM_RESIZE_WINDOW,
	BLUE_WM_ATOM_ACTIVE_WORKSPACE, // custom

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

static struct BlueWMScreen *
find_screen_from_root_window__BlueWM(Window root);

static enum BlueWMClientRole
get_role_of_atom__BlueWM(const Atom *atom);

static void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_none__BlueWM(Window window, struct BlueWMScreen *screen);

static void new_client__BlueWM(Window window, enum BlueWMClientRole role);

static void launch_terminal__BlueWM(struct BlueWMScreen *screen);

static void toggle_resize_window__BlueWM(struct BlueWMScreen *screen);

static void resize_window_left__BlueWM(struct BlueWMScreen *screen);

static void resize_window_right__BlueWM(struct BlueWMScreen *screen);

static void resize_window_up__BlueWM(struct BlueWMScreen *screen);

static void resize_window_down__BlueWM(struct BlueWMScreen *screen);

static void toggle_full_screen_window__BlueWM(struct BlueWMScreen *screen);

static void unmap_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen);

static void map_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen);

static void notify_active_workspace__BlueWM(const struct BlueWMScreen *screen);

static void toggle_workspace_n__BlueWM(struct BlueWMScreen *screen, int workspace);

static inline void toggle_workspace_1__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_2__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_3__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_4__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_5__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_6__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_7__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_8__BlueWM(struct BlueWMScreen *screen);

static inline void toggle_workspace_9__BlueWM(struct BlueWMScreen *screen);

static void toggle_workspace_0__BlueWM(struct BlueWMScreen *screen);

static void update_key_state_mask__BlueWM(const XEvent *event);

static void handle_key_press_event__BlueWM(const XEvent *event);

static void handle_key_release_event__BlueWM(const XEvent *event);

static void update_button_state_mask__BlueWM(const XEvent *event);

static void handle_button_press_event__BlueWM(const XEvent *event);

static void handle_button_release_event__BlueWM(const XEvent *event);

static void handle_motion_notify_event__BlueWM(const XEvent *event);

static void handle_map_notify_event__BlueWM(const XEvent *event);

static void update_active_window_from_workspaces__BlueWM(const struct BlueWMScreen *screen, Window window);

static void remove_client__BlueWM(const struct BlueWMScreen *screen, Window window);

static void handle_unmap_notify_event__BlueWM(const XEvent *event);

static bool window_is_on_dock__BlueWM(const struct BlueWMScreen *screen, const XWindowAttributes *window_attr, int *new_x, int *new_y);

static void handle_map_request_event__BlueWM(const XEvent *event);

static void handle_configure_request_event__BlueWM(const XEvent *event);

static void handle_events__BlueWM(void);

#include <config/bluewm.h>

static void (*const handle_event_functions[])(const XEvent *) = {
	[KeyPress] = &handle_key_press_event__BlueWM,
	[KeyRelease] = &handle_key_release_event__BlueWM,
	[ButtonPress] = &handle_button_press_event__BlueWM,
	[ButtonRelease] = &handle_button_release_event__BlueWM,
	[MotionNotify] = &handle_motion_notify_event__BlueWM,
	[MapNotify] = &handle_map_notify_event__BlueWM, 
	[UnmapNotify] = &handle_unmap_notify_event__BlueWM,
	[MapRequest] = &handle_map_request_event__BlueWM,
	[ConfigureRequest] = &handle_configure_request_event__BlueWM
};
static Atom atoms[BLUE_WM_ATOM_MAX] = {0};
static struct BlueWMWorkspace workspaces[BLUE_WM_WORKSPACE_NUMBER] = {0};
static Cursor cursor = {0};
static Display *display = NULL;
static struct BlueWMScreen *screens = NULL;
static bool is_running = true;
static Window window_to_resize = None;
static int key_state_mask = 0;
static int button_state_mask = 0;

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
	// See enum BlueWMAtom definition
	static const char *atom_names[BLUE_WM_ATOM_MAX] = {
		"_NET_WM_WINDOW_TYPE_DOCK",
		"_NET_WM_WINDOW_TYPE_SPLASH",
		"_NET_ACTIVE_WINDOW",
		"_NET_WM_ACTION_RESIZE",
		"_BLUE_WM_ACTIVE_WORKSPACE",
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
	// NOTE: We initialize at worst 10 screens
	int screen_count = ScreenCount(display) % BLUE_WM_WORKSPACE_NUMBER;

	for (int screen_number = 0, workspace = 1; screen_number < screen_count; ++screen_number, ++workspace) {
		struct BlueWMScreen *bscreen = BLUE_ZERO_ALLOC(sizeof(struct BlueWMScreen));

		bscreen->screen_number = screen_number;
		bscreen->next = screens;
		bscreen->workspace = workspace % BLUE_WM_WORKSPACE_NUMBER;

		Window window_root = XRootWindow(display, screen_number);

		XSelectInput(display, window_root,
				KeyPressMask | KeyReleaseMask | ButtonPressMask |
				ButtonReleaseMask | PointerMotionMask |
				SubstructureNotifyMask | SubstructureRedirectMask);

		notify_active_workspace__BlueWM(bscreen);

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
	launch_builtin_program__BlueWM("./bluewmbg", BLUE_WM_CONFIG_BG_PATH, NULL);
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

struct BlueWMScreen *
find_screen_from_root_window__BlueWM(Window root)
{
	struct BlueWMScreen *current = screens;

	while (current) {
		if (XRootWindow(display, current->screen_number) == root) {
			return current;
		}

		current = current->next;
	}

	BLUE_LOG_UNREACHABLE("unable to find screen\n");
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
	XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);

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

void launch_terminal__BlueWM(struct BlueWMScreen *screen)
{
	launch_builtin_program__BlueWM("xterm", NULL);
}

void toggle_resize_window__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (!workspace->active) {
		goto update_property;
	} else if (window_to_resize != None) {
		XWindowAttributes attr;

		if (XGetWindowAttributes(display, window_to_resize, &attr) == 0) {
			BLUE_LOG_ERROR("unable to get window attributes");
		}

		if (attr.map_state == IsUnmapped) {
			goto out;
		}

		window_to_resize = None;

		goto update_property;
	}

out:
	window_to_resize = workspace->active->window;

update_property:
	XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_RESIZE_WINDOW], XA_WINDOW, 32, PropModeReplace, (unsigned char*)&window_to_resize, 1);
}

#define RESIZE_WINDOW_MOTION(width_change, height_change) \
	if (window_to_resize == None) { \
		return; \
	} \
\
	XWindowAttributes window_attr; \
\
	if (XGetWindowAttributes(display, window_to_resize, &window_attr) == 0) { \
		BLUE_LOG_ERROR("unable to get window attributes"); \
	} \
\
	XResizeWindow(display, window_to_resize, window_attr.width width_change, window_attr.height height_change);

void resize_window_left__BlueWM(struct BlueWMScreen *screen)
{
	RESIZE_WINDOW_MOTION(+10, +0);
}

void resize_window_right__BlueWM(struct BlueWMScreen *screen)
{
	RESIZE_WINDOW_MOTION(-10, +0);
}

void resize_window_up__BlueWM(struct BlueWMScreen *screen)
{
	RESIZE_WINDOW_MOTION(+0, +10);
}

void resize_window_down__BlueWM(struct BlueWMScreen *screen)
{
	RESIZE_WINDOW_MOTION(+0, -10);
}

#undef RESIZE_WINDOW_MOTION

void toggle_full_screen_window__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (!workspace->active) {
		return;
	}

	Window active_window = workspace->active->window;

	if (workspace->active->is_fullscreen) {
		XResizeWindow(display, active_window, workspace->active->saved_width, workspace->active->saved_height);

		workspace->active->is_fullscreen = false;
		workspace->active->saved_width = 0;
		workspace->active->saved_height = 0;

		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, active_window, &window_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	int screen_width = DisplayWidth(display, screen->screen_number);
	int screen_height = DisplayHeight(display, screen->screen_number);

	XResizeWindow(display, active_window, screen_width, screen_height);

	workspace->active->is_fullscreen = true;
	workspace->active->saved_width = window_attr.width;
	workspace->active->saved_height = window_attr.height;
}

void unmap_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		XUnmapWindow(display, current->window);
		current = current->next;
	}
}

void map_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		XMapWindow(display, current->window);
		current = current->next;
	}
}

static void notify_active_workspace__BlueWM(const struct BlueWMScreen *screen)
{
	XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_ACTIVE_WORKSPACE], XA_INTEGER, 32, PropModeReplace, (unsigned char*)&screen->workspace, 1);
}

void toggle_workspace_n__BlueWM(struct BlueWMScreen *screen, int workspace)
{
	if (screen->workspace == workspace) {
		return;
	}

	unmap_all_windows_from_current_workspace__BlueWM(screen);
	screen->workspace = workspace;
	map_all_windows_from_current_workspace__BlueWM(screen);

	// Notify the change of the workspace
	notify_active_workspace__BlueWM(screen);
}

#define TOGGLE_WORKSPACE_N(n) \
void toggle_workspace_##n##__BlueWM(struct BlueWMScreen *screen) { \
	toggle_workspace_n__BlueWM(screen, n); \
}

TOGGLE_WORKSPACE_N(1)
TOGGLE_WORKSPACE_N(2)
TOGGLE_WORKSPACE_N(3)
TOGGLE_WORKSPACE_N(4)
TOGGLE_WORKSPACE_N(5)
TOGGLE_WORKSPACE_N(6)
TOGGLE_WORKSPACE_N(7)
TOGGLE_WORKSPACE_N(8)
TOGGLE_WORKSPACE_N(9)
TOGGLE_WORKSPACE_N(0)

#undef TOGGLE_WORKSPACE_N

static void update_key_state_mask__BlueWM(const XEvent *event)
{
	unsigned int mod4 = event->xkey.state & Mod4Mask ? Mod4Mask : None;
	unsigned int shift = event->xkey.state & ShiftMask ? ShiftMask : None;
	unsigned int control = event->xkey.state & ControlMask ? ControlMask : None;

	key_state_mask = mod4 | shift | control;
}

void handle_key_press_event__BlueWM(const XEvent *event)
{
	KeySym sym = XLookupKeysym((XKeyEvent*)&event->xkey, 0);

	update_key_state_mask__BlueWM(event);

	for (size_t i = 0; i < shortcuts_len; ++i) {
		const struct BlueWMShortcut *shortcut = &shortcuts[i];

		if (key_state_mask == shortcut->state && shortcut->sym == sym) {
			assert(shortcut->handler && "Expected to have an handler");

			struct BlueWMScreen *screen = get_screen_from_window__BlueWM(event->xkey.window);

			shortcut->handler(screen);
		}
	}
}

void handle_key_release_event__BlueWM(const XEvent *event)
{
	update_key_state_mask__BlueWM(event);
}

void update_button_state_mask__BlueWM(const XEvent *event)
{
	unsigned int button1 = event->xbutton.state & Button1Mask ? Button1Mask : None;
	unsigned int button2 = event->xbutton.state & Button2Mask ? Button2Mask : None;

	button_state_mask = button1 | button2;
}

void handle_button_press_event__BlueWM(const XEvent *event)
{
	update_button_state_mask__BlueWM(event);
}

void handle_button_release_event__BlueWM(const XEvent *event)
{
	update_button_state_mask__BlueWM(event);
}

void handle_motion_notify_event__BlueWM(const XEvent *event)
{
	if (key_state_mask & Mod4Mask && button_state_mask & Button1Mask) {
		XMoveWindow(display, event->xmotion.window, event->xmotion.x_root, event->xmotion.y_root);
	} else {
		Window window_root = event->xmotion.root;
		struct BlueWMScreen *screen = find_screen_from_root_window__BlueWM(window_root);
		const struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
		Window new_focused_window = event->xmotion.window;

		if (!workspace->active || workspace->active->window != new_focused_window) {
			update_active_window_from_workspaces__BlueWM(screen, new_focused_window);
			XSetInputFocus(display, new_focused_window, RevertToPointerRoot, CurrentTime);
		}
	}
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
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	struct BlueWMClient *current = workspace->clients;
	struct BlueWMClient *previous = NULL;

	while (current) {
		if (current->window == window) {
			if (previous) {
				previous->next = current->next;
			} else {
				workspace->clients = current->next;
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

bool window_is_on_dock__BlueWM(const struct BlueWMScreen *screen, const XWindowAttributes *window_attr, int *new_x, int *new_y)
{
	*new_x = window_attr->x;
	*new_y = window_attr->y;

	if (!screen->dock) {
		return false;
	}

	XWindowAttributes dock_attr;

	if (XGetWindowAttributes(display, screen->dock->window, &dock_attr) == 0) {
		BLUE_LOG_ERROR("unable to get dock attributes\n");
	}

	int window_y = window_attr->y;
	int window_y2 = window_attr->y + window_attr->height;
	int dock_y = dock_attr.y;
	int dock_y2 = dock_attr.y + dock_attr.height;

	if (dock_y2 > window_y) {
		*new_y += dock_y2 - window_y;

		return true;
	} else if (window_y2 > dock_y) {
		*new_y -= window_y2 - dock_y;

		return true;
	}

	int window_x = window_attr->x;
	int window_x2 = window_attr->x + window_attr->width;
	int dock_x = dock_attr.x;
	int dock_x2 = dock_attr.x + dock_attr.width;

	if (dock_x2 > window_x) {
		*new_x += dock_x2 - window_x;

		return true;
	} else if (window_x2 > dock_x) {
		*new_x -= window_x2 - dock_x;
	}

	return false;
}

void handle_map_request_event__BlueWM(const XEvent *event)
{
	Window window = event->xmaprequest.window;
	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, window, &window_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes\n");
	}

	struct BlueWMScreen *screen = find_screen__BlueWM(XScreenNumberOfScreen(window_attr.screen));
	int new_x = 0;
	int new_y = 0;

	if (window_is_on_dock__BlueWM(screen, &window_attr, &new_x, &new_y)) {
		XMoveWindow(display, window, new_x, new_y);
	}

	XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);
	XMapWindow(display, window);
}

void handle_configure_request_event__BlueWM(const XEvent *event)
{
	XWindowChanges window_changes = {0};

    window_changes.x = event->xconfigurerequest.x;
    window_changes.y = event->xconfigurerequest.y;
    window_changes.width = event->xconfigurerequest.width;
    window_changes.height = event->xconfigurerequest.height;
    window_changes.border_width = event->xconfigurerequest.border_width;
    window_changes.sibling = event->xconfigurerequest.above;
    window_changes.stack_mode = event->xconfigurerequest.detail;

	XConfigureWindow(
        display,
        event->xconfigurerequest.window,
        event->xconfigurerequest.value_mask,
        &window_changes
    );
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
			case ButtonRelease:
			case MotionNotify:
			case MapNotify:
			case UnmapNotify:
			case MapRequest:
			case ConfigureRequest:
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
