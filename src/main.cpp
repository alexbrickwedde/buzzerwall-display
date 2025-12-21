#include <Arduino.h>
#include <esp_display_panel.hpp>

#include <lvgl.h>
#include "lvgl_v8_port.h"
#include <demos/lv_demos.h>
#include "lv_7seg.h"
#include "driver/twai.h"

#include <vector>
#include <cstring>
#include <atomic>

static void twai_send_message(uint32_t id, const uint8_t* data, uint8_t len);
static void event_handler_start1(lv_event_t * e);
static void event_handler_start2(lv_event_t * e);
static void event_handler_start3(lv_event_t * e);
static void event_handler_ok(lv_event_t * e);
static void event_handler_cancel(lv_event_t * e);
static void event_handler_test(lv_event_t * e);

using namespace esp_panel::drivers;
using namespace esp_panel::board;

extern lv_font_t lv_font_robotocondensed_40;
extern lv_font_t lv_font_robotocondensed_60;
extern lv_font_t lv_font_robotocondensed_80;
extern lv_font_t lv_font_caveat_40;
extern lv_font_t lv_font_caveat_60;
extern lv_font_t lv_font_caveat_80;
extern const lv_img_dsc_t difficulty1;
extern const lv_img_dsc_t difficulty2;
extern const lv_img_dsc_t difficulty3;

class BuzzerButton {
public:
    uint16_t buzzer_id;
    uint32_t last_press_id;
    uint32_t last_press_online;
    bool used_in_game;
    bool pressed;
    bool waiting_for_press;
    bool bus_offline;
    uint32_t press_time; // Time when the button was pressed
    BuzzerButton(uint16_t id, bool used) : buzzer_id(id), used_in_game(used), pressed(false), waiting_for_press(false), press_time(0), last_press_id(0), last_press_online(0), bus_offline(true) {
        //
    }

    void clear() {
        pressed = false;
        waiting_for_press = false;
        press_time = 0;
        last_press_id = 0;
        last_press_online = 0;
    }
};

class MainScreen {
    public:
        lv_obj_t *main_screen;

        void init(lv_obj_t * parent) {
            main_screen = lv_scr_act();
            lv_obj_set_style_bg_color(main_screen, lv_color_hex(0x000000), LV_PART_MAIN);
            lv_obj_clear_flag(main_screen, LV_OBJ_FLAG_SCROLLABLE);
        }
};
MainScreen mainscreen;

class OverlayScreen {
    public:
        lv_obj_t *overlay_screen;
        lv_obj_t *label_1;

