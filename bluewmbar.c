/*
* MIT License
*
* Copyright (c) 2026 ArthurPV
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#define _GNU_SOURCE

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <alsa/asoundlib.h>

#include <bluewm.h>

#define WINDOW_HEIGHT 20
#define WINDOW_MIDDLE(font) ((WINDOW_HEIGHT / 2) + ((font)->ascent - ((font)->ascent + (font)->descent) / 2))
// Space kept on the sides of the bar, and between what is drawn on it.
#define WINDOW_PADDING 3
// Space taken by the number of a workspace, as a factor of the height of the
// font.
#define WINDOW_WORKSPACE_SPACE_FACTOR 2
// Where the numbers of the workspaces start, which is also where everything
// drawn before them has to stop.
#define WINDOW_WORKSPACES_X(font) ((int)window_width - WINDOW_WORKSPACE_SPACE_FACTOR * (font)->ascent * BLUE_WM_WORKSPACE_NUMBER)

static Display *display = NULL;
static Window window = {0};
static unsigned int window_bg_color = BLUE_RGB(66, 85, 148);
static unsigned int window_width = 0;
static unsigned int window_height = 0;
static GC window_gc = {0};
static XFontStruct *font = NULL;
static Pixmap window_pixels = {0};
static Window current_window = None;
static char *current_window_title = NULL;
static size_t current_window_title_len = 0;
static Atom active_window_atom = {0};
static Atom wm_name_atom = {0};
static Atom resize_window_atom = {0};
static Atom active_workspace_atom = {0};
static Atom xkb_rules_names_atom = {0};
// The XKB events are numbered from a base the server gives, so their type is
// only known once the extension is queried.
static int xkb_event_base = -1;
// The battery the bar reports on, looked for once and kept, as it does not
// change while the machine runs.
static char current_battery[NAME_MAX + 1] = {0};

// The wireless interface the bar reports on, looked for once and kept, like the
// battery.
static char current_wifi_interface[IFNAMSIZ + 1] = {0};

#ifndef POWER_SUPPLY_PATH
#define POWER_SUPPLY_PATH "/sys/class/power_supply"
#endif

#ifndef NET_CLASS_PATH
#define NET_CLASS_PATH "/sys/class/net"
#endif

#ifndef PROC_NET_WIRELESS_PATH
#define PROC_NET_WIRELESS_PATH "/proc/net/wireless"
#endif

// The link quality of /proc/net/wireless is given out of a maximum the driver
// reports, which is this one on every driver in practice.
#define WIFI_QUALITY_MAX 70

// The mixer is opened once and kept, as opening and loading it on every draw
// would be wasteful.
static snd_mixer_t *volume_mixer = NULL;
static snd_mixer_elem_t *volume_element = NULL;
// A mixer that cannot be opened is not tried again, as it would be opened once
// a second for nothing.
static bool volume_is_unavailable = false;

#ifndef VOLUME_MIXER_DEVICE
#define VOLUME_MIXER_DEVICE "default"
#endif

#ifndef VOLUME_MIXER_ELEMENT
#define VOLUME_MIXER_ELEMENT "Master"
#endif

// Where what is drawn on the left of the bar ends, so that what comes after it
// starts from there, and the title knows where it has to stop.
static int left_block_end_x = 0;
static bool resizing_window = false;
static int active_workspace = -1;

static void draw_bg__BlueWMBar(void);

static const char *get_current_keyboard_layout__BlueWMBar(void);

static void draw_keyboard_layout__BlueWMBar(void);

static bool read_file_line__BlueWMBar(const char *path, char *buffer, size_t buffer_len);

static bool read_battery_attribute__BlueWMBar(const char *battery, const char *attribute, char *buffer, size_t buffer_len);

static const char *find_battery__BlueWMBar(void);

static const char *get_battery_status__BlueWMBar(bool *is_low_p);

static void draw_battery__BlueWMBar(void);

static const char *find_wifi_interface__BlueWMBar(void);

static bool get_wifi_quality__BlueWMBar(const char *interface, int *quality_p);

static bool get_wifi_ssid__BlueWMBar(const char *interface, char *ssid, size_t ssid_len);

static const char *get_wifi_status__BlueWMBar(bool *is_low_p);

static void draw_wifi__BlueWMBar(void);

static snd_mixer_elem_t *find_volume_element__BlueWMBar(void);

static const char *get_volume_status__BlueWMBar(bool *is_muted_p);

static void draw_volume__BlueWMBar(void);

static void close_volume__BlueWMBar(void);

static void draw_date__BlueWMBar(void);

static void draw_window_title__BlueWMBar(void);

static void draw_workspaces_number__BlueWMBar(void);

static void draw__BlueWMBar(void);

static void set_font__BlueWMBar(void);

static void handle_active_window_notify__BlueWMBar(const XEvent *event);

static void fetch_window_title__BlueWMBar(void);

static inline void handle_wm_name_notify__BlueWMBar(void);

static void handle_resize_window_notify__BlueWMBar(const XEvent *event);

static void fetch_active_workspace__BlueWMBar(Window window);

static void handle_active_workspace_notify__BlueWMBar(const XEvent *event);

static void handle_events__BlueWMBar(void);

static void close__BlueWMBar(void);

static int handle_error__BlueWMBar(Display *error_display, XErrorEvent *error);

void draw_bg__BlueWMBar(void)
{
	XSetForeground(display, window_gc, window_bg_color);
	XFillRectangle(display, window_pixels, window_gc, 0, 0, window_width, window_height);
}

const char *get_current_keyboard_layout__BlueWMBar(void)
{
	char layout[32] = {0};
	XkbStateRec keyboard_state;

	if (XkbGetState(display, XkbUseCoreKbd, &keyboard_state) != Success) {
		return NULL;
	}

	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return = NULL;
	int status = XGetWindowProperty(display, DefaultRootWindow(display), xkb_rules_names_atom, 0, 1024, false, XA_STRING,
			&actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	// The names of the groups are full descriptions, like "English (US)", which
	// are far too long for the bar, so the short names the layouts were loaded
	// with are taken instead.
	if (status != Success || actual_type != XA_STRING || !prop_return) {
		return NULL;
	}

	// The property holds the rules, the model, the layouts, the variants and
	// the options, one after the other, each ended by a nul.
	const char *names = (const char *)prop_return;
	unsigned long offset = 0;

	for (int i = 0; i < 2 && offset < nitems_return; ++i) {
		offset += strlen(names + offset) + 1;
	}

	if (offset >= nitems_return) {
		XFree(prop_return);

		return NULL;
	}

	// The layouts are separated by a comma, and are in the order of the groups.
	const char *current = names + offset;

	for (int i = 0; i < keyboard_state.group && current; ++i) {
		const char *comma = strchr(current, ',');

		current = comma ? comma + 1 : NULL;
	}

	if (!current) {
		XFree(prop_return);

		return NULL;
	}

	size_t layout_len = strcspn(current, ",");

	if (layout_len >= sizeof(layout)) {
		layout_len = sizeof(layout) - 1;
	}

	memcpy(layout, current, layout_len);
	layout[layout_len] = '\0';
	XFree(prop_return);

	static char res[64] = {0};

	snprintf(res, sizeof(res) - 1, "lay: %s", layout);

	return layout_len > 0 ? res : NULL;
}

void draw_keyboard_layout__BlueWMBar(void)
{
	const char *layout = get_current_keyboard_layout__BlueWMBar();

	if (!layout) {
		return;
	}

	// The layout comes after the date, so it is drawn from where the date
	// ended, and the date is drawn before it.
	size_t layout_len = strlen(layout);
	int layout_x = left_block_end_x + 2 * WINDOW_PADDING;

	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, layout_x, WINDOW_MIDDLE(font), layout, layout_len);

	left_block_end_x = layout_x + XTextWidth(font, layout, layout_len);
}

bool read_file_line__BlueWMBar(const char *path, char *buffer, size_t buffer_len)
{
	FILE *f = fopen(path, "r");

	if (!f) {
		return false;
	}

	bool is_read = fgets(buffer, buffer_len, f) != NULL;

	fclose(f);

	if (!is_read) {
		return false;
	}

	// The attributes of the kernel are written with their newline.
	buffer[strcspn(buffer, "\n")] = '\0';

	return true;
}

bool read_battery_attribute__BlueWMBar(const char *battery, const char *attribute, char *buffer, size_t buffer_len)
{
	char path[PATH_MAX] = {0};

	snprintf(path, sizeof(path) - 1, POWER_SUPPLY_PATH "/%s/%s", battery, attribute);

	return read_file_line__BlueWMBar(path, buffer, buffer_len);
}

const char *find_battery__BlueWMBar(void)
{
	if (current_battery[0] != '\0') {
		return current_battery;
	}

	DIR *dir = opendir(POWER_SUPPLY_PATH);

	if (!dir) {
		return NULL;
	}

	struct dirent *entry = NULL;

	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.') {
			continue;
		}

		char attribute[64] = {0};

		if (!read_battery_attribute__BlueWMBar(entry->d_name, "type", attribute, sizeof(attribute)) || strcmp(attribute, "Battery") != 0) {
			continue;
		}

		// A mouse and a keyboard are batteries too, but they are the power
		// supply of a device and not of the machine, which is what a missing
		// scope means.
		if (read_battery_attribute__BlueWMBar(entry->d_name, "scope", attribute, sizeof(attribute)) && strcmp(attribute, "System") != 0) {
			continue;
		}

		snprintf(current_battery, sizeof(current_battery), "%s", entry->d_name);

		break;
	}

	closedir(dir);

	return current_battery[0] == '\0' ? NULL : current_battery;
}

const char *get_battery_status__BlueWMBar(bool *is_low_p)
{
	static char status[16] = {0};
	const char *battery = find_battery__BlueWMBar();

	if (!battery) {
		return NULL;
	}

	char capacity_text[16] = {0};
	char status_text[32] = {0};

	if (!read_battery_attribute__BlueWMBar(battery, "capacity", capacity_text, sizeof(capacity_text))) {
		// The battery went away, so it is looked for again on the next draw.
		current_battery[0] = '\0';

		return NULL;
	}

	int capacity = atoi(capacity_text);
	bool is_charging = read_battery_attribute__BlueWMBar(battery, "status", status_text, sizeof(status_text)) && strcmp(status_text, "Discharging") != 0;

	*is_low_p = !is_charging && capacity <= BLUE_WM_BATTERY_LOW_LEVEL;

	// A battery that is filling up is marked, as its level alone does not say
	// whether it is a worry.
	snprintf(status, sizeof(status) - 1, "bat: %s%d%%", is_charging ? "+" : "", capacity);

	return status;
}

void draw_battery__BlueWMBar(void)
{
	bool is_low = false;
	const char *status = get_battery_status__BlueWMBar(&is_low);

	// A machine without a battery has nothing to report.
	if (!status) {
		return;
	}

	size_t status_len = strlen(status);
	int status_x = left_block_end_x + 2 * WINDOW_PADDING;

	XSetForeground(display, window_gc, is_low ? BLUE_RGB(255, 0, 0) : BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, status_x, WINDOW_MIDDLE(font), status, status_len);

	left_block_end_x = status_x + XTextWidth(font, status, status_len);
}

const char *find_wifi_interface__BlueWMBar(void)
{
	if (current_wifi_interface[0] != '\0') {
		return current_wifi_interface;
	}

	DIR *dir = opendir(NET_CLASS_PATH);

	if (!dir) {
		return NULL;
	}

	struct dirent *entry = NULL;

	while ((entry = readdir(dir))) {
		if (entry->d_name[0] == '.') {
			continue;
		}

		// Only a wireless interface is given a wireless directory by the
		// kernel, which tells it apart from the wired ones and the bridges.
		char path[PATH_MAX] = {0};

		snprintf(path, sizeof(path) - 1, NET_CLASS_PATH "/%s/wireless", entry->d_name);

		DIR *wireless_dir = opendir(path);

		if (!wireless_dir) {
			continue;
		}

		closedir(wireless_dir);
		snprintf(current_wifi_interface, sizeof(current_wifi_interface), "%.*s", IFNAMSIZ, entry->d_name);

		break;
	}

	closedir(dir);

	return current_wifi_interface[0] == '\0' ? NULL : current_wifi_interface;
}

bool get_wifi_quality__BlueWMBar(const char *interface, int *quality_p)
{
	FILE *f = fopen(PROC_NET_WIRELESS_PATH, "r");

	if (!f) {
		return false;
	}

	char line[256] = {0};
	bool is_found = false;

	// The two first lines are the header of the table.
	if (fgets(line, sizeof(line), f) && fgets(line, sizeof(line), f)) {
		while (fgets(line, sizeof(line), f)) {
			char name[IFNAMSIZ + 1] = {0};
			unsigned int status = 0;
			int quality = 0;

			// The quality is written with a trailing dot, as the kernel prints
			// it as a fixed point number.
			if (sscanf(line, " %16[^:]: %x %d.", name, &status, &quality) != 3 || strcmp(name, interface) != 0) {
				continue;
			}

			*quality_p = quality;
			is_found = true;

			break;
		}
	}

	fclose(f);

	return is_found;
}

bool get_wifi_ssid__BlueWMBar(const char *interface, char *ssid, size_t ssid_len)
{
	// The wireless extensions are deprecated, but the kernel still answers them
	// for the drivers of today, and they need no library where netlink would.
	struct iwreq request = {0};

	snprintf(request.ifr_name, IFNAMSIZ, "%s", interface);
	request.u.essid.pointer = ssid;
	request.u.essid.length = ssid_len;

	int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

	if (socket_fd < 0) {
		return false;
	}

	bool is_read = ioctl(socket_fd, SIOCGIWESSID, &request) == 0;

	close(socket_fd);

	return is_read && ssid[0] != '\0';
}

const char *get_wifi_status__BlueWMBar(bool *is_low_p)
{
	static char status[BLUE_WM_WIFI_SSID_MAX_LEN + 32] = {0};
	const char *interface = find_wifi_interface__BlueWMBar();

	if (!interface) {
		return NULL;
	}

	char path[PATH_MAX] = {0};
	char operstate[16] = {0};

	snprintf(path, sizeof(path) - 1, NET_CLASS_PATH "/%s/operstate", interface);

	if (!read_file_line__BlueWMBar(path, operstate, sizeof(operstate))) {
		// The interface went away, so it is looked for again on the next draw.
		current_wifi_interface[0] = '\0';

		return NULL;
	}

	// An interface that is not up is not connected to anything.
	if (strcmp(operstate, "up") != 0) {
		return NULL;
	}

	int quality = 0;

	if (!get_wifi_quality__BlueWMBar(interface, &quality)) {
		return NULL;
	}

	int level = quality * 100 / WIFI_QUALITY_MAX;

	if (level > 100) {
		level = 100;
	}

	*is_low_p = level <= BLUE_WM_WIFI_LOW_LEVEL;

	char ssid[IW_ESSID_MAX_SIZE + 1] = {0};

	// A network can be named with up to 32 characters, which would take the
	// room of the title, so it is cut.
	if (get_wifi_ssid__BlueWMBar(interface, ssid, sizeof(ssid))) {
		snprintf(status, sizeof(status) - 1, "net: %.*s %d%%", BLUE_WM_WIFI_SSID_MAX_LEN, ssid, level);
	} else {
		snprintf(status, sizeof(status) - 1, "net: %d%%", level);
	}

	return status;
}

void draw_wifi__BlueWMBar(void)
{
	bool is_low = false;
	const char *status = get_wifi_status__BlueWMBar(&is_low);

	// A machine without a wireless interface, or one that is down, has nothing
	// to report.
	if (!status) {
		return;
	}

	size_t status_len = strlen(status);
	int status_x = left_block_end_x + 2 * WINDOW_PADDING;

	XSetForeground(display, window_gc, is_low ? BLUE_RGB(255, 0, 0) : BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, status_x, WINDOW_MIDDLE(font), status, status_len);

	left_block_end_x = status_x + XTextWidth(font, status, status_len);
}

// ALSA reports its errors on the standard error of the process, which would be
// the log of the window manager, so it is kept quiet.
static void handle_alsa_error__BlueWMBar(const char *file, int line, const char *function, int err, const char *format, ...)
{
	(void)file;
	(void)line;
	(void)function;
	(void)err;
	(void)format;
}

snd_mixer_elem_t *find_volume_element__BlueWMBar(void)
{
	if (volume_element) {
		return volume_element;
	}

	if (volume_is_unavailable) {
		return NULL;
	}

	if (!volume_mixer) {
		snd_lib_error_set_handler(&handle_alsa_error__BlueWMBar);

		if (snd_mixer_open(&volume_mixer, 0) < 0) {
			volume_mixer = NULL;
			volume_is_unavailable = true;

			return NULL;
		}

		// The default device is the one the sound server puts in front of the
		// hardware, so it carries the volume the user actually changes.
		if (snd_mixer_attach(volume_mixer, VOLUME_MIXER_DEVICE) < 0
				|| snd_mixer_selem_register(volume_mixer, NULL, NULL) < 0
				|| snd_mixer_load(volume_mixer) < 0) {
			snd_mixer_close(volume_mixer);
			volume_mixer = NULL;
			volume_is_unavailable = true;

			return NULL;
		}
	}

	snd_mixer_selem_id_t *id = NULL;

	snd_mixer_selem_id_alloca(&id);
	snd_mixer_selem_id_set_index(id, 0);
	snd_mixer_selem_id_set_name(id, VOLUME_MIXER_ELEMENT);

	volume_element = snd_mixer_find_selem(volume_mixer, id);

	return volume_element;
}

const char *get_volume_status__BlueWMBar(bool *is_muted_p)
{
	static char status[16] = {0};
	snd_mixer_elem_t *element = find_volume_element__BlueWMBar();

	// A machine whose default device has no such control, for the lack of the
	// plugin of the sound server, has nothing to report.
	if (!element) {
		return NULL;
	}

	// The mixer keeps what it was loaded with, so it is refreshed to see the
	// changes made by anything else.
	snd_mixer_handle_events(volume_mixer);

	long min = 0;
	long max = 0;
	long value = 0;

	if (snd_mixer_selem_get_playback_volume_range(element, &min, &max) < 0
			|| snd_mixer_selem_get_playback_volume(element, SND_MIXER_SCHN_FRONT_LEFT, &value) < 0
			|| max <= min) {
		return NULL;
	}

	int is_unmuted = 1;

	if (snd_mixer_selem_has_playback_switch(element)) {
		snd_mixer_selem_get_playback_switch(element, SND_MIXER_SCHN_FRONT_LEFT, &is_unmuted);
	}

	*is_muted_p = !is_unmuted;

	// The level is rounded and not truncated, so that a volume set to 40 is not
	// reported as 39.
	long level = ((value - min) * 100 + (max - min) / 2) / (max - min);

	snprintf(status, sizeof(status) - 1, *is_muted_p ? "vol: mute" : "vol: %ld%%", level);

	return status;
}

void draw_volume__BlueWMBar(void)
{
	bool is_muted = false;
	const char *status = get_volume_status__BlueWMBar(&is_muted);

	if (!status) {
		return;
	}

	size_t status_len = strlen(status);
	int status_x = left_block_end_x + 2 * WINDOW_PADDING;

	XSetForeground(display, window_gc, is_muted ? BLUE_RGB(255, 0, 0) : BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, status_x, WINDOW_MIDDLE(font), status, status_len);

	left_block_end_x = status_x + XTextWidth(font, status, status_len);
}

void close_volume__BlueWMBar(void)
{
	if (volume_mixer) {
		snd_mixer_close(volume_mixer);
		volume_mixer = NULL;
		volume_element = NULL;
	}
}

void draw_date__BlueWMBar(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1) {
		BLUE_LOG_ERROR("unable to get time of day\n");
	}

	char date[30] = {0};

	strftime(date, sizeof(date) - 1, "%Y-%m-%d %T", localtime(&tv.tv_sec));

	size_t date_len = strlen(date);

	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	XDrawString(display, window_pixels, window_gc, WINDOW_PADDING, WINDOW_MIDDLE(font), date, date_len);

	left_block_end_x = WINDOW_PADDING + XTextWidth(font, date, date_len);
}

void draw_window_title__BlueWMBar(void)
{
	if (!current_window_title) {
		return;
	}

	if (resizing_window) {
		XSetForeground(display, window_gc, BLUE_RGB(255, 0, 0));
	} else {
		XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	}

	// The title has the bar to itself only between what is drawn on its left,
	// the date and the layout, and the numbers of the workspaces on its right.
	int title_min_x = left_block_end_x + 2 * WINDOW_PADDING;
	int title_max_x = WINDOW_WORKSPACES_X(font) - 2 * WINDOW_PADDING;
	int title_max_width = title_max_x - title_min_x;

	if (title_max_width <= 0) {
		return;
	}

	// A title too long for that space is cut, so that it never runs over its
	// neighbours.
	size_t title_len = current_window_title_len;

	while (title_len > 0 && XTextWidth(font, current_window_title, title_len) > title_max_width) {
		--title_len;
	}

	if (title_len == 0) {
		return;
	}

	// The title is centered on its width in pixels, not on its number of
	// characters, and it is kept centered on the whole bar as long as it fits
	// there.
	int title_width = XTextWidth(font, current_window_title, title_len);
	int title_x = ((int)window_width - title_width) / 2;

	if (title_x < title_min_x) {
		title_x = title_min_x;
	} else if (title_x + title_width > title_max_x) {
		title_x = title_max_x - title_width;
	}

	XDrawString(display, window_pixels, window_gc, title_x, WINDOW_MIDDLE(font), current_window_title, title_len);
}

void draw_workspaces_number__BlueWMBar(void)
{
	for (int i = 0; i < BLUE_WM_WORKSPACE_NUMBER; ++i) {
		// The workspaces are laid out as on the keyboard row: 1, 2, ..., 9, 0.
		int workspace_number = (i + 1) % BLUE_WM_WORKSPACE_NUMBER;
		char number[10] = {0};

		snprintf(number, sizeof(number) - 1, "%d", workspace_number);

		if (active_workspace == workspace_number) {
			XSetForeground(display, window_gc, BLUE_RGB(252, 184, 2));
		} else {
			XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
		}

		XDrawString(display, window_pixels, window_gc, WINDOW_WORKSPACES_X(font) + WINDOW_WORKSPACE_SPACE_FACTOR * font->ascent * i, WINDOW_MIDDLE(font), number, strlen(number));
	}
}

void draw__BlueWMBar(void)
{	
	draw_bg__BlueWMBar();
	draw_date__BlueWMBar();
	draw_keyboard_layout__BlueWMBar();
	draw_battery__BlueWMBar();
	draw_wifi__BlueWMBar();
	draw_volume__BlueWMBar();
	draw_window_title__BlueWMBar();
	draw_workspaces_number__BlueWMBar();
	XCopyArea(display, window_pixels, window, window_gc, 0, 0, window_width, window_height, 0, 0);
	XFlush(display);
}

void set_font__BlueWMBar(void)
{
	if (!(font = XLoadQueryFont(display, "fixed"))) {
		BLUE_LOG_ERROR("unable to load font\n");
	}

	XSetFont(display, window_gc, font->fid);
}

void handle_active_window_notify__BlueWMBar(const XEvent *event)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, event->xproperty.window, active_window_atom, 0, 1, false, XA_WINDOW, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_WINDOW && actual_format == 32 && nitems_return == 1 && prop_return) {
		if (current_window != None) {
			// Stop to receive event from this window
			XSelectInput(display, current_window, 0);
		}

		current_window = *(Window *)prop_return;

		if (current_window != None) {
			XSelectInput(display, current_window, PropertyChangeMask | StructureNotifyMask);
		}

		fetch_window_title__BlueWMBar();
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void handle_resize_window_notify__BlueWMBar(const XEvent *event)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, event->xproperty.window, resize_window_atom, 0, 1, false, XA_WINDOW, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_WINDOW && actual_format == 32 && nitems_return == 1 && prop_return) {
		Window resized_window = *(Window*)prop_return;

		if (resized_window == None) {
			resizing_window = false;
		} else {
			resizing_window = true;
		}
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void fetch_active_workspace__BlueWMBar(Window window)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems_return;
	unsigned long bytes_after_return;
	unsigned char *prop_return;
	int status = XGetWindowProperty(display, window, active_workspace_atom, 0, 1, false, XA_INTEGER, &actual_type, &actual_format, &nitems_return, &bytes_after_return, &prop_return);

	if (status == Success && actual_type == XA_INTEGER && actual_format == 32 && nitems_return == 1 && prop_return) {
		active_workspace = *(int*)prop_return;
	}

	if (prop_return) {
		XFree(prop_return);
	}
}

void handle_active_workspace_notify__BlueWMBar(const XEvent *event)
{
	fetch_active_workspace__BlueWMBar(event->xproperty.window);
}

void fetch_window_title__BlueWMBar(void)
{
	if (current_window_title) {
		XFree(current_window_title);
		current_window_title = NULL;
		current_window_title_len = 0;
	}

	if (current_window == None) {
		return;
	}

	XFetchName(display, current_window, &current_window_title);

	if (current_window_title) {
		current_window_title_len = strlen(current_window_title);
	}
}

void handle_wm_name_notify__BlueWMBar(void)
{
	fetch_window_title__BlueWMBar();
}

void handle_events__BlueWMBar(void)
{
	XEvent event;
	time_t start;

	while (true) {
		while (XPending(display) > 0) {
			XNextEvent(display, &event);

			switch (event.type) {
				case Expose:
					draw__BlueWMBar();

					break;
				case PropertyNotify:
					if (event.xproperty.atom == active_window_atom) {
						handle_active_window_notify__BlueWMBar(&event);
					} else if (event.xproperty.atom == wm_name_atom) {
						handle_wm_name_notify__BlueWMBar();
					} else if (event.xproperty.atom == resize_window_atom) {
						handle_resize_window_notify__BlueWMBar(&event);
					} else if (event.xproperty.atom == active_workspace_atom) {
						handle_active_workspace_notify__BlueWMBar(&event);
					}

					break;
				case UnmapNotify:
					if (event.xunmap.window == current_window) {
						current_window = None;
					}

					break;
				default:
					// The state of the keyboard changed, and its group with it,
					// so the layout on the bar is not the one in use anymore.
					if (event.type == xkb_event_base) {
						draw__BlueWMBar();
					}

					break;
			}
		}

		draw__BlueWMBar();
		time(&start);

		time_t current;

		do {
			time(&current);
			usleep(100000);
		} while (difftime(start, current) && XPending(display) == 0);
	}
}

int handle_error__BlueWMBar(Display *error_display, XErrorEvent *error)
{
	// A window can be destroyed between the moment the WM notifies it and the
	// moment it is used here, and the bar must not die with it.
	if (error->error_code == BadWindow) {
		return 0;
	}

	char message[256] = {0};

	XGetErrorText(error_display, error->error_code, message, sizeof(message));
	BLUE_LOG_WARNING("%s\n", message);

	return 0;
}

void close__BlueWMBar(void)
{
	close_volume__BlueWMBar();
	XFreeGC(display, window_gc);
	XFreePixmap(display, window_pixels);
	XFreeFont(display, font);
	XCloseDisplay(display);
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	XSetErrorHandler(&handle_error__BlueWMBar);

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes\n");
	}

	window_width = window_root_attr.width;
	window_height = WINDOW_HEIGHT;
	Screen *default_screen = DefaultScreenOfDisplay(display);

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), window_bg_color);
	window_gc = XCreateGC(display, window, 0, NULL);
	window_pixels = XCreatePixmap(display, window, window_width, window_height, DefaultDepthOfScreen(default_screen));

	if (XSetGraphicsExposures(display, window_gc, false) == 0) {
		BLUE_LOG_ERROR("cannot set graphics exposures\n");
	}

	// https://specifications.freedesktop.org/wm/latest/ar01s05.html
	Atom window_type_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE", false);
	Atom dock_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", false);

	// The role of a window is advertised through _NET_WM_WINDOW_TYPE, and not
	// through WM_PROTOCOLS, which only carries the protocols the client
	// understands.
	XChangeProperty(display, window, window_type_atom, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock_atom, 1);

	active_window_atom = XInternAtom(display, "_NET_ACTIVE_WINDOW", false);
	wm_name_atom = XInternAtom(display, "_NET_WM_NAME", false);
	resize_window_atom = XInternAtom(display, "_NET_WM_ACTION_RESIZE", false);
	active_workspace_atom = XInternAtom(display, "_BLUE_WM_ACTIVE_WORKSPACE", false);
	xkb_rules_names_atom = XInternAtom(display, "_XKB_RULES_NAMES", false);

	fetch_active_workspace__BlueWMBar(window_root);

	set_font__BlueWMBar();
	// The change of the group of the keyboard is only reported by XKB, so the
	// bar follows a switch of layout as it happens, and not on its next second.
	int xkb_major = XkbMajorVersion;
	int xkb_minor = XkbMinorVersion;

	if (XkbQueryExtension(display, NULL, &xkb_event_base, NULL, &xkb_major, &xkb_minor)) {
		XkbSelectEventDetails(display, XkbUseCoreKbd, XkbStateNotify, XkbAllStateComponentsMask, XkbGroupStateMask);
	} else {
		BLUE_LOG_WARNING("xkb is missing, the keyboard layout will only be refreshed every second\n");
	}

	XSelectInput(display, window, ExposureMask | FocusChangeMask);
	XSelectInput(display, window_root, PropertyChangeMask);
	XMapWindow(display, window);
	handle_events__BlueWMBar();
	close__BlueWMBar();
}
