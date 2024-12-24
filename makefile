build: main.c tokenizer.c ast_builder.c cvm.c
	clang -Wno-multichar -o main main.c tokenizer.c ast_builder.c cvm.c

run: main main.c tokenizer.c ast_builder.c cvm.c
	./main ./code.txt ./input.txt ./output.txt
