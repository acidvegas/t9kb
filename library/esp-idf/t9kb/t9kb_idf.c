// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/esp-idf/t9kb/t9kb_idf.c

#include "t9kb_idf.h"

#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"

#define I2C_HZ      400000
#define I2C_TIMEOUT 50 // ms


static bool write_reg(void *user, uint8_t reg, uint8_t val) {
	uint8_t buf[2] = { reg, val };
	return i2c_master_transmit(((t9kb_idf *)user)->dev, buf, 2, I2C_TIMEOUT) == ESP_OK;
}

static bool read_reg(void *user, uint8_t reg, uint8_t *val) {
	return i2c_master_transmit_receive(((t9kb_idf *)user)->dev, &reg, 1, val, 1, I2C_TIMEOUT) == ESP_OK;
}


esp_err_t t9kb_idf_begin(t9kb_idf *kb, i2c_master_bus_handle_t bus, int int_gpio, int rst_gpio) {
	i2c_device_config_t dev = {
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address  = T9KB_ADDR,
		.scl_speed_hz    = I2C_HZ,
	};
	esp_err_t err = i2c_master_bus_add_device(bus, &dev, &kb->dev);
	if (err != ESP_OK)
		return err;
	kb->int_gpio = int_gpio;
	t9kb_io io = { write_reg, read_reg, kb };
	t9kb_init(&kb->core, &io);
	if (rst_gpio >= 0) { // >= 120 us low pulse and 120 us recovery per the datasheet; the board's 10k pull-up releases it
		gpio_reset_pin(rst_gpio);
		gpio_set_direction(rst_gpio, GPIO_MODE_OUTPUT);
		gpio_set_level(rst_gpio, 0);
		esp_rom_delay_us(200);
		gpio_set_direction(rst_gpio, GPIO_MODE_INPUT);
		esp_rom_delay_us(200);
	}
	if (int_gpio >= 0) {
		gpio_config_t io_conf = {
			.pin_bit_mask = 1ULL << int_gpio,
			.mode         = GPIO_MODE_INPUT,
			.pull_up_en   = GPIO_PULLUP_ENABLE,
		};
		gpio_config(&io_conf);
	}
	return t9kb_begin(&kb->core) ? ESP_OK : ESP_ERR_NOT_FOUND;
}

void t9kb_idf_update(t9kb_idf *kb) {
	t9kb_update(&kb->core, (uint32_t)(esp_timer_get_time() / 1000), kb->int_gpio >= 0 ? gpio_get_level(kb->int_gpio) : -1);
}
