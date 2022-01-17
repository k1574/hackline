#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <utf.h>

#include <aes/aes.h>

#include "config.h"
#include "defs.h"

#define ESC "\033"

enum {
	BufSiz = 512,
} ;

void finish(void);
void undopos(void);
void hndlrune(Rune);
void quit(void);
void run(void);
void disablebuffering(void);
void prevcmd(void);
void nextcmd(void);

void mvcur(int n);
void mvcurrel(int n);

static char *argv0 = 0;

static int running = 0; 
static struct termios term, term_orig;
static int linelen = 0 ;
static char *cmdhist;
static Rune prevcmdbuf[BufSiz] = {0, 0} ;
static Rune curcmdbuf[BufSiz] = {0, 0} ;

static int curpos = 0 ; /* Cursor position. */
static int curline = 0 ; /* Line in story of commands. 0 means current one. */
static Rune *linebuf = curcmdbuf ;

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
	aes_save_pos();
	while(running)
		if(r = readrune(0))
			hndlrune(r);
	quit();
}

void
update()
{
	aes_undo_pos();
	clearline();
	runestrprint(linebuf);
	aes_undo_pos();
	if(curpos) aes_move_right(curpos) ;
}

void
insrune(Rune r)
{
	static Rune buf[BufSiz];
	runestrcpy(buf, linebuf+curpos);
	linebuf[curpos++] = r ;
	runestrcpy(linebuf+curpos, buf);
	++linelen;
	update();
}

void
delrune(void)
{
	static Rune buf[BufSiz];
	if(!curpos) return ;
	runestrcpy(buf, linebuf+curpos);
	runestrcpy(linebuf+(--curpos), buf);
	--linelen;
	update();
}

void
deltobeg(void)
{
	static Rune buf[BufSiz];
	runestrcpy(buf, linebuf+curpos);
	runestrcpy(linebuf, buf);
	curpos = 0 ;
	linelen = runestrlen(linebuf) ;
	update();
}

void
cutline(int n1, int n2)
{
	static Rune buf[BufSiz];
	if(n1 == n2) return ;
	runestrcpy(buf, linebuf+n2);
	runestrcpy(linebuf+n1, buf);
	linelen -= n2 - n1 ;
}

int
isposcorrect(int newpos)
{
	if(newpos < 0 || linelen < newpos) return 0 ;
	else return 1 ;
}

void
mvcur(int n)
{
	if(!isposcorrect(n)) return ;
	curpos = n ;
	aes_undo_pos();
	if(curpos)
		aes_move_right(curpos);
}

void
mvcurrel(int n)
{
	int newpos; 
	newpos = curpos + n ;
	if(!isposcorrect(newpos)) return ;

	curpos = newpos ;
	if(n>0)
		aes_move_right(n) ;
	else
		aes_move_left(-n);
}

void
hndltab(void)
{
}

void
hndlrune(Rune r)
{
	switch(r){
	case CHAR_LEFT :
		mvcurrel(-1);
		break ;
	case CHAR_RIGHT :
		mvcurrel(+1);
		break ;
	case CHAR_END :
		mvcur(linelen);
		break ;
	case CHAR_BEG :
		mvcur(0) ;
		break ;
	case CHAR_EXIT :
		running=0 ;
		break ;
	case CHAR_DEL :
		delrune();
		break ;
	case CHAR_PREVCMD :
		prevcmd();
		break;
	case CHAR_NEXTCMD :
		nextcmd();
		break;
	case CHAR_DELTOBEG :
		deltobeg();
		break ;
	/*case CHAR_DELWORD :
		delword();
		break ;
	case CHAR_BACKWORD : backword() ; break ;*/
	//case CHAR RIGHTWORD : rightword() ; break ;
	case CHAR_TAB : hndltab() ; break ;
	case '\n' :
		finish() ;
		break ;
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
bufcmd(int n)
{
	char buf[BUFSIZ * sizeof(Rune)];
	int i, len;
	FILE *f;

	if(n == curline || n < 0) return ;

	if(!n){
		linebuf = curcmdbuf ;
	} else {
		if(!(f = fopen(cmdhist, "r"))){
			return;
		}

		for(i = 0 ; i<n ; ++i){
			if(!fgets(buf, sizeof(buf), f)){
				return;
			}
		}

		len = strlen(buf) ;
		
		if(buf[len-1] == '\n')
			buf[len-1] = 0 ;

		linebuf = prevcmdbuf ;
		utftorunes(linebuf, buf);
		
	}

	curline = n ;
	linelen = runestrlen(linebuf) ;
	curpos = linelen ;
	update();
}

void
prevcmd(void)
{
	bufcmd(curline+1);
}

void
nextcmd(void)
{
	bufcmd(curline-1);
}

void
quit(void)
{
	aes_reset_term();
	aes_apply_term_settings();
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
	cmdhist = getenv("CMDHIST") ;
	signal(SIGINT, sigint);
	aes_reset_term();
	aes_disable_input_buffering();
	aes_disable_input_echo();
	aes_apply_term_settings();
	run();
	return 0;
}
