#pragma once
#include <lvgl.h>

// Simple 7-segment display widget for LVGL v8.
// Creates a container with 7 segment rectangles + a dot. No custom LVGL class used — composed with children.

// Create a 7-segment widget. The returned object is the parent container.
// `w` and `h` are the container width/height in pixels (recommended: w ~ 0.5*h).
lv_obj_t * lv_7seg_create(lv_obj_t * parent, lv_coord_t w, lv_coord_t h);

// Set the displayed digit (0-9). If out of range, the display is cleared.
void lv_7seg_set_digit(lv_obj_t * obj, const char digit, const bool dot);
