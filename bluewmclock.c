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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include <bluewm.h>

#define NB_HOURS (12)
#define NB_MINUTES (60)
#define CIRCLE_TOTAL_ANGLE (360)
#define START_ANGLE (90)

// https://en.wikipedia.org/wiki/Radian
#define ANGLE_TO_RADIAN(angle) (angle * (M_PI / 180))
#define RADIUS (window_height / 2)
#define ANGLE_BY_HOUR (CIRCLE_TOTAL_ANGLE / NB_HOURS)

static Display *display = NULL;
static Window window = 0;
static GC window_gc = {0};
static const unsigned int window_width = 500;
static const unsigned int window_height = 500;

static void draw_circle__BlueWMClock(void);

static void draw_line_with_angle__BlueWMClock(int x, int y, int length, int angle);

static void calculate_position_according_angle__BlueWMClock(int radius, int angle, int *x, int *y);

static void draw_hour_lines__BlueWMClock(void);

static void draw_hands__BlueWMClock(void);

static void draw_hour_hand__BlueWMClock(int hour, int minute);

static void draw_minute_hand__BlueWMClock(int minute);

static void draw_second_hand__BlueWMClock(int second);

static void draw__BlueWMClock(void);

static void handle_events__BlueWMClock(void);

static void close__BlueWMClock(void);

void draw_circle__BlueWMClock(void)
{
	XDrawArc(display, window, window_gc, 0, 0, window_width - 1, window_height - 1, CIRCLE_TOTAL_ANGLE * 64, CIRCLE_TOTAL_ANGLE * 64);
}

void draw_line_with_angle__BlueWMClock(int x, int y, int length, int angle)
{
	float angle_radian = ANGLE_TO_RADIAN(angle);
	float x2 = x + cos(angle_radian) * length;
	float y2 = y + sin(angle_radian) * length;

	XDrawLine(display, window, window_gc, x, y, round(x2), round(y2));
}

void calculate_position_according_angle__BlueWMClock(int radius, int angle, int *x, int *y)
{
	float angle_radian = ANGLE_TO_RADIAN(angle);

	*x = round(radius + radius * cos(angle_radian));
	*y = round(radius + radius * sin(angle_radian));
}

void draw_hour_lines__BlueWMClock(void)
{
	int radius = RADIUS;
	int line_length = 30;

	for (int hour = 1; hour <= NB_HOURS; ++hour) {
		int current_angle = ANGLE_BY_HOUR * hour - START_ANGLE;
		int x, y;

		calculate_position_according_angle__BlueWMClock(radius, current_angle, &x, &y);
		draw_line_with_angle__BlueWMClock(x, y, -line_length, current_angle);
	}
}

void draw_hands__BlueWMClock(void)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1) {
		BLUE_LOG_ERROR("unable to get time of day\n");
	}

	struct tm *lt = localtime(&tv.tv_sec);

	draw_hour_hand__BlueWMClock(lt->tm_hour, lt->tm_min);
	draw_minute_hand__BlueWMClock(lt->tm_min);
	draw_second_hand__BlueWMClock(lt->tm_sec);
}

void draw_hour_hand__BlueWMClock(int hour, int minute)
{
	int hour_12_format = hour % NB_HOURS;
	int radius = RADIUS;
	int hand_length = 120;
	const float angle_by_hour = ANGLE_BY_HOUR;
	int current_angle = (angle_by_hour * hour_12_format) - START_ANGLE;
	float angle_by_minute = angle_by_hour / NB_MINUTES;

	current_angle += angle_by_minute * minute;

	draw_line_with_angle__BlueWMClock(radius, radius, hand_length, current_angle);
}

void draw_minute_hand__BlueWMClock(int minute)
{
	int radius = RADIUS;
	int hand_length = 200;
	const int angle_by_minute = CIRCLE_TOTAL_ANGLE / NB_MINUTES;
	int current_angle = (angle_by_minute * minute) - START_ANGLE;

	draw_line_with_angle__BlueWMClock(radius, radius, hand_length, current_angle);
}

void draw_second_hand__BlueWMClock(int second)
{
#define NB_SECONDS 60
	int radius = RADIUS;
	int hand_length = 200;
	const int angle_by_second = CIRCLE_TOTAL_ANGLE / NB_SECONDS;
	int current_angle = (angle_by_second * second) - START_ANGLE;

	XSetForeground(display, window_gc, BLUE_RGB(240, 57, 57));

	draw_line_with_angle__BlueWMClock(radius, radius, hand_length, current_angle);
#undef NB_SECONDS
}

void draw__BlueWMClock(void)
{
	XClearWindow(display, window);
	XSetForeground(display, window_gc, BLUE_RGB(255, 255, 255));
	draw_circle__BlueWMClock();
	draw_hour_lines__BlueWMClock();
	draw_hands__BlueWMClock();
}

void handle_events__BlueWMClock(void)
{
	XEvent event;
	time_t start;

	while (true) {
		while (XPending(display) > 0) {
			XNextEvent(display, &event);

			switch (event.type) {
				case Expose:
					draw__BlueWMClock();

					break;
				default:
					break;
			}
		}

		draw__BlueWMClock();
		time(&start);

		time_t current;

		do {
			time(&current);
			usleep(100000);
		} while (difftime(start, current) && XPending(display) == 0);
	}
}

void close__BlueWMClock(void)
{
	XFreeGC(display, window_gc);
	XCloseDisplay(display);
}

int main() {
	if (!(display = XOpenDisplay(NULL))) {
		BLUE_LOG_ERROR("unable to open display\n");
	}

	Window window_root = XDefaultRootWindow(display);

	window = XCreateSimpleWindow(display, window_root, 0, 0, window_width, window_height, 0, BLUE_RGB(0, 0, 0), BLUE_RGB(0, 0, 0));
	window_gc = XCreateGC(display, window, 0, NULL);

	XSelectInput(display, window, ExposureMask);
	XMapWindow(display, window);
	handle_events__BlueWMClock();
	close__BlueWMClock();
}
