// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/arduino/src/t9kb_core.c

#include "t9kb_core.h"

#include <ctype.h>
#include <string.h>

// TCA8418 registers (TI datasheet SCPS215G)
#define REG_CFG         0x01
#define REG_INT_STAT    0x02
#define REG_KEY_LCK_EC  0x03
#define REG_KEY_EVENT_A 0x04
#define REG_KP_GPIO1    0x1D
#define REG_KP_GPIO2    0x1E
#define REG_KP_GPIO3    0x1F

// CFG: KE_IEN (INT on key events) + OVR_FLOW_IEN + OVR_FLOW_M. With OVR_FLOW_M a full FIFO drops its oldest event
// instead of the newest, so a release is never lost and no key can look stuck; the errata needs both overflow bits.
#define CFG_VALUE 0x29
#define INT_K     0x01
#define INT_OVR   0x08

#define POLL_MS  10
#define CHECK_MS 1000 // how often the setup is checked, to catch an unplugged or reset keypad

// Board wiring: matrix row / column -> key
static const T9Key KEYMAP[4][6] = {
	{ T9_1,    T9_2, T9_3,    T9_SOFT_LEFT, T9_SOFT_MIDDLE, T9_SOFT_RIGHT },
	{ T9_4,    T9_5, T9_6,    T9_SPEAKER,   T9_UP,          T9_MUTE       },
	{ T9_7,    T9_8, T9_9,    T9_LEFT,      T9_OK,          T9_RIGHT      },
	{ T9_STAR, T9_0, T9_HASH, T9_CALL,      T9_DOWN,        T9_END        },
};

// Multi-tap characters in the letter modes, for T9_1 .. T9_9
static const char *const TAPS[9] = { ".,?!'\"-1", "abc2", "def3", "ghi4", "jkl5", "mno6", "pqrs7", "tuv8", "wxyz9" };
static const char SYMBOLS[] = ".,'?!\"-()@/:_;+%*=<>$[]{}\\~^#|`&";

static const char *const NAMES[] = {
	"NONE", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "*", "#",
	"SOFT_LEFT", "SOFT_MIDDLE", "SOFT_RIGHT", "SPEAKER", "MUTE", "CALL", "END",
	"UP", "DOWN", "LEFT", "RIGHT", "OK",
};

#define BIT(key) (1UL << (key))

static bool is_digit_key(T9Key k) { return k >= T9_1 && k <= T9_0; }
static char digit_of(T9Key k)     { return k == T9_0 ? '0' : '0' + (k - T9_1 + 1); }

static void push_char(t9kb *k, uint8_t c);
static void commit(t9kb *k);


// --- T9 word list ---

#if T9KB_DICT
static const char KEY_OF[] = "22233344455566677778889999"; // letter -> key

typedef struct {
	uint16_t block;
	uint8_t  i, len, rank;
	uint32_t off;
	char     w[16];
} cursor;

static void cur_start(cursor *c, uint16_t block) {
	c->block = block;
	c->i     = 0;
	c->off   = t9kb_dict_index[block];
}

// Decode the next word into c->w; false past the last one
static bool cur_next(cursor *c) {
	if (c->i == t9kb_dict_block) {
		if (++c->block >= t9kb_dict_blocks)
			return false;
		cur_start(c, c->block);
	}
	if ((uint32_t)c->block * t9kb_dict_block + c->i >= t9kb_dict_words)
		return false;
	uint8_t head = t9kb_dict_data[c->off++], shared = head >> 4, n = head & 0x0F;
	c->rank = t9kb_dict_data[c->off++];
	for (uint8_t j = 0; j < n; j++)
		c->w[shared + j] = t9kb_dict_data[c->off++];
	c->len = shared + n;
	c->w[c->len] = 0;
	c->i++;
	return true;
}

// Compare a word's key sequence with seq: < 0, 0 or > 0, a shorter sequence sorting first
static int cmp_keys(const cursor *c, const char *seq, uint8_t len) {
	for (uint8_t i = 0; i < c->len && i < len; i++) {
		char d = KEY_OF[c->w[i] - 'a'];
		if (d != seq[i])
			return d < seq[i] ? -1 : 1;
	}
	return c->len < len ? -1 : c->len > len;
}

