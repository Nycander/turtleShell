#include <unistd.h>		/* execvp */
#include <stdio.h>		/* fgets, printf */
#include <string.h>		/* strlen, strtok */
#include <sys/wait.h>	/* waitpid */
#include <sys/time.h>	/* gettimeofday */
#include <stdlib.h>
#include <errno.h>

/* Use signal detection or polling for cleaning up dead children? */
#define SIGNALDETECTION 1

/* Parameters for changing the limitations of the shell */
#define MAX_ARGUMENTS 5
#define CMD_LENGTH 70

/* Self-documentation */
#define CHILD 0

char * current_dir;
int    current_dir_len = 0;
struct sigaction sigint_action;

/*
 * Tries to parse out a command line argument array from a given string.
 * Will return the amount of parsed arguments.
 */
int parse_input(char input[CMD_LENGTH], char * args[MAX_ARGUMENTS+1], int * background_process)
{
	int i = 0;
	int len = 0;
	
	/* Remove trailing newline */
	len = strlen(input);
	input[len-1] = '\0';

	/* Start parsing out arguments (search for spaces) */
	args[i++] = strtok(input, " ");

	/* At least one argument is required. */
	if (args[0] != NULL)
	{
		/* Find additional arguments */
		while(i <= MAX_ARGUMENTS && (args[i] = strtok(NULL, " ")) != NULL)
			i++;

		/* Finish argument array */
		args[i] = NULL;

		/* Do we find a & at the end? - flag for background process */
		len = strlen(args[i-1]);
		if (args[i-1][len-1] == '&')
		{
			args[i-1][len-1] = '\0'; /* Remove &-char */
			*(background_process) = 1;
		}
		else
		{
			*(background_process) = 0;
		}

		return i;
	}

	return 0;
}

/*
 * Changes the working directory to a given path.
 * On failure, it will try the HOME directory instead.
 */
void change_working_directory(char path[CMD_LENGTH])
{
	int result = 0;
	int len = strlen(path);

	/* Path not long enough, use home instead */
	if (len <= 0)
		path = getenv("HOME");
	
	result = chdir(path);

	if (result == -1)
	{
		fprintf(stderr, "%s: %s.\n", path, strerror(errno));

		/* Not tried home dir yet? do it. */
		if (strcmp(path, getenv("HOME")) != 0)
		{
			change_working_directory(getenv("HOME"));
		}
	}
}

/*
 * Forks and runs a specified command argument array in the child process.
 */
void exec_fg_cmd(int argc, char * argv[MAX_ARGUMENTS+1])
{
	struct timeval before, after;
	int status = 0, cpid = 0;

	if (argc < 1)
		return;
	
	cpid = fork();
	gettimeofday(&before, NULL);
	if (cpid == CHILD)
	{
		/* Enable keyboard interrupts for foregrounds processes. */
		sigaction(SIGINT, &sigint_action, NULL);
		/* Execute program */
		status = execvp(argv[0], argv);
		if (status == -1)
		{
			fprintf(stderr, "%s: %s.\n", argv[0], strerror(errno));
			exit(EXIT_FAILURE);
		}
		else
		{
			exit(EXIT_SUCCESS);
		}
	}
	char spid[20];	sprintf(spid, "[pid %d]", cpid);
	printf("%*s Spawned foreground process. \n", current_dir_len+1, spid);
	/* Wait for process to actually exit */
	do
	{
		waitpid(cpid, &status, 0);
	} while(! WIFEXITED(status) && ! WIFSIGNALED(status));

	if (WIFEXITED(status))
	{
		gettimeofday(&after, NULL);

		long diffms = (after.tv_usec-before.tv_usec)/1000 + (after.tv_sec-before.tv_sec)*1000;
		printf("%*s Foreground process terminated.\n", current_dir_len+1, spid);
		printf("%*s Wall clock time spent:\t %ld ms.\n", current_dir_len+1, spid, diffms);
	}
}

/*
 * Forks and runs a specified command argument array in the child process.
 */
void exec_bg_cmd(int argc, char * argv[MAX_ARGUMENTS+1])
{
	int status = 0, cpid = 0;

	if (argc < 1)
		return;
	
	cpid = fork();
	if (cpid == CHILD)
	{
		status = execvp(argv[0], argv);
		if (status == -1)
		{
			fprintf(stderr, "%s: %s.\n", argv[0], strerror(errno));
			exit(EXIT_FAILURE);
		}
		else
		{
			exit(EXIT_SUCCESS);
		}
	}
	char spid[20];	sprintf(spid, "[pid %d]", cpid);
	printf("%*s Spawned background process.\n", current_dir_len+1, spid);
}

