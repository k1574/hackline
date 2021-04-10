#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

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
	term.c_lflag |= ECHO;
	term.c_cc[VMIN] = 0;
	term.c_cc[VTIME] = 0;
	if (tcsetattr(0, TCSANOW, &term)) {
		printf("tcsetattr failed\n");
		exit(1);
	}
}

void
run(void)
{
	running = 1 ;
	while(running)
		if (read(0, &ch, 1) > 0) 
			hndlchar(ch);
}

void
hndlchar(ch)
{
	printf("'%c' = %d\n", ch, ch);
}

int
main(int argc, char *argv[], char *envp[])
{
	argv0 = argv[0] ;

	disablebuffering();
	run();

  return 0;
}
