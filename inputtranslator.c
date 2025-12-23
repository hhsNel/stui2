#include "inputtranslator.h"
#include "inputctl.h"
#include "util.h"

#include <unistd.h>
#include <poll.h>

static int parse_char(struct input_translator *it, char c);

void
init_input_translator(struct input_translator *it)
{
	it->state = ITS_NORMAL;
	init_input_ctl(&it->ic);
}

void
free_input_translator(struct input_translator *it)
{
	free_input_ctl(&it->ic);
}

int
run_input_translator(struct input_translator *it, int fd)
{
	char c;
	int ret;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN;

	while((ret = poll(&pfd, 1, 0)) > 0) {
		if(pfd.revents & POLLIN) {
			if(read(fd, &c, 1) <= 0) {
				return STUI_ERR;
			}

			switch(it->state) {
			case ITS_NORMAL:
				if(c == 127) {
					it->state = ITS_ESC;
				} else {
					if(parse_char(it, c) != STUI_OK) return STUI_ERR;
				}
				break;
			case ITS_ESC:
				if(c == '[') {
					it->state = ITS_CSI;
				} else {
					if(add_input_ctl(&it->ic, (struct input_evt){.type=IT_KEY,.data={.key={.raw=127,.parsed=127,.mods=0}}}) != STUI_OK) return STUI_ERR;
				}
				break;
			case ITS_CSI:
				/* TODO */
				it->state = ITS_NORMAL;
				break;
			}
		}
	}
	if(ret < 0) {
		return STUI_ERR;
	}

	return STUI_OK;
}

