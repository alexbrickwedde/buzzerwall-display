#include "lv_7seg.h"
#include "esp_log.h"

static const uint8_t SEGMENTS_FOR_DIGIT[10] = {
    // bits: gfedcba  (bit0 = a, bit1 = b, ... bit6 = g)
    // 0
    0b00111111,
    // 1
    0b00000110,
    // 2
    0b01011011,
    // 3
    0b01001111,
    // 4
    0b01100110,
    // 5
    0b01101101,
    // 6
    0b01111101,
    // 7
    0b00000111,
    // 8
    0b01111111,
    // 9
    0b01101111
};

// Helper: set segment (child index) on/off
static void set_seg_on(lv_obj_t * cont, int child_index, bool on)
{
    lv_obj_t * child = lv_obj_get_child(cont, child_index);
    if (!child) {
      ESP_LOGE("", "7seg: no seg %d", child_index);
      return;
    }

    if (on) {
        lv_obj_clear_flag(child, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_opa(child, LV_OPA_COVER, 0);
    } else {
        lv_obj_add_flag(child, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t * lv_7seg_create(lv_obj_t * parent, lv_coord_t w, lv_coord_t h)
{
    static lv_style_t s_on;
    static lv_style_t s_bg;
    static bool styles_init = false;
    if (!styles_init) {
        lv_style_init(&s_on);
        lv_style_set_bg_color(&s_on, lv_color_hex(0x000000));
        lv_style_set_radius(&s_on, LV_RADIUS_CIRCLE);
        lv_style_set_pad_all(&s_on, 0);

        lv_style_init(&s_bg);
        lv_style_set_bg_opa(&s_bg, LV_OPA_TRANSP);
        lv_style_set_radius(&s_bg, 0);
        lv_style_set_pad_all(&s_bg, 0);
        styles_init = true;
    }

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_set_size(cont, w, h);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_add_style(cont, &s_bg, 0);

    // thickness relative to height
    lv_coord_t thick = lv_coord_t(h / 10);
    if (thick < 2) thick = 2;

    // Create segments. We'll use simple rectangles and position them with fixed offsets.
    // seg a (top) -- horizontal
    lv_obj_t * seg_a = lv_obj_create(cont);
    lv_obj_set_size(seg_a, w - 2 * thick, thick);
    lv_obj_align(seg_a, LV_ALIGN_TOP_MID, 0, thick/2);
    lv_obj_add_style(seg_a, &s_on, 0);

    // seg b (top-right)
    lv_obj_t * seg_b = lv_obj_create(cont);
    lv_obj_set_size(seg_b, thick, h/2 - thick * 1.50);
    lv_obj_align(seg_b, LV_ALIGN_TOP_RIGHT, -thick/2, thick * 1.25);
    lv_obj_add_style(seg_b, &s_on, 0);

    // seg c (bottom-right)
    lv_obj_t * seg_c = lv_obj_create(cont);
    lv_obj_set_size(seg_c, thick, h/2 - thick * 1.5);
    lv_obj_align(seg_c, LV_ALIGN_BOTTOM_RIGHT, -thick/2, -thick * 1.25);
    lv_obj_add_style(seg_c, &s_on, 0);

    // seg d (bottom)
    lv_obj_t * seg_d = lv_obj_create(cont);
    lv_obj_set_size(seg_d, w - 2*thick, thick);
    lv_obj_align(seg_d, LV_ALIGN_BOTTOM_MID, 0, -thick/2);
    lv_obj_add_style(seg_d, &s_on, 0);

    // seg e (bottom-left)
    lv_obj_t * seg_e = lv_obj_create(cont);
    lv_obj_set_size(seg_e, thick, h/2 - thick * 1.5);
    lv_obj_align(seg_e, LV_ALIGN_BOTTOM_LEFT, thick/2, -thick * 1.25);
    lv_obj_add_style(seg_e, &s_on, 0);

    // seg f (top-left)
    lv_obj_t * seg_f = lv_obj_create(cont);
    lv_obj_set_size(seg_f, thick, h/2 - thick * 1.5);
    lv_obj_align(seg_f, LV_ALIGN_TOP_LEFT, thick/2, thick * 1.25);
    lv_obj_add_style(seg_f, &s_on, 0);

    // seg g (middle horizontal)
    lv_obj_t * seg_g = lv_obj_create(cont);
    lv_obj_set_size(seg_g, w - 2*thick, thick);
    lv_obj_align(seg_g, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(seg_g, &s_on, 0);

    // dot
    lv_obj_t * seg_dot = lv_obj_create(cont);
    lv_coord_t dot_sz = thick * 1.2;
    if (dot_sz < 3) dot_sz = 3;
    lv_obj_set_size(seg_dot, dot_sz, dot_sz);
    lv_obj_align(seg_dot, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_style(seg_dot, &s_on, 0);

    lv_obj_add_flag(seg_b, LV_OBJ_FLAG_HIDDEN); // we'll set proper via set_digit below
    // Hide then call set_digit to set correct segments
    lv_7seg_set_digit(cont, ' ', false);

    return cont;
}

void lv_7seg_set_digit(lv_obj_t * obj, const char digit, const bool dot)
{
    if (!obj) return;

    uint8_t bits = 0;
    switch(digit) {
      case 0: bits = SEGMENTS_FOR_DIGIT[0]; break;
      case 1: bits = SEGMENTS_FOR_DIGIT[1]; break;
      case 2: bits = SEGMENTS_FOR_DIGIT[2]; break;
      case 3: bits = SEGMENTS_FOR_DIGIT[3]; break;
      case 4: bits = SEGMENTS_FOR_DIGIT[4]; break;
      case 5: bits = SEGMENTS_FOR_DIGIT[5]; break;
      case 6: bits = SEGMENTS_FOR_DIGIT[6]; break;
      case 7: bits = SEGMENTS_FOR_DIGIT[7]; break;
      case 8: bits = SEGMENTS_FOR_DIGIT[8]; break;
      case 9: bits = SEGMENTS_FOR_DIGIT[9]; break;
        case '0': bits = SEGMENTS_FOR_DIGIT[0]; break;
        case '1': bits = SEGMENTS_FOR_DIGIT[1]; break;
        case '2': bits = SEGMENTS_FOR_DIGIT[2]; break;
        case '3': bits = SEGMENTS_FOR_DIGIT[3]; break;
        case '4': bits = SEGMENTS_FOR_DIGIT[4]; break;
        case '5': bits = SEGMENTS_FOR_DIGIT[5]; break;
        case '6': bits = SEGMENTS_FOR_DIGIT[6]; break;
        case '7': bits = SEGMENTS_FOR_DIGIT[7]; break;
        case '8': bits = SEGMENTS_FOR_DIGIT[8]; break;
        case '9': bits = SEGMENTS_FOR_DIGIT[9]; break;
        case ' ': bits = 0; break;
        case '-': bits = 0b01000000; break;
        default: bits = 0; // clear all segments
    }

    // bit 0 = a -> child index 0
    for (int i = 0; i < 7; ++i) {
        bool on = (bits >> i) & 0x1;
        set_seg_on(obj, i, on);
    }
    set_seg_on(obj, 7, dot);
}
