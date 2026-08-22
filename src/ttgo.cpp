#include <memory.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include "ttgo.hpp"
#include "devkit.hpp"
#include "multibutton.h"
#include "esp_lcd_panel_io.h"
#define ADC_EN_GPIO     GPIO_NUM_14          // enable pin (output)
#define BAT_ADC_UNIT    ADC_UNIT_1
#define BAT_ADC_CHANNEL ADC_CHANNEL_6        // GPIO34
#define BAT_ADC_ATTEN   ADC_ATTEN_DB_12      // see note below re: DB_11 vs DB_12

ttgo_screen_t ttgo_default_screen;
static multibutton_t mb_0_data;
static multibutton_handle_t mb_0_handle;
static multibutton_event_t mb_0_events[MULTIBUTTON_EVENT_SIZE_DEFAULT];
static multibutton_t mb_35_data;
static multibutton_handle_t mb_35_handle;
static multibutton_event_t mb_35_events[MULTIBUTTON_EVENT_SIZE_DEFAULT];
static bool mb_0_old_pressed = false;
static bool mb_35_old_pressed = false;
__attribute__((weak)) void ttgo_on_pressed_changed(uint8_t gpio,bool pressed) {

}
__attribute__((weak)) void ttgo_on_clicks(uint8_t gpio,unsigned clicks) {

}
__attribute__((weak)) void ttgo_on_long_click(uint8_t gpio) {

}
__attribute__((weak)) void ttgo_on_battery_enabled_changed(bool enabled) {

}
__attribute__((weak)) void ttgo_on_lcd_enabled_changed(bool enabled) {

}

ttgo_screen_t& ttgo_screen(void) {
    return static_cast<ttgo_screen_t&>(devkit_display.active_screen());
}
void ttgo_screen(ttgo_screen_t& screen) {
    devkit_display.active_screen(screen);
}

static void mb_on_pressed_changed(bool pressed, void* state) {
    ttgo_on_pressed_changed((int)state,pressed);
}
static void mb_on_clicks(unsigned clicks, void* state) {
    ttgo_on_clicks((int)state,clicks);
}
static void mb_on_long_click(void* state) {
    ttgo_on_long_click((int)state);
}
static bool battery_enabled;
bool ttgo_battery_enabled(void) {
    return battery_enabled;
}
void ttgo_battery_enable(bool value) {
    if(value!=battery_enabled) {
        gpio_set_level(ADC_EN_GPIO,value);
        battery_enabled = value;
        ttgo_on_battery_enabled_changed(value);
    }
}
static adc_oneshot_unit_handle_t s_adc_handle;
static adc_cali_handle_t         s_cali_handle;

void ttgo_init(ttgo_options_t options) {
    devkit_init();
    // enable battery power
    gpio_config_t en_conf = {
        .pin_bit_mask = (1ULL << ADC_EN_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&en_conf);
    ttgo_battery_enable(true);

    // --- ADC1 oneshot unit ---
    
    adc_oneshot_unit_init_cfg_t unit_cfg;
    memset(&unit_cfg,0,sizeof(unit_cfg));
    unit_cfg.unit_id = BAT_ADC_UNIT;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg;
    memset(&chan_cfg,0,sizeof(chan_cfg));
    chan_cfg.atten    = BAT_ADC_ATTEN;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;    // 12-bit on ESP32
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHANNEL, &chan_cfg));

    // --- Calibration (line fitting for classic ESP32) ---
    adc_cali_line_fitting_config_t cali_cfg;
    memset(&cali_cfg,0,sizeof(cali_cfg));
    cali_cfg.unit_id  = BAT_ADC_UNIT;
    cali_cfg.atten    = BAT_ADC_ATTEN;
    cali_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &s_cali_handle));

    multibutton_config_t cfg;
    cfg.double_click = 0;
    cfg.long_click = 0;
    cfg.on_pressed_changed_callback = mb_on_pressed_changed;
    cfg.on_pressed_changed_callback_state = (void*)TTGO_BUTTON_0;
    if((options&TTGO_BUTTON_0_CLICKS)==TTGO_BUTTON_0_CLICKS) {
        cfg.double_click = 200;
        cfg.on_clicks_callback = mb_on_clicks;
        cfg.on_clicks_callback_state = (void*)TTGO_BUTTON_0;
    } else {
        cfg.on_clicks_callback = nullptr;
    }
    if((options&TTGO_BUTTON_0_LONG)==TTGO_BUTTON_0_LONG) {
        cfg.long_click = 500;
        cfg.on_long_click_callback = mb_on_long_click;
        cfg.on_long_click_callback_state = (void*)TTGO_BUTTON_0;
    } else {
        cfg.on_long_click_callback = nullptr;
    }
    cfg.events_size = MULTIBUTTON_EVENT_SIZE_DEFAULT;
    mb_0_handle = multibutton_init_za(&cfg,mb_0_events,&mb_0_data);
    cfg.double_click = 0;
    cfg.long_click = 0;
    cfg.on_pressed_changed_callback_state = (void*)TTGO_BUTTON_35;
    if((options&TTGO_BUTTON_35_CLICKS)==TTGO_BUTTON_35_CLICKS) {
        cfg.double_click = 200;
        cfg.on_clicks_callback = mb_on_clicks;
        cfg.on_clicks_callback_state = (void*)TTGO_BUTTON_35;
    } else {
        cfg.on_clicks_callback = nullptr;
    }
    if((options&TTGO_BUTTON_35_LONG)==TTGO_BUTTON_35_LONG) {
        cfg.long_click = 500;
        cfg.on_long_click_callback = mb_on_long_click;
        cfg.on_long_click_callback_state = (void*)TTGO_BUTTON_35;
    } else {
        cfg.on_long_click_callback = nullptr;
    }
    mb_35_handle = multibutton_init_za(&cfg,mb_35_events,&mb_35_data);

    ttgo_default_screen.dimensions(TTGO_LCD_DIM);
    devkit_display.active_screen(ttgo_default_screen);
    
}

