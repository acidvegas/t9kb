// T9KB key events: prints every press, release, long press and repeat with the raw TCA8418 key number.
// Set INT_PIN to the GPIO wired to the board's INT pad, or leave it at -1 to poll over I2C.
#include <Wire.h>
#include <T9KB.h>

#define INT_PIN -1

T9KB keypad;

void setup() {
	Serial.begin(115200);
	Wire.begin();
	if (!keypad.begin(Wire, INT_PIN)) {
		Serial.println("T9KB not found at 0x34");
		while (true)
			delay(1000);
	}
	Serial.println("T9KB ready");
}

void loop() {
	keypad.update();
	while (keypad.available()) {
		static const char *const TYPES[] = { "press", "release", "long", "repeat" };
		T9Event ev = keypad.read();
		Serial.printf("%-11s %-7s raw %d\n", T9KB::name(ev.key), TYPES[ev.type], ev.code);
	}
	while (keypad.getChar()); // text is not used here, keep the queue empty
}
