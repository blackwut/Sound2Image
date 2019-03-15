#include "time_utils.h"


//------------------------------------------------------------------------------
//
// This function fill the provided time structure with the current time.
//
//------------------------------------------------------------------------------
int time_now(struct timespec * t)
{
	return clock_gettime(CLOCK_MONOTONIC, t);
}


//------------------------------------------------------------------------------
//
// This function copies the time structure "ts" values into "td".
//
//------------------------------------------------------------------------------
void time_copy(struct timespec * td,
			   const struct timespec ts)
{
	td->tv_sec  = ts.tv_sec;
	td->tv_nsec = ts.tv_nsec;
}


//------------------------------------------------------------------------------
//
// This function compares two time structures.
// If t1 > t2  it returns  1
// If t1 == t2 it returns  0
// If t1 < t2  it returns -1
//
//------------------------------------------------------------------------------
int time_cmp(const struct timespec t1,
			 const struct timespec t2)
{
	if (t1.tv_sec > t2.tv_sec) return 1;
	if (t1.tv_sec < t2.tv_sec) return -1;
	if (t1.tv_nsec > t2.tv_nsec) return 1;
	if (t1.tv_nsec < t2.tv_nsec) return -1;
	return 0;
}


//------------------------------------------------------------------------------
//
// This functions add milliseconds to the provided time structure.
//
//------------------------------------------------------------------------------
void time_add_ms(struct timespec * td,
				 const size_t ms)
{
	td->tv_sec += TIME_MSEC_TO_SEC(ms);
	td->tv_nsec += TIME_MSEC_TO_NSEC(ms % TIME_MILLISEC);
	if (td->tv_nsec > TIME_NANOSEC) {
		td->tv_nsec -= TIME_NANOSEC;
		td->tv_sec += 1;
	}
}


//------------------------------------------------------------------------------
//
// This function return a time structure containing the difference between two
// time structures.
//
//------------------------------------------------------------------------------
void time_diff(struct timespec * td,
			   const struct timespec t1,
			   const struct timespec t2)
{
	td->tv_sec = t2.tv_sec - t1.tv_sec;
	td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
	if (td->tv_sec < 0) {
		td->tv_sec = 0;
		td->tv_nsec = 0;
	} else if (td->tv_nsec < 0) {
		if (td->tv_sec == 0) {
			td->tv_sec = 0;
			td->tv_nsec = 0;
		} else {
			td->tv_sec = td->tv_sec - 1;
			td->tv_nsec = td->tv_nsec + TIME_NANOSEC;
		}
	}
}


//------------------------------------------------------------------------------
//
// This is a wrapper function that allows to suspend the calling thread for the
// amount of time contained in the time structure.
// Since "clock_nanosleep" POSIX function is not provided under Darwin based OS,
// it is emulated under such OSes.
//
//------------------------------------------------------------------------------
int time_nanosleep(const struct timespec req)
{
/* emulate clock_nanosleep for CLOCK_MONOTONIC and TIMER_ABSTIME */
#ifdef __MACH__
	struct timespec now;
	struct timespec diff;
	int retval = time_now(&now);
	if (retval == 0) {
		time_diff(&diff, now, req);
		retval = nanosleep(&diff, NULL);
	}
	return retval;
#else /* POSIX */
	return clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &req, NULL);
#endif
}


//------------------------------------------------------------------------------
//
// This function converts the time structure into milliseconds.
//
//------------------------------------------------------------------------------
size_t time_to_ms(const struct timespec t) {
	size_t ns = TIME_SEC_TO_NSEC(t.tv_sec) + t.tv_nsec;
	return TIME_NSEC_TO_MSEC(ns);
}
