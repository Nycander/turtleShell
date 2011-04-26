#include <stdlib.h>		/* EXIT_SUCCESS */
#include <unistd.h>		/* fork */
#include <stdio.h>		/* printf */
#include <sys/wait.h>	/* waitpid */
#include <signal.h>
#include <errno.h>

#define CHILD 0

void termination_handler(int signum)
{
	int status = 0;
	int pid = getpid();
	printf("Handler ran, signum = %d\n", signum);
	printf("Handler is running in pid %d.\n", pid);
	printf("Waiting for child to die.\n");

	while(1)
	{
		pid = waitpid(-1, &status, WNOHANG);

		if (pid == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			printf("Child died.\n");
			break;
		}
		else if (pid == 0)
		{
			break;
		}
	}
}

int main(int argc, char * argv[])
{
	int pid = getpid();
	struct sigaction action;

	/* Set up signal handler */
	action.sa_handler = termination_handler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGCHLD, &action, NULL);
	printf("Parent: I have the superior pid of %d.\n", pid);
	int status = 0;
	int cpid = fork();
	if (cpid == CHILD)
	{
		printf("Child: I will die in 2 seconds!\n");
		sleep(2); 
		printf("Child: I die now. HUZZA!\n");
		exit(EXIT_SUCCESS);
	}

	printf("Parent: I'll just tend to my business and ignore my child (pid %d) and conquer the world... Please give command.\n", cpid);

	char tmp[100];
	fgets(tmp, 100, stdin);
	fgets(tmp, 100, stdin);
	printf("Input fetched.\n");

	/* Check result */
	return 0;
}