.DEFAULT_GOAL := gbasic

testlexer.o: testlexer.c gbasic.h keywords.h
	gcc -c ./testlexer.c -o ./testlexer.o -g

lexer.o: lexer.c gbasic.h keywords.h
	gcc -c ./lexer.c -o ./lexer.o -g

parser.o : parser.c gbasic.h keywords.h
	gcc -c ./parser.c -o ./parser.o -g

eval.o : eval.c gbasic.h keywords.h
	gcc -c ./eval.c -o ./eval.o -g

testlexer: testlexer.o lexer.o keywords.h
	gcc ./testlexer.o ./lexer.o -o testlexer -g

testparser.o: testparser.c gbasic.h keywords.h
	gcc -c ./testparser.c -o ./testparser.o -g

testparser: testparser.o lexer.o parser.o keywords.h
	gcc ./testparser.o ./lexer.o ./parser.o -o testparser -g

testeval.o: testeval.c gbasic.h keywords.h
	gcc -c ./testeval.c -o ./testeval.o -g

testeval: testeval.o lexer.o parser.o eval.o keywords.h
	gcc ./testeval.o ./lexer.o ./parser.o ./eval.o -o testeval -g

gbasic.o: gbasic.c gbasic.h keywords.h
	gcc -c ./gbasic.c -o ./gbasic.o -g

gbasic: gbasic.o lexer.o parser.o eval.o keywords.h
	gcc ./gbasic.o ./lexer.o ./parser.o ./eval.o -o gbasic -g

clean:
	rm ./*.o