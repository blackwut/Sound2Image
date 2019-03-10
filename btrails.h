#ifndef BTRAILS_H
#define BTRAILS_H

#include <stdio.h>
#include <pthread.h>

#define BTRAILS_SUCCESS 0
#define BTRAILS_ERROR 1

#define MAX_BTRAILS	32
#define MAX_BELEMS	8

#define NEXT_BELEM(i) (((i) + 1) % MAX_BELEMS)

typedef struct {
	float x;
	float y;
} BPoint;

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} BColor;

int btrails_init();
size_t btrails_get_top(const size_t trail_id);
BPoint btrails_get_bubble_pos(const size_t trail_id,
							  const size_t bubble_id);
BColor btrails_get_color(const size_t trail_id);
size_t btrails_get_freq(const size_t trail_id);
void btrails_put_bubble_pos(const size_t trail_id,
							const float x,
							const float y);
void btrails_set_color(const size_t trail_id,
					   const unsigned char red,
					   const unsigned char green,
					   const unsigned char blue);
void btrails_set_freq(const size_t trail_id,
					  const size_t freq);
void btrails_lock(const size_t trail_id);
void btrails_unlock(const size_t trail_id);
void btrails_free();

#endif
