CC		= gcc
CFLAGS	= --std=c99 -g -Wall -pedantic
INCLUDES= -I .
LIBS	= -lsndfile -lfftw3 -lm -lallegro -lallegro_main -lallegro_primitives -lpthread
SRCS	= Sound2Image.c
OBJS	= $(SRCS:.c=.o)
MAIN	= Sound2Image


all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS) $(CFLAGS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN)
