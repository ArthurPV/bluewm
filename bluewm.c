#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

#include <bluewm.h>

#define BLUE_WM_CLIENT_STATE_FOCUSED 1 << 0
#define BLUE_WM_CLIENT_STATE_FULLSCREEN 1 << 1

struct BlueWMShortcut;

enum BlueWMClientRole {
	BLUE_WM_CLIENT_ROLE_NONE,
	BLUE_WM_CLIENT_ROLE_DOCK,
	BLUE_WM_CLIENT_ROLE_SPLASH,
};

struct BlueWMClient {
	enum BlueWMClientRole role;
	int client_state_mask;
	Window window;
	Window decoration;
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
	BLUE_WM_ATOM_WINDOW_TYPE,
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
init_client__BlueWM(enum BlueWMClientRole role, Window window, Window decoration);

static void
deinit_client__BlueWM(struct BlueWMClient *client);

static void
init_decoration__BlueWM(void);

static void
deinit_decoration__BlueWM(void);

static void
draw_decoration__BlueWM(const struct BlueWMClient *client);

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
remove_client_from_window__BlueWM(struct BlueWMWorkspace *workspace, Window window);

static struct BlueWMClient *
find_client_from_window__BlueWM(const struct BlueWMWorkspace *workspace, Window window);

static struct BlueWMClient *
find_full_screen_client__BlueWM(const struct BlueWMWorkspace *workspace);

static struct BlueWMClient *
find_client_from_workspaces__BlueWM(Window window);

static inline Window
get_outer_window__BlueWM(const struct BlueWMClient *client);

static enum BlueWMClientRole
get_role_of_atom__BlueWM(const Atom *atom);

static enum BlueWMClientRole
get_role_of_window__BlueWM(Window window);

static void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen);

static void handle_client_role_none__BlueWM(Window window, Window decoration, struct BlueWMScreen *screen);

static Window new_decoration__BlueWM(const struct BlueWMScreen *screen, Window window);

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

static inline void toggle_workspace_0__BlueWM(struct BlueWMScreen *screen);

static void move_workspace_n__BlueWM(struct BlueWMScreen *screen, int workspace_num);

static inline void move_workspace_1__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_2__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_3__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_4__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_5__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_6__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_7__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_8__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_9__BlueWM(struct BlueWMScreen *screen);

static inline void move_workspace_0__BlueWM(struct BlueWMScreen *screen);

static void update_key_state_mask__BlueWM(const XEvent *event);

static bool is_allowed_shortcut__BlueWM(const struct BlueWMShortcut *shortcut);

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

static void handle_expose_event__BlueWM(const XEvent *event);

static void handle_property_notify_event__BlueWM(const XEvent *event);

static int handle_error__BlueWM(Display *error_display, XErrorEvent *error);

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
	[ConfigureRequest] = &handle_configure_request_event__BlueWM,
	[Expose] = &handle_expose_event__BlueWM,
	[PropertyNotify] = &handle_property_notify_event__BlueWM
};
static Atom atoms[BLUE_WM_ATOM_MAX] = {0};
static struct BlueWMWorkspace workspaces[BLUE_WM_WORKSPACE_NUMBER] = {0};
static Cursor cursor = {0};
static Display *display = NULL;
static struct BlueWMScreen *screens = NULL;
static bool is_running = true;
static Window window_to_resize = None;
// The handler Xlib installed, kept to stay fatal on the errors that are
// not expected.
static int (*default_handle_error__BlueWM)(Display *, XErrorEvent *) = NULL;
static Window window_to_move = None;
// Distance between the pointer and the origin of `window_to_move`, to keep the
// window under the same point of the pointer during the whole move.
static int window_to_move_offset_x = 0;
static int window_to_move_offset_y = 0;
static int key_state_mask = 0;
static int button_state_mask = 0;
static GC decoration_gc = {0};
static XFontStruct *decoration_font = NULL;

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
		"_NET_WM_WINDOW_TYPE",
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
init_client__BlueWM(enum BlueWMClientRole role, Window window, Window decoration)
{
	struct BlueWMClient *client = BLUE_ZERO_ALLOC(sizeof(struct BlueWMClient));

	client->role = role;
	client->window = window;
	client->decoration = decoration;

	return client;
}

