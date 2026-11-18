// T9KB - Developed by acidvegas in C (https://github.com/acidvegas)
// library/tests/test_core.c - host tests for the C core against a simulated TCA8418. Run: make -C library/tests

#include <stdio.h>
#include <string.h>

#include "t9kb_core.h"

static int fails, checks;

#define CHECK(cond) do { checks++; if (!(cond)) { fails++; printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while (0)
#define CHECK_STR(got, want) do { checks++; if (strcmp((got), (want))) { fails++; printf("FAIL %s:%d: got \"%s\", want \"%s\"\n", __FILE__, __LINE__, (got), (want)); } } while (0)


// --- simulated TCA8418 ---

static struct {
	bool    present;
	uint8_t cfg, gpio[3], ovr;
	uint8_t fifo[10], count;
} chip;

static bool chip_write(void *user, uint8_t reg, uint8_t val) {
	(void)user;
	if (!chip.present)
		return false;
	if (reg == 0x01)
		chip.cfg = val;
	else if (reg >= 0x1D && reg <= 0x1F)
		chip.gpio[reg - 0x1D] = val;
	else if (reg == 0x02 && (val & 0x08))
		chip.ovr = 0;
	return true;
}

static bool chip_read(void *user, uint8_t reg, uint8_t *val) {
	(void)user;
	if (!chip.present)
		return false;
	switch (reg) {
		case 0x01: *val = chip.cfg; break;
		case 0x02: *val = (chip.count ? 0x01 : 0) | (chip.ovr ? 0x08 : 0); break;
		case 0x03: *val = chip.count; break;
		case 0x04:
			*val = chip.count ? chip.fifo[0] : 0;
			if (chip.count)
				memmove(chip.fifo, chip.fifo + 1, --chip.count);
			break;
		default: *val = 0;
	}
	return true;
}

// An event into the FIFO; a full FIFO drops its oldest event and flags an overflow (OVR_FLOW_M = 1)
static void chip_event(uint8_t ev) {
	if (chip.count == 10) {
		memmove(chip.fifo, chip.fifo + 1, 9);
		chip.count = 9;
		chip.ovr   = 1;
	}
	chip.fifo[chip.count++] = ev;
}


// --- harness ---

static t9kb     kb;
static uint32_t now;
static char     text[256];
static int      events, modes;
static T9Event  last;

static const uint8_t CODES[] = { 0, 1, 2, 3, 11, 12, 13, 21, 22, 23, 32, 31, 33, 4, 5, 6, 14, 16, 34, 36, 15, 35, 24, 26, 25 };

static void on_mode(void *user, T9Mode mode) { (void)user; (void)mode; modes++; }

static void drain(void) {
	uint8_t c;
	while ((c = t9kb_get_char(&kb))) {
		size_t n = strlen(text);
		if (c == T9KB_BACKSPACE) {
			if (n)
				text[n - 1] = 0;
		} else if (c >= ' ' && c < 0x7F && n < sizeof(text) - 1) {
			text[n] = c;
			text[n + 1] = 0;
		}
	}
	while (t9kb_available(&kb)) {
		last = t9kb_read(&kb);
		events++;
	}
}

static void run(uint32_t ms) { // advance the clock in poll steps
	for (uint32_t t = 0; t < ms; t += 10) {
		now += 10;
		t9kb_update(&kb, now, -1);
		drain();
	}
}

static void down(T9Key k) { chip_event(0x80 | CODES[k]); run(20); }
static void up(T9Key k)   { chip_event(CODES[k]); run(20); }
static void tap(T9Key k)  { down(k); up(k); }
static void hold(T9Key k, uint32_t ms) { down(k); run(ms); up(k); }

static void keys(const char *s) { // tap digit keys, '*' and '#'
	for (; *s; s++)
		tap(*s == '*' ? T9_STAR : *s == '#' ? T9_HASH : *s == '0' ? T9_0 : (T9Key)(T9_1 + *s - '1'));
}

