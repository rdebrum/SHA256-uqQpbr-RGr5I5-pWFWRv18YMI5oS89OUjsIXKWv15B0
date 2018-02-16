/*
 * The Torero Shell (TOSH)
 *
 * Add your top-level comments here.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "tosh_q.h"

// global variables: add only globals for history list state
//                   all other variables should be allocated on the stack
// static: means these variables are only in scope in this .c module
static HistoryEntry history[MAXHIST]; 

static unsigned int start	= 0;
static unsigned int next	= 0;
static unsigned int size	= 0;
static unsigned int entry	= 0;

/**
 *
 * addEntry
 *
 * Add another element to our queue of recent command lines.
 *
 * @param cmdline; The command to add to the queue.
 * @return void.
 **/
void addEntry(char *cmd) {
	++entry;
	// Two cases we need to consider; if the queue is full or if it isn't.
	// If it is full; the next element to fill is the next element.
	if (size != MAXHIST) {
		next = size;
		// Update the size.
		++size;
	}
	// If the queue is full;
	else {
		start = (entry - MAXHIST) % MAXHIST;
		next = (next + 1) % MAXHIST;
	}
	// Set the new queue information.
	history[next].cmd_num = entry;
	strcpy(history[next].cmdline, cmd);
}

/**
 *
 * printHistory
 *
 * Prints out the running queue of recent command lines.
 *
 * @param none. 
 * @return void.
 **/
void printHistory() {
	// For each element in the queue (starting at start);
	int current = start;
	for (int i = 0; i < (int)size; ++i) {
		// If the current element reaches the end of the queue start from 0.
		if (current == MAXHIST) current = 0;
		printf("%d:\t%s\n", history[current].cmd_num, history[current].cmdline);
		++current;
	}
}

/**
 *
 * printID
 *
 * Prints the command line with the given command line number.
 *
 * @param id; the desired command line number to search for.
 * @return the command line associated with the given id.
 **/
char *executeID(unsigned int id) {
	int i = 0;
	// If the id is within range of the queue, return the command at num. 
	if (id > 0 && id < entry && id > (entry - size)) {
		while (id != history[i].cmd_num) { ++i; }
		return history[i].cmdline;
	}
	else { return NULL;	}
}







