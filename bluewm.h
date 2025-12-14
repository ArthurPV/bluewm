#ifndef BLUEWM_H
#define BLUEWM_H

#undef BLUE_LOG_ERROR
#undef BLUE_ZERO_ALLOC
#undef BLUE_RGB

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

#endif // BLUEWM_H
