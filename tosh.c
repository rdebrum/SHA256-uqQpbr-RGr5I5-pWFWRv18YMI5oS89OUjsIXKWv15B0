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
#include <readline/readline.h>
#include "parse_args.h"
#include "colors.h"

#define DEBUG	1

// TODO: add your function prototypes here as necessary


int main(){ 

	// TODO: add a call to signal to register your signal handler on 
	//       SIGCHLD here

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
		
		// (3) determine how to execute it, and then execute it
	}
	return 0;
}

