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
#include <fcntl.h>
#include <errno.h>
#include "parse_args.h"
#include "colors.h"
#include "tosh_q.h"

#define DEBUG	1

// TODO: add your function prototypes here as necessary
void interp(char *argv[], char* cmdline, int bg);

char *buildPath(char *argv[], char *build, int bg);

void Execv(char *path, char *argv[], int bg);

void historyCmd(char *argv[], char *cmdline, int bg);

int classify(char *argv[]);

void ioredir(char *argv[]);

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
		    printf("EOF REACHED\n");
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
    char *build = NULL;
    
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
	build = buildPath(argv, build, bg);
	Execv(build, argv, bg);
#ifdef DEBUG
	printf("### %s to be executed\n", build);
#endif
	
    }
    free(build);
}

/**
 *
 *
 *
 *
 *
 *
 *
**/
char *buildPath(char *argv[], char *build, int bg) {

    char *path = getenv("PATH");
    char *attempt = strdup(path);
    char *temp = strtok(attempt, ":");
    
    build = strdup(path);
    
#ifdef DEBUG
    printf("env:\t%s\n", getenv("PATH"));
#endif

    while (temp != NULL) {
	strcpy(build, temp);
	strcat(build, "/");
	strcat(build, argv[0]);
	if (!access(build, X_OK)) { break; }
	
	temp = strtok(NULL, ":");

#ifdef DEBUG
	printf("temp: %s : %d\n", build, access(build, X_OK));
#endif
	
	if (temp == NULL) {
	    build = NULL;
	    build = getcwd(build, sizeof(build));
#ifdef DEBUG
	    printf("CWD = %s\n", build); 
#endif
	    strcat(build, "/");
	    strcat(build, argv[0]);
	}
    }
#ifdef DEBUG
    printf("build: %s\n", build);
#endif
    free(attempt);
    return build;
}

/**
 *
 *
 *
 *
 *
 *
**/
void Execv(char *path, char *argv[], int bg) {
    pid_t pid;

#ifdef DEBUG
    printf("PATH: %s\n", path);
#endif

    if (classify(argv) == 1) { printf("PIPE\n"); }
    else if (classify(argv) == 2) {
	printf("IOREDIR\n");
	ioredir(argv);
    }

    if ((pid = fork()) == 0) {
	pid = getpid();

	execv(path, argv);

	red();
	printf("ERROR: Command not found\n");
	exit(0);
    } else { 
	waitpid(pid, NULL, bg);
    }
}

/**
 *
 * historyCmd
 *
 * @param argv: a list of command line arguments
 * @param cmdline: a string pointer with the command
 * @param bg: specifies if the command is to be executed in the foreground or background
**/
void historyCmd(char *argv[], char *cmdline, int bg) {
    if (argv[0][1] == '!') {

#ifdef DEBUG
	printf("success\n");
#endif

        strcpy(cmdline, repeatLast());
    
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
        strcpy(cmdline, executeID(idnum));
    }
    if (cmdline == NULL) {
	red();
	printf("ERROR: invalid command ID\n");
    } else { 
#ifdef DEBUG
	printf("HERE\n");
	printf("%s\n", cmdline);
#endif	
	bg = parseArguments(cmdline, argv);
	interp(argv, cmdline, bg);
    }
}

/**
 *
 *
 *
 *
 **/
int classify(char *argv[]) {
    if (argv[1] == NULL) { return 0; }
    for (int i = 0; i < strlen(argv[1]); ++i) {	
	if (argv[1][i] == '|') {
	    return 1;
	} else if (argv[1][i] == '>' || argv[1][i] == '<') {
	    return 2;
	} 

#ifdef DEBUG
	printf("%c\n", argv[1][i]);
#endif
    }
}
/**
 *
 *
 *
 *
 *
 *
 *
**/
// TODO: CALL EXECV IN HERE?
void ioredir(char *argv[]) {
    int fd = 1;
    int outfile = 0;
    int infile = 0;

    if (strlen(argv[1]) == 2) {
	fd = argv[1][0] - 48;
#ifdef DEBUG
	printf("fd: %d\n", fd);
	printf("OUTPUT FILE: %s\n", argv[2]);
#endif
	if (fd < 1 && fd > 2) {
		red();
		printf("ERROR: bad file descriptor\n");
		magenta();
        }
	
	outfile = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); 
	dup2(outfile, fd);
	close(outfile);
#ifdef DEBUG
	printf("ERRNO: %d\n", errno);
#endif
    } else {
	infile = open(argv[2], O_RDONLY);
	dup2(infile, 0);
	close(infile);
    } 
   
}

/**
 *
 * reapChild
 *
 * Reap any zombie children.
**/
void reapChild() {
    while (waitpid(-1, NULL, WNOHANG) > 0) {}
}
