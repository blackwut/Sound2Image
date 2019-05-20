//------------------------------------------------------------------------------
//
// BTRAILS
//
// MODULE TO SIMPLIFY THE TRAILS OF BUBBLES.
//
// This module provides a simple way to manage the trails of Bubbles
// implementing an array of circular buffers.
// All functions are thread UNSAFE, but each operation on a trail can be thread
// safe calling lock/unlock provided functions.
//
//------------------------------------------------------------------------------
#ifndef BTRAILS_H
#define BTRAILS_H


#include <stdio.h>


//------------------------------------------------------------------------------
// BTRAILS GLOBAL CONSTANTS
//------------------------------------------------------------------------------
#define BTRAILS_SUCCESS		0
#define BTRAILS_ERROR		1

#define MAX_BTRAILS			32
#define MAX_BELEMS			8

// Bubble (x, y) offscreen
#define BUBBLE_X_OFFSCREEN			-100.0f	// x point of offscreen bubble
#define BUBBLE_Y_OFFSCREEN			-100.0f	// y point of offscreen bubble


//------------------------------------------------------------------------------
// BRAILS GLOBAL MACROS
//------------------------------------------------------------------------------
#define NEXT_BELEM(i) (((i) + 1) % MAX_BELEMS)


//------------------------------------------------------------------------------
// BRAILS GLOBAL STRUCT DEFINITIONS
//------------------------------------------------------------------------------
typedef struct {
	float x;
	float y;
} btrail_point;

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} btrail_color;


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function initializes all data required.
//
// RETURN
// If not all data required can be initialized, this function returns
// BTRAILS_ERROR.
// Otherwise it returns BTRAILS_SUCCESS.
//
//------------------------------------------------------------------------------
int btrails_init();


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns a "btrail_point" containing the coordinates (x, y) of
// the "bubble_id" bubble in the specified "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
// bubble_id: the id of the bubble in the specified trail
//
// RETURN
// The "btrail_point" of the specified bubble in the trail
//
//------------------------------------------------------------------------------
btrail_point btrails_get_bubble_pos(const size_t trail_id,
									const size_t bubble_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the index of the last bubble enqueued into the
// specified "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
//
// RETURN
// The "top" value
//
//------------------------------------------------------------------------------
size_t btrails_get_top(const size_t trail_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns the frequency assigned to the "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
//
// RETURN
// The frequency assigned to the specified trail
//
//------------------------------------------------------------------------------
size_t btrails_get_freq(const size_t trail_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function returns a "btrail_color" containing the red, green and blue
// color channels of a the specified "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
//
// RETURN
// The "btrail_color" of the specified trail
//
//------------------------------------------------------------------------------
btrail_color btrails_get_color(const size_t trail_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function enqueue (possibly overwriting) the bubble coordinates (x, y)
// into the "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
// x: the x coordinates
// y: the y coordinates
//
//------------------------------------------------------------------------------
void btrails_put_bubble_pos(const size_t trail_id,
							const float x,
							const float y);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function assigns the frequency to the "trail_id" trail
//
// PARAMETERS
// trail_id: the id of the trail
// freq: the frequency value to be assigned
//
//------------------------------------------------------------------------------
void btrails_set_freq(const size_t trail_id,
					  const size_t freq);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function assigns the red, green and blue color channels to the
// "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
// red: the red channel of the color
// green: the green channel of the color
// blue: the blue channel of the color
//
//------------------------------------------------------------------------------
void btrails_set_color(const size_t trail_id,
					   const unsigned char red,
					   const unsigned char green,
					   const unsigned char blue);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function allows to acquire the lock on the specified "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
//
//------------------------------------------------------------------------------
void btrails_lock(const size_t trail_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function allows to release the lock on the specified "trail_id" trail.
//
// PARAMETERS
// trail_id: the id of the trail
//
//------------------------------------------------------------------------------
void btrails_unlock(const size_t trail_id);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function frees all data and data structures used.
//
//------------------------------------------------------------------------------
void btrails_free();


#endif