void
deinit_client__BlueWM(struct BlueWMClient *client)
{
	// A decoration is created by the window manager, so it is not destroyed
	// with the client it holds.
	if (client->decoration != None) {
		XWindowAttributes decoration_attr;

		// The client is put back on the root by the window manager itself, so
		// the server does not have to do it in its place anymore.
		XRemoveFromSaveSet(display, client->window);

		// Destroying a decoration destroys the client it contains, and a client
		// leaving a workspace can be only unmapped, and not gone, so it is put
		// back on the root first, where it was taken from.
		if (XGetWindowAttributes(display, client->decoration, &decoration_attr)) {
			XReparentWindow(display, client->window, decoration_attr.root, decoration_attr.x, decoration_attr.y);
		}

		XDestroyWindow(display, client->decoration);
	}

	free(client);
}

void
init_decoration__BlueWM(void)
{
	Window window_root = DefaultRootWindow(display);

	decoration_gc = XCreateGC(display, window_root, 0, NULL);

	if (!(decoration_font = XLoadQueryFont(display, "fixed"))) {
		BLUE_LOG_ERROR("unable to load font\n");
	}

	XSetFont(display, decoration_gc, decoration_font->fid);
}

// The title of a client is drawn by the window manager on the decoration
// holding it, as the client knows nothing about its own frame.
void
draw_decoration__BlueWM(const struct BlueWMClient *client)
{
	if (client->decoration == None) {
		return;
	}

	XWindowAttributes decoration_attr;

	if (XGetWindowAttributes(display, client->decoration, &decoration_attr) == 0) {
		return;
	}

	// The whole title bar is repainted first, so that the previous title is not
	// left under the new one.
	XSetForeground(display, decoration_gc, BLUE_WM_DECORATION_COLOR);
	XFillRectangle(display, client->decoration, decoration_gc, 0, 0, decoration_attr.width, BLUE_WM_DECORATION_TITLE_HEIGHT);

	char *title = NULL;

	XFetchName(display, client->window, &title);

	if (!title) {
		return;
	}

	int title_len = strlen(title);
	int title_max_width = decoration_attr.width - BLUE_WM_DECORATION_EXTRA_WIDTH;

	// A title wider than its decoration is cut, so that it does not run over
	// the border.
	while (title_len > 0 && XTextWidth(decoration_font, title, title_len) > title_max_width) {
		--title_len;
	}

	if (title_len > 0) {
		int title_width = XTextWidth(decoration_font, title, title_len);

		XSetForeground(display, decoration_gc, BLUE_WM_DECORATION_TITLE_COLOR);
		XDrawString(display, client->decoration, decoration_gc, (decoration_attr.width - title_width) / 2,
				BLUE_WM_DECORATION_TITLE_BASELINE(decoration_font), title, title_len);
	}

	XFree(title);
}

void
deinit_decoration__BlueWM(void)
{
	XFreeGC(display, decoration_gc);
	XFreeFont(display, decoration_font);
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

	// An unmap or a destroy is only known once the client has done it, so the
	// window can already be gone here, and the default screen is then the only
	// answer left.
	if (XGetWindowAttributes(display, window, &window_attr) == 0) {
		return find_screen__BlueWM(DefaultScreen(display));
	}

	return find_screen__BlueWM(XScreenNumberOfScreen(window_attr.screen));
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
remove_client_from_window__BlueWM(struct BlueWMWorkspace *workspace, Window window)
{
	struct BlueWMClient *previous = NULL;
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		if (current->window == window || (current->decoration != None && current->decoration == window)) {
			if (previous) {
				previous->next = current->next;
			} else {
				workspace->clients = current->next;
			}

			current->next = NULL;

			return current;
		}

		previous = current;
		current = current->next;
	}

	return NULL;
}

