CC = gcc
CFLAGS = -g -Wall -lreadline 

PARSE = parse_args.c
COLORS = color_tosh.c
TARGETS = tosh 

tosh: tosh.c 
	$(CC) $(CFLAGS) $(COLORS) $(PARSE) -o $@ $^