static bool starts_with(const cursor *c, const char *seq, uint8_t len) {
	if (c->len <= len)
		return false;
	for (uint8_t i = 0; i < len; i++)
		if (KEY_OF[c->w[i] - 'a'] != seq[i])
			return false;
	return true;
}

// The nth most common word for seq. Returns how many words match; with none, out is the start of the most common
// longer word, or empty.
static uint8_t dict_match(const char *seq, uint8_t len, uint8_t nth, char *out) {
	uint16_t lo = 0, hi = t9kb_dict_blocks;
	cursor c;
	while (lo < hi) { // first block whose first word is not below seq
		uint16_t mid = (lo + hi) / 2;
		cur_start(&c, mid);
		cur_next(&c);
		if (cmp_keys(&c, seq, len) < 0)
			lo = mid + 1;
		else
			hi = mid;
	}
	cur_start(&c, lo ? lo - 1 : 0);
	uint8_t  count = 0, best = 0xFF;
	uint16_t longer = 0;
	out[0] = 0;
	while (cur_next(&c)) {
		int r = cmp_keys(&c, seq, len);
		if (r < 0)
			continue;
		if (r == 0) {
			if (count++ == nth)
				memcpy(out, c.w, c.len + 1);
			continue;
		}
		if (count || !starts_with(&c, seq, len) || ++longer > 4000)
			break;
		if (c.rank < best) {
			best = c.rank;
			memcpy(out, c.w, len);
			out[len] = 0;
		}
	}
	return count;
}
#endif

// Show the word for the current key sequence, rewriting only the letters that changed
static void t9_render(t9kb *k) {
	char next[T9KB_WORD_MAX + 1] = "";
#if T9KB_DICT
	uint8_t n = dict_match(k->seq, k->seq_len, k->cand, next);
	if (n && k->cand >= n) {
		k->cand = 0;
		dict_match(k->seq, k->seq_len, 0, next);
	}
#endif
	if (!next[0])
		memcpy(next, k->seq, k->seq_len + 1); // nothing matches: show the keys
	if (k->word_cap || !strcmp(next, "i"))
		next[0] = toupper((unsigned char)next[0]);
	uint8_t old = strlen(k->word), same = 0;
	while (same < old && next[same] == k->word[same])
		same++;
	for (uint8_t i = same; i < old; i++)
		push_char(k, T9KB_BACKSPACE);
	for (uint8_t i = same; next[i]; i++)
		push_char(k, next[i]);
	memcpy(k->word, next, sizeof(next));
}

// Take one key off the T9 word, or remove it when it was the last
static void t9_unkey(t9kb *k) {
	if (--k->seq_len) {
		k->seq[k->seq_len] = 0;
		k->cand = 0;
		t9_render(k);
		return;
	}
	for (uint8_t i = strlen(k->word); i; i--)
		push_char(k, T9KB_BACKSPACE);
	k->composing = false;
	k->word[0]   = 0;
}


// --- text ---

static bool sentence_start(const t9kb *k) {
	if (!k->typed)
		return true;
	return k->hist_len >= 2 && k->hist[k->hist_len - 1] == ' ' && strchr(".!?", k->hist[k->hist_len - 2]);
}

static void commit(t9kb *k) {
	k->composing = false;
	k->seq_len   = 0;
	k->word[0]   = 0;
}

static void next_mode(t9kb *k) {
	commit(k);
	k->pending = false;
	k->mode    = (k->mode + 1) % 5;
	if (k->mode == T9_WORD && !T9KB_DICT)
		k->mode = T9_SENTENCE;
	if (k->mode_cb)
		k->mode_cb(k->mode_user, k->mode);
}

static uint8_t special_char(const t9kb *k, T9Key key) {
	if (key == k->backspace_key)
		return T9KB_BACKSPACE;
	switch (key) {
		case T9_LEFT:  return T9KB_LEFT;
		case T9_UP:    return T9KB_UP;
		case T9_DOWN:  return T9KB_DOWN;
		case T9_RIGHT: return T9KB_RIGHT;
		default:       return 0;
	}
}

// A key that types no text: backspace, arrows, or nothing (OK, call, end, ...). Also runs on auto-repeat.
static void type_special(t9kb *k, T9Key key) {
	k->pending = false;
	if (key == k->backspace_key && k->composing) {
		t9_unkey(k);
		return;
	}
	commit(k);
	push_char(k, special_char(k, key));
}

