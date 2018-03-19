/*
 * The Torero Shell (TOSH)
 * 
 * A Shell program with piping and IO redirect cpabilities
 *
 * Authors:
 *	Nathan Kramer 
 *	Robert de Brum
 *
 * Written for USD COMP 310 Class; Project 1
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

typedef struct cmd {
    char	*cmdline;
    char	*argv[MAXARGS];
    char	*path;
    char	*infile;
    char	*outfile;
    char	*outfile2;
    int		bg;
    int		fd;
    int	        fd2;
    int		infd;
    int		outfd;
    int	        outfd2;
    int		pipefd[2];
    struct cmd	*pipe;
} Command;

// TODO: add your function prototypes here as necessary
void interp(Command *cmd);

char *buildPath(Command *cmd);

void Execv(Command *cmd);

void historyCmd(Command *cmd);

void IOredir(Command *cmd);

void Pipe(Command *cmd);

void reapChild();

void Error(char *err);

void cmdInit(Command *cmd);

int main(){ 

	// Set up the SIGCHLD signal handler
	struct sigaction sa;
	sa.sa_handler = reapChild;
	sa.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);
	    
	// Create and allocate memory for the next
	Command *cmd;
	cmd = malloc(sizeof(Command));
	while(1) {
		
		/* Read in the next command entered by the user */
		cyan();
		cmdInit(cmd);
		cmd->cmdline = readline("tosh$ ");
		
		/* NULL indicates EOF was reached, which in this case means someone
		    probably typed in CTRL-d */
		if (cmd->cmdline == NULL) {
		    red();
		    Error("EOF REACHED");
		    fflush(stdout);
		    exit(0);
		} else if (strlen(cmd->cmdline) <= 1) { continue; }
		
		/* Parse the command line and set up argv and bg */
		cmd->bg = parseArguments(cmd->cmdline, cmd->argv);
#ifdef DEBUG
		int i = 0;
		printf("BACKGROUND = %d\n", cmd->bg);
		while (cmd->argv[i] != NULL) {
		    printf("%d: #%s#\n", i + 1, cmd->argv[i]);
		    i++;
		}
#endif		
		/* Set the bg to WNOHANG if '&' is specified */
		if (cmd->bg) { cmd->bg = WNOHANG; }
		/* Interpret and execute */
		interp(cmd);
	}
	return 0;
}

/**
 *
 * Error
 *
 * Prints stderr error messages in red OR the current errno
 *
 * @param err: a string of the error text OR can be NULL (to print errno).
**/
void Error(char *err) {
    red();
    if (err != NULL) { /* Print error messages in red */
	fprintf(stderr, "%s\n", err);
    } else { /* Print errno's and related error messages */
	fprintf(stderr, "ERRNO %d: %s\n", errno, strerror(errno));
    }
    magenta();
}

/**
 *
 * cmdInit
 *
 * Set the initial states and values for a given Command struct
 *
 * @param cmd: Command struct
**/
void cmdInit(Command *cmd) {	
	/* All *char are NULL and int values are 0 */	
	cmd->cmdline	= NULL;
	cmd->path	= NULL;
	cmd->infile	= NULL;
	cmd->outfile	= NULL;
	cmd->outfile2	= NULL;
	cmd->bg		= 0;
	cmd->fd		= 0;
	cmd->fd2	= 0;
	cmd->infd	= 0;
	cmd->outfd	= 0;
	cmd->outfd2	= 0;
	cmd->pipe	= NULL;
}	


/**
 *
 * Interp
 *
 * Interpret the command line to be be executed.
 *
 * @param cmd: Command struct
 **/
void interp(Command *cmd) {
    /* Print everything in Magenta: check the command, add it into the queue,
	and then send it to the proper function */
    magenta();
    if (!strcmp(cmd->argv[0], "history")) {
	addEntry(cmd->cmdline);
	printHistory();
    } else if (!strcmp(cmd->argv[0], "exit")) {
	printf("Bye\n");
	free(cmd);	    /*	Free the allocated	*/
	free(cmd->pipe);    /*	memory before exiting	*/
	exit(0);
    } else if (cmd->argv[0][0] == '!') { 
	/* No need to add the entry here (interp to be called again) */ 
	historyCmd(cmd); 
    } else if (!strcmp(cmd->argv[0], "cd")) {
	addEntry(cmd->cmdline);
	// For nothing specified, cd to '~'. Or use argv[1]
	if (cmd->argv[1] == NULL) { chdir(getenv("HOME")); }
	else { chdir(cmd->argv[1]); }
    } else { /* For everything else that is NOT built-in */
	addEntry(cmd->cmdline);
	cmd->path = buildPath(cmd);
	Execv(cmd);
    }
}

