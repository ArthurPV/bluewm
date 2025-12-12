#ifndef BLUEWM_H
#define BLUEWM_H

#undef BLUE_LOG_ERROR
#undef BLUE_ZERO_ALLOC
#undef BLUE_RGB

#define BLUE_LOG_ERROR(msg, ...) \
	fprintf(stderr, msg, ##__VA_ARGS__); \
	exit(1);

#define BLUE_ZERO_ALLOC(size) ({ \
	void *_mem = calloc(1, size); \
\
	if (!_mem) { \
		BLUE_LOG_ERROR("unable to allocate memory"); \
	} \
\
	_mem; \
})

#define BLUE_RGB(r, g, b) (r << 16 | g << 8 | b)

#endif // BLUEWM_H
