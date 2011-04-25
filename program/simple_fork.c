#include <stdlib.h>		/* EXIT_SUCCESS */
#include <unistd.h>		/* fork */
#include <stdio.h>		/* printf */
#include <sys/wait.h>	/* waitpid */

#define CHILD 0

int main(int argc, char * argv[])
{
	int status = 0;
	int cpid = fork();
	if (cpid == CHILD)
	{
		printf("Child\n");
		exit(EXIT_SUCCESS);
	}
	printf("Parent\n");

	/* Wait for process to actually exit */
	do
	{
		waitpid(cpid, &status, 0);
	} while(! WIFEXITED(status));

	/* Check result */
    status = WEXITSTATUS(status);
	printf("Child died with exit code %d.\n", status);
	return 0;
}