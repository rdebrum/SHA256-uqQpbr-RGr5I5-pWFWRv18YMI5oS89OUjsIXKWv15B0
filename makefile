CC = gcc
CFLAGS = -g -Wall -lreadline 

PARSE = parse_args.c
COLORS = color_tosh.c
HIST = tosh_q.c
TARGETS = tosh 

tosh: tosh.c 
	$(CC) $(CFLAGS) $(COLORS) $(PARSE) $(HIST) -o $@ $^

hist: tosh_q.c
	$(CC) $(CFLAGS) $(COLORS) $(PARSE) $(HIST) -o $@ $^
	
