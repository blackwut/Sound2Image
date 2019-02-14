CC		= gcc
CFLAGS	= --std=c99 -g -Wall -pedantic #-fno-omit-frame-pointer -fsanitize=address
INCLUDES= -I.
LIBS	= -lsndfile -lfftw3 -lfftw3f -lm -lallegro -lallegro_main -lallegro_font -lallegro_ttf -lallegro_primitives -lallegro_audio -lallegro_acodec -lpthread
SRCS	= Sound2Image.c fft_audio.c ptask.c
OBJS	= $(SRCS:.c=.o)
MAIN	= Sound2Image


all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(INCLUDES) $(LIBS) $(CFLAGS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN) $(BQUEUE_TEST)
