// T9KB - Arduino library for the T9KB I2C phone keypad (TI TCA8418 @ 0x34)
// https://github.com/acidvegas/t9kb
#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "t9kb_core.h"

typedef void (*T9Callback)(T9Event ev);
typedef void (*T9ModeCallback)(T9Mode mode);

class T9KB {
public:
	// Call Wire.begin() first. intPin: GPIO wired to the INT pad, or -1 to poll over I2C.
	// rstPin: GPIO wired to the RST pad to reset the TCA8418 first, or -1.
	bool begin(TwoWire &wire = Wire, int intPin = -1, int rstPin = -1);

	// Call often from loop(). Reads the keypad and runs the multi-tap, long press and repeat timers.
	void update();
	bool connected() const; // false while unplugged; it is set up again by itself when it comes back

	// Key events: set a callback (called from update()), or read them from the queue
	void    onKey(T9Callback cb);
	bool    available() const;
	T9Event read();
	bool    isPressed(T9Key key) const;

	// Text, CardKB style: returns the next character or special code, 0 when there is none.
	// Multi-tap is a live preview: tapping 2 twice gives 'a', then T9KB_BACKSPACE and 'b'. T9 words rewrite the same way.
	uint8_t     getChar();
	bool        pending() const;   // the last letter or the T9 word can still change
	const char *tapSet() const;    // characters on the key being multi-tapped while pending(), else nullptr
	const char *word() const;      // the T9 word being typed, else nullptr
	void        clearText();       // call when your text is cleared, so the next letter starts a sentence

	void   setMode(T9Mode mode);
	T9Mode mode() const;
	void   onModeChange(T9ModeCallback cb); // called when the mode key changes the mode

	void setTapTimeout(uint16_t ms);                   // default 800
	void setLongPress(uint16_t ms);                    // default 600
	void setRepeat(uint16_t delayMs, uint16_t rateMs); // default 500, 80
	void setTaps(T9Key key, const char *chars);        // multi-tap characters for 1-9, e.g. setTaps(T9_1, ".,?!1"); keep the string alive
	void setSymbols(const char *chars);                // multi-tap characters for *
	void setBackspaceKey(T9Key key);                   // default T9_SOFT_RIGHT, T9_NONE for none
	void setModeKey(T9Key key);                        // long press changes mode, default T9_HASH
	void setRepeating(T9Key key, bool on);             // auto-repeat while held instead of a long press

	static const char *name(T9Key key);

private:
	t9kb           _core;
	TwoWire       *_wire = nullptr;
	int            _intPin = -1;
	T9Callback     _cb = nullptr;
	T9ModeCallback _modeCb = nullptr;

	static bool writeReg(void *self, uint8_t reg, uint8_t val);
	static bool readReg(void *self, uint8_t reg, uint8_t *val);
	static void keyThunk(void *self, T9Event ev);
	static void modeThunk(void *self, T9Mode mode);
};