static void reset(T9Mode mode) {
	memset(&chip, 0, sizeof(chip));
	chip.present = true;
	t9kb_io io = { chip_write, chip_read, NULL };
	t9kb_init(&kb, &io);
	t9kb_begin(&kb);
	t9kb_set_mode(&kb, mode);
	t9kb_on_mode(&kb, on_mode, NULL);
	text[0] = 0;
	events = modes = 0;
	now += 5000;
}


// --- tests ---

static void test_setup(void) {
	reset(T9_SENTENCE);
	CHECK(chip.cfg == 0x29);
	CHECK(chip.gpio[0] == 0x0F && chip.gpio[1] == 0x3F && chip.gpio[2] == 0x00);
	CHECK(t9kb_connected(&kb));
	tap(T9_5);
	CHECK(events == 2 && last.key == T9_5 && last.type == T9_RELEASE && last.code == 12);
	CHECK(!t9kb_is_pressed(&kb, T9_5));
	down(T9_OK);
	CHECK(t9kb_is_pressed(&kb, T9_OK));
	up(T9_OK);
}

static void test_multitap(void) {
	reset(T9_SENTENCE);
	keys("44");
	run(1000);
	keys("444");
	CHECK_STR(text, "Hi");
	run(1000);
	keys("0");
	keys("2");
	CHECK(t9kb_pending(&kb));
	CHECK_STR(t9kb_tap_set(&kb), "abc2");
	run(1000);
	CHECK(!t9kb_pending(&kb));
	CHECK_STR(text, "Hi a");

	reset(T9_SENTENCE); // capital again after ". "
	keys("2");
	run(1000);
	keys("1");
	run(1000);
	keys("02");
	CHECK_STR(text, "A. A");

	reset(T9_UPPER);
	keys("33");
	CHECK_STR(text, "E");
	reset(T9_LOWER);
	keys("33");
	CHECK_STR(text, "e");
}

static void test_long_press(void) {
	reset(T9_SENTENCE);
	hold(T9_7, 700);
	CHECK_STR(text, "7");
	hold(T9_1, 700);
	CHECK_STR(text, "71");
	text[0] = 0;
	keys("1111111");
	CHECK_STR(text, "-");
	run(1000);
	keys("11111111");
	CHECK_STR(text, "-1");
	CHECK(last.type == T9_RELEASE);
	reset(T9_NUMERIC);
	keys("12");
	hold(T9_0, 700);
	CHECK_STR(text, "12+");
	keys("*#");
	CHECK_STR(text, "12+*#");
}

static void test_clear_text(void) {
	reset(T9_SENTENCE);
	keys("666");
	run(1000);
	keys("55");
	CHECK_STR(text, "Ok");
	text[0] = 0;
	t9kb_clear_text(&kb);
	keys("666");
	CHECK_STR(text, "O");
}

static void test_repeat(void) {
	reset(T9_LOWER);
	keys("2");
	run(1000);
	keys("3");
	run(1000);
	keys("4");
	run(1000);
	keys("5");
	run(1000);
	hold(T9_SOFT_RIGHT, 700); // one on the press, then repeats after 500 ms every 80 ms
	CHECK_STR(text, "");
	hold(T9_LEFT, 100);
	CHECK(last.type == T9_RELEASE);
}

static void test_modes(void) {
	reset(T9_SENTENCE);
	hold(T9_HASH, 700);
	CHECK(t9kb_mode(&kb) == T9_LOWER);
	CHECK_STR(text, "");
	CHECK(modes == 1);
	hold(T9_HASH, 700);
	hold(T9_HASH, 700);
	hold(T9_HASH, 700);
	CHECK(t9kb_mode(&kb) == (T9KB_DICT ? T9_WORD : T9_SENTENCE));
	tap(T9_HASH);
	CHECK_STR(text, "#");
}

