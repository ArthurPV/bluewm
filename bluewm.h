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

#endif // BLUEWM_H
