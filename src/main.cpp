#include <Arduino.h>
#include <esp_display_panel.hpp>

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <demos/lv_demos.h>
#include "lv_7seg.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

uint32_t x = 0;
lv_obj_t * seven1;
lv_obj_t * seven2;
lv_obj_t * seven3;
lv_obj_t * seven4;
lv_obj_t * seven5;

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
    }
    else if(code == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGE("x", "Toggled");
    }
}

void setup()
{
    String title = "LVGL porting example";

    Serial.begin(115200);

    Serial.println("Initializing board");
    Board *board = new Board();
    board->init();

    #if LVGL_PORT_AVOID_TEARING_MODE
    auto lcd = board->getLCD();
    // When avoid tearing function is enabled, the frame buffer number should be set in the board driver
    lcd->configFrameBufferNumber(LVGL_PORT_DISP_BUFFER_NUM);
#if ESP_PANEL_DRIVERS_BUS_ENABLE_RGB && CONFIG_IDF_TARGET_ESP32S3
    auto lcd_bus = lcd->getBus();
    /**
     * As the anti-tearing feature typically consumes more PSRAM bandwidth, for the ESP32-S3, we need to utilize the
     * "bounce buffer" functionality to enhance the RGB data bandwidth.
     * This feature will consume `bounce_buffer_size * bytes_per_pixel * 2` of SRAM memory.
     */
    if (lcd_bus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
        static_cast<BusRGB *>(lcd_bus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
    }
#endif
#endif
    assert(board->begin());

    Serial.println("Initializing LVGL");
    lvgl_port_init(board->getLCD(), board->getTouch());

    Serial.println("Creating UI");

    lvgl_port_lock(-1);

    /**
     * Create the simple labels
     */
    lv_obj_t *label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, "Hello World!");
    lv_obj_set_style_text_font(label_1, &lv_font_montserrat_16, 0);
    lv_obj_align(label_1, LV_ALIGN_CENTER, 0, -300);

    lv_obj_t *label_2 = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label_2, "ESP32_Display_Panel (%d.%d.%d)", ESP_PANEL_VERSION_MAJOR, ESP_PANEL_VERSION_MINOR, ESP_PANEL_VERSION_PATCH);
    lv_obj_set_style_text_font(label_2, &lv_font_montserrat_16, 0);
    lv_obj_align_to(label_2, label_1, LV_ALIGN_OUT_BOTTOM_MID, 0, 90);

    lv_obj_t *label_3 = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label_3, "1.234 ms");
    lv_obj_set_style_text_font(label_3, &lv_font_montserrat_42, 0);
    lv_obj_align_to(label_3, label_2, LV_ALIGN_OUT_BOTTOM_MID, 0, 90);


    seven1 = lv_7seg_create(lv_scr_act(), 150, 320);
    lv_obj_align(seven1, LV_ALIGN_TOP_LEFT, 5, 5);

    seven2 = lv_7seg_create(lv_scr_act(), 150, 320);
    lv_obj_align(seven2, LV_ALIGN_TOP_LEFT, 5 + 155, 5);

    seven3 = lv_7seg_create(lv_scr_act(), 150, 320);
    lv_obj_align(seven3, LV_ALIGN_TOP_LEFT, 5 + 155 * 2, 5);

    seven4 = lv_7seg_create(lv_scr_act(), 150, 320);
    lv_obj_align(seven4, LV_ALIGN_TOP_LEFT, 5 + 155 * 3, 5);

    seven5 = lv_7seg_create(lv_scr_act(), 150, 320  );
    lv_obj_align(seven5, LV_ALIGN_TOP_LEFT, 5 + 155 * 4, 5);


    lv_obj_t * label;

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn1, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);

    label = lv_label_create(btn1);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(btn2, event_handler, LV_EVENT_ALL, NULL);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 40);
    lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_height(btn2, LV_SIZE_CONTENT);

    label = lv_label_create(btn2);
    lv_label_set_text(label, "Toggle");
    lv_obj_center(label);

    lvgl_port_unlock();
}

void loop()
{
    Serial.println("IDLE loop");
    x = millis();
    lvgl_port_lock(-1);
    lv_7seg_set_digit(seven1, (x / 10000) % 10, false);
    lv_7seg_set_digit(seven2, (x / 1000) % 10, true);
    lv_7seg_set_digit(seven3, (x / 100) % 10, false);
    lv_7seg_set_digit(seven4, (x / 10) % 10, false);
    lv_7seg_set_digit(seven5, x % 10, false);
    lvgl_port_unlock();
}
