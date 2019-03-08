#include "bubble_trails.h"

BubbleTrail bubbleTrails[MAX_NUMBER_OF_TRAILS];

void BubbleTrails_init()
{
	size_t i;
	size_t j;
	BubbleTrail * bt;
	Bubble * b;

	for (i = 0; i < MAX_NUMBER_OF_TRAILS; ++i) {
		bt = &(bubbleTrails[i]); //TODO: check if *bt = &(bubbleTrails[i]); bt.start = 0; works (same for b)
		bt->top		= 0;
		pthread_mutex_init(&(bt->lock), NULL);

		for (j = 0; j < MAX_BUBBLES_IN_TRAIL; ++j) {
			b = &(bubbleTrails[i].bubbles[j]);
			b->x		= 0.0f;
			b->y		= 0.0f;
			b->radius	= 0.0f;
			b->red		= 0;
			b->green	= 0;
			b->blue		= 0;
			b->freq		= 0.0f;
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
	BubbleTrail * bt;
	Bubble * b;

	bt = &(bubbleTrails[id]);

	next = NEXT_BUBBLE(bt->top);
	bt->top = next;

	b = &(bt->bubbles[next]);
	b->x		= bubble.x;
	b->y		= bubble.y;
	b->radius	= bubble.radius;
	b->red		= bubble.red;
	b->green	= bubble.green;
	b->blue		= bubble.blue;
	b->freq		= bubble.freq;
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