/**
 *
 * buildPath
 *
 * Calls getenv and access to find the correct path for the desired executable
 *
 * @param cmd: Command struct
**/
char *buildPath(Command *cmd) {
    /* Need lots of variables to build the correct path */
    char *path = getenv("PATH");
    char *attempt = strdup(path);
    char *temp = strtok(attempt, ":"); 
    /* Allocate as much memory as all paths combined to ensure enough space */
    cmd->path = strdup(path); 
#ifdef DEBUG
    printf("env:\t%s\n", getenv("PATH"));
#endif
    /* Tokenize through the path (looking for ':'
	and append a '/' and the executable name (argv[0]) */
    while (temp != NULL) {
	strcpy(cmd->path, temp);
	strcat(cmd->path, "/");
	strcat(cmd->path, cmd->argv[0]);
	/* If we are granted access, then break */
	if (!access(cmd->path, X_OK)) { break; }
	temp = strtok(NULL, ":");
#ifdef DEBUG
	printf("temp: %s : %d\n", cmd->path, access(cmd->path, X_OK));
#endif
	/* If the next element is null, the executable is in the cwd */
	if (temp == NULL) {
	    cmd->path = NULL; /* Reset path to be rebuilt */
	    cmd->path = getcwd(cmd->path, sizeof(cmd->path));
#ifdef DEBUG
	    printf("CWD = %s\n", cmd->path); 
#endif
	    strcat(cmd->path, "/");
	    strcat(cmd->path, cmd->argv[0]);
	}
    }
#ifdef DEBUG
    printf("build: %s\n", cmd->path);
#endif
    /* Free the allocated memory */
    free(attempt);
    return cmd->path;
}

