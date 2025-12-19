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
    BuzzerButton(uint8_t id) : buzzer_id(id), used_in_game(false), pressed(false), waiting_for_press(false), press_time(0) {
        //
    }

    void clear() {
        pressed = false;
        waiting_for_press = false;
        press_time = 0;
    }
};

lv_obj_t * startbtn;
lv_obj_t * startlabel;
lv_obj_t * cancelbtn;
lv_obj_t * cancellabel;
lv_obj_t * testbtn;
lv_obj_t * testlabel;


std::vector<BuzzerButton> buzzers;

enum GameState {
    GAME_IDLE,
    GAME_PREPARING,
    GAME_STARTING,
    GAME_WAIT_FOR_BUZZER,
    GAME_ROUND_COMPLETE,
    GAME_FINISHED,
    GAME_END
};

GameState game_state = GAME_IDLE;
uint8_t current_round = 0;
uint8_t current_buzzer = 0;
uint32_t total_time = 0;
const uint8_t TOTAL_ROUNDS = 5;


void display_time(uint32_t time_ms) {
    lv_7seg_set_digit(seven1, (time_ms / 10000) % 10, false);
    lv_7seg_set_digit(seven2, (time_ms / 1000) % 10, true);
    lv_7seg_set_digit(seven3, (time_ms / 100) % 10, false);
    lv_7seg_set_digit(seven4, (time_ms / 10) % 10, false);
    lv_7seg_set_digit(seven5, time_ms % 10, false);
}

void game_tick() {
    switch (game_state) {
        case GAME_IDLE:
            break;

        case GAME_PREPARING:
            current_round = 0;
            total_time = 0;
            for (BuzzerButton &buzzer : buzzers) {
                buzzer.clear();
            }

            lvgl_port_lock(-1);
            display_time(total_time);
            lvgl_port_unlock();

            game_state = GAME_STARTING;
            break;

        case GAME_STARTING:
            current_round++;
            if (current_round <= TOTAL_ROUNDS) {
                // Find a random buzzer that is online (bit 7) and should be used in the game (bit 6)
                std::vector<uint8_t> available_buzzers;
                for (BuzzerButton &buzzer : buzzers) {
                    if (buzzer.used_in_game) {
                        available_buzzers.push_back(buzzer.buzzer_id);
                    }
                }
                if (!available_buzzers.empty()) {
                    current_buzzer = available_buzzers[random(0, available_buzzers.size())];
                } else {
                    game_state = GAME_FINISHED; // No available buzzers
                    break;
                }
                buzzers[current_buzzer].waiting_for_press = true;
                game_state = GAME_WAIT_FOR_BUZZER;
            } else {
                game_state = GAME_FINISHED;
            }
            break;

        case GAME_WAIT_FOR_BUZZER:
            lvgl_port_lock(-1);
            display_time(total_time);
            lvgl_port_unlock();

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
            break;

        case GAME_ROUND_COMPLETE:
            game_state = GAME_STARTING;
            break;

        case GAME_FINISHED:
            lvgl_port_lock(-1);
            display_time(total_time);
            lv_label_set_text(startlabel, "OK");
            lv_obj_clear_flag(startbtn, LV_OBJ_FLAG_HIDDEN);
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
                game_state = GAME_PREPARING;
                lvgl_port_lock(-1);
                display_time(0);
                lv_obj_add_flag(startbtn, LV_OBJ_FLAG_HIDDEN);
                lvgl_port_unlock();
                break;

            case GAME_END:
                lvgl_port_lock(-1);
                display_time(0);
                lv_label_set_text(startlabel, "START");
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
        lvgl_port_unlock();
        game_state = GAME_IDLE;
    }
}

static void event_handler_test(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        switch (game_state) {
            case GAME_WAIT_FOR_BUZZER:
                buzzers[current_buzzer].pressed = true;
                buzzers[current_buzzer].press_time = random(500, 5000);
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

    /**
    lv_obj_t *label_1 = lv_label_create(lv_scr_act());
    lv_label_set_text(label_1, "Hello World!");
    lv_obj_set_style_text_font(label_1, &lv_font_montserrat_16, 0);
    lv_obj_align(label_1, LV_ALIGN_CENTER, 0, -300);
     */

    startbtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(startbtn, event_handler_start, LV_EVENT_ALL, NULL);
    lv_obj_align(startbtn, LV_ALIGN_BOTTOM_MID, 0, -30);
    startlabel = lv_label_create(startbtn);
    lv_label_set_text(startlabel, "START");
    lv_obj_set_style_text_font(startlabel, &lv_font_montserrat_36, 0);
    lv_obj_center(startlabel);

    cancelbtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(cancelbtn, event_handler_cancel, LV_EVENT_ALL, NULL);
    lv_obj_align(cancelbtn, LV_ALIGN_BOTTOM_RIGHT, 0, -30);
    cancellabel = lv_label_create(cancelbtn);
    lv_label_set_text(cancellabel, "CANCEL");
    lv_obj_set_style_text_font(cancellabel, &lv_font_montserrat_36, 0);
    lv_obj_center(cancellabel);

    testbtn = lv_btn_create(lv_scr_act());
    lv_obj_add_event_cb(testbtn, event_handler_test, LV_EVENT_ALL, NULL);
    lv_obj_align(testbtn, LV_ALIGN_BOTTOM_LEFT, 0, -30);
    testlabel = lv_label_create(testbtn);
    lv_label_set_text(testlabel, "TEST");
    lv_obj_set_style_text_font(testlabel, &lv_font_montserrat_36, 0);
    lv_obj_center(testlabel);

    seven1 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven1, LV_ALIGN_TOP_LEFT, 5, 5);

    seven2 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven2, LV_ALIGN_TOP_LEFT, 5 + 150, 5);

    seven3 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven3, LV_ALIGN_TOP_LEFT, 5 + 150 * 2, 5);

    seven4 = lv_7seg_create(lv_scr_act(), 170, 320);
    lv_obj_align(seven4, LV_ALIGN_TOP_LEFT, 5 + 150 * 3, 5);

    seven5 = lv_7seg_create(lv_scr_act(), 170, 320  );
    lv_obj_align(seven5, LV_ALIGN_TOP_LEFT, 5 + 150 * 4, 5);

    lvgl_port_unlock();

    buzzers.insert(buzzers.end(), BuzzerButton(0));
    buzzers.insert(buzzers.end(), BuzzerButton(1));
    buzzers.insert(buzzers.end(), BuzzerButton(2));
    buzzers.insert(buzzers.end(), BuzzerButton(3));
}

void loop()
{
    Serial.println("IDLE loop");
    game_tick();
}