static void on_press(t9kb *k, T9Key key, uint32_t now) {
	k->press_typed = false;
	if (!is_digit_key(key) && key != T9_STAR && key != T9_HASH) {
		type_special(k, key);
		return;
	}
	if (k->mode == T9_WORD) {
		if (key >= T9_2 && key <= T9_9) {
			k->pending = false;
			if (!k->composing) {
				k->composing = true;
				k->word_cap  = sentence_start(k);
				k->cand      = 0;
			}
			if (k->seq_len < T9KB_WORD_MAX) {
				k->seq[k->seq_len++] = digit_of(key);
				k->seq[k->seq_len]   = 0;
				k->cand = 0;
				t9_render(k);
			}
			return;
		}
		if (key == T9_STAR && k->composing) { // next word for these keys
			k->cand++;
			t9_render(k);
			return;
		}
		commit(k); // 1, 0, # and * carry on as in the letter modes: punctuation, space, '#' and symbols
	}
	k->press_typed = true;
	if (k->mode == T9_NUMERIC || key == T9_HASH || key == T9_0) {
		k->pending = false;
		if (k->mode == T9_NUMERIC)
			push_char(k, key == T9_STAR ? '*' : key == T9_HASH ? '#' : digit_of(key));
		else
			push_char(k, key == T9_HASH ? '#' : ' ');
		return;
	}
	// Multi-tap: 1-9 and * in the letter modes, 1 and * in T9
	const char *set = key == T9_STAR ? k->symbols : k->taps[key - T9_1];
	if (k->pending && key == k->tap_key && now - k->last_tap < k->tap_timeout) {
		k->tap_index = (k->tap_index + 1) % strlen(set);
		push_char(k, T9KB_BACKSPACE);
	} else {
		k->tap_key   = key;
		k->tap_index = 0;
		k->tap_upper = k->mode == T9_UPPER || ((k->mode == T9_SENTENCE || k->mode == T9_WORD) && sentence_start(k));
	}
	char c = set[k->tap_index];
	push_char(k, k->tap_upper ? toupper((unsigned char)c) : c);
	k->pending  = true;
	k->last_tap = now;
}

static void on_long(t9kb *k, T9Key key) {
	if (key == k->mode_key) { // drop what the press typed and change mode
		if (k->press_typed)
			push_char(k, T9KB_BACKSPACE);
		next_mode(k);
	} else if (k->mode == T9_NUMERIC) {
		if (key == T9_0) {
			push_char(k, T9KB_BACKSPACE);
			push_char(k, '+');
		}
	} else if (k->mode == T9_WORD && k->composing && key >= T9_2 && key <= T9_9) { // the digit instead of a letter
		t9_unkey(k);
		commit(k);
		push_char(k, digit_of(key));
	} else if (is_digit_key(key)) { // swap the preview (or the 0 key's space) for the digit
		k->pending = false;
		push_char(k, T9KB_BACKSPACE);
		push_char(k, digit_of(key));
	}
}


// --- queues ---

static void push_event(t9kb *k, T9Key key, T9EventType type, uint8_t code) {
	T9Event ev = { key, type, code };
	if (k->key_cb) {
		k->key_cb(k->key_user, ev);
		return;
	}
	if (k->ev_count == sizeof(k->events) / sizeof(k->events[0]))
		return; // full: drop the newest
	k->events[(k->ev_head + k->ev_count++) % (sizeof(k->events) / sizeof(k->events[0]))] = ev;
}

static void push_char(t9kb *k, uint8_t c) {
	if (!c)
		return;
	if (c == T9KB_BACKSPACE) {
		if (k->typed)
			k->typed--;
		if (k->hist_len)
			k->hist_len--;
	} else if (c >= ' ' && c < 0x7F) {
		k->typed++;
		if (k->hist_len == sizeof(k->hist))
			memmove(k->hist, k->hist + 1, --k->hist_len);
		k->hist[k->hist_len++] = c;
	}
	if (k->ch_count < sizeof(k->chars))
		k->chars[(k->ch_head + k->ch_count++) % sizeof(k->chars)] = c;
}


// --- keypad ---

