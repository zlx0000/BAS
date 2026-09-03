.DEFAULT_GOAL := bas

ifeq ($(OS),Windows_NT)
	CC = gcc
	CLEAN = del /q *.o
	TARGET = bas.exe
    LDLIBS = -lm
else
	CC = cc
	DUBUG_FLAGS = -fsanitize=address -fno-omit-frame-pointer -DDEBUG
	CLEAN = rm ./*.o
	TARGET = bas
    LDLIBS = -lm -lreadline
endif

testlexer-g.o: testlexer.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./testlexer.c -o ./testlexer-g.o -g

lexer-g.o: lexer.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./lexer.c -o ./lexer-g.o -g

parser-g.o : parser.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./parser.c -o ./parser-g.o -g

eval-g.o : eval.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./eval.c -o ./eval-g.o -g

testlexer: testlexer-g.o lexer-g.o
	$(CC) $(DUBUG_FLAGS) ./testlexer-g.o ./lexer-g.o -o testlexer -g

testparser.o: testparser.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./testparser.c -o testparser.o -g

testparser: testparser.o lexer-g.o parser-g.o
	$(CC) $(DUBUG_FLAGS) ./testparser.o ./lexer-g.o ./parser-g.o -o testparser -g

bas-g.o: bas.c bas.h
	$(CC) $(DUBUG_FLAGS) -c ./bas.c -o ./bas-g.o -g

debug: bas-g.o lexer-g.o parser-g.o eval-g.o
	$(CC) $(DUBUG_FLAGS) ./bas-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o bas-debug -g $(LDLIBS)


lexer.o: lexer.c bas.h
	$(CC) -c ./lexer.c -o ./lexer.o -O3

parser.o : parser.c bas.h
	$(CC) -c ./parser.c -o ./parser.o -O3

eval.o : eval.c bas.h
	$(CC) -c ./eval.c -o ./eval.o -O3

bas.o: bas.c bas.h
	$(CC) -c ./bas.c -o ./bas.o -O3

bas: bas.o lexer.o parser.o eval.o
	$(CC) ./bas.o ./lexer.o ./parser.o ./eval.o -o bas -O3 $(LDLIBS)
	strip $(TARGET)

clean:
	$(CLEAN)