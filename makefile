target: parser

parser: main.c lex.yy.c syntax.tab.c symbol.c tree.c
	gcc main.c syntax.tab.c symbol.c tree.c -lfl -o parser

lex.yy.c: lexical.l
	flex lexical.l

syntax.tab.c: syntax.y
	bison -t syntax.y

clean:
	rm parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output

debug:
	bison -d syntax.y -v
	vim syntax.output

test: tests/*
# ./parser tests/test00.cmm

	./parser tests/test01.cmm
	./parser tests/test02.cmm
	./parser tests/test03.cmm
	./parser tests/test04.cmm
	./parser tests/test05.cmm
	./parser tests/test06.cmm
	./parser tests/test07.cmm
	./parser tests/test08.cmm
	./parser tests/test09.cmm
	./parser tests/test10.cmm
	./parser tests/test11.cmm
	./parser tests/test12.cmm
	./parser tests/test13.cmm
	./parser tests/test14.cmm
	./parser tests/test15.cmm
	./parser tests/test16.cmm
	./parser tests/test17.cmm

	# ./parser tests/test18.cmm
	# ./parser tests/test19.cmm
	# ./parser tests/test20.cmm
	# ./parser tests/test21.cmm
	# ./parser tests/test22.cmm
	# ./parser tests/test23.cmm
