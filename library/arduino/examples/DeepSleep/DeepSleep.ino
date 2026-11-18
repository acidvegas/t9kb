// T9KB deep sleep: the ESP32 sleeps until a key is pressed, using the INT wire to wake.
// Wire the board's INT pad to INT_PIN. It must be an RTC GPIO (ESP32: 0, 2, 4, 12-15, 25-27, 32-39).
// The keypad keeps scanning while the ESP32 sleeps, so the key that woke it is read after boot.
#include <Wire.h>
#include <T9KB.h>
#include <esp_sleep.h>

#define INT_PIN  4
#define IDLE_MS  10000

T9KB keypad;
uint32_t lastKey;

void setup() {
	Serial.begin(115200);
	Wire.begin();
	if (!keypad.begin(Wire, INT_PIN)) {
		Serial.println("T9KB not found at 0x34");
		while (true)
			delay(1000);
	}
	Serial.println(esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0 ? "woke on a keypress" : "power on");
	lastKey = millis();
}

void loop() {
	keypad.update();
	while (keypad.available()) {
		T9Event ev = keypad.read();
		if (ev.type == T9_PRESS)
			Serial.printf("key %s\n", T9KB::name(ev.key));
		lastKey = millis();
	}
	while (keypad.getChar());
	if (millis() - lastKey > IDLE_MS && digitalRead(INT_PIN) == HIGH) {
		Serial.println("sleeping, press any key");
		Serial.flush();
		esp_sleep_enable_ext0_wakeup((gpio_num_t)INT_PIN, 0); // INT goes low on a key event
		esp_deep_sleep_start();
	}
}
