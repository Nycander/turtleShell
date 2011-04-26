#include <unistd.h>		/* execvp */
#include <stdio.h>		/* fgets, printf */
#include <string.h>		/* strlen, strtok */
#include <sys/wait.h>	/* waitpid */
#include <sys/time.h>	/* gettimeofday */
#include <stdlib.h>
#include <errno.h>

#define MAX_ARGUMENTS 5
#define CMD_LENGTH 70

/* Self-documentation */
#define CHILD 0

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

		/* Do we find a & at the end? Remove it and flag for background process */
		len = strlen(args[i-1]);
		if (args[i-1][len-1] == '&')
		{
			args[i-1][len-1] = '\0';
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
	printf("Spawned foreground process pid: %d\n", cpid);
	/* Wait for process to actually exit */
	do
	{
		waitpid(cpid, &status, 0);
	} while(! WIFEXITED(status));
	gettimeofday(&after, NULL);

	printf("Foreground process %d terminated.\n", cpid);
	long diffms = (after.tv_usec-before.tv_usec)/1000 + (after.tv_sec-before.tv_sec)*1000;
	printf("Wall clock time spent:\t %ld ms.\n", diffms);

	/* Check result */
    status = WEXITSTATUS(status);
}

char * get_workingdir(char buf[128])
{
	char * cwd = getcwd(buf, 128);
	char * home = getenv("HOME");
	int hlen = strlen(home);
	if (strncmp(cwd, home, hlen) == 0)
	{
		cwd[0] = '~';
		strcpy(cwd+1, cwd+hlen);
	}
	return cwd;
}

int main(int argc, char * argv[])
{
	int running = 1;
	int argsc = 0;
	int background_process = 0;
	char * args[MAX_ARGUMENTS+1];
	char input[CMD_LENGTH];

	while(running)
	{
		/* Read command line from prompt */	
		char buf[128];
		printf("%s>", get_workingdir(buf));
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
			if (len == 3)
			{
				change_working_directory(getenv("HOME"));
			}
			else
			{
				char path[70]; /* Skip "cd ", the rest should be out path */
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
				printf("Supposed to run bg process.\n");
			}
			else
			{
				exec_fg_cmd(argsc, args);
			}
		}
	}
	return 0;
}