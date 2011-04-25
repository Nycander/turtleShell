#include <unistd.h>		/* execvp */
#include <stdio.h>		/* fgets, printf */
#include <string.h>		/* strlen, strtok */

#define MAX_ARGUMENTS 5
#define CMD_LENGTH 70

/*
 * Tries to parse out a command line argument array from a given string.
 * Will return the amount of parsed arguments.
 */
int parse_input(char input[CMD_LENGTH], char * args[MAX_ARGUMENTS+1])
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
		while(i <= MAX_ARGUMENTS && (args[i++] = strtok(NULL, " ")) != NULL);

		/* Finish argument array */
		args[i] = NULL;
		return i;
	}

	return 0;
}

int main(int argc, char * argv[])
{
	int argsc = 0;
	char * args[MAX_ARGUMENTS+1];
	char input[CMD_LENGTH];

	/* Read command line from prompt */	
	printf("> ");
	fgets(input, CMD_LENGTH, stdin);

	/* Get input and parse */
	argsc = parse_input(input, args);

	/* Run command */
	if (argsc > 0)
		execvp(args[0], args);
	
	return 0;
}