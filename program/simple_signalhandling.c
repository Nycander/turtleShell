#include <stdlib.h>		/* EXIT_SUCCESS */
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>	/* waitpid */
#include <errno.h>

#define CHILD 0

/* How does Ctrl+c only kill foreground process and not parent? */


int sigchld_handler(int signum)
{
	int status;
	int pid = getpid();
	int ppid = getppid();
	printf("SIGCHLD = %d, signum = %d, pid = %d, ppid = %d!\n", SIGCHLD, signum, pid, ppid);

	while(1)
	{
		pid = waitpid(-1, &status, WNOHANG);

		if (pid == -1)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		else if (pid == 0)
		{
			break;
		}
		/* Did the process terminate? Print message. */
		else if (WIFEXITED(status))
		{
			printf("Child terminated normally.\n");
		}
		else
		{
			printf("Child just st00pid\n");
		}
	}

	return 0;
}

int main(int argc, char * argv[])
{
	int status = 0;
	int cpid;
	struct sigaction sigchld_action;
	struct sigaction sigint_action;
	struct sigaction ignore_action;

	sigchld_action.sa_handler = sigchld_handler;
	sigemptyset(&sigchld_action.sa_mask);
	sigchld_action.sa_flags = 0;
	sigaction(SIGCHLD, &sigchld_action, NULL);

	ignore_action.sa_handler = SIG_IGN;
	sigemptyset(&ignore_action.sa_mask);
	ignore_action.sa_flags = 0;
	sigaction(SIGINT, &ignore_action, &sigint_action);

	cpid = fork();
	if (cpid == CHILD)
	{
		sigaction(SIGINT, &sigint_action, NULL);
		printf("Child running\n");
		while(1)
			sleep(1);
		printf("Child exited\n");
		exit(EXIT_SUCCESS);
	}
	printf("Parent\n");

	while(1)
		sleep(1);
	
	return 0;
}