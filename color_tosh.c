#include <stdio.h>
#include "colors.h"

void red() 		{ printf("\033[0;31m"); }
void boldRed() 		{ printf("\033[1;31m"); }
void green() 		{ printf("\033[0;32m"); }
void boldGreen() 	{ printf("\033[1;32m"); }
void yellow() 		{ printf("\033[0;33m"); }
void boldYellow() 	{ printf("\033[1;33m"); }
void blue() 		{ printf("\033[0;34m"); }
void boldBlue()		{ printf("\033[1;34m"); }
void magenta()		{ printf("\033[0;35m"); }
void boldMagenta() 	{ printf("\033[1;35m"); }
void cyan()		{ printf("\033[0;36m"); }
void boldCyan()		{ printf("\033[1;36m"); }
void reset()		{ printf("\033[0;0m"); }
void bold()		{ printf("\033[1;0m"); }