static uint8_t code_of(T9Key key) {
	for (uint8_t r = 0; r < 4; r++)
		for (uint8_t c = 0; c < 6; c++)
			if (KEYMAP[r][c] == key)
				return r * 10 + c + 1;
	return 0;
}

// The keypad was reset or unplugged: whatever was held is released
static void release_all(t9kb *k) {
	for (T9Key key = T9_1; key <= T9_OK; key++)
		if (k->down & BIT(key))
			push_event(k, key, T9_RELEASE, code_of(key));
	k->down = 0;
	k->held = T9_NONE;
}

static bool configure(t9kb *k) {
	return k->io.write(k->io.user, REG_KP_GPIO1, 0x0F) && // ROW0-3
	       k->io.write(k->io.user, REG_KP_GPIO2, 0x3F) && // COL0-5
	       k->io.write(k->io.user, REG_KP_GPIO3, 0x00) &&
	       k->io.write(k->io.user, REG_CFG, CFG_VALUE);
}

// A power cycle clears CFG, so a changed CFG means the keypad was replugged or reset and needs setting up again
static void check(t9kb *k) {
	uint8_t cfg;
	if (!k->io.read(k->io.user, REG_CFG, &cfg)) {
		if (k->connected)
			release_all(k);
		k->connected = false;
	} else if (cfg != CFG_VALUE) {
		release_all(k);
		k->connected = configure(k);
	} else {
		k->connected = true;
	}
}

static void handle(t9kb *k, uint8_t code, bool pressed, uint32_t now) {
	if (code < 1 || code > 80)
		return;
	uint8_t row = (code - 1) / 10, col = (code - 1) % 10;
	if (row > 3 || col > 5)
		return;
	T9Key key = KEYMAP[row][col];
	if (!pressed && !(k->down & BIT(key)))
		return; // its press was lost in an overflow or a reset
	push_event(k, key, pressed ? T9_PRESS : T9_RELEASE, code);
	if (pressed) {
		k->down       |= BIT(key);
		k->held        = key;
		k->held_code   = code;
		k->held_since  = now;
		k->next_repeat = now + k->repeat_delay;
		k->long_sent   = false;
		on_press(k, key, now);
	} else {
		k->down &= ~BIT(key);
		if (key == k->held)
			k->held = T9_NONE;
	}
}

// TI's procedure: read the event count, pop KEY_EVENT_A until empty, then clear the interrupt
static void read_fifo(t9kb *k, uint32_t now) {
	uint8_t stat, count;
	if (!k->io.read(k->io.user, REG_INT_STAT, &stat) || !(stat & (INT_K | INT_OVR)))
		return;
	if (!k->io.read(k->io.user, REG_KEY_LCK_EC, &count))
		return;
	count &= 0x0F;
	for (uint8_t i = 0; i < count; i++) {
		uint8_t ev;
		if (!k->io.read(k->io.user, REG_KEY_EVENT_A, &ev) || !ev)
			break;
		handle(k, ev & 0x7F, ev & 0x80, now);
	}
	k->io.write(k->io.user, REG_INT_STAT, stat & (INT_K | INT_OVR));
}


// --- public ---

void t9kb_init(t9kb *k, const t9kb_io *io) {
	memset(k, 0, sizeof(*k));
	k->io            = *io;
	k->tap_timeout   = 800;
	k->long_press    = 600;
	k->repeat_delay  = 500;
	k->repeat_rate   = 80;
	k->symbols       = SYMBOLS;
	k->backspace_key = T9_SOFT_RIGHT;
	k->mode_key      = T9_HASH;
	k->repeat_keys   = BIT(T9_SOFT_RIGHT) | BIT(T9_LEFT) | BIT(T9_UP) | BIT(T9_DOWN) | BIT(T9_RIGHT);
	k->mode          = T9KB_DICT ? T9_WORD : T9_SENTENCE;
	for (uint8_t i = 0; i < 9; i++)
		k->taps[i] = TAPS[i];
}

bool t9kb_begin(t9kb *k) {
	k->connected = configure(k);
	return k->connected;
}