static void test_custom(void) {
	reset(T9_LOWER);
	t9kb_set_taps(&kb, T9_1, ".?1");
	t9kb_set_symbols(&kb, "@%");
	keys("111");
	run(1000);
	keys("**");
	CHECK_STR(text, "1%");
	t9kb_set_backspace_key(&kb, T9_END);
	tap(T9_SOFT_RIGHT);
	CHECK_STR(text, "1%");
	tap(T9_END);
	CHECK_STR(text, "1");
	t9kb_set_mode_key(&kb, T9_SOFT_MIDDLE);
	hold(T9_HASH, 700);
	CHECK(t9kb_mode(&kb) == T9_LOWER);
	CHECK_STR(text, "1#");
	hold(T9_SOFT_MIDDLE, 700);
	CHECK(t9kb_mode(&kb) == T9_UPPER);
	CHECK_STR(text, "1#");
	t9kb_set_repeating(&kb, T9_OK, true);
	hold(T9_OK, 700);
	CHECK(events > 0);
}

static void test_hotplug(void) {
	reset(T9_SENTENCE);
	down(T9_CALL);
	CHECK(t9kb_is_pressed(&kb, T9_CALL));
	chip.present = false;
	run(1100);
	CHECK(!t9kb_connected(&kb));
	CHECK(!t9kb_is_pressed(&kb, T9_CALL));
	CHECK(last.key == T9_CALL && last.type == T9_RELEASE);
	memset(&chip, 0, sizeof(chip)); // plugged back in: power-on defaults
	chip.present = true;
	run(1100);
	CHECK(t9kb_connected(&kb));
	CHECK(chip.cfg == 0x29);
	tap(T9_2);
	CHECK_STR(text, "A");
}

static void test_overflow(void) {
	reset(T9_SENTENCE);
	for (int i = 0; i < 6; i++) { // 12 events while nobody reads: the oldest two drop
		chip_event(0x80 | CODES[T9_OK]);
		chip_event(CODES[T9_OK]);
	}
	CHECK(chip.ovr);
	run(20);
	CHECK(!chip.ovr);
	CHECK(!t9kb_is_pressed(&kb, T9_OK));
	chip_event(CODES[T9_CALL]); // a release whose press was lost is ignored
	run(20);
	CHECK(last.key == T9_OK);
}

#if T9KB_DICT
static void test_t9(void) {
	reset(T9_WORD);
	keys("4663");
	CHECK_STR(text, "Good");
	CHECK_STR(t9kb_word(&kb), "Good");
	CHECK(t9kb_pending(&kb));
	keys("*");
	CHECK_STR(text, "Home");
	keys("0");
	CHECK_STR(text, "Home ");
	CHECK(!t9kb_pending(&kb));
	CHECK(t9kb_word(&kb) == NULL);
	keys("43556");
	CHECK_STR(text, "Home hello");
	keys("04");
	CHECK_STR(text, "Home hello I");
	keys("0");
	keys("968");
	CHECK_STR(text, "Home hello I you");
	tap(T9_SOFT_RIGHT); // backspace takes a key off the word
	CHECK(strlen(t9kb_word(&kb)) == 2);
	tap(T9_SOFT_RIGHT);
	tap(T9_SOFT_RIGHT);
	CHECK_STR(text, "Home hello I ");
	CHECK(t9kb_word(&kb) == NULL);

	reset(T9_WORD); // 1 types punctuation and starts a sentence
	keys("43556");
	keys("1");
	keys("0");
	keys("968");
	CHECK_STR(text, "Hello. You");

	reset(T9_WORD); // long press gives the digit
	keys("4663");
	hold(T9_2, 700);
	CHECK_STR(text, "Good2");

	reset(T9_WORD); // nothing matches: the keys or a word start show, never nothing
	keys("99999999");
	CHECK(strlen(text) == 8);

	reset(T9_WORD); // more presses of * than words wrap around
	keys("4663****");
	CHECK(strlen(text) == 4);
	run(1000);
	CHECK(t9kb_pending(&kb)); // a word doesn't time out

	reset(T9_WORD); // other keys accept the word
	keys("4663");
	tap(T9_OK);
	CHECK(!t9kb_pending(&kb));
	keys("4663");
	CHECK_STR(text, "Goodgood");
}
#endif

int main(void) {
	test_setup();
	test_multitap();
	test_long_press();
	test_clear_text();
	test_repeat();
	test_modes();
	test_custom();
	test_hotplug();
	test_overflow();
#if T9KB_DICT
	test_t9();
#endif
	printf("%d checks, %d failed\n", checks, fails);
	return fails != 0;
}
