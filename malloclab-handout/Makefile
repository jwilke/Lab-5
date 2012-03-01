#
# Students' Makefile for the Malloc Lab
#
TURNIN := /lusr/bin/turnin
GRADER := ctlight
LAB_NAME := handin-439-malloclab

CC = gcc
CFLAGS = -Wall -O2 -m32

OBJS = unit.o mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o


unit: unit.o
	$(CC) $(CFLAGS) -o unit memlib.c mm.c unit.c

mdriver: $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h
unit.o: unit.c

handin:
	echo "FIX MAKEFILE TO INCLUDE THE TA user ID for the cs machines"
	echo "Turning in mm.c"
	$(TURNIN) --submit $(GRADER) $(LAB_NAME) mm.c


clean:
	rm -f *~ *.o mdriver


