#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <string.h>

#include "config.h"

#define ESC "\033"

void finish(void);
void undopos(void);
void savepos(void);
void hndlchar(void);
void quit(void);

static char *argv0 = 0;

static int running = 0; 
static struct termios term, term_orig;
static char ch = 0;
static int curpos = 0 ;
static int linelen = 0 ;
static char linebuf[512] = {'\0'} ;

void
sigint(int signum)
{
	quit();
}

void
eprint(const char *s)
{
	write(2, s, strlen(s));
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
}

void
clearline(void)
{
	eprint(ESC "[0K");
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
	undopos();
	clearline();
	eprint(linebuf);
	undopos();
	charnright(curpos);
}

void
charnright(int n)
{
	char buf[32];
	sprintf(buf, ESC "[%dC", n);
	eprint(buf);
}

void
savepos(void)
{
	eprint(ESC "[s");
}

void
undopos(void)
{
	eprint(ESC "[u");
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
	if(!curpos) return ;

	write(1, "\033[1D", 4);
	--curpos;
}

void
charbeg(void)
{
	mvcursor(0);
}

void
charright(void)
{
	if(curpos + 1 >  linelen) return ;

	write(1, "\033[1C", 4);
	++curpos;
}

void
charend(void)
{
	mvcursor(linelen);
}

void
mvcursor(int n)
{
	curpos = n ;
	undopos();
	if(n) charnright(n) ;
}

void
hndlchar(void)
{
	switch(ch){
	case CHAR_LEFT : charleft() ; ; break ;
	case CHAR_RIGHT : charright() ; break ;
	case CHAR_END :  charend() ; break ;
	case CHAR_BEG : charbeg() ; break ;
	case CHAR_EXIT : running=0 ; break ;
	case '\n' : finish() ; break ;
	default: inschr();
	}
}

void
quit(void)
{
	tcsetattr(0, TCSANOW, &term_orig);
	exit(0);
}

void
finish(void)
{
	write(3, linebuf, linelen);
	quit();
}

int
main(int argc, char *argv[], char *envp[])
{
	argv0 = argv[0] ;
	signal(SIGINT, sigint);
	disablebuffering();
	run();
	return 0;
}
