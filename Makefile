CC       = gcc
CFLAGS   = --std=c99 -g -Wall -pedantic
INCLUDES = -I .
LIBS	= -lsndfile -lfftw3 -lm `pkg-config --libs allegro-5` -lpthread
SRCS	= example.c ptask.c
OBJS	= $(SRCS:.c=.o)
MAIN	= example


all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS) $(CFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@


.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN)

