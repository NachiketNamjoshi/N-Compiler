#include <stdio.h>

#define WEAK __attribute__((weak))
extern int num_func(void) WEAK;
extern char *str_func(void) WEAK;

int main(int argc, char **argv)
{
	if(num_func) {
		printf("%d\n", num_func());
	} else if(str_func) {
		printf("%s\n", str_func());
	} else {
		printf("Shouldn't Happen\n");
	}
		return 0;
}