struct BlueWMClient *
find_client_from_window__BlueWM(const struct BlueWMWorkspace *workspace, Window window)
{
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		if (current->window == window || (current->decoration != None && current->decoration == window)) {
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

// A hidden client keeps asking to be configured, so it is looked for in every
// workspace and not only in the visible one.
struct BlueWMClient *
find_client_from_workspaces__BlueWM(Window window)
{
	for (size_t i = 0; i < BLUE_WM_WORKSPACE_NUMBER; ++i) {
		struct BlueWMClient *client = find_client_from_window__BlueWM(&workspaces[i], window);

		if (client) {
			return client;
		}
	}

	return NULL;
}

// A client is placed and stacked through its decoration, which is its parent,
// so moving or resizing the client alone would only shift it inside its own
// frame.
Window
get_outer_window__BlueWM(const struct BlueWMClient *client)
{
	return client->decoration == None ? client->window : client->decoration;
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

enum BlueWMClientRole
get_role_of_window__BlueWM(Window window)
{
	Atom property_type = None;
	int property_format = 0;
	unsigned long property_count = 0;
	unsigned long property_bytes_after = 0;
	unsigned char *property = NULL;

	// The role is advertised through _NET_WM_WINDOW_TYPE, not through
	// WM_PROTOCOLS, which only carries the protocols the client understands.
	if (XGetWindowProperty(display, window, atoms[BLUE_WM_ATOM_WINDOW_TYPE], 0, LONG_MAX, false, XA_ATOM,
			&property_type, &property_format, &property_count, &property_bytes_after, &property) != Success) {
		return BLUE_WM_CLIENT_ROLE_NONE;
	}

	enum BlueWMClientRole role = BLUE_WM_CLIENT_ROLE_NONE;

	if (property_type == XA_ATOM && property_format == 32) {
		const Atom *window_types = (const Atom *)property;

		// _NET_WM_WINDOW_TYPE is ordered by decreasing preference, so the
		// first recognized type is the one to honor.
		for (unsigned long i = 0; i < property_count; ++i) {
			role = get_role_of_atom__BlueWM(&window_types[i]);

			if (role != BLUE_WM_CLIENT_ROLE_NONE) {
				break;
			}
		}
	}

	if (property) {
		XFree(property);
	}

	return role;
}

void handle_client_role_splash__BlueWM(Window window, struct BlueWMScreen *screen)
{
	screen->splash = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_SPLASH, window, None);
	XConfigureWindow(display, window, CWStackMode, &(XWindowChanges){ .stack_mode = Below });
}

void handle_client_role_dock__BlueWM(Window window, struct BlueWMScreen *screen)
{
	screen->dock = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_DOCK, window, None);
	XConfigureWindow(display, window, CWStackMode, &(XWindowChanges){ .stack_mode = Above });
}

void handle_client_role_none__BlueWM(Window window, Window decoration, struct BlueWMScreen *screen)
{
	struct BlueWMClient *client = init_client__BlueWM(BLUE_WM_CLIENT_ROLE_NONE, window, decoration);
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	client->next = workspace->clients;
	workspace->clients = client;

	notify_active_window__BlueWM(screen, window);
	workspace->active = client;
}

Window new_decoration__BlueWM(const struct BlueWMScreen *screen, Window window)
{
	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, window, &window_attr) == 0) {
		return None;
	}

	int window_decoration_x = window_attr.x;
	int window_decoration_y = window_attr.y;
	int window_decoration_width = window_attr.width + BLUE_WM_DECORATION_EXTRA_WIDTH;
	int window_decoration_height = window_attr.height + BLUE_WM_DECORATION_EXTRA_HEIGHT;
	Window window_root = RootWindow(display, screen->screen_number);
	Window window_decoration = XCreateSimpleWindow(display, window_root, window_decoration_x, window_decoration_y, window_decoration_width, window_decoration_height, 0, BLUE_WM_DECORATION_COLOR, BLUE_WM_DECORATION_COLOR);

	// A decoration belongs to the window manager, so it must not be redirected
	// back to it as a client of its own.
	XChangeWindowAttributes(display, window_decoration, CWOverrideRedirect, &(XSetWindowAttributes){ .override_redirect = true });
	// Destroying a decoration destroys the client it contains, so the client is
	// added to the save set: the server puts it back on the root by itself if
	// the window manager dies, instead of taking it down with the decoration.
	XAddToSaveSet(display, window);
	// The client is still unmapped here, so the reparenting does not report an
	// unmap of its own.
	XReparentWindow(display, window, window_decoration, BLUE_WM_DECORATION_BORDER_SIZE, BLUE_WM_DECORATION_TITLE_HEIGHT);
	// A reparented client is a child of its decoration and no longer of the
	// root, so the unmap, destroy and configure requests of the client are only
	// reported to, and redirected by, the decoration.
	//
	// The enter events are selected as well, so that the border and the title
	// are not holes in the focus follow the pointer.
	XSelectInput(display, window_decoration, SubstructureNotifyMask | SubstructureRedirectMask | EnterWindowMask | ExposureMask);
	XMapWindow(display, window_decoration);

	return window_decoration;
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
		case BLUE_WM_CLIENT_ROLE_NONE: {
			Window decoration = new_decoration__BlueWM(screen, window);

			handle_client_role_none__BlueWM(window, decoration, screen);

			break;
		}
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
			goto out;
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

	const struct BlueWMClient *client = find_client_from_window__BlueWM(&workspaces[screen->workspace], window_to_resize);

	if (!client) {
		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, client->window, &window_attr) == 0) {
		return;
	}

	int new_width;
	int new_height;

	if (__builtin_add_overflow(window_attr.width, width_change, &new_width) || __builtin_add_overflow(window_attr.height, height_change, &new_height)) {
		return;
	}

	if (new_width < 0 || new_height < 0) {
		return;
	}

	// The decoration is grown with its client, otherwise the client would be
	// clipped by the frame it is contained in.
	if (client->decoration != None) {
		XResizeWindow(display, client->decoration, new_width + BLUE_WM_DECORATION_EXTRA_WIDTH, new_height + BLUE_WM_DECORATION_EXTRA_HEIGHT);
	}

	XResizeWindow(display, client->window, new_width, new_height);
	// A shrinking decoration exposes nothing, so its title is drawn again by
	// hand to stay centered.
	draw_decoration__BlueWM(client);
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
	Window outer_window = get_outer_window__BlueWM(workspace->active);

	if (workspace->active->is_fullscreen) {
		// The saved geometry is the one of the decoration, so the client is put
		// back inside it at its own offset.
		XMoveResizeWindow(display, outer_window, workspace->active->saved_x, workspace->active->saved_y, workspace->active->saved_width, workspace->active->saved_height);

		if (workspace->active->decoration != None) {
			XMoveResizeWindow(display, active_window, BLUE_WM_DECORATION_BORDER_SIZE, BLUE_WM_DECORATION_TITLE_HEIGHT,
					workspace->active->saved_width - BLUE_WM_DECORATION_EXTRA_WIDTH, workspace->active->saved_height - BLUE_WM_DECORATION_EXTRA_HEIGHT);
		}

		// The client was covering its whole decoration, so the title bar is
		// uncovered and has to be drawn again.
		draw_decoration__BlueWM(workspace->active);

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

	if (XGetWindowAttributes(display, outer_window, &window_attr) == 0) {
		return;
	}

	int screen_width = DisplayWidth(display, screen->screen_number);
	int screen_height = DisplayHeight(display, screen->screen_number);

	// A full screen client covers its own decoration, so the client fills the
	// whole frame instead of being inset in it.
	XMoveResizeWindow(display, outer_window, 0, 0, screen_width, screen_height);

	if (workspace->active->decoration != None) {
		XMoveResizeWindow(display, active_window, 0, 0, screen_width, screen_height);
	}

	XRaiseWindow(display, outer_window);

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
		// Unmapping the decoration hides the client without unmapping it, so
		// no unmap of the client is reported and the client is kept.
		XUnmapWindow(display, get_outer_window__BlueWM(current));
		current = current->next;
	}
}

