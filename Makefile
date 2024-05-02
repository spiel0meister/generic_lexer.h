CFLAGS=-Wall -Wextra -ggdb
BUILD=build

all: $(BUILD) test

$(BUILD):
	mkdir -p $@

test: test.c
	gcc $(CFLAGS) -o $(BUILD)/$@ $^

%.o: %.c %.h
	gcc $(CFLAGS) -o $(BUILD)/$@ $<
