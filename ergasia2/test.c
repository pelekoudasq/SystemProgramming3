#include <unistd.h>
#include <stdio.h>
#include <signal.h>

volatile sig_atomic_t stop;

void
inthand(int signum)
{
	if (signum == SIGINT){
    	stop = 1;
	}
}

int
main(int argc, char **argv)
{
    signal(SIGINT, inthand);
    printf("%d\n", stop);

    while (!stop){
        pause();
    }
    printf("exiting safely\n");
    printf("i can still do things\n");
    printf("%d\n", stop);
    printf("not done yet\n");
    return 0;
}