void t9kb_update(t9kb *k, uint32_t now, int int_level) {
	if (now - k->last_check >= CHECK_MS) {
		k->last_check = now;
		check(k);
	}
	if (k->connected && (int_level >= 0 ? int_level == 0 : now - k->last_poll >= POLL_MS)) {
		k->last_poll = now;
		read_fifo(k, now);
	}
	if (k->pending && now - k->last_tap >= k->tap_timeout)
		k->pending = false;
	if (k->held == T9_NONE)
		return;
	if (k->repeat_keys & BIT(k->held)) {
		if ((int32_t)(now - k->next_repeat) >= 0) {
			push_event(k, k->held, T9_REPEAT, k->held_code);
			type_special(k, k->held);
			k->next_repeat = now + k->repeat_rate;
		}
	} else if (!k->long_sent && now - k->held_since >= k->long_press) {
		k->long_sent = true;
		push_event(k, k->held, T9_LONG, k->held_code);
		on_long(k, k->held);
	}
}

bool t9kb_connected(const t9kb *k) { return k->connected; }

void t9kb_on_key(t9kb *k, t9kb_key_cb cb, void *user) {
	k->key_cb   = cb;
	k->key_user = user;
}

void t9kb_on_mode(t9kb *k, t9kb_mode_cb cb, void *user) {
	k->mode_cb   = cb;
	k->mode_user = user;
}

bool t9kb_available(const t9kb *k) { return k->ev_count > 0; }

T9Event t9kb_read(t9kb *k) {
	T9Event ev = { T9_NONE, T9_PRESS, 0 };
	if (!k->ev_count)
		return ev;
	ev = k->events[k->ev_head];
	k->ev_head = (k->ev_head + 1) % (sizeof(k->events) / sizeof(k->events[0]));
	k->ev_count--;
	return ev;
}

bool t9kb_is_pressed(const t9kb *k, T9Key key) { return key != T9_NONE && key <= T9_OK && (k->down & BIT(key)); }

uint8_t t9kb_get_char(t9kb *k) {
	if (!k->ch_count)
		return 0;
	uint8_t c = k->chars[k->ch_head];
	k->ch_head = (k->ch_head + 1) % sizeof(k->chars);
	k->ch_count--;
	return c;
}

bool t9kb_pending(const t9kb *k) { return k->pending || k->composing; }

const char *t9kb_tap_set(const t9kb *k) {
	if (!k->pending)
		return NULL;
	return k->tap_key == T9_STAR ? k->symbols : k->taps[k->tap_key - T9_1];
}

const char *t9kb_word(const t9kb *k) { return k->composing ? k->word : NULL; }

void t9kb_clear_text(t9kb *k) {
	commit(k);
	k->pending  = false;
	k->typed    = 0;
	k->hist_len = 0;
}

void t9kb_set_mode(t9kb *k, T9Mode mode) {
	commit(k);
	k->pending = false;
	k->mode    = (mode == T9_WORD && !T9KB_DICT) || mode > T9_NUMERIC ? T9_SENTENCE : mode;
}

T9Mode t9kb_mode(const t9kb *k) { return k->mode; }

void t9kb_set_tap_timeout(t9kb *k, uint16_t ms) { k->tap_timeout = ms; }
void t9kb_set_long_press(t9kb *k, uint16_t ms)  { k->long_press = ms; }

void t9kb_set_repeat(t9kb *k, uint16_t delay_ms, uint16_t rate_ms) {
	k->repeat_delay = delay_ms;
	k->repeat_rate  = rate_ms;
}

void t9kb_set_taps(t9kb *k, T9Key key, const char *chars) {
	if (key >= T9_1 && key <= T9_9 && chars && chars[0])
		k->taps[key - T9_1] = chars;
}

void t9kb_set_symbols(t9kb *k, const char *chars) {
	if (chars && chars[0])
		k->symbols = chars;
}

void t9kb_set_backspace_key(t9kb *k, T9Key key) {
	k->repeat_keys &= ~BIT(k->backspace_key);
	k->backspace_key = key;
	if (key != T9_NONE)
		k->repeat_keys |= BIT(key);
}

void t9kb_set_mode_key(t9kb *k, T9Key key) { k->mode_key = key; }

void t9kb_set_repeating(t9kb *k, T9Key key, bool on) {
	if (key == T9_NONE || key > T9_OK)
		return;
	if (on)
		k->repeat_keys |= BIT(key);
	else
		k->repeat_keys &= ~BIT(key);
}

const char *t9kb_name(T9Key key) { return key <= T9_OK ? NAMES[key] : "?"; }
