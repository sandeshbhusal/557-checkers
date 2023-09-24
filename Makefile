CC              := gcc
CFLAGS          := -Wall
CPPFLAGS        := -I./ -I/usr/X11R6/include/Xm -I/usr/X11R6/include -I/opt/homebrew/Cellar/openmotif/2.3.8_2/include -I/opt/homebrew/include
LDFLAGS         := -L/usr/lib/X11R6 -lXm -lXaw -lXmu -lXt -lX11 -lpthread
#LDFLAGS         := -L/usr/X11R6/lib -L /usr/X11R6/LessTif/Motif1.2/lib -L /opt/homebrew/lib -lXm -lXaw -lXmu -lXt -lX11 -lICE -lSM

# Uncomment this next line if you'd like to compile the graphical version of
# the checkers server.
#CFLAGS          += -DGRAPHICS

all: checkers computer
checkers: graphics.o
#computer: myprog.o
computer: player.o playerHelper.o
	${CC} ${CPPFLAGS} ${CFLAGS} -o $@ $^


.PHONY: clean
clean:	
	@-rm checkers computer *.o
