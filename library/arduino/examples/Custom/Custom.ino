// T9KB custom setup: own punctuation on 1, a shorter symbol list, End as backspace, the middle soft key changes mode,
// a callback on every mode change, and a message when the keypad is unplugged or plugged back in.
#include <Wire.h>
#include <T9KB.h>

static const char *const MODES[] = { "T9", "Abc", "abc", "ABC", "123" };

T9KB keypad;

void onMode(T9Mode mode) {
	Serial.print("mode: ");
	Serial.println(MODES[mode]);
}

void setup() {
	Serial.begin(115200);
	Wire.begin();
	if (!keypad.begin()) {
		Serial.println("T9KB not found at 0x34");
		while (true)
			delay(1000);
	}
	keypad.setTaps(T9_1, ".,?!1");
	keypad.setSymbols("@#$%&*+-=/");
	keypad.setBackspaceKey(T9_END);
	keypad.setModeKey(T9_SOFT_MIDDLE);
	keypad.onModeChange(onMode);
	keypad.setMode(T9_SENTENCE);
	Serial.println("T9KB ready");
}

void loop() {
	keypad.update();
	static bool was = true;
	if (keypad.connected() != was) {
		was = keypad.connected();
		Serial.println(was ? "keypad back" : "keypad unplugged");
	}
	while (keypad.available())
		keypad.read();
	uint8_t c;
	while ((c = keypad.getChar())) {
		if (c == T9KB_BACKSPACE)
			Serial.print("\b \b");
		else if (c >= ' ' && c < 0x7F)
			Serial.print((char)c);
	}
}
