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
	int saved_x;
	int saved_y;
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
	BLUE_WM_ATOM_PROTOCOLS,
	BLUE_WM_ATOM_DELETE_WINDOW,

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

static struct BlueWMClient *
find_client_from_window__BlueWM(const struct BlueWMWorkspace *workspace, Window window);

static struct BlueWMClient *
find_full_screen_client__BlueWM(const struct BlueWMWorkspace *workspace);

static enum BlueWMClientRole
get_role_of_atom__BlueWM(const Atom *atom);

static void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_none__BlueWM(Window window, struct BlueWMScreen *screen);

static void new_client__BlueWM(Window window, enum BlueWMClientRole role);

static void launch_terminal__BlueWM(struct BlueWMScreen *screen);

static void launch_launcher__BlueWM(struct BlueWMScreen *screen);

static void toggle_resize_window__BlueWM(struct BlueWMScreen *screen);

static void resize_window__BlueWM(struct BlueWMScreen *screen, int width_change, int height_change);

static inline void resize_window_left__BlueWM(struct BlueWMScreen *screen);

static inline void resize_window_right__BlueWM(struct BlueWMScreen *screen);

static inline void resize_window_up__BlueWM(struct BlueWMScreen *screen);

static inline void resize_window_down__BlueWM(struct BlueWMScreen *screen);

static void toggle_full_screen_window__BlueWM(struct BlueWMScreen *screen);

static bool window_supports_delete__BlueWM(Window window);

static void close_window__BlueWM(struct BlueWMScreen *screen);

static void unmap_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen);

static void map_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen);

static void grab_keys__BlueWM(Window window_root, bool with_modifier);

static void ungrab_keys__BlueWM(Window window_root, bool with_modifier);

static void grab_buttons__BlueWM(Window window_root);

static struct BlueWMClient *find_client_from_window__BlueWM(const struct BlueWMWorkspace *workspace, Window window);

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

static void handle_enter_notify_event__BlueWM(const XEvent *event);

static void focus_window__BlueWM(struct BlueWMScreen *screen, Window window);

static void notify_active_window__BlueWM(const struct BlueWMScreen *screen, Window window);

static void handle_map_notify_event__BlueWM(const XEvent *event);

static void update_active_window_from_workspaces__BlueWM(const struct BlueWMScreen *screen, Window window);

static void remove_client__BlueWM(const struct BlueWMScreen *screen, Window window);

static void handle_unmap_notify_event__BlueWM(const XEvent *event);

static void handle_destroy_notify_event__BlueWM(const XEvent *event);

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
	[EnterNotify] = &handle_enter_notify_event__BlueWM,
	[MapNotify] = &handle_map_notify_event__BlueWM, 
	[UnmapNotify] = &handle_unmap_notify_event__BlueWM,
	[DestroyNotify] = &handle_destroy_notify_event__BlueWM,
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
static Window window_to_move = None;
// Distance between the pointer and the origin of `window_to_move`, to keep the
// window under the same point of the pointer during the whole move.
static int window_to_move_offset_x = 0;
static int window_to_move_offset_y = 0;
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
		"WM_PROTOCOLS",
		"WM_DELETE_WINDOW",
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

// The lock modifiers are not part of a shortcut, but they are part of the state
// of the key event, so each shortcut must be grabbed with all their combinations.
static const unsigned int lock_masks[] = {
	None,
	LockMask,
	Mod2Mask,
	LockMask | Mod2Mask
};
static const size_t lock_masks_len = sizeof(lock_masks) / sizeof(lock_masks[0]);

void
grab_keys__BlueWM(Window window_root, bool with_modifier)
{
	for (size_t i = 0; i < shortcuts_len; ++i) {
		const struct BlueWMShortcut *shortcut = &shortcuts[i];

		if ((shortcut->state != None) != with_modifier) {
			continue;
		}

		KeyCode keycode = XKeysymToKeycode(display, shortcut->sym);

		if (keycode == 0) {
			continue;
		}

		for (size_t j = 0; j < lock_masks_len; ++j) {
			XGrabKey(display, keycode, shortcut->state | lock_masks[j], window_root, false, GrabModeAsync, GrabModeAsync);
		}
	}
}