bool ttgo_pressed(uint8_t gpio) {
    if(gpio==0) {
        return multibutton_pressed(mb_0_handle);
    } else if(gpio==35) {
        return multibutton_pressed(mb_35_handle);
    }
    return false;
}
// Resting-voltage -> percentage points for a single LiPo cell.
// Approximate; based on typical discharge curves. Descending by mV.
typedef struct { int mv; int pct; } batt_point_t;

static const batt_point_t k_curve[] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80},
    {3980, 75},  {3950, 70}, {3910, 65}, {3870, 60}, {3850, 55},
    {3840, 50},  {3820, 45}, {3800, 40}, {3790, 35}, {3770, 30},
    {3750, 25},  {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5},
    {3270, 0},
};
#define CURVE_LEN (sizeof(k_curve) / sizeof(k_curve[0]))
uint16_t ttgo_battery_voltage(void)
{
    int raw = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, BAT_ADC_CHANNEL, &raw));

    int mv = 0;
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali_handle, raw, &mv));

    return mv * 2;   // undo the on-board 1:2 voltage divider
}
// Convert a battery voltage (mV) to an estimated percentage (0-100).
uint8_t ttgo_battery_level(void)
{
    uint16_t mv = ttgo_battery_voltage();
    if (mv >= k_curve[0].mv)            return 100;
    if (mv <= k_curve[CURVE_LEN-1].mv) return 0;

    for (size_t i = 0; i < CURVE_LEN - 1; i++) {
        int hi_mv = k_curve[i].mv,   hi_p = k_curve[i].pct;
        int lo_mv = k_curve[i+1].mv, lo_p = k_curve[i+1].pct;
        if (mv <= hi_mv && mv >= lo_mv) {
            // linear interpolation within this segment
            return lo_p + (mv - lo_mv) * (hi_p - lo_p) / (hi_mv - lo_mv);
        }
    }
    return 0; // unreachable
}
static uint8_t lcd_backlight_percent = 100;
static uint8_t lcd_fade_level=0;
static bool lcd_enabled = false;
bool ttgo_lcd_enabled(void) {
    return lcd_enabled;
}
void ttgo_lcd_enable(bool value) {
    if(value!=lcd_enabled) {
        uint8_t cmd = 0x10|value;
        esp_lcd_panel_io_handle_t h = (esp_lcd_panel_io_handle_t )panel_lcd_io_handle();
        if(h!=nullptr) {
            esp_lcd_panel_io_tx_param(h,cmd,nullptr,0);
        }
        lcd_enabled = value;
        ttgo_on_lcd_enabled_changed(value);
    }
    lcd_fade_level = 0;
    panel_lcd_backlight(value*(lcd_backlight_percent*255/100));
}

void ttgo_backlight(uint8_t percent) {
    if(percent>100) percent = 100;
    lcd_backlight_percent = percent;
    panel_lcd_backlight(percent*255/100);
}
static TickType_t lcd_fade_ts=0;
void ttgo_lcd_fade_to_sleep(void) {
    lcd_fade_level = lcd_backlight_percent*255/100;
}

void ttgo_update(void) {
    bool pressed = panel_button_read(0);
    if(pressed!=mb_0_old_pressed) {
        multibutton_event(mb_0_handle,pdTICKS_TO_MS(xTaskGetTickCount()),pressed);
        mb_0_old_pressed = pressed;
    }
    pressed = panel_button_read(35);
    if(pressed!=mb_35_old_pressed) {
        multibutton_event(mb_35_handle,pdTICKS_TO_MS(xTaskGetTickCount()),pressed);
        mb_35_old_pressed = pressed;
    }
    multibutton_update(mb_0_handle,pdTICKS_TO_MS(xTaskGetTickCount()));
    multibutton_update(mb_35_handle,pdTICKS_TO_MS(xTaskGetTickCount()));
    if(lcd_fade_level>0) {
        if(xTaskGetTickCount()>=lcd_fade_ts+pdMS_TO_TICKS(5)) {
            lcd_fade_ts = xTaskGetTickCount();
            --lcd_fade_level;
            panel_lcd_backlight(lcd_fade_level);
            if(lcd_fade_level==0) {
                ttgo_lcd_enable(false);
                lcd_fade_ts = 0;
            }
        }
    }
    devkit_update();
}