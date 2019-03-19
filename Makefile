UNAME_S := $(shell uname -s)

CC	= gcc

ifeq ($(UNAME_S), Darwin)
	CFLAGS = --std=c99 -g -Wall -pedantic
else 
	CFLAGS = --std=gnu99 -g -Wall -pedantic
endif

LIBS	= -lsndfile \
		-lfftw3 -lfftw3f -lm \
		-lallegro -lallegro_main -lallegro_audio -lallegro_acodec \
		-lallegro_primitives -lallegro_font -lallegro_ttf \
		-lpthread

SRCS	= Sound2Image.c time_utils.c fft_audio.c ptask.c btrails.c
OBJS	= $(SRCS:.c=.o)
MAIN	= Sound2Image


all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) -o $@ $^ $(LIBS) $(CFLAGS)

.c.o:
	$(CC) -c $< -o $@ $(CFLAGS)


.PHONY: clean
clean:
	$(RM) *.o *~ $(MAIN) $(BQUEUE_TEST)