        void init(MainScreen& mainscreen) {
            overlay_screen = lv_obj_create(mainscreen.main_screen);
            lv_obj_set_size(overlay_screen, LV_PCT(100), LV_PCT(100));
            lv_obj_align(overlay_screen, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_bg_opa(overlay_screen, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(overlay_screen, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(overlay_screen, 0, LV_PART_MAIN);
            lv_obj_clear_flag(overlay_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(overlay_screen, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_add_flag(overlay_screen, LV_OBJ_FLAG_EVENT_BUBBLE);

            label_1 = lv_label_create(overlay_screen);
            lv_label_set_text(label_1, "");
            lv_obj_set_style_text_font(label_1, &lv_font_caveat_80, 0);
            lv_obj_align(label_1, LV_ALIGN_BOTTOM_MID, 0, -90);
        }
};
OverlayScreen overlayscreen;

class StartScreen {
    public:
        lv_obj_t *start_screen;
        lv_obj_t *start1btn;
        lv_obj_t *start1label;
        lv_obj_t *start2btn;
        lv_obj_t *start2label;
        lv_obj_t *start3btn;
        lv_obj_t *start3label;

        void init(MainScreen& mainscreen) {
            start_screen = lv_obj_create(mainscreen.main_screen);
            lv_obj_set_size(start_screen, LV_PCT(100), LV_PCT(100));
            lv_obj_align(start_screen, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_bg_color(start_screen, lv_color_hex(0xe4032e), LV_PART_MAIN);
            lv_obj_set_style_border_width(start_screen, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(start_screen, 0, LV_PART_MAIN);
            lv_obj_clear_flag(start_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_clear_flag(start_screen, LV_OBJ_FLAG_HIDDEN);

            start1btn = lv_img_create(start_screen);
            lv_img_set_src(start1btn, &difficulty1);
            lv_obj_set_style_bg_color(start1btn, lv_color_hex(0xc5c405), LV_PART_MAIN);
            lv_obj_set_style_text_color(start1btn, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_size(start1btn, 200, 200);
            lv_obj_add_event_cb(start1btn, event_handler_start1, LV_EVENT_ALL, NULL);
            lv_obj_align(start1btn, LV_ALIGN_CENTER, 0, -100);
            start1label = lv_label_create(start1btn);
            lv_label_set_text(start1label, "START1");
            lv_obj_set_style_text_font(start1label, &lv_font_robotocondensed_60, 0);
            lv_obj_center(start1label);

            start2btn = lv_img_create(start_screen);
            lv_img_set_src(start2btn, &difficulty2);
            lv_obj_set_style_bg_color(start2btn, lv_color_hex(0xc5c405), LV_PART_MAIN);
            lv_obj_set_style_text_color(start2btn, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_size(start2btn, 200, 200);
            lv_obj_add_event_cb(start2btn, event_handler_start2, LV_EVENT_ALL, NULL);
            lv_obj_align_to(start2btn, start1btn, LV_ALIGN_OUT_LEFT_TOP, -10, 0);
            start2label = lv_label_create(start2btn);
            lv_label_set_text(start2label, "START2");
            lv_obj_set_style_text_font(start2label, &lv_font_robotocondensed_60, 0);
            lv_obj_center(start2label);

            start3btn = lv_img_create(start_screen);
            lv_img_set_src(start3btn, &difficulty3);
            lv_obj_set_style_bg_color(start3btn, lv_color_hex(0xc5c405), LV_PART_MAIN);
            lv_obj_set_style_text_color(start3btn, lv_color_black(), LV_PART_MAIN);
            lv_obj_set_size(start3btn, 200, 200);
            lv_obj_add_event_cb(start3btn, event_handler_start3, LV_EVENT_ALL, NULL);
            lv_obj_align_to(start3btn, start1btn, LV_ALIGN_OUT_RIGHT_TOP, 10, 0);
            start3label = lv_label_create(start3btn);
            lv_label_set_text(start3label, "START3");
            lv_obj_set_style_text_font(start3label, &lv_font_robotocondensed_60, 0);
            lv_obj_center(start3label);

        }

        void show() {
            lv_obj_clear_flag(start_screen, LV_OBJ_FLAG_HIDDEN);
        }

        void hide() {
            lv_obj_add_flag(start_screen, LV_OBJ_FLAG_HIDDEN);
        }

};
StartScreen startscreen;

class GameScreen {
    public:
        lv_obj_t * seven1;
        lv_obj_t * seven2;
        lv_obj_t * seven3;
        lv_obj_t * seven4;
        lv_obj_t * seven5;
        lv_obj_t *game_screen;
        lv_obj_t *okbtn;
        lv_obj_t *oklabel;
        lv_obj_t *cancelbtn;
        lv_obj_t *cancellabel;
        lv_obj_t *testbtn;
        lv_obj_t *testlabel;

        void init(MainScreen& mainscreen) {
            game_screen = lv_obj_create(mainscreen.main_screen);
            lv_obj_set_size(game_screen, LV_PCT(100), LV_PCT(100));
            lv_obj_align(game_screen, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_style_bg_color(game_screen, lv_color_hex(0xe4032e), LV_PART_MAIN);
            lv_obj_set_style_border_width(game_screen, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(game_screen, 0, LV_PART_MAIN);
            lv_obj_clear_flag(game_screen, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_add_flag(game_screen, LV_OBJ_FLAG_HIDDEN);

            /*
            lv_obj_t *label = lv_label_create(lv_scr_act());
            lv_label_set_text(label, "12.345");
            lv_obj_set_style_text_font(label, &lv_font_dosis_340, 0);
            lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
            */

            okbtn = lv_btn_create(game_screen);
            lv_obj_set_style_bg_color(okbtn, lv_color_hex(0xc5c405), LV_PART_MAIN);
            lv_obj_set_style_text_color(okbtn, lv_color_black(), LV_PART_MAIN);
            lv_obj_add_event_cb(okbtn, event_handler_ok, LV_EVENT_ALL, NULL);
            lv_obj_align(okbtn, LV_ALIGN_BOTTOM_MID, 0, -2);
            oklabel = lv_label_create(okbtn);
            lv_label_set_text(oklabel, "OK");
            lv_obj_set_style_text_font(oklabel, &lv_font_robotocondensed_60, 0);
            lv_obj_center(oklabel);

            cancelbtn = lv_btn_create(game_screen);
            lv_obj_add_event_cb(cancelbtn, event_handler_cancel, LV_EVENT_ALL, NULL);
            lv_obj_set_style_bg_color(cancelbtn, lv_color_hex(0xe4032e), LV_PART_MAIN);
            lv_obj_set_style_text_color(cancelbtn, lv_color_black(), LV_PART_MAIN);
            lv_obj_align(cancelbtn, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
            cancellabel = lv_label_create(cancelbtn);
            lv_label_set_text(cancellabel, "STOPP");
            lv_obj_set_style_text_font(cancellabel, &lv_font_robotocondensed_60, 0);
            lv_obj_center(cancellabel);
            lv_obj_add_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);

            testbtn = lv_btn_create(game_screen);
            lv_obj_add_event_cb(testbtn, event_handler_test, LV_EVENT_ALL, NULL);
            lv_obj_set_style_bg_color(testbtn, lv_color_hex(0xe4032e), LV_PART_MAIN);
            lv_obj_set_style_text_color(testbtn, lv_color_black(), LV_PART_MAIN);
            lv_obj_align(testbtn, LV_ALIGN_BOTTOM_LEFT, 2, -2);
            testlabel = lv_label_create(testbtn);
            lv_label_set_text(testlabel, "TEST");
            lv_obj_set_style_text_font(testlabel, &lv_font_robotocondensed_60, 0);
            lv_obj_center(testlabel);
            lv_obj_add_flag(testbtn, LV_OBJ_FLAG_HIDDEN);

            seven1 = lv_7seg_create(game_screen, 170, 320);
            lv_obj_align(seven1, LV_ALIGN_TOP_LEFT, 5, 0);

            seven2 = lv_7seg_create(game_screen, 170, 320);
            lv_obj_align(seven2, LV_ALIGN_TOP_LEFT, 5 + 150, 0);

            seven3 = lv_7seg_create(game_screen, 170, 320);
            lv_obj_align(seven3, LV_ALIGN_TOP_LEFT, 5 + 150 * 2, 0);

            seven4 = lv_7seg_create(game_screen, 170, 320);
            lv_obj_align(seven4, LV_ALIGN_TOP_LEFT, 5 + 150 * 3, 0);

            seven5 = lv_7seg_create(game_screen, 170, 320  );
            lv_obj_align(seven5, LV_ALIGN_TOP_LEFT, 5 + 150 * 4, 0);

            lv_7seg_set_digit(seven1, ' ', false);
            lv_7seg_set_digit(seven2, ' ', false);
            lv_7seg_set_digit(seven3, ' ', false);
            lv_7seg_set_digit(seven4, ' ', false);
            lv_7seg_set_digit(seven5, ' ', false);
        }

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

        void show() {
            lv_7seg_set_digit(seven1, ' ', true);
            lv_7seg_set_digit(seven2, ' ', true);
            lv_7seg_set_digit(seven3, ' ', true);
            lv_7seg_set_digit(seven4, ' ', true);
            lv_7seg_set_digit(seven5, ' ', true);
            lv_obj_set_style_bg_color(game_screen, lv_color_hex(0xe4032e), LV_PART_MAIN);
            lv_obj_clear_flag(game_screen, LV_OBJ_FLAG_HIDDEN);
        }

        void hide() {
            lv_obj_add_flag(game_screen, LV_OBJ_FLAG_HIDDEN);
        }

        void readysetgo() {
            lvgl_port_lock(-1);
            lv_obj_add_flag(okbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(testbtn, LV_OBJ_FLAG_HIDDEN);
            lv_7seg_set_digit(seven1, ' ', false);
            lv_7seg_set_digit(seven2, ' ', false);
            lv_7seg_set_digit(seven3, '3', false);
            lv_7seg_set_digit(seven4, ' ', false);
            lv_7seg_set_digit(seven5, ' ', false);
            lv_label_set_text(overlayscreen.label_1, "Bereit?");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven3, '2', false);
            lv_label_set_text(overlayscreen.label_1, "Auf die Plätze...");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven3, '1', false);
            lv_label_set_text(overlayscreen.label_1, "Fertig...");
            lvgl_port_unlock();
            delay(1000);
            lvgl_port_lock(-1);
            lv_7seg_set_digit(seven5, ' ', false);
            lv_label_set_text(overlayscreen.label_1, "Los!");
            lvgl_port_unlock();
        }

        void gameended() {
            lv_obj_clear_flag(okbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(cancelbtn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(testbtn, LV_OBJ_FLAG_HIDDEN);
            lv_7seg_set_color(seven1, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven2, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven3, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven4, lv_color_hex(0xc5c405));
            lv_7seg_set_color(seven5, lv_color_hex(0xc5c405));
            lv_obj_set_style_bg_color(game_screen, lv_color_hex(0xe4032e), LV_PART_MAIN);
        }

        void sevensegcolor(lv_color_t color) {
            lv_7seg_set_color(seven1, color);
            lv_7seg_set_color(seven2, color);
            lv_7seg_set_color(seven3, color);
            lv_7seg_set_color(seven4, color);
            lv_7seg_set_color(seven5, color);
        }
};
GameScreen gamescreen;



std::vector<BuzzerButton> buzzers;
std::vector<lv_obj_t*> buzzer_indicators;

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
uint16_t waitforbuzzer_index = 0xffff;
uint16_t waitforbuzzer_id = 0;
uint32_t total_time = 0;
std::atomic<uint32_t> buzzer_time = 0;
uint32_t local_time = 0;
uint32_t random_start_time = 0;
const uint8_t TOTAL_ROUNDS = 5;



uint32_t next_can_packet_millis = 0;

void game_tick() {
    bool bSendCan = millis() > next_can_packet_millis;
    if(bSendCan) {
        next_can_packet_millis = millis() + 1000;
    }

    lvgl_port_lock(-1);
    for(uint16_t i = 0; i < buzzers.size(); i++) {
        BuzzerButton &buzzer = buzzers[i];

        if (!buzzer.bus_offline) {
            if (millis() - buzzer.last_press_online > 1000) {
                buzzer.bus_offline = true;
                printf("!!!! Buzzer %d went offline\n", buzzer.buzzer_id);
            }
        }

        if(buzzer_indicators.size() <= i) {
            lv_obj_t * indicator = lv_obj_create(overlayscreen.overlay_screen);
            buzzer_indicators.push_back(indicator);

            lv_obj_set_size(buzzer_indicators[i], 15, 15);
            lv_obj_align(buzzer_indicators[i], LV_ALIGN_TOP_RIGHT, -5, 5 + i * 20);
            lv_obj_set_style_radius(buzzer_indicators[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        }

        if(buzzers[i].bus_offline) {
            lv_obj_set_style_bg_color(buzzer_indicators[i], lv_color_hex(0xff8800), LV_PART_MAIN);
        } else {
            if(buzzers[i].used_in_game) {
                if (buzzers[i].pressed) {
                    lv_obj_set_style_bg_color(buzzer_indicators[i], lv_color_hex(0x000000), LV_PART_MAIN);
                } else {
                    lv_obj_set_style_bg_color(buzzer_indicators[i], lv_color_hex(0x00ff00), LV_PART_MAIN);
                }
            } else {
                lv_obj_set_style_bg_color(buzzer_indicators[i], lv_color_hex(0x008800), LV_PART_MAIN);
            }
        }
    }
    lvgl_port_unlock();

    switch (game_state) {
        case GAME_IDLE:
            if (bSendCan) {
                uint8_t data[8] = {0};
                data[0] = 0x00;
                twai_send_message(0x7ff, data, 1);
            }
            break;

        case GAME_READYSETGO:
            lvgl_port_lock(-1);
            startscreen.hide();
            gamescreen.show();
            lvgl_port_unlock();
            gamescreen.readysetgo();
            game_state = GAME_PREPARING;
            break;

        case GAME_PREPARING:
            current_round = 0;
            total_time = 0;

            lvgl_port_lock(-1);
            gamescreen.display_time(total_time);
            gamescreen.sevensegcolor(lv_color_hex(0xe4032e));
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
                for(uint16_t i = 0; i < buzzers.size(); i++) {
                    BuzzerButton &buzzer = buzzers[i];
                    if (buzzer.used_in_game) {
                        available_buzzers.push_back(i);
                    }
                }
                if (!available_buzzers.empty()) {
                    waitforbuzzer_index = available_buzzers[random(0, available_buzzers.size())];
                    waitforbuzzer_id = buzzers[waitforbuzzer_index].buzzer_id;
                } else {
                    lvgl_port_lock(-1);
                    lv_label_set_text(overlayscreen.label_1, "Keine Buzzer vorhanden!");
                    lvgl_port_unlock();
                    game_state = GAME_FINISHED; // No available buzzers
                    break;
                }
                random_start_time = millis() + random(1500, 3000);

                char meldung[32];
                snprintf(meldung, sizeof(meldung), "Runde %d", current_round);
                lvgl_port_lock(-1);
                lv_obj_set_style_bg_color(gamescreen.game_screen, lv_color_hex(0xc5c405), LV_PART_MAIN);
                lv_label_set_text(overlayscreen.label_1, meldung);
                lvgl_port_unlock();
                game_state = GAME_WAIT_FOR_BUZZER1;
            } else {
                lvgl_port_lock(-1);
                lv_label_set_text(overlayscreen.label_1, "Spiel beendet!");
                lvgl_port_unlock();
                game_state = GAME_FINISHED;
            }
            break;

        case GAME_WAIT_FOR_BUZZER1:
            {
                if (millis() < random_start_time) {
                    break;
                }
                buzzers[waitforbuzzer_index].waiting_for_press = true;
                local_time = millis();
                buzzer_time = 0xffffffff;

                uint8_t data[8] = {0};
                data[0] = 0x01;
                twai_send_message(buzzers[waitforbuzzer_index].buzzer_id | 0x100, data, 1);

                char meldung[32];
                snprintf(meldung, sizeof(meldung), "Buzzer %d !!!", waitforbuzzer_id);
                lvgl_port_lock(-1);
                lv_label_set_text(overlayscreen.label_1, meldung);
                lvgl_port_unlock();

                game_state = GAME_WAIT_FOR_BUZZER2;
            }
            break;

        case GAME_WAIT_FOR_BUZZER2:
            {
                lvgl_port_lock(-1);
                if (buzzer_time != 0xffffffff) {
                    gamescreen.display_time(total_time + buzzer_time);
                } else {
                    printf("!!! buzzer_time still not set\n");
                    gamescreen.display_time(total_time + (millis() - local_time));
                }
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

                lv_obj_set_style_bg_color(gamescreen.game_screen, color, LV_PART_MAIN);


                for(BuzzerButton &buzzer : buzzers) {
                    if (buzzer.pressed) {
                        if (buzzer.waiting_for_press) {
                            total_time += buzzer.press_time;
                            game_state = GAME_ROUND_COMPLETE;
                            break;
                        } else {
                            total_time += 1000; // Add 1000ms penalty for incorrect buzzer
                            printf("!!! penalty for buzzer %d\n", buzzer.buzzer_id);
                        }
                        buzzer.pressed = false;
                    }
                }

                if (bSendCan) {
                    uint8_t data[8] = {0};
                    data[0] = 0x01;
                    twai_send_message(buzzers[waitforbuzzer_index].buzzer_id | 0x100, data, 1);
                }
            }
            break;

        case GAME_ROUND_COMPLETE:
            {
                waitforbuzzer_index = 0xffff;
                waitforbuzzer_id = 0;
                if (bSendCan) {
                    uint8_t data[8] = {0};
                    data[0] = 0x00;
                    twai_send_message(0x7ff, data, 1);
                }
                for (BuzzerButton &buzzer : buzzers) {
                    buzzer.clear();
                }
                game_state = GAME_STARTING;
            }
            break;

        case GAME_FINISHED:
            lvgl_port_lock(-1);
            gamescreen.display_time(total_time);
            gamescreen.gameended();
            lvgl_port_unlock();
            game_state = GAME_END;
            break;

        case GAME_END:
            // wait key press
            break;
    }
}

static void event_handler_start1(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_IDLE:
                game_state = GAME_READYSETGO;
                break;

            default:
                break;
        }
    }
}

static void event_handler_start2(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_IDLE:
                game_state = GAME_READYSETGO;
                break;

            default:
                break;
        }
    }
}

static void event_handler_start3(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_IDLE:
                game_state = GAME_READYSETGO;
                break;

            default:
                break;
        }
    }
}

static void event_handler_ok(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_END:
                lvgl_port_lock(-1);
                gamescreen.hide();
                startscreen.show();
                lv_label_set_text(overlayscreen.label_1, "");
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
        gamescreen.hide();
        startscreen.show();
        lv_label_set_text(overlayscreen.label_1, "Spiel abgebrochen");
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
                buzzers[waitforbuzzer_index].pressed = true;
                buzzers[waitforbuzzer_index].press_time = millis() - local_time;
                break;
        }
    }
}

static void twai_send_message(uint32_t id, const uint8_t* data, uint8_t len) {
  twai_message_t message;
  message.extd = (id & 0x80000000) != 0;
  message.identifier = id & 0x1FFFFFFF;
  message.data_length_code = len;
  for (int i = 0; i < message.data_length_code; i++) {
    message.data[i] = data[i];
  }
  if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
    printf("Message queued for transmission\n"); // Print success message
  } else {
    printf("Failed to queue message for transmission\n"); // Print failure message
  }
  memset(message.data, 0, sizeof(message.data)); // Clear the entire array
}

static void handle_rx_message(const twai_message_t &message)
{
    // Assuming the message ID corresponds to the buzzer ID
    uint16_t buzzer_id = message.identifier;
    uint32_t press_id = message.data[0];
    uint32_t press_millis = message.data[4] << 24 | message.data[5] << 16 | message.data[6] << 8 | message.data[7];
    printf("Message received from buzzer %d\n", buzzer_id);
    for(BuzzerButton &buzzer : buzzers) {
        if(buzzer.buzzer_id == buzzer_id) {
            if (buzzer.last_press_id != press_id) {
                if (game_state == GAME_WAIT_FOR_BUZZER2) {
                    if (buzzer.last_press_online != 0) {
                        buzzer.last_press_id = press_id;
                        if (!buzzer.pressed) {
                            buzzer.pressed = true;
                            buzzer.press_time = press_millis;
                        }
                    }
                } else {
                    printf("!!! press buzzer_id %d, id %d not in game\n", buzzer_id, press_id);
                }
            }
            
            if (waitforbuzzer_index != 0xffffffff) {
                if (buzzers[waitforbuzzer_index].buzzer_id == buzzer_id) {
                    buzzer_time = press_millis;
                }
            }

            buzzer.last_press_online = millis();
            if (buzzer.bus_offline) {
                printf("!!!! Buzzer %d went online\n", buzzer.buzzer_id);
                buzzer.bus_offline = false;
            }
        }
    }
}

#define TWAI_RX_PIN 19
#define TWAI_TX_PIN 20

bool twai_init()
{
  // Initialize configuration structures using macro initializers
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)TWAI_TX_PIN, (gpio_num_t)TWAI_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK)
  {
    Serial.println("Failed to install driver"); // Print error message
    return false;                               // Return false if driver installation fails
  }
  Serial.println("Driver installed"); // Print success message

  // Start TWAI driver
  if (twai_start() != ESP_OK)
  {
    Serial.println("Failed to start driver"); // Print error message
    return false;
  }
  Serial.println("Driver started"); // Print success message

  twai_clear_receive_queue();
  twai_clear_transmit_queue();

  // Reconfigure alerts to detect frame receive, Bus-Off error, and RX queue full states
  uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR | TWAI_ALERT_RX_QUEUE_FULL | TWAI_ALERT_RECOVERY_IN_PROGRESS | TWAI_ALERT_BUS_RECOVERED | TWAI_ALERT_ERR_PASS;
  if (twai_reconfigure_alerts(alerts_to_enable, NULL) == ESP_OK)
  {
    Serial.println("CAN Alerts reconfigured"); // Print success message
  }
  else
  {
    Serial.println("Failed to reconfigure alerts"); // Print error message
    return false;
  }
  return true;
}

void twai_receive()
{
  // Check if alert happened
  uint32_t alerts_triggered;
  if (twai_read_alerts(&alerts_triggered, 0) == ESP_OK) {
    if (alerts_triggered & TWAI_ALERT_BUS_ERROR)
    {
        twai_status_info_t twaistatus;                                       // Create status info structure
        twai_get_status_info(&twaistatus);                                   // Get status information
        Serial.println("Alert: A (Bit, Stuff, CRC, Form, ACK) error has occurred on the bus."); // Print bus error alert
        Serial.printf("Bus error count: %d\n", twaistatus.bus_error_count);                     // Print bus error count
    }
    else if (alerts_triggered & TWAI_ALERT_ERR_PASS)
    {
        Serial.println("Alert: TWAI controller has become error passive."); // Print passive error alert
    }
    else if (alerts_triggered & TWAI_ALERT_RX_QUEUE_FULL)
    {
        twai_status_info_t twaistatus;                                       // Create status info structure
        twai_get_status_info(&twaistatus);                                   // Get status information
        Serial.println("Alert: The RX queue is full causing a received frame to be lost."); // Print RX queue full alert
        Serial.printf("RX buffered: %d\t", twaistatus.msgs_to_rx);                          // Print buffered RX messages
        Serial.printf("RX missed: %d\t", twaistatus.rx_missed_count);                       // Print missed RX count
        Serial.printf("RX overrun %d\n", twaistatus.rx_overrun_count);                      // Print RX overrun count
        twai_clear_receive_queue();
        twai_clear_transmit_queue();
    }

    if (alerts_triggered & TWAI_ALERT_RX_DATA)
    {
        // One or more messages received. Handle all.
        twai_message_t message;
        while (twai_receive(&message, 0) == ESP_OK)
        {                             // Receive messages
        handle_rx_message(message); // Handle each received message
        }
    }
  }

}

void setup()
{
    delay(3000); // Wait for 2 seconds to allow serial monitor to connect

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

    Serial.println("Initializing TWAI");
    twai_init();

    Serial.println("Creating UI");

    lvgl_port_lock(-1);
    mainscreen.init(lv_scr_act());
    startscreen.init(mainscreen);
    gamescreen.init(mainscreen);
    overlayscreen.init(mainscreen);
    lvgl_port_unlock();

    buzzers.insert(buzzers.end(), BuzzerButton(1, true));
    buzzers.insert(buzzers.end(), BuzzerButton(2, true));
    buzzers.insert(buzzers.end(), BuzzerButton(3, false));
    buzzers.insert(buzzers.end(), BuzzerButton(4, true));
}

void loop()
{
    game_tick();
    twai_receive();
}
