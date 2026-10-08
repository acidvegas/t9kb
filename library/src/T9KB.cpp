// T9KB - Arduino library for the T9KB I2C phone keypad (TI TCA8418 @ 0x34)
// https://github.com/acidvegas/t9kb
#include "T9KB.h"

// TCA8418 registers (TI datasheet SCPS215G)
#define REG_CFG         0x01
#define REG_INT_STAT    0x02
#define REG_KEY_LCK_EC  0x03
#define REG_KEY_EVENT_A 0x04
#define REG_KP_GPIO1    0x1D
#define REG_KP_GPIO2    0x1E
#define REG_KP_GPIO3    0x1F

#define POLL_MS 10

// Board wiring: matrix row / column -> key
static const T9Key KEYMAP[4][6] = {
	{ T9_1,    T9_2, T9_3,    T9_SOFT_LEFT, T9_SOFT_MIDDLE, T9_SOFT_RIGHT },
	{ T9_4,    T9_5, T9_6,    T9_SPEAKER,   T9_UP,          T9_MUTE       },
	{ T9_7,    T9_8, T9_9,    T9_LEFT,      T9_OK,          T9_RIGHT      },
	{ T9_STAR, T9_0, T9_HASH, T9_CALL,      T9_DOWN,        T9_END        },
};

// Multi-tap characters in the letter modes, for T9_1 .. T9_9
static const char *const TAPS[9] = { "1.,'?!\"-", "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9" };
static const char SYMBOLS[] = ".,'?!\"-()@/:_;+%*=<>$[]{}\\~^#|`&";

static const char *const NAMES[] = {
	"NONE", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "*", "#",
	"SOFT_LEFT", "SOFT_MIDDLE", "SOFT_RIGHT", "SPEAKER", "MUTE", "CALL", "END",
	"UP", "DOWN", "LEFT", "RIGHT", "OK",
};

static bool isDigitKey(T9Key k) { return k >= T9_1 && k <= T9_0; }
static char digitOf(T9Key k)    { return k == T9_0 ? '0' : '0' + (k - T9_1 + 1); }

// Keys that type a special code and auto-repeat while held
static uint8_t specialChar(T9Key k) {
	switch (k) {
		case T9_SOFT_RIGHT: return T9KB_BACKSPACE;
		case T9_LEFT:       return T9KB_LEFT;
		case T9_UP:         return T9KB_UP;
		case T9_DOWN:       return T9KB_DOWN;
		case T9_RIGHT:      return T9KB_RIGHT;
		default:            return 0;
	}
}


bool T9KB::begin(TwoWire &wire, int intPin, int rstPin) {
	_wire = &wire;
	_intPin = intPin;
	if (rstPin >= 0) { // >= 120 us low pulse and 120 us recovery per the datasheet; the board's 10k pull-up releases it
		pinMode(rstPin, OUTPUT);
		digitalWrite(rstPin, LOW);
		delayMicroseconds(200);
		pinMode(rstPin, INPUT);
		delayMicroseconds(200);
	}
	if (_intPin >= 0)
		pinMode(_intPin, INPUT_PULLUP);
	_wire->beginTransmission(T9KB_ADDR);
	if (_wire->endTransmission() != 0)
		return false;
	writeReg(REG_KP_GPIO1, 0x0F); // ROW0-3
	writeReg(REG_KP_GPIO2, 0x3F); // COL0-5
	writeReg(REG_KP_GPIO3, 0x00);
	return writeReg(REG_CFG, 0x01); // KE_IEN: INT goes low on key events
}


void T9KB::update() {
	uint32_t now = millis();
	bool check = _intPin >= 0 ? digitalRead(_intPin) == LOW : now - _lastPoll >= POLL_MS;
	if (check) {
		_lastPoll = now;
		readFifo(now);
	}
	if (_pending && now - _lastTap >= _tapTimeout)
		_pending = false;
	if (_held == T9_NONE)
		return;
	if (specialChar(_held)) {
		if ((int32_t)(now - _nextRepeat) >= 0) {
			pushEvent(_held, T9_REPEAT, _heldCode);
			pushChar(specialChar(_held));
			_nextRepeat = now + _repeatRate;
		}
	} else if (!_longSent && now - _heldSince >= _longPress) {
		_longSent = true;
		pushEvent(_held, T9_LONG, _heldCode);
		onLong(_held);
	}
}


void T9KB::readFifo(uint32_t now) {
	// TI's procedure: read the event count, pop KEY_EVENT_A until empty, then clear K_INT
	uint8_t count = readReg(REG_KEY_LCK_EC) & 0x0F;
	for (uint8_t i = 0; i < count; i++) {
		uint8_t ev = readReg(REG_KEY_EVENT_A);
		if (!ev)
			break;
		handle(ev & 0x7F, ev & 0x80, now);
	}
	if (count || _intPin >= 0)
		writeReg(REG_INT_STAT, 0x01);
}


void T9KB::handle(uint8_t code, bool pressed, uint32_t now) {
	if (code < 1 || code > 80)
		return;
	uint8_t row = (code - 1) / 10, col = (code - 1) % 10;
	if (row > 3 || col > 5)
		return;
	T9Key key = KEYMAP[row][col];
	pushEvent(key, pressed ? T9_PRESS : T9_RELEASE, code);
	if (pressed) {
		_down |= 1UL << key;
		_held = key;
		_heldCode = code;
		_heldSince = now;
		_nextRepeat = now + _repeatDelay;
		_longSent = false;
		onPress(key, now);
	} else {
		_down &= ~(1UL << key);
		if (key == _held)
			_held = T9_NONE;
	}
}


