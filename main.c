#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "config.h"

static char *argv0 = 0;

static int running = 0; 
static struct termios term, term_orig;
static char ch = 0;

void
disablebuffering(void)
{
	if(tcgetattr(0, &term_orig)) {
		printf("tcgetattr failed\n");
		exit(1);
	}
	term = term_orig;
	term.c_lflag &= ~ICANON;
	term.c_lflag &= ~ECHO;
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &term)) {
		printf("tcsetattr failed\n");
		exit(1);
	}
	setvbuf(stdin, 0, _IONBF, 0);
}

void
run(void)
{
	running = 1 ;
	while(running)
		if (read(0, &ch, 1) > 0) 
			hndlchar();
}

void
hndlchar(void)
{
	switch(ch){
	case CHAR_LEFT :
		write(1, "\033[1D", 4);
	break;
	case CHAR_RIGHT :
		write(1, "\033[1C", 4);
	break;
	case CHAR_EXIT :
		exit(0);
	default:
		write(1, &ch, 1);
	}
}

int
main(int argc, char *argv[], char *envp[])
{
	argv0 = argv[0] ;

	disablebuffering();
	run();

	return 0;
}
