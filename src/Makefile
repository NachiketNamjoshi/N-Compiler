CFLAGS=-Wall -std=gnu99 -g
OBJS=main.o lexer.o string.o

nc: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

clean:
	rm -f nc *.o tmp.* *.s