// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/esp-idf/t9kb/include/t9kb_idf.h - ESP-IDF glue for the T9KB core: I2C master driver, INT / RST pins, clock.
// Everything else is the core API on &kb.core: t9kb_get_char(), t9kb_read(), t9kb_set_mode() ... (see t9kb_core.h).
#pragma once

#include "driver/i2c_master.h"
#include "esp_err.h"
#include "t9kb_core.h"

typedef struct {
	t9kb                    core;
	i2c_master_dev_handle_t dev;
	int                     int_gpio;
} t9kb_idf;

// bus: an I2C master bus you created with i2c_new_master_bus(). int_gpio / rst_gpio: the INT / RST pads, or -1.
// Returns ESP_ERR_NOT_FOUND if the keypad doesn't answer.
esp_err_t t9kb_idf_begin(t9kb_idf *kb, i2c_master_bus_handle_t bus, int int_gpio, int rst_gpio);

// Call often, every few milliseconds: reads the keypad and runs the multi-tap, long press and repeat timers.
void t9kb_idf_update(t9kb_idf *kb);
