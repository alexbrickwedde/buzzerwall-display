#include <Arduino.h>
#include <esp_display_panel.hpp>

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <demos/lv_demos.h>
#include "lv_7seg.h"

#include <vector>
#include <cstring>

using namespace esp_panel::drivers;
using namespace esp_panel::board;

extern lv_font_t lv_font_robotocondensed_40;
extern lv_font_t lv_font_robotocondensed_60;
extern lv_font_t lv_font_robotocondensed_80;
extern lv_font_t lv_font_caveat_40;
extern lv_font_t lv_font_caveat_60;
extern lv_font_t lv_font_caveat_80;

uint32_t x = 0;
lv_obj_t * seven1;
lv_obj_t * seven2;
lv_obj_t * seven3;
lv_obj_t * seven4;
lv_obj_t * seven5;

class BuzzerButton {
public:
    uint8_t buzzer_id;
    bool used_in_game;
    bool pressed;
    bool waiting_for_press;
    uint32_t press_time; // Time when the button was pressed
    BuzzerButton(uint8_t id, bool used) : buzzer_id(id), used_in_game(used), pressed(false), waiting_for_press(false), press_time(0) {
        //
    }

    void clear() {
        pressed = false;
        waiting_for_press = false;
        press_time = 0;
    }
};

lv_obj_t *label_1;
lv_obj_t *startbtn;
lv_obj_t *startlabel;
lv_obj_t *cancelbtn;
lv_obj_t *cancellabel;
lv_obj_t *testbtn;
lv_obj_t *testlabel;


std::vector<BuzzerButton> buzzers;

enum GameState {
    GAME_IDLE,
    GAME_READYSETGO,
    GAME_PREPARING,
    GAME_STARTING,
    GAME_WAIT_FOR_BUZZER1,
    GAME_WAIT_FOR_BUZZER2,
    GAME_ROUND_COMPLETE,
    GAME_FINISHED,
    GAME_END
};

GameState game_state = GAME_IDLE;
uint8_t current_round = 0;
uint8_t current_buzzer = 0;
uint32_t total_time = 0;
uint32_t local_time = 0;
uint32_t random_start_time = 0;
const uint8_t TOTAL_ROUNDS = 5;


void display_time(uint32_t time_ms) {
    uint8_t upper = (time_ms / 10000);
    if (upper == 0) {
        lv_7seg_set_digit(seven1, ' ', false);
        lv_7seg_set_digit(seven2, (time_ms / 1000) % 10, true);
        lv_7seg_set_digit(seven3, (time_ms / 100) % 10, false);
        lv_7seg_set_digit(seven4, (time_ms / 10) % 10, false);
        lv_7seg_set_digit(seven5, time_ms % 10, false);
    } else if (upper < 10) {
        lv_7seg_set_digit(seven1, upper, false);
        lv_7seg_set_digit(seven2, (time_ms / 1000) % 10, true);
        lv_7seg_set_digit(seven3, (time_ms / 100) % 10, false);
        lv_7seg_set_digit(seven4, (time_ms / 10) % 10, false);
        lv_7seg_set_digit(seven5, time_ms % 10, false);
    } else if (upper < 100) {
        lv_7seg_set_digit(seven1, (time_ms / 100000) % 10, false);
        lv_7seg_set_digit(seven2, (time_ms / 10000) % 10, false);
        lv_7seg_set_digit(seven3, (time_ms / 1000) % 10, true);
        lv_7seg_set_digit(seven4, (time_ms / 100) % 10, false);
        lv_7seg_set_digit(seven5, (time_ms / 10) % 10, false);
    } else if (upper < 1000) {
        lv_7seg_set_digit(seven1, (time_ms / 1000000) % 10, false);
        lv_7seg_set_digit(seven2, (time_ms / 100000) % 10, false);
        lv_7seg_set_digit(seven3, (time_ms / 10000) % 10, false);
        lv_7seg_set_digit(seven4, (time_ms / 1000) % 10, true);
        lv_7seg_set_digit(seven5, (time_ms / 100) % 10, false);
    } else {
        lv_7seg_set_digit(seven1, ' ', true);
        lv_7seg_set_digit(seven2, ' ', true);
        lv_7seg_set_digit(seven3, ' ', true);
        lv_7seg_set_digit(seven4, ' ', true);
        lv_7seg_set_digit(seven5, ' ', true);
    }
}