/**
 *
 * Execv
 * 
 * Calls IOredir and Pipe to set up the input/outputs
 *	before calling execv
 *
 * @param cmd: Command struct
**/
void Execv(Command *cmd) {
    pid_t pid;
    pid_t pid2;
    // Save and restore the stdin and stderr fd's
    int saved_stdout = dup(1); 
    int saved_stderr = dup(2); 

#ifdef DEBUG
    printf("PATH: %s\n", cmd->path);
#endif
    // Set up the necessary components
    IOredir(cmd);
    Pipe(cmd);
    /* If IOredir sets up infile: */ 
    if (cmd->infile != NULL) {
#ifdef DEBUG
	printf("INPUTTING\n");
	printf("fd: %d\n", cmd->infd);
	printf("INPUT FILE: %s\n", cmd->infile);
#endif
	/* Open the file and duplicate the fd's */
	cmd->infd = open(cmd->infile, O_RDONLY);
	dup2(cmd->infd, 0);
	close(cmd->infd);
    }
    /* If IOredir sets up infile: */ 
    if (cmd->outfile != NULL) {
#ifdef DEBUG
	printf("OUTPUTTING\n");
	printf("fd: %d\n", cmd->fd);
	printf("OUTPUT FILE: %s\n", cmd->outfile);
#endif
	/* Open the file and duplicate the fd's */
	cmd->outfd = open(cmd->outfile, O_RDWR | O_CREAT, 0666 | O_CLOEXEC); 
	dup2(cmd->outfd, cmd->fd);
	close(cmd->outfd);
	/* If TWO outfiles are specifies, set up the second fd */
	if (cmd->outfile2 != NULL) {
#ifdef DEBUG
	    printf("OUTPUTTING\n");
	    printf("fd: %d\n", cmd->fd2);
	    printf("OUTPUT FILE: %s\n", cmd->outfile2);
#endif
	    /* Open the file and duplicate the fd's */
	    cmd->outfd2 = open(cmd->outfile2, O_RDWR | O_CREAT, 0666 | O_CLOEXEC); 
	    dup2(cmd->outfd2, cmd->fd2);
	    close(cmd->outfd2);
	}
    }
    /* FORKING PROCESSES */
    /* Open the pipe (in case it gets used) */
    pipe(cmd->pipefd);
    pid = fork();
    if (pid == -1) { /* ERROR CHECKING */
	Error("ERROR: Fork failure");
	return;
    } else if (pid == 0) {
	/* CHILD PROCESS */
	pid = getpid();
	if (cmd->pipe != NULL) {
	    /* Change fd's to the write end 
	       of the pipe and execute */
	    close(cmd->pipefd[1]);
	    dup2(cmd->pipefd[0], 0);
	    close(cmd->pipefd[0]);
#ifdef DEBUG
	    printf("I AM EXECUTING GREP\n");
#endif
	    /* Execute using the cmd->pipe struct */
	    execv(cmd->pipe->path, cmd->pipe->argv);
	} else { /* If not a pipe commad, execute using cmd struct */
#ifdef DEBUG
	    printf("I AM EXECUTING NORMALLY\n");
#endif
	    execv(cmd->path, cmd->argv);
	}
	/* ERROR CHECKING (should have terminated) */
	Error("ERROR: Command not found");
	exit(0);
    } else { 
	/* PARENT PROCESS */
	/* Check if it is a piping command */
	if (cmd->pipe != NULL) {
	    pid2 = fork(); 
	    if (pid2 == -1) { /* ERROR CHECKING */
		Error("ERROR: Pipe fork failure");
		return;
	    /* CHILD 2 PROCESS */
	    } else if (pid2 == 0) {
#ifdef DEBUG
		printf("FORK 2\n");
		printf("%s\n", cmd->path);
#endif
		pid2 = getpid();
		/* Change fd's to the reading 
		    end of pipe and execute */
		close(cmd->pipefd[0]);
		dup2(cmd->pipefd[1], 1);
		close(cmd->pipefd[1]);
		Error(NULL);	
		execv(cmd->path, cmd->argv);
		/* ERROR CHECKING (should have terminated) */
		Error("ERROR: Command not found");
		exit(0);
	    } else { /* BACK TO PARENT PROCESS */
		pid2 = waitpid(pid2, NULL, 0);
		/* restore stdout/stderr */
		dup2(saved_stdout, 1);
		close(saved_stdout);
		dup2(saved_stderr, 2);
		close(saved_stderr);
		return;
	    }
	}
	/* Only reaches here if not a pipe command */
	waitpid(pid, NULL, cmd->bg);
#ifdef DEBUG
	pid = printf("pid1: %d\n", pid);
	Error(NULL);
#endif
	/* Restore file descriptors */	
	dup2(saved_stdout, 1);
	close(saved_stdout);
	dup2(saved_stderr, 2);
	close(saved_stderr);
    }
}

/**
 *
 * historyCmd
 *
 * Calls on functions in tosh_q.c to execute either the last command
 *	or a specified command.
 *
 * @param cmd: Command struct
**/
void historyCmd(Command *cmd) {
    /* First '!' has been found, so look for the second */
    if (cmd->argv[0][1] == '!') {
#ifdef DEBUG
	printf("success\n");
#endif
	/* Get the last command line executed */
        strcpy(cmd->cmdline, repeatLast());
    } else {
	/* Prepare for a 4 digit command ID (won't be more than 2 chars) */
	char idchar[4];
        unsigned idnum = 0;
	/* Copy JUST the ID number, not the '!' */
        for (int i = 1; i < strlen(cmd->cmdline); i++) {
	    idchar[i - 1] = cmd->cmdline[i];
        }
	/* Use this number to search within the history queue */
	idnum = strtol(idchar, NULL, 10);
        strcpy(cmd->cmdline, executeID(idnum));
#ifdef DEBUG
	printf("CMD NUM TO EXECV: %d\n", idnum);
#endif
    }
    /* ERROR CHECKING (repeatLast/executeID return NULL on failure) */
    if (cmd->cmdline == NULL) {
	Error("ERROR: invalid command ID");
    } else { 
#ifdef DEBUG
	printf("HERE\n");
	printf("%s\n", cmd->cmdline);
#endif
	/* Parse the new (old) command line, set bg, and interpret to be executed */
	cmd->bg = parseArguments(cmd->cmdline, cmd->argv);
	if (cmd->bg) { cmd->bg = WNOHANG; }
	interp(cmd);
    }
}