void T9KB::onPress(T9Key key, uint32_t now) {
	bool text = isDigitKey(key) || key == T9_STAR || key == T9_HASH;
	if (!text || _mode == T9_NUMERIC || key == T9_HASH || key == T9_0) {
		_pending = false;
		if (!text)
			pushChar(specialChar(key)); // arrows and backspace; OK, call, end and the rest only send events
		else if (_mode == T9_NUMERIC)
			pushChar(key == T9_STAR ? '*' : key == T9_HASH ? '#' : digitOf(key));
		else
			pushChar(key == T9_HASH ? '#' : ' ');
		return;
	}
	// Multi-tap: 1-9 and * in the letter modes
	const char *set = key == T9_STAR ? SYMBOLS : TAPS[key - T9_1];
	if (_pending && key == _tapKey && now - _lastTap < _tapTimeout) {
		_tapIndex = (_tapIndex + 1) % strlen(set);
		pushChar(T9KB_BACKSPACE);
	} else {
		_tapKey = key;
		_tapIndex = 0;
		_tapUpper = _mode == T9_UPPER || (_mode == T9_SENTENCE && sentenceStart());
	}
	char c = set[_tapIndex];
	pushChar(_tapUpper ? toupper(c) : c);
	_pending = true;
	_lastTap = now;
}


void T9KB::onLong(T9Key key) {
	if (key == T9_HASH) { // remove the '#' and change mode
		pushChar(T9KB_BACKSPACE);
		nextMode();
	} else if (_mode == T9_NUMERIC) {
		if (key == T9_0) {
			pushChar(T9KB_BACKSPACE);
			pushChar('+');
		}
	} else if (isDigitKey(key)) { // swap the preview (or the 0 key's space) for the digit
		_pending = false;
		pushChar(T9KB_BACKSPACE);
		pushChar(digitOf(key));
	}
}


bool T9KB::sentenceStart() const {
	if (!_typed)
		return true;
	return _histLen >= 2 && _hist[_histLen - 1] == ' ' && strchr(".!?", _hist[_histLen - 2]);
}


void T9KB::nextMode() {
	_pending = false;
	_mode = (T9Mode)((_mode + 1) % 4);
}


void T9KB::pushEvent(T9Key key, T9EventType type, uint8_t code) {
	if (_cb) {
		_cb({ key, type, code });
		return;
	}
	if (_evCount == sizeof(_events) / sizeof(_events[0]))
		return; // full: drop the newest
	_events[(_evHead + _evCount++) % (sizeof(_events) / sizeof(_events[0]))] = { key, type, code };
}


void T9KB::pushChar(uint8_t c) {
	if (!c)
		return;
	if (c == T9KB_BACKSPACE) {
		if (_typed)
			_typed--;
		if (_histLen)
			_histLen--;
	} else if (c >= ' ' && c < 0x7F) {
		_typed++;
		if (_histLen == sizeof(_hist))
			memmove(_hist, _hist + 1, --_histLen);
		_hist[_histLen++] = c;
	}
	if (_chCount < sizeof(_chars))
		_chars[(_chHead + _chCount++) % sizeof(_chars)] = c;
}


void T9KB::onKey(T9Callback cb) { _cb = cb; }

bool T9KB::available() const { return _evCount > 0; }

T9Event T9KB::read() {
	if (!_evCount)
		return { T9_NONE, T9_PRESS, 0 };
	T9Event ev = _events[_evHead];
	_evHead = (_evHead + 1) % (sizeof(_events) / sizeof(_events[0]));
	_evCount--;
	return ev;
}

bool T9KB::isPressed(T9Key key) const { return key != T9_NONE && (_down & (1UL << key)); }

uint8_t T9KB::getChar() {
	if (!_chCount)
		return 0;
	uint8_t c = _chars[_chHead];
	_chHead = (_chHead + 1) % sizeof(_chars);
	_chCount--;
	return c;
}

bool   T9KB::pending() const { return _pending; }
T9Mode T9KB::mode() const    { return _mode; }

void T9KB::setMode(T9Mode mode) {
	_pending = false;
	_mode = mode;
}

void T9KB::setTapTimeout(uint16_t ms) { _tapTimeout = ms; }
void T9KB::setLongPress(uint16_t ms)  { _longPress = ms; }

void T9KB::setRepeat(uint16_t delayMs, uint16_t rateMs) {
	_repeatDelay = delayMs;
	_repeatRate = rateMs;
}

const char *T9KB::name(T9Key key) { return key <= T9_OK ? NAMES[key] : "?"; }


bool T9KB::writeReg(uint8_t reg, uint8_t val) {
	_wire->beginTransmission(T9KB_ADDR);
	_wire->write(reg);
	_wire->write(val);
	return _wire->endTransmission() == 0;
}

uint8_t T9KB::readReg(uint8_t reg) {
	_wire->beginTransmission(T9KB_ADDR);
	_wire->write(reg);
	if (_wire->endTransmission(false) != 0)
		return 0;
	if (_wire->requestFrom((uint8_t)T9KB_ADDR, (uint8_t)1) != 1)
		return 0;
	return _wire->read();
}
