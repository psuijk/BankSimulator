all: lab4 lab4.zip

lab4.zip: lab4.c Makefile writerFile1 writerFile2 README
	zip $@ $^

lab4: lab4.c
	gcc -o lab4 lab4.c -std=c99 -pthread

clean:
	rm -rf *o lab4 lab4.zip
