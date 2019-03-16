#include "btrails.h"
#include <pthread.h>
#include <assert.h>


//------------------------------------------------------------------------------
// BTRAILS LOCAL STRUCT DEFINITIONS
//------------------------------------------------------------------------------
typedef struct {
	btrail_point pos[MAX_BELEMS];	// (x, y) bubble coordinates of the trail
	size_t top;						// Last bubble coordinates index enqueued
	size_t freq;					// Frequency assigned to the trail
	btrail_color color;				// RGB color of the bubbles in the trail
	pthread_mutex_t lock;			// Mutex to protect the trail
} btrail;


//------------------------------------------------------------------------------
// BRAILS LOCAL DATA
//------------------------------------------------------------------------------
static btrail trails[MAX_BTRAILS];


//------------------------------------------------------------------------------
//
// This function initializes all data required to zero and initializes also the
// mutex associated to each trail.
//
//------------------------------------------------------------------------------
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


//------------------------------------------------------------------------------
//
// This function returns a "btrail_point" containing the coordinates (x, y) of the 
// "bubble_id" bubble in the specified "trail_id" trail.
//
//------------------------------------------------------------------------------
btrail_point btrails_get_bubble_pos(const size_t trail_id,
									const size_t bubble_id)
{
	assert(trail_id < MAX_BTRAILS);
	assert(bubble_id < MAX_BELEMS);
	return trails[trail_id].pos[bubble_id];
}


//------------------------------------------------------------------------------
//
// This function returns the index of the last bubble enqueued into the
// specified "trail_id" trail.
//
//------------------------------------------------------------------------------
size_t btrails_get_top(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].top;
}


//------------------------------------------------------------------------------
//
// This function returns the frequency assigned to the "trail_id" trail.
//
//------------------------------------------------------------------------------
size_t btrails_get_freq(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].freq;
}


//------------------------------------------------------------------------------
//
// This function returns a "btrail_color" containing the red, green and blue color
// channels of a the specified "trail_id" trail.
//
//------------------------------------------------------------------------------
btrail_color btrails_get_color(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	return trails[trail_id].color;
}


//------------------------------------------------------------------------------
//
// This function enqueue (possibly overwriting) the bubble coordinates (x, y)
// into the "trail_id" trail.
//
//------------------------------------------------------------------------------
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


//------------------------------------------------------------------------------
//
// This function assigns the frequency to the "trail_id" trail
//
//------------------------------------------------------------------------------
void btrails_set_freq(const size_t trail_id,
					  const size_t freq)
{
	assert(trail_id < MAX_BTRAILS);
	trails[trail_id].freq = freq;
}


//------------------------------------------------------------------------------
//
// This function assigns the red, green and blue color channels to the
// "trail_id" trail.
//
//------------------------------------------------------------------------------
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


//------------------------------------------------------------------------------
//
// This function allows to acquire the lock on the specified "trail_id" trail.
//
//------------------------------------------------------------------------------
void btrails_lock(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	pthread_mutex_lock(&(trails[trail_id].lock));
}


//------------------------------------------------------------------------------
//
// This function allows to release the lock on the specified "trail_id" trail.
//
//------------------------------------------------------------------------------

void btrails_unlock(const size_t trail_id)
{
	assert(trail_id < MAX_BTRAILS);
	pthread_mutex_unlock(&(trails[trail_id].lock));
}


//------------------------------------------------------------------------------
//
// This function frees all data and data structures used.
//
//------------------------------------------------------------------------------
void btrails_free()
{
	size_t i;

	for (i = 0; i < MAX_BTRAILS; ++i) {
		pthread_mutex_destroy(&(trails[i].lock));
	}
}
