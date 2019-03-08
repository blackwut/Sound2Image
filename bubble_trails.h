#ifndef BUBBLE_TRAILS_H
#define BUBBLE_TRAILS_H

#include <stdio.h>
#include <pthread.h>

#define MAX_NUMBER_OF_TRAILS	32
#define MAX_BUBBLES_IN_TRAIL	8

#define NEXT_BUBBLE(i) (((i) + 1) % MAX_BUBBLES_IN_TRAIL)

typedef struct {
	float x;
	float y;
	float radius;

	unsigned char red;
	unsigned char green;
	unsigned char blue;

	size_t freq;
} Bubble;


typedef struct {
	Bubble bubbles[MAX_BUBBLES_IN_TRAIL];
	size_t top;
	pthread_mutex_t lock;
} BubbleTrail;

void BubbleTrails_init();

// Thread Unsafe
size_t BubbleTrails_top_unsafe(size_t id);
Bubble BubbleTrails_get_bubble_unsafe(size_t id, size_t pos);
void BubbleTrails_put_unsafe(size_t id, Bubble bubble);

// Thread Safe
void BubbleTrails_lock(size_t id);
void BubbleTrails_unlock(size_t id);

size_t BubbleTrails_top(size_t id);
Bubble BubbleTrails_get_bubble(size_t id, size_t pos);
void BubbleTrails_put(size_t id, Bubble bubble);

void BubbleTrails_free();

#endif
