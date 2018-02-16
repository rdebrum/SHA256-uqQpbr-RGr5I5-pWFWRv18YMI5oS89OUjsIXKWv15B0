/*
 * The Torero Shell (TOSH)
 *
 * Add your top-level comments here.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>
#include <readline/readline.h>
#include "parse_args.h"
#include "colors.h"
#include "tosh_q.h"

#define DEBUG	1

// TODO: add your function prototypes here as necessary
void interp(char **argv, char* cmdline, int bg);

void Execv(char **argv, int bg);

void reapChild();

int main(){ 

	// TODO: add a call to signal to register your signal handler on 
	//       SIGCHLD here

	struct sigaction sa;
	sa.sa_handler = reapChild;
	sa.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);

	while(1) {
		// (1) read in the next command entered by the user
		cyan();
		char *cmdline = readline("tosh$ ");
		green();

		// NULL indicates EOF was reached, which in this case means someone
		// probably typed in CTRL-d
		if (cmdline == NULL) {
			fflush(stdout);
			exit(0);
		}

#ifdef DEBUG	
		magenta();
		fprintf(stdout, "DEBUG: %s\n", cmdline);
#endif
		// TODO: complete the following top-level steps
		// (2) parse the cmdline
		char *argv[MAXARGS];

		int bg = parseArguments(cmdline, argv);
		int i = 0;
#ifdef DEBUG
		magenta();
		printf("BACKGROUND = %d\n", bg);
		while (argv[i] != NULL) {
		    printf("%d: #%s#\n", i + 1, argv[i]);
		    i++;
		}
#endif		
		if (bg) { bg = WNOHANG; }
		interp(argv, cmdline, bg);
		// (3) determine how to execute it, and then execute it
	}
	return 0;
}

void interp(char **argv, char *cmdline, int bg) {
    if (!strcmp(argv[0], "history")) {
	addEntry(cmdline);
	printHistory();
    } else if (!strcmp(argv[0], "exit")) {
	printf("Bye\n");
	exit(0);
/*    } else if (argv[0][0] == '!') { 
	historyID(argv); 
*/    } else if (!strcmp(argv[0], "cd")) {
	addEntry(cmdline);
	if (argv[1] == NULL) { chdir(getenv("HOME")); }
	else { chdir(argv[1]); }
    } else { Execv(argv, bg); }
}

void Execv(char **argv, int bg) {
    pid_t pid;
    int i = 0;

    char *path = getenv("PATH");
    char *temp = strtok(path, ":");
    char *cpy = malloc(strlen(path));
    int acc = 0;

    while (temp != NULL) {
	strcpy(cpy, temp);
	strcat(cpy, "/");
	strcat(cpy, argv[0]);
	acc = access(cpy, X_OK);
#ifdef DEBUG
	printf("temp: %s:%d\n", cpy, acc);
//	exit(0);
#endif
//	temp = NULL;
	temp = strtok(NULL, ":");
    }

    /* if ((pid = fork()) == 0) {
	pid = getpid();
    */
}

void reapChild() {
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}
