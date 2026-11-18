// T9KB - Arduino library for the T9KB I2C phone keypad (TI TCA8418 @ 0x34)
// https://github.com/acidvegas/t9kb
#include "T9KB.h"


bool T9KB::begin(TwoWire &wire, int intPin, int rstPin) {
	_wire   = &wire;
	_intPin = intPin;
	t9kb_io io = { writeReg, readReg, this };
	t9kb_init(&_core, &io);
	if (rstPin >= 0) { // >= 120 us low pulse and 120 us recovery per the datasheet; the board's 10k pull-up releases it
		pinMode(rstPin, OUTPUT);
		digitalWrite(rstPin, LOW);
		delayMicroseconds(200);
		pinMode(rstPin, INPUT);
		delayMicroseconds(200);
	}
	if (_intPin >= 0)
		pinMode(_intPin, INPUT_PULLUP);
	return t9kb_begin(&_core);
}


void T9KB::update() {
	t9kb_update(&_core, millis(), _intPin >= 0 ? digitalRead(_intPin) : -1);
}


bool T9KB::writeReg(void *self, uint8_t reg, uint8_t val) {
	TwoWire *w = ((T9KB *)self)->_wire;
	w->beginTransmission(T9KB_ADDR);
	w->write(reg);
	w->write(val);
	return w->endTransmission() == 0;
}

bool T9KB::readReg(void *self, uint8_t reg, uint8_t *val) {
	TwoWire *w = ((T9KB *)self)->_wire;
	w->beginTransmission(T9KB_ADDR);
	w->write(reg);
	if (w->endTransmission(false) != 0)
		return false;
	if (w->requestFrom((uint8_t)T9KB_ADDR, (uint8_t)1) != 1)
		return false;
	*val = w->read();
	return true;
}

void T9KB::keyThunk(void *self, T9Event ev)   { ((T9KB *)self)->_cb(ev); }
void T9KB::modeThunk(void *self, T9Mode mode) { ((T9KB *)self)->_modeCb(mode); }


bool T9KB::connected() const { return t9kb_connected(&_core); }

void T9KB::onKey(T9Callback cb) {
	_cb = cb;
	t9kb_on_key(&_core, cb ? keyThunk : nullptr, this);
}

void T9KB::onModeChange(T9ModeCallback cb) {
	_modeCb = cb;
	t9kb_on_mode(&_core, cb ? modeThunk : nullptr, this);
}

bool        T9KB::available() const           { return t9kb_available(&_core); }
T9Event     T9KB::read()                      { return t9kb_read(&_core); }
bool        T9KB::isPressed(T9Key key) const  { return t9kb_is_pressed(&_core, key); }
uint8_t     T9KB::getChar()                   { return t9kb_get_char(&_core); }
bool        T9KB::pending() const             { return t9kb_pending(&_core); }
const char *T9KB::tapSet() const              { return t9kb_tap_set(&_core); }
const char *T9KB::word() const                { return t9kb_word(&_core); }
void        T9KB::clearText()                 { t9kb_clear_text(&_core); }
void        T9KB::setMode(T9Mode mode)        { t9kb_set_mode(&_core, mode); }
T9Mode      T9KB::mode() const                { return t9kb_mode(&_core); }

void T9KB::setTapTimeout(uint16_t ms)                   { t9kb_set_tap_timeout(&_core, ms); }
void T9KB::setLongPress(uint16_t ms)                    { t9kb_set_long_press(&_core, ms); }
void T9KB::setRepeat(uint16_t delayMs, uint16_t rateMs) { t9kb_set_repeat(&_core, delayMs, rateMs); }
void T9KB::setTaps(T9Key key, const char *chars)        { t9kb_set_taps(&_core, key, chars); }
void T9KB::setSymbols(const char *chars)                { t9kb_set_symbols(&_core, chars); }
void T9KB::setBackspaceKey(T9Key key)                   { t9kb_set_backspace_key(&_core, key); }
void T9KB::setModeKey(T9Key key)                        { t9kb_set_mode_key(&_core, key); }
void T9KB::setRepeating(T9Key key, bool on)             { t9kb_set_repeating(&_core, key, on); }

const char *T9KB::name(T9Key key) { return t9kb_name(key); }