void
ungrab_keys__BlueWM(Window window_root, bool with_modifier)
{
	for (size_t i = 0; i < shortcuts_len; ++i) {
		const struct BlueWMShortcut *shortcut = &shortcuts[i];

		if ((shortcut->state != None) != with_modifier) {
			continue;
		}

		KeyCode keycode = XKeysymToKeycode(display, shortcut->sym);

		if (keycode == 0) {
			continue;
		}

		for (size_t j = 0; j < lock_masks_len; ++j) {
			XUngrabKey(display, keycode, shortcut->state | lock_masks[j], window_root);
		}
	}
}

void
grab_buttons__BlueWM(Window window_root)
{
	for (size_t i = 0; i < lock_masks_len; ++i) {
		XGrabButton(display, Button1, BLUE_WM_MOVE_WINDOW_MASK | lock_masks[i], window_root, false, ButtonPressMask | ButtonReleaseMask | PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
	}
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

		// The motion of a move is reported by the grab itself, so it is not
		// selected here.
		XSelectInput(display, window_root,
				ButtonPressMask | ButtonReleaseMask |
				SubstructureNotifyMask | SubstructureRedirectMask);

		// The key and button events of a focused client are only received
		// through a grab.
		grab_keys__BlueWM(window_root, true);
		grab_buttons__BlueWM(window_root);

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

struct BlueWMClient *
find_client_from_window__BlueWM(const struct BlueWMWorkspace *workspace, Window window)
{
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		if (current->window == window) {
			return current;
		}

		current = current->next;
	}

	return NULL;
}

struct BlueWMClient *
find_full_screen_client__BlueWM(const struct BlueWMWorkspace *workspace)
{
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		if (current->is_fullscreen) {
			return current;
		}

		current = current->next;
	}

	return NULL;
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

	client->next = workspace->clients;
	workspace->clients = client;

	notify_active_window__BlueWM(screen, window);
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

void launch_launcher__BlueWM(struct BlueWMScreen *screen)
{
	launch_builtin_program__BlueWM("./bluewmlauncher", NULL);
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

		// The shortcuts without any modifier are only grabbed while resizing a
		// window, otherwise they would be stolen from every client.
		ungrab_keys__BlueWM(RootWindow(display, screen->screen_number), false);

		goto update_property;
	}

out:
	window_to_resize = workspace->active->window;

	grab_keys__BlueWM(RootWindow(display, screen->screen_number), false);

update_property:
	XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_RESIZE_WINDOW], XA_WINDOW, 32, PropModeReplace, (unsigned char*)&window_to_resize, 1);
}

void resize_window__BlueWM(struct BlueWMScreen *screen, int width_change, int height_change)
{
	if (window_to_resize == None) {
		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, window_to_resize, &window_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	int new_width;
	int new_height;

	if (__builtin_add_overflow(window_attr.width, width_change, &new_width) || __builtin_add_overflow(window_attr.height, height_change, &new_height)) {
		return;
	}

	if (new_width < 0 || new_height < 0) {
		return;
	}

	XResizeWindow(display, window_to_resize, new_width, new_height);
}

void resize_window_left__BlueWM(struct BlueWMScreen *screen)
{
	resize_window__BlueWM(screen, +10, +0);
}

void resize_window_right__BlueWM(struct BlueWMScreen *screen)
{
	resize_window__BlueWM(screen, -10, +0);
}

void resize_window_up__BlueWM(struct BlueWMScreen *screen)
{
	resize_window__BlueWM(screen, +0, +10);
}

void resize_window_down__BlueWM(struct BlueWMScreen *screen)
{
	resize_window__BlueWM(screen, +0, -10);
}

void toggle_full_screen_window__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (!workspace->active) {
		return;
	}

	Window active_window = workspace->active->window;

	if (workspace->active->is_fullscreen) {
		XMoveResizeWindow(display, active_window, workspace->active->saved_x, workspace->active->saved_y, workspace->active->saved_width, workspace->active->saved_height);

		workspace->active->is_fullscreen = false;
		workspace->active->saved_width = 0;
		workspace->active->saved_height = 0;
		workspace->active->saved_x = 0;
		workspace->active->saved_y = 0;

		// The dock is back over the clients.
		if (screen->dock) {
			XConfigureWindow(display, screen->dock->window, CWStackMode, &(XWindowChanges){ .stack_mode = Above });
		}

		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, active_window, &window_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes");
	}

	int screen_width = DisplayWidth(display, screen->screen_number);
	int screen_height = DisplayHeight(display, screen->screen_number);

	XMoveResizeWindow(display, active_window, 0, 0, screen_width, screen_height);
	XRaiseWindow(display, active_window);

	workspace->active->is_fullscreen = true;
	workspace->active->saved_width = window_attr.width;
	workspace->active->saved_height = window_attr.height;
	workspace->active->saved_x = window_attr.x;
	workspace->active->saved_y = window_attr.y;
}

