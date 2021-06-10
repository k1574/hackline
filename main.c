#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <utf.h>

#include "config.h"
#include "defs.h"

#define ESC "\033"

enum {
	BufSiz = 512,
} ;

void finish(void);
void undopos(void);
void savepos(void);
void hndlrune(Rune);
void quit(void);
void run(void);
void disablebuffering(void);

void curnright(int n);
void mvcur(int n);

static char *argv0 = 0;

static int running = 0; 
static struct termios term, term_orig;
static int curpos = 0 ;
static int linelen = 0 ;
static Rune linebuf[512] = {0, 0} ;

void
sigint(int signum)
{
	quit();
}


void
fstrprint(int fd, char *s)
{
	write(fd, s, strlen(s));
}

void
strprint(char *s)
{
	fstrprint(1, s);
}

void
frunestrprint(int fd, Rune *rstr)
{
	static char buf[BufSiz];
	fstrprint(fd, runestoutf(buf, rstr));
}

void
runestrprint(Rune *rstr)
{
	frunestrprint(1, rstr);
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
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &term)) {
		printf("tcsetattr failed\n");
		exit(1);
	}
}

void
clearline(void)
{
	fstrprint(1, ESC "[0K");
}

Rune
readrune(int fd)
{
	unsigned char c[4];
	Rune r;
	int len;

	if(!read(fd, c, 1)) return 0 ;

	len = runelenbyhd(*c) ;
	if(len>1) read(fd, c+1, len-1) ;
	chartorune(&r, c);

	return r ;	
}

void
run(void)
{
	Rune r;

	running = 1 ;
	savepos();
	while(running)
		if(r = readrune(0))
			hndlrune(r);
	quit();
}

void
update()
{
	undopos();
	clearline();
	runestrprint(linebuf);

	undopos();
	curnright(curpos);
}

void
curnright(int n)
{
	char buf[32];
	if(!n) return ;
	sprintf(buf, ESC "[%dC", n);
	strprint(buf);
}

void
savepos(void)
{
	strprint(ESC "[s");
}

void
undopos(void)
{
	strprint(ESC "[u");
}

void
insrune(Rune r)
{
	static Rune buf[512];
	runestrcpy(buf, linebuf+curpos);
	linebuf[curpos++] = r ;
	runestrcpy(linebuf+curpos, buf);
	++linelen;
	update();
}

void
delrune(void)
{
	static Rune buf[512];
	if(!curpos) return ;
	runestrcpy(buf, linebuf+curpos);
	runestrcpy(linebuf+(--curpos), buf);
	--linelen;
	update();
}

void
deltobeg(void)
{
	static Rune buf[512];
	runestrcpy(buf, linebuf+curpos);
	runestrcpy(linebuf, buf);
	curpos = 0 ;
	linelen = runestrlen(linebuf) ;
	update();
}

/*int
isbracket(char c)
{
	return strchr("\"`<>(){}[]", c) ?
		1 : 0 ;
}

int
issep(char c)
{
	return strchr("$-_/\\+=;:*?*&%@~!#", c) ?
		1 : 0 ;
}

int
delwordidx(void)
{
	char c;
	char *lp = linebuf + curpos ;

	if(!linelen) return 0 ;
	--lp ;
	while(isspace(*lp)) --lp ;

	c = *lp ;
	if (isalnum(c)) {
		while(isalnum(*lp) && (lp - linebuf)) --lp ;
	}

	return lp - linebuf + ((lp - linebuf) ? 1 : 0) ;
}

void
delword(void)
{
	int idx = delwordidx() ;
	cutline(idx, curpos);
	curpos = idx ;
	update();
}

int
backwordidx(void)
{
	char *lp = linebuf + curpos ;

	if(!linelen) return ;

	--lp;
	if(isspace(*lp))
		while(isspace(*lp)) --lp ;
	else if(isalnum(*lp))
		while(isalnum(*lp)) --lp ;

	return lp - linebuf + ((lp - linebuf) ? 1 : 0) ;
}

void
backword(void)
{
	int idx = backwordidx() ;
	mvcur(idx);
}*/

void
cutline(int n1, int n2)
{
	static Rune buf[512];
	if(n1 == n2) return ;
	runestrcpy(buf, linebuf+n2);
	runestrcpy(linebuf+n1, buf);
	linelen -= n2 - n1 ;
}

void
curleft(void)
{
	if(!curpos) return ;

	write(1, "\033[1D", 4);
	--curpos;
}

void
curbeg(void)
{
	mvcur(0);
}

void
curright(void)
{
	if(curpos + 1 >  linelen) return ;

	write(1, "\033[1C", 4);
	++curpos;
}

void
curend(void)
{
	mvcur(linelen);
}

void
mvcur(int n)
{
	curpos = n ;
	undopos();
	if(n) curnright(n) ;
}

void
hndltab(void)
{
}

void
hndlrune(Rune r)
{
	switch(r){
	case CHAR_LEFT : curleft() ; break ;
	case CHAR_RIGHT : curright() ; break ;
	case CHAR_END :  curend() ; break ;
	case CHAR_BEG : curbeg() ; break ;
	case CHAR_EXIT : running=0 ; break ;
	case CHAR_DEL : delrune() ; break ;
	case CHAR_DELTOBEG : deltobeg() ; break ;
	/*case CHAR_DELWORD : delword() ; break ;
	case CHAR_BACKWORD : backword() ; break ;*/
	//case CHAR RIGHTWORD : rightword() ; break ;
	case CHAR_TAB : hndltab() ; break ;
	case '\n' : finish() ; break ;
	default:
		insrune(r) ;
	}
	D {
		printf("\n%d %d %d\n", r, curpos, linelen);
		runestrprint(linebuf);
		puts("");
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
	frunestrprint(3, linebuf);
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
