CFLAGS=-Wall -std=gnu99 -g
OBJS=lexer.o string.o util.o

$(OBJS) unittest.o main.o: nc.h

nc: nc.h main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ main.o $(OBJS)

unittest: nc.h unittest.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ unittest.o $(OBJS)

test: unittest
	./unittest
	./testing.sh

clean:
	rm -f nc *.o tmp.* *.s
