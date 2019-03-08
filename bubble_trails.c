#include "bubble_trails.h"

BubbleTrail bubbleTrails[MAX_NUMBER_OF_TRAILS];

void BubbleTrails_init()
{
	size_t i;
	size_t j;

	for (i = 0; i < MAX_NUMBER_OF_TRAILS; ++i) {
		pthread_mutex_init(&(bubbleTrails[i].lock), NULL);

		for (j = 0; j < MAX_BUBBLES_IN_TRAIL; ++j) {
			bubbleTrails[i].bubbles[j].x		= 0.0f;
			bubbleTrails[i].bubbles[j].y		= 0.0f;
			bubbleTrails[i].bubbles[j].radius	= 0.0f;
			bubbleTrails[i].bubbles[j].red		= 0;
			bubbleTrails[i].bubbles[j].green	= 0;
			bubbleTrails[i].bubbles[j].blue		= 0;
			bubbleTrails[i].bubbles[j].freq		= 0.0f;
		}
	}
}

size_t BubbleTrails_top_unsafe(size_t id)
{
	return bubbleTrails[id].top;
}

Bubble BubbleTrails_get_bubble_unsafe(size_t id, size_t pos)
{
	return bubbleTrails[id].bubbles[pos];
}

void BubbleTrails_put_unsafe(size_t id, Bubble bubble)
{
	size_t next;

	next = BubbleTrails_top_unsafe(id);
	bubbleTrails[id].top = NEXT_BUBBLE(next);

	bubbleTrails[id].bubbles[next].x		= bubble.x;
	bubbleTrails[id].bubbles[next].y		= bubble.y;
	bubbleTrails[id].bubbles[next].radius	= bubble.radius;
	bubbleTrails[id].bubbles[next].red		= bubble.red;
	bubbleTrails[id].bubbles[next].green	= bubble.green;
	bubbleTrails[id].bubbles[next].blue		= bubble.blue;
	bubbleTrails[id].bubbles[next].freq		= bubble.freq;
}

void BubbleTrails_lock(size_t id)
{
	pthread_mutex_lock(&(bubbleTrails[id].lock));
}

void BubbleTrails_unlock(size_t id)
{
	pthread_mutex_unlock(&(bubbleTrails[id].lock));
}


size_t BubbleTrails_top(size_t id)
{
	size_t top;

	BubbleTrails_lock(id);
	top = BubbleTrails_top_unsafe(id);
	BubbleTrails_unlock(id);

	return top;
}

Bubble BubbleTrails_get_bubble(size_t id, size_t pos)
{
	Bubble bubble;

	BubbleTrails_lock(id);
	bubble = BubbleTrails_get_bubble_unsafe(id, pos);
	BubbleTrails_unlock(id);

	return bubble;
}

void BubbleTrails_put(size_t id, Bubble bubble)
{
	BubbleTrails_lock(id);
	BubbleTrails_put_unsafe(id, bubble);
	BubbleTrails_unlock(id);
}

void BubbleTrails_free()
{
	size_t i;
	for (i = 0; i < MAX_NUMBER_OF_TRAILS; ++i) {
		pthread_mutex_destroy(&(bubbleTrails[i].lock));
	}
}
