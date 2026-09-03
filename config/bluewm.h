#ifndef BLUEWM_CONFIG_H
#define BLUEWM_CONFIG_H

struct BlueWMShortcut {
	unsigned int state;
	KeySym sym;
	void (*handler)(struct BlueWMScreen*);
};

#undef BLUE_WM_SHORTCUT

#define BLUE_WM_SHORTCUT(_state, _sym, _handler) ((struct BlueWMShortcut){ .state = _state, .sym = _sym, .handler = _handler })

static const struct BlueWMShortcut shortcuts[] = {
	BLUE_WM_SHORTCUT(Mod4Mask, XK_Return, &launch_terminal__BlueWM),
	BLUE_WM_SHORTCUT(Mod4Mask, XK_r, &toggle_resize_window__BlueWM),
	BLUE_WM_SHORTCUT(None, XK_Left, &resize_window_left__BlueWM),
	BLUE_WM_SHORTCUT(None, XK_Right, &resize_window_right__BlueWM),
	BLUE_WM_SHORTCUT(None, XK_Up, &resize_window_up__BlueWM),
	BLUE_WM_SHORTCUT(None, XK_Down, &resize_window_down__BlueWM),
	BLUE_WM_SHORTCUT(Mod4Mask, XK_f, &toggle_full_screen_window__BlueWM),

#define BLUE_WM_SHORTCUT_WORKSPACE(n) \
	BLUE_WM_SHORTCUT(Mod4Mask, XK_##n, &toggle_workspace_##n##__BlueWM)

	BLUE_WM_SHORTCUT_WORKSPACE(1),
	BLUE_WM_SHORTCUT_WORKSPACE(2),
	BLUE_WM_SHORTCUT_WORKSPACE(3),
	BLUE_WM_SHORTCUT_WORKSPACE(4),
	BLUE_WM_SHORTCUT_WORKSPACE(5),
	BLUE_WM_SHORTCUT_WORKSPACE(6),
	BLUE_WM_SHORTCUT_WORKSPACE(7),
	BLUE_WM_SHORTCUT_WORKSPACE(8),
	BLUE_WM_SHORTCUT_WORKSPACE(9),
	BLUE_WM_SHORTCUT_WORKSPACE(0)

#undef BLUE_WM_SHORTCUT_WORKSPACE
};
static const size_t shortcuts_len = sizeof(shortcuts) / sizeof(shortcuts[0]);

#undef BLUE_WM_CONFIG_BG_PATH

#define BLUE_WM_CONFIG_BG_PATH "./bg.jpg"

#endif // BLUEWM_CONFIG_H
