#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <spng.h>
#include <jpeglib.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <bluewm.h>

Display *display = NULL;
Window window = 0;
unsigned int window_width = 0;
unsigned int window_height = 0;

static void read_jpg_image__BlueWMBg(const char *path, size_t *image_width_p, size_t *image_height_p, uint8_t **image_pixels_p);

static void read_png_image__BlueWMBg(const char *path, size_t *image_width_p, size_t *image_height_p, uint8_t **image_pixels_p);

static void put_image_pixels__BlueWMBg(size_t image_width, size_t image_height, uint8_t *image_pixels, XImage **ximage_p);

static inline void put_image__BlueWMBg(XImage *ximage);

static void read_and_put_image__BlueWMBg(const char *path);

static void close__BlueWMBg(void);

void read_jpg_image__BlueWMBg(const char *path, size_t *image_width_p, size_t *image_height_p, uint8_t **image_pixels_p)
{
	FILE *f = fopen(path, "rb");

	if (!f) {
		BLUE_LOG_ERROR("something went wrong: %s\n", strerror(errno));
	}

	struct jpeg_error_mgr jpeg_error;
	struct jpeg_decompress_struct jpeg_decompress;

	jpeg_decompress.err = jpeg_std_error(&jpeg_error);

	jpeg_create_decompress(&jpeg_decompress);
	jpeg_stdio_src(&jpeg_decompress, f);
	jpeg_read_header(&jpeg_decompress, true);

	jpeg_decompress.out_color_space = JCS_RGB;

	jpeg_start_decompress(&jpeg_decompress);

	size_t image_width = jpeg_decompress.output_width;
	size_t image_height = jpeg_decompress.output_height;
	uint8_t *image_pixels = BLUE_ALLOC(image_width * jpeg_decompress.output_components * image_height);

	*image_width_p = image_width;
	*image_height_p = image_height;
	*image_pixels_p = image_pixels;

	while (jpeg_decompress.output_scanline < jpeg_decompress.output_height) {
		JSAMPROW line = image_pixels + jpeg_decompress.output_scanline * image_width * jpeg_decompress.output_components;

		jpeg_read_scanlines(&jpeg_decompress, &line, 1);
	}

	jpeg_finish_decompress(&jpeg_decompress);
	jpeg_destroy_decompress(&jpeg_decompress);
	fclose(f);
}

void read_png_image__BlueWMBg(const char *path, size_t *image_width_p, size_t *image_height_p, uint8_t **image_pixels_p)
{
	FILE *f = fopen(path, "rb");

	if (!f) {
		BLUE_LOG_ERROR("something went wrong: %s\n", strerror(errno));
	}

	spng_ctx *ctx = spng_ctx_new(0);

	if (!ctx) {
		BLUE_LOG_ERROR("not enough memory\n");
	}

	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);
	spng_set_png_file(ctx, f);

	struct spng_ihdr ihdr;

	if (spng_get_ihdr(ctx, &ihdr) != 0) {
		BLUE_LOG_ERROR("failed to get header of image\n");
	}

	*image_width_p = ihdr.width;
	*image_height_p = ihdr.height;

	size_t image_size = 0;

	spng_decoded_image_size(ctx, SPNG_FMT_RGBA8, &image_size);

	*image_pixels_p = BLUE_ALLOC(image_size);

	if (spng_decode_image(ctx, *image_pixels_p, image_size, SPNG_FMT_RGBA8, 0) != 0) {
		BLUE_LOG_ERROR("failed to decode image\n");
	}

	spng_ctx_free(ctx);
	fclose(f);
}

void put_image_pixels__BlueWMBg(size_t image_width, size_t image_height, uint8_t *image_pixels, XImage **ximage_p)
{
	int screen_number = XDefaultScreen(display);
	Visual *visual = XDefaultVisual(display, screen_number);
	int depth = XDefaultDepth(display, screen_number);

	char *ximage_buffer = BLUE_ZERO_ALLOC(image_width * image_height);
	XImage *ximage = XCreateImage(display, visual, depth, ZPixmap, 0, ximage_buffer, image_width, image_height, 32, 0);

	for (size_t y = 0; y < image_height; ++y) {
		for (size_t x = 0; x < image_width; ++x) {
			uint8_t *slice = image_pixels + (y * image_width + x * 3);
			unsigned long pixel = slice[0] << 16 | slice[1] << 8 | slice[2];

			XPutPixel(ximage, x, y, pixel);
		}
	}

	*ximage_p = ximage;
}

void put_image__BlueWMBg(XImage *ximage)
{
	GC window_gc = XCreateGC(display, window, 0, NULL);

	XPutImage(display, window, window_gc, ximage, 0, 0, 0, 0, window_width, window_height);
	XFlush(display);
	XFreeGC(display, window_gc);
}

void read_and_put_image__BlueWMBg(const char *path)
{
	size_t image_width;
	size_t image_height;
	uint8_t *image_pixels;

	if (strstr(path, ".png")) {
		read_png_image__BlueWMBg(path, &image_width, &image_height, &image_pixels);
	} else if (strstr(path, ".jpg")) {
		read_jpg_image__BlueWMBg(path, &image_width, &image_height, &image_pixels);
	} else {
		BLUE_LOG_ERROR("unsupported image format");
	}

	XImage *ximage = NULL;

	put_image_pixels__BlueWMBg(image_width, image_height, image_pixels, &ximage);
	put_image__BlueWMBg(ximage);

	XDestroyImage(ximage);
	// free(image_pixels);
}

void close__BlueWMBg(void)
{
	XCloseDisplay(display);
}

int main(int argc, char **argv) {
	if (argc < 2) {
		BLUE_LOG_ERROR("expected to have a path\n");
	}

	const char *path = argv[1];

	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	Window window_root = XDefaultRootWindow(display);
	XWindowAttributes window_root_attr;

	if (XGetWindowAttributes(display, window_root, &window_root_attr) == 0) {
		BLUE_LOG_ERROR("unable to get window attributes\n");
	}

	window_width = window_root_attr.width;
	window_height = window_root_attr.height;
	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), BLUE_RGB(255, 255, 255));

	// https://specifications.freedesktop.org/wm/latest/ar01s05.html
	Atom splash_atom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_SPLASH", false);

	if (XSetWMProtocols(display, window, &splash_atom, 1) == 0) {
		BLUE_LOG_ERROR("cannot set WM_PROTOCOLS\n");
	}

	XMapWindow(display, window);
	read_and_put_image__BlueWMBg(path);

	while (true) {
		sleep(1);
	}

	close__BlueWMBg();
}
