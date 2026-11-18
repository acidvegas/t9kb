SRC = ../arduino/src

test: test_core.c $(SRC)/t9kb_core.c $(SRC)/t9kb_dict.c
	$(CC) -std=c99 -Wall -Wextra -pedantic -I$(SRC) -o $@ $^
	./$@
	$(CC) -std=c99 -Wall -Wextra -pedantic -DT9KB_DICT=0 -I$(SRC) -o $@_nodict $^
	./$@_nodict
	@rm -f $@ $@_nodict

.PHONY: test