/**
 *
 * IOredir
 *
 * Sets up the input and output file names and file descriptors 
 *	for Execv to use when executing
 *
 * @param cmd: Command struct
**/
void IOredir(Command *cmd) {
    /* Go through the list of argv elements */ 
    int i = 0;
    while (cmd->argv[i] != NULL) {
	/* Compare each character to the '<' or '>' char */
	for (int j = 0; j < strlen(cmd->argv[i]); ++j) {
	    if (cmd->argv[i][j] == '<') { /* INPUT SPECIFIED */
		/* ERROR CHECKING */
		if (cmd->argv[i + 1] == NULL) { /* Check for file */
		    Error("ERROR: No input file");
		    return;
		}
		if (strlen(cmd->argv[i]) != 1) { /* Make sure the '<' is alone */
		    Error("ERROR: Bad re-direct specification");
		    return;
		}
		/* Set input file (to the right of the '<' */
		cmd->infile = cmd->argv[i + 1];
	    } else if (cmd->argv[i][j] == '>') { /* OUTPUT SPECIFIED */
		/* SET UP FILE DESCRIPTOR */ 
		if (strlen(cmd->argv[i]) == 2) {
		    if (cmd->fd == 0) { /* CHECK IF fd HAS ALREADY BEEN SET */
#ifdef DEBUG
			printf("fd\n");
#endif
			/* Set fd */
			cmd->fd = cmd->argv[i][0] - 48;
			/* ERROR CHECKING */
			if (cmd->fd < 1 || cmd->fd > 2) {
		            Error("ERROR: Bad file descriptor");
			    return;
			}
		    } else { /* SET UP SECOND OUTPUT FILE */
#ifdef DEBUG
			printf("fd2\n");
#endif
			/* Set fd2 */
			cmd->fd2 = cmd->argv[i][0] - 48;
			/* ERROR CHECKING */
			if (cmd->fd2 < 1 || cmd->fd2 > 2) {
			    Error("ERROR: Bad file descriptor");
			    return;
			}
		    }
		}
		/* ERROR CHECKING */	
		if (cmd->argv[i + 1] == NULL) {
		    Error("ERROR: No output file");
		    return;
		}
		/* Set outfile2 if outfile 1 has not already been set */
		if (cmd->outfile == NULL) {
		    cmd->outfile = cmd->argv[i + 1];
		} else {
		    cmd->outfile2 = cmd->argv[i + 1];
		}
	    }
	}
	/* Don't forget to keep incrementing */
	++i;
    }
}

/**
 *
 * Pipe
 *
 * Sets up the pipe struct and commands for Execv to execute
 *
 * @param cmd: Command struct
**/
void Pipe(Command *cmd) {
    int i = 0;
    int j = 0;
    /* Search for the '|' symbol */ 
    while (cmd->argv[i][0] != '|' && strlen(cmd->argv[i]) != 1) {
	++i;
	/* Exit the function if we reached the last element */
	if (cmd->argv[i] == NULL) {
	    return;
	}
    }
    /* Allocate the memory (was previously null) */
    cmd->pipe = malloc(sizeof(Command));
    cmd->pipe->bg = 0;
#ifdef DEBUG
    printf("PIPING\n");
    printf("i: %d: %s\n", i, cmd->argv[i + 1]);
#endif
    /* Set the '|' argv element to null so that argv is split into two lists */
    cmd->argv[i] = NULL;
    ++i; /* Move to the next argv element and build the second argv (pipe->argv) */
    while (cmd->argv[i] != 0) {
	cmd->pipe->argv[j] = cmd->argv[i];
#ifdef DEBUG
	printf("%s\n", cmd->pipe->argv[j]);
#endif
	++i;
	++j;
    }
    /* Build the path before you send it back to Execv to be executed */
    cmd->pipe->path = buildPath(cmd->pipe); 
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
