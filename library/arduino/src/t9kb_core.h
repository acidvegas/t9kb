// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/arduino/src/t9kb_core.h - platform independent T9KB driver: key events, multi-tap text and T9 words.
// The Arduino class (T9KB.h) and the ESP-IDF component (library/esp-idf) wrap this; anything else only has to
// supply an I2C register read / write and a millisecond clock.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "t9kb_dict.h"

#ifdef __cplusplus
extern "C" {
#endif

#define T9KB_ADDR 0x34

// Special codes in the text stream, the same values the M5Stack CardKB uses
#define T9KB_BACKSPACE 0x08
#define T9KB_LEFT      0xB4
#define T9KB_UP        0xB5
#define T9KB_DOWN      0xB6
#define T9KB_RIGHT     0xB7

#define T9KB_WORD_MAX 24 // longest T9 key sequence

typedef uint8_t T9Key;
enum {
	T9_NONE,
	T9_1, T9_2, T9_3, T9_4, T9_5, T9_6, T9_7, T9_8, T9_9, T9_0, T9_STAR, T9_HASH,
	T9_SOFT_LEFT, T9_SOFT_MIDDLE, T9_SOFT_RIGHT,
	T9_SPEAKER, T9_MUTE, T9_CALL, T9_END,
	T9_UP, T9_DOWN, T9_LEFT, T9_RIGHT, T9_OK,
};

typedef uint8_t T9EventType;
enum {
	T9_PRESS,
	T9_RELEASE,
	T9_LONG,   // held past the long press time (keys that don't auto-repeat)
	T9_REPEAT, // auto-repeat while held (arrows and the backspace key by default)
};

// Long press of the mode key (# by default) cycles through these in order. T9_WORD is skipped without the word list.
typedef uint8_t T9Mode;
enum {
	T9_WORD,     // T9: one press per letter, the word list picks the word; * next word, 0 accepts with a space
	T9_SENTENCE, // Abc: multi-tap, capital at the start and after ". " "! " "? "
	T9_LOWER,    // abc
	T9_UPPER,    // ABC
	T9_NUMERIC,  // 123
};

typedef struct {
	T9Key       key;
	T9EventType type;
	uint8_t     code; // raw TCA8418 key number (row * 10 + column + 1)
} T9Event;

// I2C access to the TCA8418 at T9KB_ADDR: write one register, read one register. Return false on a bus error.
typedef struct {
	bool (*write)(void *user, uint8_t reg, uint8_t val);
	bool (*read)(void *user, uint8_t reg, uint8_t *val);
	void *user;
} t9kb_io;

typedef void (*t9kb_key_cb)(void *user, T9Event ev);
typedef void (*t9kb_mode_cb)(void *user, T9Mode mode);

typedef struct {
	t9kb_io      io;
	bool         connected;
	uint32_t     last_poll, last_check;
	t9kb_key_cb  key_cb;
	t9kb_mode_cb mode_cb;
	void        *key_user, *mode_user;

	uint16_t tap_timeout, long_press, repeat_delay, repeat_rate;
	const char *taps[9]; // multi-tap characters for 1-9 in the letter modes
	const char *symbols; // multi-tap characters for *
	T9Key    backspace_key, mode_key;
	uint32_t repeat_keys; // bit per T9Key

	uint32_t down; // bit per T9Key
	T9Key    held; // last pressed key, for long press and repeat
	uint8_t  held_code;
	uint32_t held_since, next_repeat;
	bool     long_sent, press_typed;

	T9Mode   mode;
	bool     pending, tap_upper;
	T9Key    tap_key;
	uint8_t  tap_index;
	uint32_t last_tap;

	bool    composing, word_cap; // T9 word in progress
	char    seq[T9KB_WORD_MAX + 1];
	uint8_t seq_len, cand;
	char    word[T9KB_WORD_MAX + 1]; // what the T9 word currently shows

	uint16_t typed; // characters typed and not deleted, for sentence case
	uint8_t  hist[32];
	uint8_t  hist_len;

	T9Event events[16];
	uint8_t ev_head, ev_count;
	uint8_t chars[64];
	uint8_t ch_head, ch_count;
} t9kb;

void t9kb_init(t9kb *k, const t9kb_io *io);             // defaults, no I2C traffic
bool t9kb_begin(t9kb *k);                               // set up the TCA8418, false if it doesn't answer
void t9kb_update(t9kb *k, uint32_t now_ms, int int_level); // int_level: INT pin level (0 = key waiting), -1 to poll
bool t9kb_connected(const t9kb *k);                     // false while unplugged; set up again when it comes back

void    t9kb_on_key(t9kb *k, t9kb_key_cb cb, void *user);   // called from update() instead of queueing events
void    t9kb_on_mode(t9kb *k, t9kb_mode_cb cb, void *user); // called when the mode key changes the mode
bool    t9kb_available(const t9kb *k);
T9Event t9kb_read(t9kb *k);
bool    t9kb_is_pressed(const t9kb *k, T9Key key);

uint8_t     t9kb_get_char(t9kb *k);       // next character or special code, 0 when there is none
bool        t9kb_pending(const t9kb *k);  // the last letter or word can still change
const char *t9kb_tap_set(const t9kb *k);  // characters on the key being multi-tapped while pending, else NULL
const char *t9kb_word(const t9kb *k);     // the T9 word being typed, else NULL
void        t9kb_clear_text(t9kb *k);     // the app cleared its text: start a new sentence

void   t9kb_set_mode(t9kb *k, T9Mode mode);
T9Mode t9kb_mode(const t9kb *k);

void t9kb_set_tap_timeout(t9kb *k, uint16_t ms);                   // default 800
void t9kb_set_long_press(t9kb *k, uint16_t ms);                    // default 600
void t9kb_set_repeat(t9kb *k, uint16_t delay_ms, uint16_t rate_ms); // default 500, 80
void t9kb_set_taps(t9kb *k, T9Key key, const char *chars);         // 1-9 in the letter modes; the string must stay valid
void t9kb_set_symbols(t9kb *k, const char *chars);                 // * in the letter modes
void t9kb_set_backspace_key(t9kb *k, T9Key key);                   // default T9_SOFT_RIGHT, T9_NONE for none
void t9kb_set_mode_key(t9kb *k, T9Key key);                        // long press changes mode, default T9_HASH
void t9kb_set_repeating(t9kb *k, T9Key key, bool on);              // auto-repeat instead of a long press

const char *t9kb_name(T9Key key);

#ifdef __cplusplus
}
#endif
