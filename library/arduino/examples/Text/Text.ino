// T9KB text entry: type on the keypad, the line shows on the serial monitor as it changes.
// Long press # changes mode (T9 / Abc / abc / ABC / 123), right soft key is backspace, OK sends the line.
// In T9 mode press each key once per letter; * shows the next word for the same keys, 0 accepts it with a space.
#include <Wire.h>
#include <T9KB.h>

static const char *const MODES[] = { "T9", "Abc", "abc", "ABC", "123" };

T9KB keypad;
String line;

void setup() {
	Serial.begin(115200);
	Wire.begin();
	if (!keypad.begin()) {
		Serial.println("T9KB not found at 0x34");
		while (true)
			delay(1000);
	}
	Serial.println("T9KB ready");
}

void loop() {
	keypad.update();
	bool changed = false;
	while (keypad.available()) {
		T9Event ev = keypad.read();
		if (ev.key == T9_OK && ev.type == T9_PRESS) {
			Serial.print("sent: ");
			Serial.println(line);
			line = "";
			keypad.clearText(); // the next letter starts a new sentence
		}
		changed |= ev.key == T9_HASH && ev.type == T9_LONG; // mode change
	}
	uint8_t c;
	while ((c = keypad.getChar())) {
		if (c == T9KB_BACKSPACE) {
			if (line.length())
				line.remove(line.length() - 1);
		} else if (c >= ' ' && c < 0x7F) {
			line += (char)c;
		}
		changed = true;
	}
	static bool wasPending = false;
	if (changed || wasPending != keypad.pending()) {
		wasPending = keypad.pending();
		// [_] marks a letter or T9 word that the next key can still change
		Serial.print('[');
		Serial.print(MODES[keypad.mode()]);
		Serial.print("] ");
		Serial.print(line);
		Serial.println(wasPending ? " [_]" : "");
	}
}
