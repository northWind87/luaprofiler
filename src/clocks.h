/*
** LuaProfiler
** Copyright Kepler Project 2005-2007 (http://www.keplerproject.org/luaprofiler)
** $Id: clocks.h,v 1.4 2007-08-22 19:23:53 carregal Exp $
*/

/*****************************************************************************
clocks.h:
   Module to register the time (seconds) between two events

Design:
   'lprofC_start_timer()' marks the first event
   'lprofC_get_seconds()' gives you the seconds elapsed since the timer
                          was started
*****************************************************************************/

#include "lpfloat.h"
#ifdef PERF_COUNTER
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>
#else
#include <time.h>
#endif

#ifdef PERF_COUNTER
void lprofC_start_timer(int64_t *time_marker);
LPFLOAT lprofC_get_seconds(int64_t time_marker);
#else
void lprofC_start_timer(clock_t *time_marker);
LPFLOAT lprofC_get_seconds(clock_t time_marker);
#endif