/*
 * Retrieves and simplies the current working directory.
 * Will use the array parameter and return the same pointer if all went well.
 */
char * get_workingdir(char buf[128])
{
	char * cwd = getcwd(buf, 128);

	/* Attempt replacing home directory with '~' */
	char * home = getenv("HOME");
	int hlen = strlen(home);
	if (strncmp(cwd, home, hlen) == 0)
	{
		cwd[0] = '~';
		strcpy(cwd+1, cwd+hlen);
	}
	current_dir = cwd;
	current_dir_len = strlen(cwd);
	return cwd;
}

/* 
 * Attempt to release all terminated child processes. 
 */
void release_children()
{
	int pid, status;
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
			char spid[20];	sprintf(spid, "[pid %d]", pid);
			printf("%*s Background process terminated.\n", current_dir_len+1, spid);
		}
	}
}

/*
 * Signal handler for termination of background processes.
 */
void child_termination_handler(int signum)
{
	if (signum != SIGCHLD)
		return;
	
	release_children();	
}

int main(int argc, char * argv[])
{
	int running = 1;
	int argsc = 0;
	int background_process = 0;
	char * args[MAX_ARGUMENTS+1];
	char input[CMD_LENGTH];
	struct sigaction sigchld_action;
	struct sigaction ignore_action;
                                            
#if SIGNALDETECTION
	/* Set up signal handler for child processes. */
	sigchld_action.sa_handler = child_termination_handler;
	sigemptyset(&sigchld_action.sa_mask);
	sigchld_action.sa_flags = 0;
	sigaction(SIGCHLD, &sigchld_action, NULL);
#endif

	/* Make turtleShell "immortal" to interrupts from keyboard. */
	ignore_action.sa_handler = SIG_IGN;
	sigemptyset(&ignore_action.sa_mask);
	ignore_action.sa_flags = 0;
	sigaction(SIGINT, &ignore_action, &sigint_action);

	printf("Welcome to...\n");
	printf(" ______               __    ___             \n");
	printf("/\\__  _\\             /\\ \\__/\\_ \\            \n");
	printf("\\/_/\\ \\/ __  __  _ __\\ \\ ,_\\//\\ \\      __   \n");
	printf("   \\ \\ \\/\\ \\/\\ \\/\\`'__\\ \\ \\/ \\ \\ \\   /'__`\\ \n");
	printf("    \\ \\ \\ \\ \\_\\ \\ \\ \\/ \\ \\ \\_ \\_\\ \\_/\\  __/ \n");
	printf("     \\ \\_\\ \\____/\\ \\_\\  \\ \\__\\/\\____\\ \\____\\\n");
	printf("      \\/_/\\/___/  \\/_/   \\/__/\\/____/\\/____/\n");
	printf("                                 shell v 0.1   \n");

	while(running)
	{
		#if ! SIGNALDETECTION 
		/* Poll for dead children and clean up */
		release_children();
		#endif

		/* Read command line from prompt */	
		char buf[128];
		printf("%s>", get_workingdir(buf));
		memset(input, 0, CMD_LENGTH);
		fgets(input, CMD_LENGTH, stdin);
		/* Exit? */
		if (strcmp(input, "exit\n") == 0)
		{
			running = 0;
			continue;
		}
		/* Change directory? */
		else if(strncmp(input, "cd", 2) == 0)
		{
			int len = strlen(input);
			input[len-1] = '\0'; 	/* Remove trailing newline */

			/* Special case for typing merely "cd", go to home dir. */
			if (len == 3)
			{
				change_working_directory(getenv("HOME"));
			}
			else
			{
				char path[70]; /* Skip "cd ", the rest should be our path */
				strcpy(path, input+3);
				change_working_directory(path);
			}
			
			continue;
		}

		/* Get input and parse */
		argsc = parse_input(input, args, &background_process);

		/* Run command */
		if (argsc > 0)
		{
			if (background_process)
			{
				exec_bg_cmd(argsc, args);
			}
			else
			{
				exec_fg_cmd(argsc, args);
			}
		}
	}
	/* Clean up any orphans */
	release_children();
	return 0;
}