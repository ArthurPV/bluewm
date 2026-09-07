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

#define BLUE_LOG_ERROR(msg, ...) \
	fprintf(stderr, "Error(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__); \
	exit(1);

#define BLUE_LOG_UNREACHABLE(msg, ...) \
	fprintf(stderr, "Unreachable(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__); \
	exit(1);

#define BLUE_LOG_WARNING(msg, ...) \
	fprintf(stderr, "Warning(%s:%d): "msg, __FILE__, __LINE__, ##__VA_ARGS__);

#define BLUE_F_ALLOC(f, ...) ({ \
	void *_mem = f(__VA_ARGS__); \
\
	if (!_mem) { \
		BLUE_LOG_ERROR("unable to allocate memory"); \
	} \
\
	_mem; \
})

#define BLUE_ZERO_ALLOC(size) BLUE_F_ALLOC(calloc, 1, size)

#define BLUE_ALLOC(size) BLUE_F_ALLOC(malloc, size)

#define BLUE_RGB(r, g, b) ((r) << 16 | (g) << 8 | (b))

#define BLUE_WM_WORKSPACE_NUMBER 10

#endif // BLUEWM_H