static int
parse_char(struct input_translator *it, char c)
{
#define SIMPLE_KEY(RAW, KEY, SHIFT, CONTROL) { .type=IT_KEY, .data={.key={.raw=RAW, .parsed=KEY, .mods=SHIFT?IM_SHIFT:0 | CONTROL?IM_CONTROL:0 }} },
#define SPECIAL_KEY(RAW, KEY, SHIFT, CONTROL) {.type=IT_SPECIALKEY, .data={.special={.raw={RAW,'\0'}, .parsed=KEY, .mods=SHIFT?IM_SHIFT:0 | CONTROL?IM_CONTROL:0}} },
	static struct input_evt lookup[127] = {
		        /* raw  parsed     shift control */
		SIMPLE_KEY('\0','2',          1, 1)
		SIMPLE_KEY(1,   'a',          0, 1)
		SIMPLE_KEY(2,   'b',          0, 1)
		SIMPLE_KEY(3,   'c',          0, 1)
		SIMPLE_KEY(4,   'd',          0, 1)
		SIMPLE_KEY(5,   'e',          0, 1)
		SIMPLE_KEY(6,   'f',          0, 1)
		SIMPLE_KEY(7,   'g',          0, 1)
		SPECIAL_KEY(8,  SK_BACKSPACE, 0, 0)
		SPECIAL_KEY(9,  SK_TAB,       0, 0)
		SPECIAL_KEY(10, SK_RETURN,    0, 0) /* one of two possible return key codes */
		SIMPLE_KEY(11,  'k',          0, 1)
		SIMPLE_KEY(12,  'l',          0, 1)
		SPECIAL_KEY(13, SK_RETURN,    0, 0) /* one of two possible return key codes */
		SIMPLE_KEY(14,  'n',          0, 1)
		SIMPLE_KEY(15,  'o',          0, 1)
		SIMPLE_KEY(16,  'p',          0, 1)
		SIMPLE_KEY(17,  'q',          0, 1)
		SIMPLE_KEY(18,  'r',          0, 1)
		SIMPLE_KEY(19,  's',          0, 1)
		SIMPLE_KEY(20,  't',          0, 1)
		SIMPLE_KEY(21,  'u',          0, 1)
		SIMPLE_KEY(22,  'v',          0, 1)
		SIMPLE_KEY(23,  'w',          0, 1)
		SIMPLE_KEY(24,  'x',          0, 1)
		SIMPLE_KEY(25,  'y',          0, 1)
		SIMPLE_KEY(26,  'z',          0, 1)
		SPECIAL_KEY(27, SK_ESCAPE,    0, 0)
		SIMPLE_KEY(28,  '\\',         0, 1)
		SIMPLE_KEY(29,  ']',          0, 1)
		SIMPLE_KEY(30,  '6',          1, 1)
		SIMPLE_KEY(30,  '-',          1, 1)
		SIMPLE_KEY(' ', ' ',          0, 0)
		SIMPLE_KEY('!', '1',          1, 0)
		SIMPLE_KEY('"', '\'',         1, 0)
		SIMPLE_KEY('#', '3',          1, 0)
		SIMPLE_KEY('$', '4',          1, 0)
		SIMPLE_KEY('%', '5',          1, 0)
		SIMPLE_KEY('&', '7',          1, 0)
		SIMPLE_KEY('\'','\'',         0, 0)
		SIMPLE_KEY('(', '9',          1, 0)
		SIMPLE_KEY(')', '0',          1, 0)
		SIMPLE_KEY('*', '*',          1, 0)
		SIMPLE_KEY('+', '=',          1, 0)
		SIMPLE_KEY(',', ',',          0, 0)
		SIMPLE_KEY('-', '-',          0, 0)
		SIMPLE_KEY('.', '.',          0, 0)
		SIMPLE_KEY('/', '/',          0, 0)
		SIMPLE_KEY('0', '0',          0, 0)
		SIMPLE_KEY('1', '1',          0, 0)
		SIMPLE_KEY('2', '2',          0, 0)
		SIMPLE_KEY('3', '3',          0, 0)
		SIMPLE_KEY('4', '4',          0, 0)
		SIMPLE_KEY('5', '5',          0, 0)
		SIMPLE_KEY('6', '6',          0, 0)
		SIMPLE_KEY('7', '7',          0, 0)
		SIMPLE_KEY('8', '8',          0, 0)
		SIMPLE_KEY('9', '9',          0, 0)
		SIMPLE_KEY(':', ';',          1, 0)
		SIMPLE_KEY(';', ';',          0, 0)
		SIMPLE_KEY('<', ',',          1, 0)
		SIMPLE_KEY('=', '=',          0, 0)
		SIMPLE_KEY('>', '.',          1, 0)
		SIMPLE_KEY('?', '/',          1, 0)
		SIMPLE_KEY('@', '2',          1, 0)
		SIMPLE_KEY('A', 'a',          1, 0)
		SIMPLE_KEY('B', 'b',          1, 0)
		SIMPLE_KEY('C', 'c',          1, 0)
		SIMPLE_KEY('D', 'd',          1, 0)
		SIMPLE_KEY('E', 'e',          1, 0)
		SIMPLE_KEY('F', 'f',          1, 0)
		SIMPLE_KEY('G', 'g',          1, 0)
		SIMPLE_KEY('H', 'h',          1, 0)
		SIMPLE_KEY('I', 'i',          1, 0)
		SIMPLE_KEY('J', 'j',          1, 0)
		SIMPLE_KEY('K', 'k',          1, 0)
		SIMPLE_KEY('L', 'l',          1, 0)
		SIMPLE_KEY('M', 'm',          1, 0)
		SIMPLE_KEY('N', 'n',          1, 0)
		SIMPLE_KEY('O', 'o',          1, 0)
		SIMPLE_KEY('P', 'p',          1, 0)
		SIMPLE_KEY('Q', 'q',          1, 0)
		SIMPLE_KEY('R', 'r',          1, 0)
		SIMPLE_KEY('S', 's',          1, 0)
		SIMPLE_KEY('T', 't',          1, 0)
		SIMPLE_KEY('U', 'u',          1, 0)
		SIMPLE_KEY('V', 'v',          1, 0)
		SIMPLE_KEY('W', 'w',          1, 0)
		SIMPLE_KEY('X', 'x',          1, 0)
		SIMPLE_KEY('Y', 'y',          1, 0)
		SIMPLE_KEY('Z', 'z',          1, 0)
		SIMPLE_KEY('[', '[',          0, 0)
		SIMPLE_KEY('\\','\\',         0, 0)
		SIMPLE_KEY(']', ']',          0, 0)
		SIMPLE_KEY('^', '6',          1, 0)
		SIMPLE_KEY('_', '-',          1, 0)
		SIMPLE_KEY('`', '`',          0, 0)
		SIMPLE_KEY('a', 'a',          0, 0)
		SIMPLE_KEY('b', 'b',          0, 0)
		SIMPLE_KEY('c', 'c',          0, 0)
		SIMPLE_KEY('d', 'd',          0, 0)
		SIMPLE_KEY('e', 'e',          0, 0)
		SIMPLE_KEY('f', 'f',          0, 0)
		SIMPLE_KEY('g', 'g',          0, 0)
		SIMPLE_KEY('h', 'h',          0, 0)
		SIMPLE_KEY('i', 'i',          0, 0)
		SIMPLE_KEY('j', 'j',          0, 0)
		SIMPLE_KEY('k', 'k',          0, 0)
		SIMPLE_KEY('l', 'l',          0, 0)
		SIMPLE_KEY('m', 'm',          0, 0)
		SIMPLE_KEY('n', 'n',          0, 0)
		SIMPLE_KEY('o', 'o',          0, 0)
		SIMPLE_KEY('p', 'p',          0, 0)
		SIMPLE_KEY('q', 'q',          0, 0)
		SIMPLE_KEY('r', 'r',          0, 0)
		SIMPLE_KEY('s', 's',          0, 0)
		SIMPLE_KEY('t', 't',          0, 0)
		SIMPLE_KEY('u', 'u',          0, 0)
		SIMPLE_KEY('v', 'v',          0, 0)
		SIMPLE_KEY('w', 'w',          0, 0)
		SIMPLE_KEY('x', 'x',          0, 0)
		SIMPLE_KEY('y', 'y',          0, 0)
		SIMPLE_KEY('z', 'z',          0, 0)
		SIMPLE_KEY('{', '[',          1, 0)
		SIMPLE_KEY('|', '\\',         1, 0)
		SIMPLE_KEY('}', ']',          1, 0)
		SIMPLE_KEY('~', '`',          1, 0)
	};
#undef SIMPLE_KEY
#undef SPECIAL_KEY

	if(c >= 127) {
		return STUI_ERR;
	}

	return add_input_ctl(&it->ic, lookup[(uint8_t)c]);
}

