#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void	main(){
  	int retval=0;

  	printf("I am process %d and I will execute an 'ls -l .; \n", \
			getpid());

  	// retval=execl("/bin/ls", "ls", "-l", ".", NULL); 
  	retval=execlp("ls", "ls", "-l", ".", NULL);  

  	if (retval==-1)
		perror("execl");
}
