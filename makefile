.DEFAULT_GOAL := gbasic

ifeq ($(OS),Windows_NT)
	TARGET = gbasic.exe
    LDLIBS = -lm
else
	TARGET = gbasic
    LDLIBS = -lm -lreadline
endif

testlexer-g.o: testlexer.c gbasic.h
	cc -c ./testlexer.c -o ./testlexer-g.o -g

lexer-g.o: lexer.c gbasic.h
	cc -c ./lexer.c -o ./lexer-g.o -g

parser-g.o : parser.c gbasic.h
	cc -c ./parser.c -o ./parser-g.o -g

eval-g.o : eval.c gbasic.h
	cc -c ./eval.c -o ./eval-g.o -g

testlexer: testlexer-g.o lexer-g.o
	cc ./testlexer-g.o ./lexer-g.o -o testlexer -g

testparser.o: testparser.c gbasic.h
	cc -c ./testparser.c -o testparser.o -g

testparser: testparser.o lexer-g.o parser-g.o
	cc ./testparser.o ./lexer-g.o ./parser-g.o -o testparser -g

testeval-g.o: testeval.c gbasic.h
	cc -c ./testeval.c -o ./testeval.o -g

testeval: testeval-g.o lexer-g.o parser-g.o eval-g.o
	cc ./testeval-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o testeval -g $(LDLIBS)

gbasic-g.o: gbasic.c gbasic.h
	cc -c ./gbasic.c -o ./gbasic-g.o -g

gbasic-debug: gbasic-g.o lexer-g.o parser-g.o eval-g.o
	cc ./gbasic-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o gbasic-debug -g $(LDLIBS)


lexer.o: lexer.c gbasic.h
	cc -c ./lexer.c -o ./lexer.o -O3

parser.o : parser.c gbasic.h
	cc -c ./parser.c -o ./parser.o -O3

eval.o : eval.c gbasic.h
	cc -c ./eval.c -o ./eval.o -O3

gbasic.o: gbasic.c gbasic.h
	cc -c ./gbasic.c -o ./gbasic.o -O3

gbasic: gbasic.o lexer.o parser.o eval.o
	cc ./gbasic.o ./lexer.o ./parser.o ./eval.o -o gbasic -O3 $(LDLIBS)
	strip $(TARGET)

clean:
	rm ./*.o