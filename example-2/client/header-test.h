
#ifndef	__HEADER_TEST__
#define	__HEADER_TEST__
#include	<stdint.h>
#include	<stdio.h>

class headerTest {
private:
	uint8_t b [8];
	int16_t	counter;
public:
		headerTest (void) {
	counter	= 0;
}
		~headerTest (void) {
}

void	shift (uint8_t d) {
	if ((counter == 0) && (d == 0xFF)) {
	   b [counter ++] = 0xFF;
	   return;
	}
	if ((counter == 1) && (d == 0x00)) {
	   b [counter ++] = 0x00;
	   return;
	}
	if ((counter == 2) && (d == 0xFF)) {
	   b [counter ++] = 0xFF;
	   return;
	}
	if ((counter == 3) && (d == 0x00)) {
	   b [counter ++] = 0x00;
	   return;
	}
	if (counter == 4) {
	   b [counter ++] = d;
	   return;
	}
	if (counter == 5) {
	   b [counter ++] = d;
	   return;
	}
	if ((counter == 6) && (d == 0x00)) {
	   b [counter ++] = d;
	   return;
	}
	if ((counter == 7) && ((d == 0x00) || (d = 0xFF))) {
	   b [counter ++] = d;
	fprintf (stderr, "header detected, %o %o %o (%d)\n",
	                  b [4], b [5], b [7], 
	                  (b [4] << 8) | (b [5] & 0xFF));
	   return;
	}
	counter = 0;
}

void	reset	(void) {
	counter = 0;
}

bool	hasHeader (void) {
	return counter == 8;
}

int16_t	length () {
	return (b [4] << 8) | b [5];
}
};

#endif

