//------------------------------------------------------------------------------
//
// TIME_UTILS
//
// LIBRARY TO SIMPLIFY THE MANAGEMENT OF TIME
//
//------------------------------------------------------------------------------
#ifndef TIME_UTILS_H
#define TIME_UTILS_H


#include <stdlib.h>
#include <time.h>


//------------------------------------------------------------------------------
// TIME_UTIL PREPROCESSOR DIRECTIVES
//------------------------------------------------------------------------------
#define _GNU_SOURCE


//------------------------------------------------------------------------------
// TIME_UTIL GLOBAL CONSTANTS
//------------------------------------------------------------------------------
#define TIME_MILLISEC	(1000)
#define TIME_MICROSEC	(1000000)
#define TIME_NANOSEC	(1000000000)


//------------------------------------------------------------------------------
// TIME_UTIL GLOBAL MACROS
//------------------------------------------------------------------------------
#define TIME_SEC_TO_MSEC(t)		((t) * TIME_MILLISEC)
#define TIME_SEC_TO_NSEC(t)		((t) * TIME_NANOSEC)
#define TIME_MSEC_TO_SEC(t)		((t) / TIME_MILLISEC)
#define TIME_MSEC_TO_NSEC(t)	((t) * TIME_MICROSEC)
#define TIME_NSEC_TO_SEC(t)		((t) / TIME_NANOSEC)
#define TIME_NSEC_TO_MSEC(t)	((t) / TIME_MICROSEC)


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function fills the provided time structure with the current time.
//
// PARAMETERS
// t: time structure to be filled
//
// RETURN
// It returns -1 if an error occurred, otherwise 0.
//
//------------------------------------------------------------------------------
int time_now(struct timespec * t);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function copies the time structure "ts" values into "td".
//
// PARAMETERS
// td: the destination time structure
// ts: the source time structure
//
//------------------------------------------------------------------------------
void time_copy(struct timespec * td,
			   const struct timespec ts);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function compares two time structures.
//
// PARAMETERS
// t1: the first time structure
// t2: the second time structure
//
// RETURN
// It returns an integer value according to:
// - If t1 > t2  it returns  1
// - If t1 == t2 it returns  0
// - If t1 < t2  it returns -1
//
//------------------------------------------------------------------------------
int time_cmp(const struct timespec t1,
			 const struct timespec t2);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This functions add milliseconds to the provided time structure.
//
// PARAMETERS
// td: the time structure to which to add time
// ms: the amount of milliseconds to add
//
//------------------------------------------------------------------------------
void time_add_ms(struct timespec * td,
				 const size_t ms);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function return a time structure containing the difference between two
// time structures.
//
// PARAMETERS
// td: the time structure containing the difference
// t1: the first time structure
// t2: the second time structure
//
//------------------------------------------------------------------------------
void time_diff(struct timespec * td,
			   const struct timespec t1,
			   const struct timespec t2);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function suspends the calling thread for the time contained in the time
// structure.
//
// PARAMETERS
// req: the time structure containing the suspending amount of time
//
// RETURN
// It returns -1 if an error occurred, otherwise 0.
//
//------------------------------------------------------------------------------
int time_nanosleep(const struct timespec req);


//------------------------------------------------------------------------------
//
// DESCRIPTION
// This function converts the time structure into milliseconds.
//
// PARAMETERS
// t: the time structure to be converted
//
// RETURN
// It returns the amount of time contained in the time structures expressed in
// milliseconds
//
//------------------------------------------------------------------------------
size_t time_to_ms(const struct timespec t);


#endif
