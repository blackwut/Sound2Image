#include "btrails.h"
#include <assert.h>

typedef struct {
	BPoint pos[MAX_BELEMS];
	size_t top;
	BColor color;
	size_t freq;

	pthread_mutex_t lock;
} Trail;

static Trail trails[MAX_BTRAILS];


int btrails_init()
{
	size_t i;
	size_t j;
	int ret;

	for (i = 0; i < MAX_BTRAILS; ++i) {
		for (j = 0; j < MAX_BELEMS; ++j) {
			trails[i].pos[j].x = 0.0f;
			trails[i].pos[j].y = 0.0f;
		}
		trails[i].top = 0;
		
		ret = pthread_mutex_init(&(trails[i].lock), NULL);
		if (ret != 0) {
			return BTRAILS_ERROR;
		}
		trails[i].color.red = 0;
		trails[i].color.green = 0;
		trails[i].color.blue = 0;
		trails[i].freq = 0;
	}

	return BTRAILS_SUCCESS;
}

size_t btrails_get_top(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].top;
}

BPoint btrails_get_bubble_pos(const size_t trail_id,
							  const size_t bubble_id)
{
	assert(trail_id < MAX_BTRAILS);
	assert(bubble_id < MAX_BELEMS);

	return trails[trail_id].pos[bubble_id];
}

BColor btrails_get_color(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].color;
}

size_t btrails_get_freq(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].freq;
}

void btrails_put_bubble_pos(const size_t trail_id,
							const float x,
							const float y)
{
	size_t next;

	assert(trail_id < MAX_BTRAILS);

	next = btrails_get_top(trail_id);
	trails[trail_id].top = NEXT_BELEM(next);
	trails[trail_id].pos[next].x = x;
	trails[trail_id].pos[next].y = y;
}

void btrails_set_color(const size_t trail_id,
					   const unsigned char red,
					   const unsigned char green,
					   const unsigned char blue)
{
	assert(trail_id < MAX_BTRAILS);

	trails[trail_id].color.red = red;
	trails[trail_id].color.green = green;
	trails[trail_id].color.blue = blue;
}

void btrails_set_freq(const size_t trail_id,
					  const size_t freq)
{
	assert(trail_id < MAX_BTRAILS);
	trails[trail_id].freq = freq;
}

void btrails_lock(const size_t trail_id)
{
	pthread_mutex_lock(&(trails[trail_id].lock));
}

void btrails_unlock(const size_t trail_id)
{
	pthread_mutex_unlock(&(trails[trail_id].lock));
}

void btrails_free()
{
	size_t i;
	for (i = 0; i < MAX_BTRAILS; ++i) {
		pthread_mutex_destroy(&(trails[i].lock));
	}
}
