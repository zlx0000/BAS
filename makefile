.DEFAULT_GOAL := gbasic

ifeq ($(OS),Windows_NT)
	CC = gcc
	CLEAN = del /q *.o
	TARGET = gbasic.exe
    LDLIBS = -lm
else
	CC = cc
	CLEAN = rm ./*.o
	TARGET = gbasic
    LDLIBS = -lm -lreadline
endif

testlexer-g.o: testlexer.c gbasic.h
	$(CC) -c ./testlexer.c -o ./testlexer-g.o -g

lexer-g.o: lexer.c gbasic.h
	$(CC) -c ./lexer.c -o ./lexer-g.o -g

parser-g.o : parser.c gbasic.h
	$(CC) -c ./parser.c -o ./parser-g.o -g

eval-g.o : eval.c gbasic.h
	$(CC) -c ./eval.c -o ./eval-g.o -g

testlexer: testlexer-g.o lexer-g.o
	$(CC) ./testlexer-g.o ./lexer-g.o -o testlexer -g

testparser.o: testparser.c gbasic.h
	$(CC) -c ./testparser.c -o testparser.o -g

testparser: testparser.o lexer-g.o parser-g.o
	$(CC) ./testparser.o ./lexer-g.o ./parser-g.o -o testparser -g

testeval-g.o: testeval.c gbasic.h
	$(CC) -c ./testeval.c -o ./testeval.o -g

testeval: testeval-g.o lexer-g.o parser-g.o eval-g.o
	$(CC) ./testeval-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o testeval -g $(LDLIBS)

gbasic-g.o: gbasic.c gbasic.h
	$(CC) -c ./gbasic.c -o ./gbasic-g.o -g

gbasic-debug: gbasic-g.o lexer-g.o parser-g.o eval-g.o
	$(CC) ./gbasic-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o gbasic-debug -g $(LDLIBS)


lexer.o: lexer.c gbasic.h
	$(CC) -c ./lexer.c -o ./lexer.o -O3

parser.o : parser.c gbasic.h
	$(CC) -c ./parser.c -o ./parser.o -O3

eval.o : eval.c gbasic.h
	$(CC) -c ./eval.c -o ./eval.o -O3

gbasic.o: gbasic.c gbasic.h
	$(CC) -c ./gbasic.c -o ./gbasic.o -O3

gbasic: gbasic.o lexer.o parser.o eval.o
	$(CC) ./gbasic.o ./lexer.o ./parser.o ./eval.o -o gbasic -O3 $(LDLIBS)
	strip $(TARGET)

clean:
	$(CLEAN)