void map_all_windows_from_current_workspace__BlueWM(struct BlueWMScreen *screen)
{
	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];
	struct BlueWMClient *current = workspace->clients;

	while (current) {
		XMapWindow(display, get_outer_window__BlueWM(current));
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

void move_workspace_n__BlueWM(struct BlueWMScreen *screen, int workspace_num)
{
	if (screen->workspace == workspace_num) {
		return;
	}

	struct BlueWMWorkspace *workspace = &workspaces[screen->workspace];

	if (!workspace->active) {
		return;
	}

	Window window = workspace->active->window;
	struct BlueWMClient *moved_client = remove_client_from_window__BlueWM(workspace, window);

	XUnmapWindow(display, get_outer_window__BlueWM(moved_client));

	workspace = &workspaces[workspace_num];

	moved_client->next = workspace->clients;
	workspace->clients = moved_client;
}

#define MOVE_WORKSPACE_N(n) \
void move_workspace_##n##__BlueWM(struct BlueWMScreen *screen) { \
	move_workspace_n__BlueWM(screen, n); \
}

MOVE_WORKSPACE_N(1)
MOVE_WORKSPACE_N(2)
MOVE_WORKSPACE_N(3)
MOVE_WORKSPACE_N(4)
MOVE_WORKSPACE_N(5)
MOVE_WORKSPACE_N(6)
MOVE_WORKSPACE_N(7)
MOVE_WORKSPACE_N(8)
MOVE_WORKSPACE_N(9)
MOVE_WORKSPACE_N(0)

