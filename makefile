




.PHONY:clean,default

default: stnc

stnc: stnc.o
	gcc -o stnc stnc.o

stnc.o: stnc.c
	gcc -c stnc.c


clean:
	rm -f *.o stnc