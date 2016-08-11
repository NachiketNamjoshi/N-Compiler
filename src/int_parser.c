#include <stdio.h>

extern int nmain(void);

int main(int argc, char **argv)
{
	int val = nmain();
	printf("%d\n", val);
	return 0;
}