void game_tick() {
    switch (game_state) {
        case GAME_IDLE:
            break;

        case GAME_READYSETGO:
            lvgl_port_lock(-1);
            lv_obj_clear_flag(seven1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(seven2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(seven3, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(seven4, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(seven5, LV_OBJ_FLAG_HIDDEN);
            lv_7seg_set_digit(seven1, ' ', false);
            lv_7seg_set_digit(seven2, ' ', false);
            lv_7seg_set_digit(seven3, '3', false);
            lv_7seg_set_digit(seven4, ' ', false);
            lv_7seg_set_digit(seven5, ' ', false);
            lv_label_set_text(label_1, "Bereit?");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven3, '2', false);
            lv_label_set_text(label_1, "Auf die Plätze...");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven3, '1', false);
            lv_label_set_text(label_1, "Fertig...");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven5, ' ', false);
            lv_label_set_text(label_1, "Los!");
            lvgl_port_unlock();
            game_state = GAME_PREPARING;
            break;

        case GAME_PREPARING:
            current_round = 0;
            total_time = 0;

            lvgl_port_lock(-1);
            display_time(total_time);
            lv_obj_clear_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(testbtn, LV_OBJ_FLAG_HIDDEN);
            lv_7seg_set_color(seven1, lv_color_hex(0xe4032e));
            lv_7seg_set_color(seven2, lv_color_hex(0xe4032e));
            lv_7seg_set_color(seven3, lv_color_hex(0xe4032e));
            lv_7seg_set_color(seven4, lv_color_hex(0xe4032e));
            lv_7seg_set_color(seven5, lv_color_hex(0xe4032e));
            lvgl_port_unlock();

            game_state = GAME_STARTING;
            break;

        case GAME_STARTING:
            for (BuzzerButton &buzzer : buzzers) {
                buzzer.clear();
            }
            current_round++;
            if (current_round <= TOTAL_ROUNDS) {
                std::vector<uint8_t> available_buzzers;
                for (BuzzerButton &buzzer : buzzers) {
                    if (buzzer.used_in_game) {
                        available_buzzers.push_back(buzzer.buzzer_id);
                    }
                }
                if (!available_buzzers.empty()) {
                    current_buzzer = available_buzzers[random(0, available_buzzers.size())];
                } else {
                    lvgl_port_lock(-1);
                    lv_label_set_text(label_1, "Keine Buzzer vorhanden!");
                    lvgl_port_unlock();
                    game_state = GAME_FINISHED; // No available buzzers
                    break;
                }
                random_start_time = millis() + random(1500, 3000);

                char meldung[32];
                snprintf(meldung, sizeof(meldung), "Runde %d", current_round);
                lvgl_port_lock(-1);
                lv_label_set_text(label_1, meldung);
                lvgl_port_unlock();
                game_state = GAME_WAIT_FOR_BUZZER1;
            } else {
                lvgl_port_lock(-1);
                lv_label_set_text(label_1, "Spiel beendet!");
                lvgl_port_unlock();
                game_state = GAME_FINISHED;
            }
            break;

        case GAME_WAIT_FOR_BUZZER1:
            if (millis() < random_start_time) {
                break;
            }
            buzzers[current_buzzer].waiting_for_press = true;
            local_time = millis();

            char meldung[32];
            snprintf(meldung, sizeof(meldung), "Buzzer %d !!!", current_buzzer + 1);
            lvgl_port_lock(-1);
            lv_label_set_text(label_1, meldung);
            lvgl_port_unlock();
            game_state = GAME_WAIT_FOR_BUZZER2;
            break;

        case GAME_WAIT_FOR_BUZZER2:
            {
                lvgl_port_lock(-1);
                display_time(total_time + (millis() - local_time));
                lvgl_port_unlock();

                lv_color_t color;
                uint8_t col = (millis() >> 1) % 256;
                if (col < 128) {
                    // Fade from col1 (0xe4032e) to col2 (0xc5c405)
                    uint8_t r = ((0xe4 * (127 - col)) + (0xc5 * col)) / 127;
                    uint8_t g = ((0x03 * (127 - col)) + (0xc4 * col)) / 127;
                    uint8_t b = ((0x2e * (127 - col)) + (0x05 * col)) / 127;
                    color = lv_color_make(r, g, b);
                } else {
                    // Fade from col2 (0xc5c405) back to col1 (0xe4032e)
                    uint8_t t = col - 128;
                    uint8_t r = ((0xc5 * (127 - t)) + (0xe4 * t)) / 127;
                    uint8_t g = ((0xc4 * (127 - t)) + (0x03 * t)) / 127;
                    uint8_t b = ((0x05 * (127 - t)) + (0x2e * t)) / 127;
                    color = lv_color_make(r, g, b);
                }

                lv_obj_set_style_bg_color(lv_scr_act(), color, LV_PART_MAIN);


                for(BuzzerButton &buzzer : buzzers) {
                    if (buzzer.pressed) {
                        if (buzzer.waiting_for_press) {
                            total_time += buzzer.press_time;
                            game_state = GAME_ROUND_COMPLETE;
                        } else {
                            total_time += 1000; // Add 1000ms penalty for incorrect buzzer
                        }
                    }
                }
            }
            break;

        case GAME_ROUND_COMPLETE:
            for (BuzzerButton &buzzer : buzzers) {
                buzzer.clear();
            }
            game_state = GAME_STARTING;
            break;

        case GAME_FINISHED:
            lvgl_port_lock(-1);
            display_time(total_time);
            lv_label_set_text(startlabel, "OK");
            lv_obj_clear_flag(startbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(testbtn, LV_OBJ_FLAG_HIDDEN);
            lv_7seg_set_color(seven1, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven2, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven3, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven4, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven5, lv_color_hex(0xc5c405));
            lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xe4032e), LV_PART_MAIN);
            lvgl_port_unlock();
            game_state = GAME_END;
            break;

        case GAME_END:
            // wait key press
            break;
    }
}

