mkparser: parser.o generator-c.o	
	gcc *.o -o mkparser
parser.o: parser.c
	gcc -c parser.c -Wno-implicit-function-declaration

generator-c.o: generator-c.c
	gcc -c generator-c.c -Wno-implicit-function-declaration

.PHONY: clean
clean:
	rm *.o mkparser
