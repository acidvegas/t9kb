// T9KB - Arduino library for the T9KB I2C phone keypad (TI TCA8418 @ 0x34)
// https://github.com/acidvegas/t9kb
#pragma once

#include <Arduino.h>
#include <Wire.h>

#define T9KB_ADDR 0x34

// Special codes returned by getChar(), the same values the M5Stack CardKB uses
#define T9KB_BACKSPACE 0x08
#define T9KB_LEFT      0xB4
#define T9KB_UP        0xB5
#define T9KB_DOWN      0xB6
#define T9KB_RIGHT     0xB7

enum T9Key : uint8_t {
	T9_NONE,
	T9_1, T9_2, T9_3, T9_4, T9_5, T9_6, T9_7, T9_8, T9_9, T9_0, T9_STAR, T9_HASH,
	T9_SOFT_LEFT, T9_SOFT_MIDDLE, T9_SOFT_RIGHT,
	T9_SPEAKER, T9_MUTE, T9_CALL, T9_END,
	T9_UP, T9_DOWN, T9_LEFT, T9_RIGHT, T9_OK,
};

enum T9EventType : uint8_t {
	T9_PRESS,
	T9_RELEASE,
	T9_LONG,   // held past the long press time (keys without auto-repeat)
	T9_REPEAT, // auto-repeat while held (arrows and the right soft key)
};

// Long press # cycles through these in order
enum T9Mode : uint8_t {
	T9_SENTENCE, // Abc: capital at the start and after ". " "! " "? "
	T9_LOWER,    // abc
	T9_UPPER,    // ABC
	T9_NUMERIC,  // 123
};

struct T9Event {
	T9Key       key;
	T9EventType type;
	uint8_t     code; // raw TCA8418 key number (row * 10 + column + 1)
};

typedef void (*T9Callback)(T9Event ev);

class T9KB {
public:
	// Call Wire.begin() first. intPin: GPIO wired to the INT pad, or -1 to poll over I2C.
	// rstPin: GPIO wired to the RST pad to reset the TCA8418 first, or -1.
	bool begin(TwoWire &wire = Wire, int intPin = -1, int rstPin = -1);

	// Call often from loop(). Reads the keypad and runs the multi-tap, long press and repeat timers.
	void update();

	// Key events: set a callback (called from update()), or read them from the queue
	void    onKey(T9Callback cb);
	bool    available() const;
	T9Event read();
	bool    isPressed(T9Key key) const;

	// Text, CardKB style: returns the next character or special code, 0 when there is none.
	// Multi-tap is a live preview: tapping 2 twice gives 'a', then T9KB_BACKSPACE and 'b'.
	uint8_t getChar();
	bool    pending() const; // the last character is a preview that can still change

	void   setMode(T9Mode mode);
	T9Mode mode() const;

	void setTapTimeout(uint16_t ms);                   // default 800
	void setLongPress(uint16_t ms);                    // default 600
	void setRepeat(uint16_t delayMs, uint16_t rateMs); // default 500, 80

	static const char *name(T9Key key);

private:
	TwoWire   *_wire = nullptr;
	int        _intPin = -1;
	uint32_t   _lastPoll = 0;
	T9Callback _cb = nullptr;

	uint16_t _tapTimeout = 800;
	uint16_t _longPress = 600;
	uint16_t _repeatDelay = 500;
	uint16_t _repeatRate = 80;

	uint32_t _down = 0; // bit per T9Key
	T9Key    _held = T9_NONE; // last pressed key, for long press and repeat
	uint8_t  _heldCode = 0;
	uint32_t _heldSince = 0;
	uint32_t _nextRepeat = 0;
	bool     _longSent = false;

	T9Mode   _mode = T9_SENTENCE;
	bool     _pending = false;
	bool     _tapUpper = false;
	T9Key    _tapKey = T9_NONE;
	uint8_t  _tapIndex = 0;
	uint32_t _lastTap = 0;

	uint16_t _typed = 0; // characters typed and not deleted, for sentence case
	uint8_t  _hist[32];
	uint8_t  _histLen = 0;

	T9Event _events[16];
	uint8_t _evHead = 0, _evCount = 0;
	uint8_t _chars[32];
	uint8_t _chHead = 0, _chCount = 0;

	bool    writeReg(uint8_t reg, uint8_t val);
	uint8_t readReg(uint8_t reg);
	void    readFifo(uint32_t now);
	void    handle(uint8_t code, bool pressed, uint32_t now);
	void    onPress(T9Key key, uint32_t now);
	void    onLong(T9Key key);
	bool    sentenceStart() const;
	void    nextMode();
	void    pushEvent(T9Key key, T9EventType type, uint8_t code);
	void    pushChar(uint8_t c);
};
