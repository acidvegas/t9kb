// T9KB phone dialer: 123 mode, digits build a number, Call dials it, End hangs up or clears.
// Shows the onKey() callback; text still comes from getChar().
#include <Wire.h>
#include <T9KB.h>

T9KB keypad;
String number;
bool inCall = false;

void onKey(T9Event ev) {
	if (ev.type != T9_PRESS)
		return;
	if (ev.key == T9_CALL && number.length() && !inCall) {
		inCall = true;
		Serial.printf("calling %s\n", number.c_str());
	} else if (ev.key == T9_END) {
		Serial.println(inCall ? "hung up" : "cleared");
		inCall = false;
		number = "";
	}
}

void setup() {
	Serial.begin(115200);
	Wire.begin();
	if (!keypad.begin()) {
		Serial.println("T9KB not found at 0x34");
		while (true)
			delay(1000);
	}
	keypad.setMode(T9_NUMERIC);
	keypad.onKey(onKey);
	Serial.println("dial a number, Call to dial, End to hang up");
}

void loop() {
	keypad.update();
	uint8_t c;
	while ((c = keypad.getChar())) {
		if (inCall)
			continue; // digits during a call would be DTMF; ignored here
		if (c == T9KB_BACKSPACE && number.length())
			number.remove(number.length() - 1);
		else if (isdigit(c) || c == '*' || c == '#' || c == '+')
			number += (char)c;
		else
			continue;
		Serial.printf("number: %s\n", number.c_str());
	}
}