bool window_supports_delete__BlueWM(Window window)
{
	Atom *protocols = NULL;
	int protocols_count = 0;
	bool supports_delete = false;

	if (XGetWMProtocols(display, window, &protocols, &protocols_count) == 0) {
		return false;
	}

	for (int i = 0; i < protocols_count; ++i) {
		if (protocols[i] == atoms[BLUE_WM_ATOM_DELETE_WINDOW]) {
			supports_delete = true;

			break;
		}
	}

	if (protocols) {
		XFree(protocols);
	}

	return supports_delete;
}

void close_window__BlueWM(struct BlueWMScreen *screen)
{
	const struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (!workspace->active) {
		return;
	}

	Window window = workspace->active->window;

	// The client is only removed from the workspace when the window is
	// effectively unmapped or destroyed.
	if (!window_supports_delete__BlueWM(window)) {
		XKillClient(display, window);

		return;
	}

	XEvent event = {0};

	event.xclient.type = ClientMessage;
	event.xclient.window = window;
	event.xclient.message_type = atoms[BLUE_WM_ATOM_PROTOCOLS];
	event.xclient.format = 32;
	event.xclient.data.l[0] = atoms[BLUE_WM_ATOM_DELETE_WINDOW];
	event.xclient.data.l[1] = CurrentTime;

	XSendEvent(display, window, false, NoEventMask, &event);
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

void notify_active_window__BlueWM(const struct BlueWMScreen *screen, Window window)
{
	XChangeProperty(display, RootWindow(display, screen->screen_number), atoms[BLUE_WM_ATOM_ACTIVE_WINDOW], XA_WINDOW, 32, PropModeReplace, (unsigned char*)&window, 1);
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

	// The window with the input focus has just been unmapped, so the focus is
	// given to the active window of the new workspace.
	const struct BlueWMWorkspace *new_workspace = &workspaces[screen->workspace];
	Window new_active_window = new_workspace->active ? new_workspace->active->window : None;

	XSetInputFocus(display, new_active_window == None ? PointerRoot : new_active_window, RevertToPointerRoot, CurrentTime);
	notify_active_window__BlueWM(screen, new_active_window);

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
	// The state of a button event is the state before the event itself, so the
	// button of the event is added (or removed) by hand.
	unsigned int mask = event->xbutton.state & (Button1Mask | Button2Mask);

	switch (event->xbutton.button) {
		case Button1:
			mask = event->type == ButtonPress ? mask | Button1Mask : mask & ~Button1Mask;

			break;
		case Button2:
			mask = event->type == ButtonPress ? mask | Button2Mask : mask & ~Button2Mask;

			break;
		default:
			break;
	}

	button_state_mask = mask;
}

void handle_button_press_event__BlueWM(const XEvent *event)
{
	update_button_state_mask__BlueWM(event);

	// The grab is done on the root window, so the client under the pointer is
	// the sub-window of the event.
	Window window = event->xbutton.subwindow;

	if (event->xbutton.button != Button1 || !(event->xbutton.state & BLUE_WM_MOVE_WINDOW_MASK) || window == None) {
		return;
	}

	// Only a regular client can be moved: the dock and the splash are placed by
	// the WM itself, and they are the only clients out of the workspaces.
	struct BlueWMScreen *screen = find_screen_from_root_window__BlueWM(event->xbutton.root);

	if (!find_client_from_window__BlueWM(&workspaces[screen->workspace], window)) {
		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, window, &window_attr) == 0) {
		return;
	}

	window_to_move = window;
	window_to_move_offset_x = event->xbutton.x_root - window_attr.x;
	window_to_move_offset_y = event->xbutton.y_root - window_attr.y;

	XRaiseWindow(display, window_to_move);
}

