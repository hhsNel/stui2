SRCDIR = src
MODULES = base layout shm elements agfx
BUILDDIR = build
TESTDIR = tests
MAIN = $(SRCDIR)/stui2.c
TARGET = libstui2.a
HEADER = $(SRCDIR)/stui2.h
LIBDIR = /usr/local/lib
INCLUDEDIR = /usr/local/include

SRC = $(foreach MODULE, $(MODULES), $(wildcard $(SRCDIR)/$(MODULE)/*.c))
OBJ = $(SRC:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TESTSRC = $(wildcard $(TESTDIR)/*.c)
TESTOBJ = $(TESTSRC:$(TESTDIR)/%.c=$(BUILDDIR)/tests/%.o)
MAINOBJ = $(MAIN:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TESTS = $(TESTSRC:$(TESTDIR)/%.c=test-%)

CC = gcc
CFLAGS += -std=c99 -Wall -Wextra -Werror -Wshadow -Wno-missing-field-initializers -Wno-unused-parameter -fstack-protector-strong -fPIE -g -I$(SRCDIR) -D_GNU_SOURCE -D_POSIX_C_SOURCE
LDFLAGS += -pie -g -L. -l$(TARGET:lib%.a=%)

all: tests lib

tests: $(TESTS)

lib: $(TARGET)

$(BUILDDIR):
	mkdir -p $(foreach MODULE, $(MODULES), $(BUILDDIR)/$(MODULE))
	mkdir -p $(BUILDDIR)/tests

test-%: $(BUILDDIR)/tests/%.o $(OBJ) $(TARGET)
	$(CC) $(OBJ) $< $(LDFLAGS) -o $@

$(MAINOBJ): $(BUILDDIR)
	$(CC) $(CFLAGS) -c $(MAIN) -o $(MAINOBJ)

$(BUILDDIR)/tests/%.o: $(TESTDIR)/%.c $(OBJ) | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(MAINOBJ) $(OBJ)
	ar rcs $(TARGET) $(OBJ) $(MAINOBJ)

clean:
	rm -rf $(BUILDDIR)
	rm -f $(TARGET) $(TESTS) $(TESTOBJ)

install: lib $(HEADER)
	cp $(TARGET) $(LIBDIR)/
	chmod 644 $(LIBDIR)/$(shell basename $(TARGET))
	cp $(HEADER) $(INCLUDEDIR)/
	chmod 644 $(INCLUDEDIR)/$(shell basename $(HEADER))

uninstall:
	rm $(LIBDIR)/$(shell basename $(TARGET))
	rm $(INCLUDEDIR)/$(shell basename $(HEADER))

.PHONY: all clean install uninstall tests lib

