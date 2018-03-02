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

//#define DEBUG	1

// TODO: add your function prototypes here as necessary
void interp(char *argv[], char* cmdline, int bg);

void Execv(char *argv[], int bg);

void historyCmd(char *argv[], char *cmdline, int bg);

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
		magenta();

		// NULL indicates EOF was reached, which in this case means someone
		// probably typed in CTRL-d
		if (cmdline == NULL) {
		    red();
		    fflush(stdout);
		    exit(0);
		} else if (strlen(cmdline) <= 1) { continue; }
		
		// TODO: complete the following top-level steps
		// (2) parse the cmdline
		char *argv[MAXARGS];

		int bg = parseArguments(cmdline, argv);
		int i = 0;
#ifdef DEBUG
		printf("BACKGROUND = %d\n", bg);
		while (argv[i] != NULL) {
		    printf("%d: #%s#\n", i + 1, argv[i]);
		    i++;
		}
		magenta();
#endif		
		if (bg) { bg = WNOHANG; }
		interp(argv, cmdline, bg);
		// (3) determine how to execute it, and then execute it
	}
	return 0;
}
/**
 *
 * Interp
 *
 * Interpret the command line to be be executed.
 *
 * @param argv: list of command line parameters
 * @param cmdline: the original command entry
 * @param bg: 0 for foreground, 1 for background execution
 **/
void interp(char *argv[], char *cmdline, int bg) {
    if (!strcmp(argv[0], "history")) {
	addEntry(cmdline);
	printHistory();
    } else if (!strcmp(argv[0], "exit")) {
	printf("Bye\n");
	exit(0);
    } else if (argv[0][0] == '!') { 
	 historyCmd(argv, cmdline, bg); 
    } else if (!strcmp(argv[0], "cd")) {
	addEntry(cmdline);
	if (argv[1] == NULL) { chdir(getenv("HOME")); }
	else { chdir(argv[1]); }
    } else { 
	addEntry(cmdline);
	Execv(argv, bg);
    }
}

void Execv(char *argv[], int bg) {
    pid_t pid;

    char *path = getenv("PATH");
    char *attempt = strdup(path);
    char build[strlen(path)];
    char *temp = strtok(attempt, ":");
    
#ifdef DEBUG
    printf("env:\t%s\n", getenv("PATH"));
#endif

    while (temp != NULL) {
	strcpy(build, temp);
	strcat(build, "/");
	strcat(build, argv[0]);
#ifdef DEBUG
	printf("temp: %s : %d\n", build, access(build, X_OK));
#endif
	if (!access(build, X_OK)) { break; }
	temp = strtok(NULL, ":");
	if (temp == NULL) {
	    getcwd(build, sizeof(build));
	    strcat(build, "/");
	    strcat(build, argv[0]);
#ifdef DEBUG
	    printf("build: %s\n", build);
#endif
	}
    }
    
    if ((pid = fork()) == 0) {
	pid = getpid();
	execv(build, argv);
	red();
	printf("ERROR: Command not found\n");
	exit(0);
    } else { 
	waitpid(pid, NULL, bg);
    }
    path = NULL;
    free(attempt);
}

void historyCmd(char *argv[], char *cmdline, int bg) {
    if (argv[0][1] == '!') {

#ifdef DEBUG
	printf("success\n");
#endif

	cmdline = repeatLast();
    } else {
	char idchar[4];
        unsigned idnum = 0;
 
        for (int i = 1; i < strlen(cmdline); i++) {
	    idchar[i - 1] = cmdline[i];
        }

	idnum = strtol(idchar, NULL, 10);
#ifdef DEBUG
	printf("CMD NUM TO EXECV: %d\n", idnum);
#endif
        cmdline = executeID(idnum);
    }
    if (cmdline == NULL) {
	red();
	printf("ERROR: invalid command ID\n");
    } else { 
	bg = parseArguments(cmdline, argv);
	interp(argv, cmdline, bg);
    }
}

void reapChild() {
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}
