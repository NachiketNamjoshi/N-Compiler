#include <stdio.h>

#define WEAK __attribute__((weak))
extern int int_compile(void) WEAK;
extern char *str_compile(void) WEAK;

int main(int argc, char **argv)
{
	if(int_compile) {
		printf("%d\n", int_compile());
	} else if(str_compile) {
		printf("%s\n", str_compile());
	} else {
		printf("Shouldn't Happen\n");
	}
	return 0;
}