static void event_handler_start(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_IDLE:
                game_state = GAME_READYSETGO;
                lvgl_port_lock(-1);
                display_time(0);
                lv_obj_add_flag(startbtn, LV_OBJ_FLAG_HIDDEN);
                lvgl_port_unlock();
                break;

            case GAME_END:
                lvgl_port_lock(-1);
                display_time(0);
                lv_label_set_text(startlabel, "START");
                lv_obj_add_flag(seven1, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(seven2, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(seven3, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(seven4, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(seven5, LV_OBJ_FLAG_HIDDEN);
                lv_label_set_text(label_1, "");
                lvgl_port_unlock();
                game_state = GAME_IDLE;
                break;

            default:
                break;
        }
    }
}

static void event_handler_cancel(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        lvgl_port_lock(-1);
        display_time(0);
        lv_label_set_text(startlabel, "START");
        lv_obj_clear_flag(startbtn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(testbtn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(seven1, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(seven2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(seven3, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(seven4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(seven5, LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(label_1, "Spiel abgebrochen");
        lvgl_port_unlock();
        game_state = GAME_IDLE;
    }
}

static void event_handler_test(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_WAIT_FOR_BUZZER2:
                buzzers[current_buzzer].pressed = true;
                buzzers[current_buzzer].press_time = millis() - local_time;
                break;
        }
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

    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xe4032e), LV_PART_MAIN);

    /*
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "12.345");
    lv_obj_set_style_text_font(label, &lv_font_dosis_340, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
    */

    label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, "");
    lv_obj_set_style_text_font(label_1, &lv_font_caveat_80, 0);
    lv_obj_align(label_1, LV_ALIGN_BOTTOM_MID, 0, -90);

    startbtn = lv_btn_create(lv_scr_act());
    lv_obj_set_style_bg_color(startbtn, lv_color_hex(0xc5c405), LV_PART_MAIN);
    lv_obj_set_style_text_color(startbtn, lv_color_black(), LV_PART_MAIN);
    lv_obj_add_event_cb(startbtn, event_handler_start, LV_EVENT_ALL, NULL);
    lv_obj_align(startbtn, LV_ALIGN_BOTTOM_MID, 0, -2);
    startlabel = lv_label_create(startbtn);
    lv_label_set_text(startlabel, "START");
    lv_obj_set_style_text_font(startlabel, &lv_font_robotocondensed_60, 0);
    lv_obj_center(startlabel);

    cancelbtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(cancelbtn, event_handler_cancel, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(cancelbtn, lv_color_hex(0xe4032e), LV_PART_MAIN);
    lv_obj_set_style_text_color(cancelbtn, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(cancelbtn, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    cancellabel = lv_label_create(cancelbtn);
    lv_label_set_text(cancellabel, "STOPP");
    lv_obj_set_style_text_font(cancellabel, &lv_font_robotocondensed_60, 0);
    lv_obj_center(cancellabel);
    lv_obj_add_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);

    testbtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(testbtn, event_handler_test, LV_EVENT_ALL, NULL);
    lv_obj_set_style_bg_color(testbtn, lv_color_hex(0xe4032e), LV_PART_MAIN);
    lv_obj_set_style_text_color(testbtn, lv_color_black(), LV_PART_MAIN);
    lv_obj_align(testbtn, LV_ALIGN_BOTTOM_LEFT, 2, -2);
    testlabel = lv_label_create(testbtn);
    lv_label_set_text(testlabel, "TEST");
    lv_obj_set_style_text_font(testlabel, &lv_font_robotocondensed_60, 0);
    lv_obj_center(testlabel);
    lv_obj_add_flag(testbtn, LV_OBJ_FLAG_HIDDEN);

    seven1 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven1, LV_ALIGN_TOP_LEFT, 5, 0);

    seven2 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven2, LV_ALIGN_TOP_LEFT, 5 + 150, 0);

    seven3 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven3, LV_ALIGN_TOP_LEFT, 5 + 150 * 2, 0);

    seven4 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven4, LV_ALIGN_TOP_LEFT, 5 + 150 * 3, 0);

    seven5 = lv_7seg_create(lv_scr_act(), 170, 320  );
    lv_obj_align(seven5, LV_ALIGN_TOP_LEFT, 5 + 150 * 4, 0);

    lv_obj_add_flag(seven1, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(seven2, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(seven3, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(seven4, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(seven5, LV_OBJ_FLAG_HIDDEN);
    lv_7seg_set_digit(seven1, ' ', false);
    lv_7seg_set_digit(seven2, ' ', false);
    lv_7seg_set_digit(seven3, ' ', false);
    lv_7seg_set_digit(seven4, ' ', false);
    lv_7seg_set_digit(seven5, ' ', false);
    
    lvgl_port_unlock();

    buzzers.insert(buzzers.end(), BuzzerButton(0, true));
    buzzers.insert(buzzers.end(), BuzzerButton(1, true));
    buzzers.insert(buzzers.end(), BuzzerButton(2, true));
    buzzers.insert(buzzers.end(), BuzzerButton(3, true));
}

void loop()
{
    game_tick();
}
