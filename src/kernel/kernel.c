#include "kernel.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <string.h>

static const char scancode2ascii[256] = {
	[0x1C] = 'A',
	[0x32] = 'B',
	[0x21] = 'C',
	[0x23] = 'D',
	[0x24] = 'E',
	[0x2B] = 'F',
	[0x34] = 'G',
	[0x33] = 'H',
	[0x43] = 'I',
	[0x3B] = 'J',
	[0x42] = 'K',
	[0x4B] = 'L',
	[0x3A] = 'M',
	[0x31] = 'N',
	[0x44] = 'O',
	[0x4D] = 'P',
	[0x15] = 'Q',
	[0x2D] = 'R',
	[0x1B] = 'S',
	[0x2C] = 'T',
	[0x3C] = 'U',
	[0x2A] = 'V',
	[0x1D] = 'W',
	[0x22] = 'X',
	[0x35] = 'Y',
	[0x1A] = 'Z',
	[0x5A] = '\n',
};

struct {
	unsigned char x;
	unsigned char y;
} cursor = {0, 0};

int kernel_getchar()
{
	getchar_char = GETCHAR_WAITING;
	while (GETCHAR_WAITING == getchar_char) {
		__asm__("hlt");
	}

	int chr = getchar_char;
	getchar_char = GETCHAR_NOTWAITING;
	return chr;
}

void printCharAt(char chr, uint8_t color, uint8_t x, uint8_t y)
{
	unsigned char *videomem_start = (unsigned char *) 0xb8000;
	uint16_t offset = 160*y + 2*x;

	videomem_start[offset] = (unsigned char) chr;
	videomem_start[offset + 1] = color;
}

void printChar(char chr)
{
	switch (chr) {
	case '\n':
		++cursor.y;
		cursor.x = 0;
		break;
	default:
		printCharAt(chr, 0x07, cursor.x, cursor.y);
		++cursor.x;
	}
}

void printString(char *str) {
	unsigned char *videomem_start = (unsigned char *) 0xb8000;

	if (cursor.y >= 24) {
		memmove(videomem_start, videomem_start+160, 160*24);
	}

	size_t len = strlen(str);
	for (size_t i = 0; i < len; i++) {
		printChar(str[i]);
	}
}

void printHex(unsigned int hex) {
	static int iters = 0;
	char hex_str[] = "0x00000000";

	for (int i = 9; i >= 2; --i) {
		unsigned char nibble = hex & 0xF;
		char hex_chr = '0' + nibble;
		if (nibble > 9) {
			hex_chr += 7;
		}
		hex_str[i] = hex_chr;
		hex /= 16;
	}
	printString(hex_str);

	++iters;
}

void handleAsciiCode(char asciicode) {
	if (GETCHAR_WAITING == getchar_char) {
		getchar_char = asciicode;
	}
}
void handleScanCode(unsigned char scancode) {
	static bool is_break = false;
	if (0xF0 == scancode) {
		is_break = true;
		return;
	}
	if (0xE0 == scancode) {
		return;
	}

	if ( ! is_break) {
		handleAsciiCode(scancode2ascii[scancode]);
	}
	is_break = false;
}

void main() {
	printString("Hello World from the booted C kernel!\n");
	printString("And here's another line!\n");

	printHex(0xDEADBEEF);
	printString("\n");

	while (true) {
		#define CMD_BUF_LEN 256
		char buffer[CMD_BUF_LEN] = {0};
		uint16_t buffer_len = 0;

		printString("$ ");

		for (uint16_t i = 0; i < CMD_BUF_LEN - 1; ++i) {
			int chr = kernel_getchar();
			printChar(chr);

			if ('\n' == chr) {
				break;
			}

			buffer[buffer_len] = chr;
			++buffer_len;
		}
		buffer[buffer_len] = 0;

		printString("You typed: ");
		printString(buffer);
		printString("\n");
	}
}

