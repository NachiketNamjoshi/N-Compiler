#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

#define MAX_BUFFER_LENGTH 256

void error(char *fmt, ...) {
	va_list args;
	va_start(args,fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(1);
}

void num_compile(int n) {
	int c;
	while((c=getc(stdin)) != EOF) {
		if(isspace(c)){
			break;
		}
		else if(!isdigit(c)){
			error("Invalid Symbol: '%c'",c);
		}
		n = n*10+(c-'0');
	}
		printf("\t.text\n\t"
		".global int_compile\n"
		"int_compile:\n\t"
		"mov $%d, %%eax\n\t"
		"ret\n",n);
}

void str_compile(void) {
	char buffer[MAX_BUFFER_LENGTH];
	int i=0;
	while(1) {
		char c = getc(stdin);
		if(c==EOF)
			error("String Not Terminated");
		else if (c=='"')
			break;
		else if (c=='\\') {
			c=getc(stdin);
			if (c==EOF)
				error("Unterminated \\");						
			}
			buffer[i++]=c;
			if(i==MAX_BUFFER_LENGTH-1)
				error("String Too long to handle");
		}
		buffer[i]='\0';
		printf("\t.data\n"
			".strdata:\n\t"
			".string \"%s\"\n\t"
			".text\n\t"
			".global str_compile\nstr_compile:\n\t"
			"lea .strdata(%%rip), %%rax\n\t"
			"ret\n",buffer);
}

void main_compile(void) {
	int c = getc(stdin);
	if(isdigit(c)) 
		return num_compile(c-'0');
	if(c== '"')
		return str_compile();
	error("Cannot handle '%c'",c);
	
}

int main(int argc, char **argv)
{
	main_compile();
	return 0;
}