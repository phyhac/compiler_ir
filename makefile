target: parser

parser: main.c lex.yy.c syntax.tab.c symbol.c tree.c
	gcc main.c syntax.tab.c symbol.c tree.c -lfl -o parser

lex.yy.c: lexical.l
	flex lexical.l

syntax.tab.c: syntax.y
	bison -t syntax.y

clean:
	rm parser lex.yy.c syntax.tab.c syntax.tab.h syntax.output a.out

debug:
	bison -d syntax.y -v
	vim syntax.output

test: ir_tests/*
	# ./parser ir_tests/test01.cmm
	./parser ir_tests/test05.cmm
	# ./parser ir_tests/test06.cmm
	# ./parser ir_tests/test07.cmm
	# ./parser ir_tests/test08.cmm


test_lex: lex_tests/*
	./parser lex_tests/test01.cmm
	./parser lex_tests/test02.cmm
	./parser lex_tests/test03.cmm
	./parser lex_tests/test04.cmm
	./parser lex_tests/test05.cmm
	./parser lex_tests/test06.cmm
	./parser lex_tests/test07.cmm
	./parser lex_tests/test08.cmm
	./parser lex_tests/test09.cmm
	./parser lex_tests/test10.cmm


test_syntax: syntax_tests/*
# ./parser tests/test00.cmm

	./parser syntax_tests/test01.cmm
	./parser syntax_tests/test02.cmm
	./parser syntax_tests/test03.cmm
	./parser syntax_tests/test04.cmm
	./parser syntax_tests/test05.cmm
	./parser syntax_tests/test06.cmm
	./parser syntax_tests/test07.cmm
	./parser syntax_tests/test08.cmm
	./parser syntax_tests/test09.cmm
	./parser syntax_tests/test10.cmm
	./parser syntax_tests/test11.cmm
	./parser syntax_tests/test12.cmm
	./parser syntax_tests/test13.cmm
	./parser syntax_tests/test14.cmm
	./parser syntax_tests/test15.cmm
	./parser syntax_tests/test16.cmm
	./parser syntax_tests/test17.cmm

	# ./parser syntax_tests/test18.cmm
	# ./parser syntax_tests/test19.cmm
	# ./parser syntax_tests/test20.cmm
	# ./parser syntax_tests/test21.cmm
	# ./parser syntax_tests/test22.cmm
	# ./parser syntax_tests/test23.cmm
