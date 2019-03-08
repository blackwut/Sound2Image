UNAME_S := $(shell uname -s)

CC		= gcc

ifeq ($(UNAME_S), Darwin)
	CFLAGS = --std=c99 -g -Wall -pedantic #-fno-omit-frame-pointer -fsanitize=address
else 
	CFLAGS = --std=gnu99 -g -Wall -pedantic
endif

INCLUDES= -I.
LIBS	= -lsndfile -lfftw3 -lfftw3f -lm -lallegro -lallegro_main -lallegro_font -lallegro_ttf -lallegro_primitives -lallegro_audio -lallegro_acodec -lpthread
SRCS	= Sound2Image.c time_utils.c fft_audio.c ptask.c bubble_trails.c
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
