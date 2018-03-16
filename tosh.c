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

	// TODO: add a call to signal to register your signal handler on 
	//       SIGCHLD here

	struct sigaction sa;
	sa.sa_handler = reapChild;
	sa.sa_flags = SA_NOCLDSTOP;
	sigaction(SIGCHLD, &sa, NULL);
	    
	Command *cmd;
	cmd = malloc(sizeof(Command));
	while(1) {
		
		// (1) read in the next command entered by the user
		cyan();
		cmdInit(cmd);
		cmd->cmdline = readline("tosh$ ");
		
		// NULL indicates EOF was reached, which in this case means someone
		// probably typed in CTRL-d
		if (cmd->cmdline == NULL) {
		    red();
		    Error("EOF REACHED");
		    fflush(stdout);
		    exit(0);
		} else if (strlen(cmd->cmdline) <= 1) { continue; }
		
		// TODO: complete the following top-level steps
		// (2) parse the cmdline
		
		cmd->bg = parseArguments(cmd->cmdline, cmd->argv);
		int i = 0;
#ifdef DEBUG
		printf("BACKGROUND = %d\n", cmd->bg);
		while (cmd->argv[i] != NULL) {
		    printf("%d: #%s#\n", i + 1, cmd->argv[i]);
		    i++;
		}
#endif		
		if (cmd->bg) { cmd->bg = WNOHANG; }
		interp(cmd);
		// (3) determine how to execute it, and then execute it
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
    if (err != NULL) {
	fprintf(stderr, "%s\n", err);
    } else {
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
    magenta();
    if (!strcmp(cmd->argv[0], "history")) {
	addEntry(cmd->cmdline);
	printHistory();
    } else if (!strcmp(cmd->argv[0], "exit")) {
	printf("Bye\n");
	free(cmd);
	free(cmd->pipe);
	exit(0);
    } else if (cmd->argv[0][0] == '!') { 
	 historyCmd(cmd); 
    } else if (!strcmp(cmd->argv[0], "cd")) {
	addEntry(cmd->cmdline);
	if (cmd->argv[1] == NULL) { chdir(getenv("HOME")); }
	else { chdir(cmd->argv[1]); }
    } else { 
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

    char *path = getenv("PATH");
    char *attempt = strdup(path);
    char *temp = strtok(attempt, ":");
    
    cmd->path = strdup(path);
    
#ifdef DEBUG
    printf("env:\t%s\n", getenv("PATH"));
#endif

    while (temp != NULL) {
	strcpy(cmd->path, temp);
	strcat(cmd->path, "/");
	strcat(cmd->path, cmd->argv[0]);
	if (!access(cmd->path, X_OK)) { break; }
	
	temp = strtok(NULL, ":");

#ifdef DEBUG
	printf("temp: %s : %d\n", cmd->path, access(cmd->path, X_OK));
#endif
	
	if (temp == NULL) {
	    cmd->path = NULL;
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
    int saved_stdout = dup(1); 
    int saved_stderr = dup(2); 

#ifdef DEBUG
    printf("PATH: %s\n", cmd->path);
#endif

    IOredir(cmd);
    Pipe(cmd);

    if (cmd->infile != NULL) {
#ifdef DEBUG
	printf("INPUTTING\n");
	printf("fd: %d\n", cmd->infd);
	printf("INPUT FILE: %s\n", cmd->infile);
#endif
	cmd->infd = open(cmd->infile, O_RDONLY);
	dup2(cmd->infd, 0);
	close(cmd->infd);
    }

    if (cmd->outfile != NULL) {
#ifdef DEBUG
	printf("OUTPUTTING\n");
	printf("fd: %d\n", cmd->fd);
	printf("OUTPUT FILE: %s\n", cmd->outfile);
#endif
	cmd->outfd = open(cmd->outfile, O_RDWR | O_CREAT, 0666 | O_CLOEXEC); 
	dup2(cmd->outfd, cmd->fd);
	close(cmd->outfd);

	if (cmd->outfile2 != NULL) {
#ifdef DEBUG
	    printf("OUTPUTTING\n");
	    printf("fd: %d\n", cmd->fd2);
	    printf("OUTPUT FILE: %s\n", cmd->outfile2);
#endif
	    cmd->outfd2 = open(cmd->outfile2, O_RDWR | O_CREAT, 0666 | O_CLOEXEC); 
	    dup2(cmd->outfd2, cmd->fd2);
	    close(cmd->outfd2);
	}

    }
    
    pipe(cmd->pipefd);
    pid = fork();
    if (pid == -1) {
	Error("ERROR: Fork failure");
	return;
    } else if (pid == 0) {
	/* CHILD PROCESS */
	pid = getpid();
	
	if (cmd->pipe != NULL) {
	    dup2(cmd->pipefd[0], 0);
	    close(cmd->pipefd[1]);
#ifdef DEBUG
	    printf("I AM EXECUTING GREP\n");
#endif
	    execv(cmd->pipe->path, cmd->pipe->argv);
	} else {
#ifdef DEBUG
	    printf("I AM EXECUTING NORMALLY\n");
#endif
	    execv(cmd->path, cmd->argv);
	}

	Error("ERROR: Command not found");
	exit(0);
    } else { 
	/* PARENT PROCESS */
	if (cmd->pipe != NULL) {
	    pid = fork();
	    if (pid == -1) {
		Error("ERROR: Pipe fork failure");
		return;
	    } else if (pid == 0) {
		pid = getpid();
		dup2(cmd->pipefd[1], 1);
		close(cmd->pipefd[0]);
#ifdef DEBUG
		printf("I AM EXECUTING CAT\n");
#endif
		execv(cmd->path, cmd->argv);
		Error("ERROR: Command not found");
		exit(0);
	    }
	}
	
	waitpid(pid, NULL, cmd->bg);
	
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
    if (cmd->argv[0][1] == '!') {

#ifdef DEBUG
	printf("success\n");
#endif

        strcpy(cmd->cmdline, repeatLast());
    
    } else {
	char idchar[4];
        unsigned idnum = 0;
 
        for (int i = 1; i < strlen(cmd->cmdline); i++) {
	    idchar[i - 1] = cmd->cmdline[i];
        }

	idnum = strtol(idchar, NULL, 10);
#ifdef DEBUG
	printf("CMD NUM TO EXECV: %d\n", idnum);
#endif
        strcpy(cmd->cmdline, executeID(idnum));
    }
    if (cmd->cmdline == NULL) {
	Error("ERROR: invalid command ID");
    } else { 
#ifdef DEBUG
	printf("HERE\n");
	printf("%s\n", cmd->cmdline);
#endif	
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
    
    int i = 0;
    while (cmd->argv[i] != NULL) {
	for (int j = 0; j < strlen(cmd->argv[i]); ++j) {
	    if (cmd->argv[i][j] == '<') {
		if (cmd->argv[i + 1] == NULL) {
		    Error("ERROR: No input file");
		    return;
		}
		if (strlen(cmd->argv[i]) != 1) {
		    Error("ERROR: Bad re-direct specification");
		    return;
		}
		cmd->infile = cmd->argv[i + 1];
	    
	    } else if (cmd->argv[i][j] == '>') {
		
		if (strlen(cmd->argv[i]) == 2) {
		    if (cmd->fd == 0) {
#ifdef DEBUG
			printf("fd\n");
#endif
			cmd->fd = cmd->argv[i][0] - 48;
			/* ERROR CHECKING */
			if (cmd->fd < 1 || cmd->fd > 2) {
		            Error("ERROR: Bad file descriptor");
			    return;
			}
		    } else {
#ifdef DEBUG
			printf("fd2\n");
#endif
			cmd->fd2 = cmd->argv[i][0] - 48;
			/* ERROR CHECKING */
			if (cmd->fd2 < 1 || cmd->fd2 > 2) {
			    Error("ERROR: Bad file descriptor");
			    return;
			}
		    }
		}
		
		if (cmd->argv[i + 1] == NULL) {
		    Error("ERROR: No output file");
		    return;
		}
		if (cmd->outfile == NULL) {
		    cmd->outfile = cmd->argv[i + 1];
		} else {
		    cmd->outfile2 = cmd->argv[i + 1];
		}
	    }
	}
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
    
    while (cmd->argv[i][0] != '|' && strlen(cmd->argv[i]) != 1) {
	++i;
	if (cmd->argv[i] == NULL) {
	    return;
	}
    }
    cmd->pipe = malloc(sizeof(Command));
    cmd->pipe->bg = 0;
#ifdef DEBUG
    printf("PIPING\n");
    printf("i: %d: %s\n", i, cmd->argv[i + 1]);
#endif
    cmd->argv[i] = NULL;

    ++i;
    while (cmd->argv[i] != 0) {
	cmd->pipe->argv[j] = cmd->argv[i];
#ifdef DEBUG
	printf("%s\n", cmd->pipe->argv[j]);
#endif
	++i;
	++j;
    }
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
