CC		= gcc
CFLAGS	= --std=c99 -g -Wall -pedantic
INCLUDES= -I.
LIBS	= -lsndfile -lfftw3 -lfftw3f -lm -lallegro -lallegro_main -lallegro_image -lallegro_primitives -lallegro_audio -lallegro_acodec -lpthread
SRCS	= Sound2Image.c fft_audio.c ptask.c bqueue.c
OBJS	= $(SRCS:.c=.o)
MAIN	= Sound2Image

BQUEUE_TEST = bqueue_test


all: $(MAIN) $(BQUEUE_TEST)

$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS) $(CFLAGS)

$(BQUEUE_TEST): $(BQUEUE_TEST).c bqueue.c
	$(CC) -o $@ $^ -I. -lpthread $(CFLAGS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN) $(BQUEUE_TEST)