#undef MOVE_WORKSPACE_N

static void update_key_state_mask__BlueWM(const XEvent *event)
{
	unsigned int mod4 = event->xkey.state & Mod4Mask ? Mod4Mask : None;
	unsigned int shift = event->xkey.state & ShiftMask ? ShiftMask : None;
	unsigned int control = event->xkey.state & ControlMask ? ControlMask : None;

	key_state_mask = mod4 | shift | control;
}

bool is_allowed_shortcut__BlueWM(const struct BlueWMShortcut *shortcut)
{
	if (window_to_resize != None) {
		static void (*allowed_shortcuts[])(struct BlueWMScreen*) = {
			&toggle_resize_window__BlueWM,
			&resize_window_left__BlueWM,
			&resize_window_right__BlueWM,
			&resize_window_up__BlueWM,
			&resize_window_down__BlueWM
		};
		static size_t allowed_shortcuts_len = sizeof(allowed_shortcuts) / sizeof(*allowed_shortcuts);

		for (int i = 0; i < allowed_shortcuts_len; ++i) {
			if (shortcut->handler == allowed_shortcuts[i]) {
				return true;
			}
		}

		return false;
	}

	return true;
}

void handle_key_press_event__BlueWM(const XEvent *event)
{
	KeySym sym = XLookupKeysym((XKeyEvent*)&event->xkey, 0);

	update_key_state_mask__BlueWM(event);

	for (size_t i = 0; i < shortcuts_len; ++i) {
		const struct BlueWMShortcut *shortcut = &shortcuts[i];

		if (key_state_mask == shortcut->state && shortcut->sym == sym && is_allowed_shortcut__BlueWM(shortcut)) {
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
	const struct BlueWMClient *client = find_client_from_window__BlueWM(&workspaces[screen->workspace], window);

	if (!client) {
		return;
	}

	// The sub-window of the event is the decoration, as it is the child of the
	// root, but a client without one is moved directly.
	Window outer_window = get_outer_window__BlueWM(client);
	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, outer_window, &window_attr) == 0) {
		return;
	}

	window_to_move = outer_window;
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

	// The window can be the decoration, but only a client can hold the focus.
	notify_active_window__BlueWM(screen, client->window);
	XSetInputFocus(display, client->window, RevertToPointerRoot, CurrentTime);
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

	// A client is taken in charge on its map request, before it is mapped, so
	// the only windows left here are the ones the window manager does not
	// manage, the decorations included.
	if (event->xmap.override_redirect) {
		return;
	}

	// A new window is mapped over the others, so the full screen window has to
	// be raised again to stay the only one visible.
	struct BlueWMScreen *screen = get_screen_from_window__BlueWM(event->xmap.event);
	const struct BlueWMClient *full_screen_client = find_full_screen_client__BlueWM(&workspaces[screen->workspace]);

	if (full_screen_client && full_screen_client->window != window) {
		// The decoration is the parent of the client, so raising the client
		// alone would leave it behind its own frame.
		XRaiseWindow(display, full_screen_client->decoration != None ? full_screen_client->decoration : full_screen_client->window);
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
	struct BlueWMClient *removed_client = remove_client_from_window__BlueWM(workspace, window);

	if (removed_client) {
		deinit_client__BlueWM(removed_client);
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
		return false;
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
		return;
	}

	struct BlueWMScreen *screen = find_screen__BlueWM(XScreenNumberOfScreen(window_attr.screen));

	// A client remapping itself is already taken in charge, so it only has to
	// be mapped back, without being given a second decoration.
	if (find_client_from_window__BlueWM(&workspaces[screen->workspace], window)) {
		XMapWindow(display, window);

		return;
	}

	int new_x = 0;
	int new_y = 0;

	if (window_is_on_dock__BlueWM(screen, &window_attr, &new_x, &new_y)) {
		XMoveWindow(display, window, new_x, new_y);
	}

	// The enter events are what drives the focus follow the pointer, and they
	// are only received by selecting them on the client itself.
	XSelectInput(display, window, KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | PropertyChangeMask);

	// The client is taken in charge here, and not on its map notification,
	// because only the map requests are limited to the windows the window
	// manager has to manage, and because the reparenting into a decoration has
	// to happen while the client is still unmapped.
	enum BlueWMClientRole window_role = get_role_of_window__BlueWM(window);

	new_client__BlueWM(window, window_role);
	XMapWindow(display, window);

	// The focus is only given once the client is viewable, as an unviewable
	// window cannot hold it.
	if (window_role == BLUE_WM_CLIENT_ROLE_NONE) {
		XSetInputFocus(display, window, RevertToPointerRoot, CurrentTime);
	}
}

void handle_configure_request_event__BlueWM(const XEvent *event)
{
	const XConfigureRequestEvent *request = &event->xconfigurerequest;
	XWindowChanges window_changes = {
		.x = request->x,
		.y = request->y,
		.width = request->width,
		.height = request->height,
		.border_width = request->border_width,
		.sibling = request->above,
		.stack_mode = request->detail
	};
	const struct BlueWMClient *client = find_client_from_workspaces__BlueWM(request->window);

	// A window that is not taken in charge yet, and the dock and the splash,
	// which are placed by the window manager itself, have no decoration to keep
	// in step, so the request is only forwarded.
	if (!client || client->decoration == None) {
		XConfigureWindow(display, request->window, request->value_mask, &window_changes);

		return;
	}

	// A full screen client already covers the whole screen, so it must not put
	// itself back to its own size.
	if (client->is_fullscreen) {
		return;
	}

	XWindowAttributes window_attr;

	if (XGetWindowAttributes(display, client->window, &window_attr) == 0) {
		return;
	}

	// Only the requested fields are honored, the others keep the size the
	// client already has, as the decoration needs both of them to be resized.
	int new_width = request->value_mask & CWWidth ? request->width : window_attr.width;
	int new_height = request->value_mask & CWHeight ? request->height : window_attr.height;
	XWindowChanges decoration_changes = {
		.x = request->x,
		.y = request->y,
		.width = new_width + BLUE_WM_DECORATION_EXTRA_WIDTH,
		.height = new_height + BLUE_WM_DECORATION_EXTRA_HEIGHT,
		.sibling = request->above,
		.stack_mode = request->detail
	};

	// A window can only be stacked against one of its own siblings, and the
	// siblings of a decoration are the other decorations.
	if (request->value_mask & CWSibling) {
		const struct BlueWMClient *sibling = find_client_from_workspaces__BlueWM(request->above);

		if (sibling) {
			decoration_changes.sibling = get_outer_window__BlueWM(sibling);
		}
	}

	// The decoration is placed and stacked in the place of its client, and it
	// is always resized, so that the client is never clipped by its own frame.
	// The border width is left out, as it only means something for the client.
	XConfigureWindow(display, client->decoration, (request->value_mask & (CWX | CWY | CWSibling | CWStackMode)) | CWWidth | CWHeight, &decoration_changes);

	// The client keeps its place inside its decoration, so only its size is
	// taken from the request.
	XConfigureWindow(display, client->window, CWWidth | CWHeight, &(XWindowChanges){ .width = new_width, .height = new_height });
	// A shrinking decoration exposes nothing, so its title is drawn again by
	// hand to stay centered.
	draw_decoration__BlueWM(client);
}

void handle_expose_event__BlueWM(const XEvent *event)
{
	// Only the decorations are drawn by the window manager, the clients draw
	// themselves.
	const struct BlueWMClient *client = find_client_from_workspaces__BlueWM(event->xexpose.window);

	if (client && client->decoration == event->xexpose.window) {
		draw_decoration__BlueWM(client);
	}
}

void handle_property_notify_event__BlueWM(const XEvent *event)
{
	// A client renames itself while it runs, so its title has to be drawn
	// again.
	if (event->xproperty.atom != XA_WM_NAME) {
		return;
	}

	const struct BlueWMClient *client = find_client_from_workspaces__BlueWM(event->xproperty.window);

	if (client) {
		draw_decoration__BlueWM(client);
	}
}

int handle_error__BlueWM(Display *error_display, XErrorEvent *error)
{
	// The requests on which a race with a client is expected. Outside of them
	// the same error is a mistake of the window manager, so it is not ignored.
	static const struct {
		unsigned char request_code;
		unsigned char error_code;
	} expected_errors[] = {
		// A window that is no longer viewable cannot take the focus, nor be
		// stacked against a sibling it has lost.
		{ X_SetInputFocus, BadMatch },
		{ X_ConfigureWindow, BadMatch },
		// A decoration is drawn on an expose, which can be handled once the
		// decoration is already destroyed.
		{ X_PolyText8, BadDrawable },
		{ X_PolyFillRectangle, BadDrawable },
		{ X_PolySegment, BadDrawable },
		{ X_CopyArea, BadDrawable },
		// Another client can already hold the grab of a shortcut.
		{ X_GrabButton, BadAccess },
		{ X_GrabKey, BadAccess }
	};
	static const size_t expected_errors_len = sizeof(expected_errors) / sizeof(*expected_errors);

	// A client can be gone between the moment a request is sent and the moment
	// the server handles it, so a request on a window that no longer exists is
	// expected whatever the request is.
	if (error->error_code == BadWindow) {
		return 0;
	}

	for (size_t i = 0; i < expected_errors_len; ++i) {
		if (error->request_code == expected_errors[i].request_code && error->error_code == expected_errors[i].error_code) {
			return 0;
		}
	}

	// An unexpected error is a mistake of the window manager itself, and it is
	// left fatal so that it is not hidden.
	return default_handle_error__BlueWM(error_display, error);
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
			case Expose:
			case PropertyNotify:
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

	default_handle_error__BlueWM = XSetErrorHandler(&handle_error__BlueWM);
	init_decoration__BlueWM();
	set_atoms__BlueWM();
	set_screens__BlueWM();
	set_cursor__BlueWM();
	launch_startup_program__BlueWM();
	handle_events__BlueWM();
	unset_screens__BlueWM();
	unset_cursor__BlueWM();
	deinit_decoration__BlueWM();

	XCloseDisplay(display);
}
