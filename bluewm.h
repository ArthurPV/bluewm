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

#ifndef BLUEWM_H
#define BLUEWM_H

#undef BLUE_LOG_ERROR

#define BLUE_LOG_ERROR(msg, ...) \
	fprintf(stderr, "Error(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__); \
	exit(1);

#undef BLUE_LOG_UNREACHABLE

#define BLUE_LOG_UNREACHABLE(msg, ...) \
	fprintf(stderr, "Unreachable(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__); \
	exit(1);

#undef BLUE_LOG_WARNING

#define BLUE_LOG_WARNING(msg, ...) \
	fprintf(stderr, "Warning(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__);

#undef BLUE_F_ALLOC

#define BLUE_F_ALLOC(f, ...) ({ \
	void *_mem = f(__VA_ARGS__); \
\
	if (!_mem) { \
		BLUE_LOG_ERROR("unable to allocate memory"); \
	} \
\
	_mem; \
})

#undef BLUE_ZERO_ALLOC

#define BLUE_ZERO_ALLOC(size) BLUE_F_ALLOC(calloc, 1, size)

#undef BLUE_ALLOC

#define BLUE_ALLOC(size) BLUE_F_ALLOC(malloc, size)

#undef BLUE_RGB

#define BLUE_RGB(r, g, b) ((r) << 16 | (g) << 8 | (b))

#undef BLUE_WM_WORKSPACE_NUMBER

#define BLUE_WM_WORKSPACE_NUMBER 10

#undef BLUE_WM_BATTERY_LOW_LEVEL

// Level under which the battery is drawn as low on the bar, in percent.
#define BLUE_WM_BATTERY_LOW_LEVEL 20

#undef BLUE_WM_WIFI_LOW_LEVEL

// Signal under which the wifi is drawn as low on the bar, in percent.
#define BLUE_WM_WIFI_LOW_LEVEL 30

#undef BLUE_WM_WIFI_SSID_MAX_LEN

// A network can be named with up to 32 characters, so the name is cut to this
// length to leave room for the title of the window.
#define BLUE_WM_WIFI_SSID_MAX_LEN 16

#endif // BLUEWM_H
