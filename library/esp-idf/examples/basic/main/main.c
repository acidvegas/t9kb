// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/esp-idf/examples/basic/main/main.c - prints key events and the typed line. Set the pins for your board.

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "t9kb_idf.h"

#define SDA_GPIO 6
#define SCL_GPIO 7
#define INT_GPIO -1 // GPIO wired to the INT pad, or -1 to poll

static const char *const MODES[] = { "T9", "Abc", "abc", "ABC", "123" };

void app_main(void) {
	i2c_master_bus_config_t bus_conf = {
		.i2c_port          = -1,
		.sda_io_num        = SDA_GPIO,
		.scl_io_num        = SCL_GPIO,
		.clk_source        = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		.flags.enable_internal_pullup = true,
	};
	i2c_master_bus_handle_t bus;
	ESP_ERROR_CHECK(i2c_new_master_bus(&bus_conf, &bus));

	static t9kb_idf kb;
	if (t9kb_idf_begin(&kb, bus, INT_GPIO, -1) != ESP_OK)
		printf("T9KB not found at 0x34, waiting for it\n");

	char line[128] = "";
	while (true) {
		t9kb_idf_update(&kb);
		while (t9kb_available(&kb.core)) {
			T9Event ev = t9kb_read(&kb.core);
			if (ev.type == T9_PRESS)
				printf("key %s\n", t9kb_name(ev.key));
			if (ev.key == T9_OK && ev.type == T9_PRESS) {
				printf("sent: %s\n", line);
				line[0] = 0;
				t9kb_clear_text(&kb.core);
			}
		}
		bool changed = false;
		uint8_t c;
		while ((c = t9kb_get_char(&kb.core))) {
			size_t n = strlen(line);
			if (c == T9KB_BACKSPACE && n)
				line[n - 1] = 0;
			else if (c >= ' ' && c < 0x7F && n < sizeof(line) - 1) {
				line[n]     = c;
				line[n + 1] = 0;
			}
			changed = true;
		}
		if (changed)
			printf("[%s] %s%s\n", MODES[t9kb_mode(&kb.core)], line, t9kb_pending(&kb.core) ? " [_]" : "");
		vTaskDelay(pdMS_TO_TICKS(5));
	}
}
