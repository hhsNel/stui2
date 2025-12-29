SRCDIR = src
BUILDDIR = build
TESTDIR = tests
#TARGET = test

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)
TESTSRC = $(wildcard $(TESTDIR)/*.c)
TESTOBJ = $(TESTSRC:$(TESTDIR)/%.c=$(TESTDIR)/%.o)
TESTS = $(TESTSRC:$(TESTDIR)/%.c=test-%)

CC = gcc
CFLAGS = -Wall -Wextra -Werror -Wshadow -Wno-missing-field-initializers -Wno-unused-parameter -fstack-protector-strong -fPIE -g -I$(SRCDIR) -D_GNU_SOURCE
LDFLAGS = -pie -g

all: $(TESTS) #$(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

test-%: $(TESTDIR)/%.o $(OBJ)
	$(CC) $(OBJ) $< $(LDFLAGS) -o $@

$(TESTDIR)/%.o: $(TESTDIR)/%.c $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

# $(TARGET): $(OBJ)
# 	$(CC) $(OBJ) $(LDFLAGS) -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILDDIR) $(TARGET) $(TESTS)
	rm -f $(TESTDIR)/*.o

.PHONY: all clean