void handle_button_release_event__BlueWM(const XEvent *event)
{
	update_button_state_mask__BlueWM(event);

	if (event->xbutton.button == Button1) {
		window_to_move = None;
	}
}

void handle_motion_notify_event__BlueWM(const XEvent *event)
{
	if (window_to_move != None) {
		XMoveWindow(display, window_to_move, event->xmotion.x_root - window_to_move_offset_x, event->xmotion.y_root - window_to_move_offset_y);
	}
}

void focus_window__BlueWM(struct BlueWMScreen *screen, Window window)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	// Only a regular client can be focused: the dock and the splash are the
	// only clients out of the workspaces.
	struct BlueWMClient *client = find_client_from_window__BlueWM(workspace, window);

	if (!client || workspace->active == client) {
		return;
	}

	workspace->active = client;

	notify_active_window__BlueWM(screen, window);
	XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
}

void handle_enter_notify_event__BlueWM(const XEvent *event)
{
	// A grab (moving a window) or a window appearing under the pointer also
	// sends an enter event, but the pointer did not enter a new window.
	if (event->xcrossing.mode != NotifyNormal || event->xcrossing.detail == NotifyInferior) {
		return;
	}

	focus_window__BlueWM(find_screen_from_root_window__BlueWM(event->xcrossing.root), event->xcrossing.window);
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

	// A new window is mapped over the others, so the full screen window has to
	// be raised again to stay the only one visible.
	struct BlueWMScreen *screen = get_screen_from_window__BlueWM(event->xmap.event);
	const struct BlueWMClient *full_screen_client = find_full_screen_client__BlueWM(&workspaces[screen->workspace]);

	if (full_screen_client && full_screen_client->window != window) {
		XRaiseWindow(display, full_screen_client->window);
		// The new window is hidden behind, so it must not keep the focus.
		focus_window__BlueWM(screen, full_screen_client->window);
	}
}

void update_active_window_from_workspaces__BlueWM(const struct BlueWMScreen *screen, Window window)
{
	for (size_t i = 0; i < BLUE_WM_WORKSPACE_NUMBER; ++i) {
		struct BlueWMWorkspace *workspace = &workspaces[i];

		if (workspace->active && workspace->active->window == window) {
			notify_active_window__BlueWM(screen, None);
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

void handle_destroy_notify_event__BlueWM(const XEvent *event)
{
	// A killed client never unmaps its windows, so the destruction is the only
	// notification of its removal.
	struct BlueWMScreen *screen = get_screen_from_window__BlueWM(event->xdestroywindow.event);
	Window destroyed_window = event->xdestroywindow.window;

	update_active_window_from_workspaces__BlueWM(screen, destroyed_window);
	remove_client__BlueWM(screen, destroyed_window);
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

	// Two rectangles only overlap if they overlap on both axes.
	bool overlap_x = window_attr->x < dock_attr.x + dock_attr.width && dock_attr.x < window_attr->x + window_attr->width;
	bool overlap_y = window_attr->y < dock_attr.y + dock_attr.height && dock_attr.y < window_attr->y + window_attr->height;

	if (!overlap_x || !overlap_y) {
		return false;
	}

	// The dock spans the whole width of the screen, so the window is moved out
	// of it on the vertical axis, on the side where the dock is not.
	if (dock_attr.y <= window_attr->y) {
		*new_y = dock_attr.y + dock_attr.height;
	} else {
		*new_y = dock_attr.y - window_attr->height;
	}

	return true;
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

	// The enter events are what drives the focus follow the pointer, and they
	// are only received by selecting them on the client itself.
	XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask);
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
			case EnterNotify:
			case MapNotify:
			case UnmapNotify:
			case DestroyNotify:
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
