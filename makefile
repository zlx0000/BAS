.DEFAULT_GOAL := gbasic

testlexer-g.o: testlexer.c gbasic.h keywords.h
	gcc -c ./testlexer.c -o ./testlexer-g.o -g

lexer-g.o: lexer.c gbasic.h keywords.h
	gcc -c ./lexer.c -o ./lexer-g.o -g

parser-g.o : parser.c gbasic.h keywords.h
	gcc -c ./parser.c -o ./parser-g.o -g

eval-g.o : eval.c gbasic.h keywords.h
	gcc -c ./eval.c -o ./eval-g.o -g

testlexer: testlexer-g.o lexer-g.o keywords.h
	gcc ./testlexer-g.o ./lexer-g.o -o testlexer -g

testparser.o: testparser.c gbasic.h keywords.h
	gcc -c ./testparser.c -o testparser.o -g

testparser: testparser.o lexer-g.o parser-g.o keywords.h
	gcc ./testparser.o ./lexer-g.o ./parser-g.o -o testparser -g

testeval-g.o: testeval.c gbasic.h keywords.h
	gcc -c ./testeval.c -o ./testeval.o -g

testeval: testeval-g.o lexer-g.o parser-g.o eval-g.o keywords.h
	gcc ./testeval-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o testeval -g -lm

gbasic-g.o: gbasic.c gbasic.h keywords.h
	gcc -c ./gbasic.c -o ./gbasic-g.o -g

gbasic-debug: gbasic-g.o lexer-g.o parser-g.o eval-g.o keywords.h
	gcc ./gbasic-g.o ./lexer-g.o ./parser-g.o ./eval-g.o -o gbasic-debug -g -lm


lexer.o: lexer.c gbasic.h keywords.h
	gcc -c ./lexer.c -o ./lexer.o -O3

parser.o : parser.c gbasic.h keywords.h
	gcc -c ./parser.c -o ./parser.o -O3

eval.o : eval.c gbasic.h keywords.h
	gcc -c ./eval.c -o ./eval.o -O3

gbasic.o: gbasic.c gbasic.h keywords.h
	gcc -c ./gbasic.c -o ./gbasic.o -O3

gbasic: gbasic.o lexer.o parser.o eval.o keywords.h
	gcc ./gbasic.o ./lexer.o ./parser.o ./eval.o -o gbasic -O3 -lm

clean:
	rm ./*.o