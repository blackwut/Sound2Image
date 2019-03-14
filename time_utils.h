#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#define _GNU_SOURCE

#include <stdio.h>
#include <time.h>

#define TIME_MILLISEC	(1000)
#define TIME_MICROSEC	(1000000)
#define TIME_NANOSEC	(1000000000)

#define TIME_SEC_TO_MSEC(t)		((t) * TIME_MILLISEC)
#define TIME_SEC_TO_NSEC(t)		((t) * TIME_NANOSEC)
#define TIME_MSEC_TO_SEC(t)		((t) / TIME_MILLISEC)
#define TIME_MSEC_TO_NSEC(t)	((t) * TIME_MICROSEC)
#define TIME_NSEC_TO_SEC(t)		((t) / TIME_NANOSEC)
#define TIME_NSEC_TO_MSEC(t)	((t) / TIME_MICROSEC)


int time_now(struct timespec * t);

void time_copy(struct timespec * td,
			   const struct timespec ts);

int time_cmp(const struct timespec t1,
			 const struct timespec t2);

void time_add_ms(struct timespec * td,
				 const size_t ms);

void time_diff(struct timespec * td,
			   const struct timespec t1,
			   const struct timespec t2);

int time_nanosleep(const struct timespec req);

size_t time_to_ms(const struct timespec t);

#endif
