#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "config.h"

#define ESC "\033"

void hndlchar(void);
void quit(void);

static char *argv0 = 0;

static int running = 0; 
static struct termios term, term_orig;
static char ch = 0;
static int curpos = 0 ;
static int linelen = 0 ;
static char linebuf[512] = "buf" ;

void
print(const char *s)
{
	write(1, s, strlen(s));
}

void
disablebuffering(void)
{
	if(tcgetattr(0, &term_orig)) {
		fprintf(stderr, "tcgetattr failed\n");
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
	//setvbuf(stdin, 0, _IONBF, 0);
}

void
clearline(void)
{
	print(ESC "[2K");
}

void
run(void)
{
	running = 1 ;
	savepos();
	while(running)
		if (read(0, &ch, 1) > 0) 
			hndlchar();
	quit();
}

void
update()
{
	int oldpos;
	char buf[512];
	clearline();
	undopos();
	print(linebuf);
	undopos();
	sprintf(buf, ESC "[%dC", curpos);
	print(buf);
}

void
savepos(void)
{
	print(ESC "[s");
}

void
undopos(void)
{
	print(ESC "[u");
}

void
inschr(void)
{
	char buf[512];
	strcpy(buf, linebuf+curpos);
	linebuf[curpos++] = ch ;
	strcpy(linebuf+curpos, buf);
	++linelen;
	update();
}

void
charleft(void)
{
	write(1, "\033[1D", 4);
	--curpos;
}

void
charright(void)
{
	write(1, "\033[1C", 4);
	++curpos;
}

void
hndlchar(void)
{
	switch(ch){
	case CHAR_LEFT : charleft() ; ; break ;
	case CHAR_RIGHT : charright() ; break ;
	case CHAR_EXIT : running=0 ; break ;
	default: inschr();
	}
}

void
quit(void)
{
	tcsetattr(0, TCSANOW, &term_orig);
	exit(0);
}

int
main(int argc, char *argv[], char *envp[])
{
	argv0 = argv[0] ;
	disablebuffering();
	run();
	return 0;
}
