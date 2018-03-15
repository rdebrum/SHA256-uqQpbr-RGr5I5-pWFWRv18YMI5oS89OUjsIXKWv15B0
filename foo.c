#include <stdio.h>
#include <stdlib.h>

int main () {
    for (int i = 0; i < 10; i++) {
	fprintf(stdout, "%d haha\n", rand() % 100);
    }
    fprintf(stderr, "ERROR: testing\n");
}
