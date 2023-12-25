/* timer.c -- Subroutines for (system-) timed events */

#include "copyright.h"
#include "autoconf.h"

#include <signal.h>

#include <math.h>

#include "mudconf.h"
#include "config.h"
#include "db.h"
#include "interface.h"
#include "match.h"
#include "externs.h"
#include "command.h"
#include "rwho_clilib.h"
#include "local.h"

/******************************************************************************
 * Mac OSX does not have clock_gettime, so this is their method of reproducing
 * it.  This will likely have to occur for any other systems also missing this
 * functionality, so define below when required for each arch-type
 ******************************************************************************/
#ifdef __MACH__
#ifndef MACH_TIMER
#include <sys/time.h>
#define CLOCK_REALTIME  0
#define CLOCK_MONOTONIC 0
/* clock_gettime mac OSX implementation */
int clock_gettime(int clk_id, struct timespec* t) {
   struct timeval now;
   int rv = gettimeofday(&now, NULL);
   if (rv)
      return rv;
   t->tv_sec  = now.tv_sec;
   t->tv_nsec = now.tv_usec * 1000;
   return 0;
}
#endif
#endif

/*#include <sys/resource.h>*/

extern void		NDECL(do_second);
extern void		FDECL(fork_and_dump, (int key, char *msg));
extern void		FDECL(do_recache_vattrs, (dbref, int, char *, int));
//extern unsigned int	FDECL(alarm, (unsigned int seconds));
extern void		NDECL(pcache_trim);

/* Version of alarm() that works with floating point numbers, and
 * values larger than one second, combining functionality of alarm()
 * and ualarm() into one.
 * --Ambrosia
*/
int alarm_msec(double time)
{
struct itimerval it_val;
double time_remainder, time_usec, time_sec;
double i_rounder;
  // This function will _never_ stop the timer; use alarm_stop() for that.
  i_rounder = 0.1;
  if ( mudconf.mtimer != 0 )
     i_rounder = 1.0 / (double)mudconf.mtimer;

  time_remainder = fmod(time,1);
  time_sec = time - time_remainder;
  time_usec = roundf(1000000 * time_remainder * mudconf.mtimer) / mudconf.mtimer;

/*
  time = (time <= 0 ? i_rounder : time );
  time_rounded = roundf(time * (double)mudconf.mtimer) / (double)mudconf.mtimer; // Round to specified decimal place
  it_val.it_value.tv_sec = floor(time_rounded); // Second
  if ( mudconf.mtimer == 1 ) {
     it_val.it_value.tv_usec = 0;
  } else {
     it_val.it_value.tv_usec = floor(1000000 * fmod(time_rounded,((double)mudconf.mtimer / 10.0))); // Decimal
  }
*/

  it_val.it_value.tv_sec = (time_t) time_sec;
  it_val.it_value.tv_usec = (suseconds_t) time_usec;
  it_val.it_interval.tv_sec = 0; // both set to '0' so the timer is one-shot
  it_val.it_interval.tv_usec = 0;
/* Debugging
  fprintf(stderr, "Time Triggered -- Value: %ld/%ld      Value2: %f/%f/%f\n", it_val.it_value.tv_sec, it_val.it_value.tv_usec, time_sec, time_usec, time);
 */
  mudstate.alarm_triggered = 0;
  return setitimer(ITIMER_REAL,&it_val,NULL);
}

int alarm_stop()
{
struct itimerval it_val;
  it_val.it_value.tv_sec = 0;
  it_val.it_value.tv_usec = 0;
  it_val.it_interval.tv_sec = 0; // both set to '0' so the timer is one-shot
  it_val.it_interval.tv_usec = 0;

  mudstate.alarm_triggered = 0;
  return setitimer(ITIMER_REAL,&it_val,NULL);
}

/* Version of time() that returns the timestamp
 * as a double, with milliseconds after the decimal,
 * floor'd down to a single decimal. Do not use for timers shorter than 0.1s!
 * --Ambrosia
 */
double time_ng(double *t)
{
  struct timespec spec;
  long ms;
  time_t s;
  double result;
  clock_gettime(CLOCK_REALTIME, &spec);
  s = spec.tv_sec;
  ms = round(spec.tv_nsec / 1.0e6);
  /* result = s + (floor((((1000.0+ms)/1000.0)-1.0) * 10) / 10); */
  result = (double)s + (floor((((1000.0+ms)/1000.0)-1.0) * (double)mudconf.mtimer) / (double)mudconf.mtimer);
  /*if(t != NULL)
    *t = result;*/
  return result;
}

double NDECL(next_timer)
{
double	result, i_rounder;
          
	i_rounder = 0.1;
	if ( mudconf.mtimer != 0 )
		i_rounder = 1.0 / (double)mudconf.mtimer;

	result = mudstate.dump_counter;
	if (mudstate.check_counter < result)
		result = mudstate.check_counter;
	if (mudstate.idle_counter < result)
		result = mudstate.idle_counter;
	if (mudstate.rwho_counter < result)
		result = mudstate.rwho_counter;
	if (mudstate.mstats_counter < result)
		result = mudstate.mstats_counter;
/*	result = result-(mudstate.nowmsec); */
        result = (mudstate.nowmsec) - result;
	if (result <= 0.0)
		result = i_rounder;
	return result;
}

void NDECL(init_timer)
{
	mudstate.nowmsec = time_ng(NULL);
	mudstate.now = (time_t) floor(mudstate.nowmsec);
	mudstate.lastnowmsec = mudstate.nowmsec;
	mudstate.lastnow = mudstate.now;
	mudstate.dump_counter = ((mudconf.dump_offset == 0) ? 
		mudconf.dump_interval : mudconf.dump_offset) + mudstate.nowmsec;
	mudstate.check_counter = ((mudconf.check_offset == 0) ?
		mudconf.check_interval : mudconf.check_offset) + mudstate.nowmsec;
	mudstate.idle_counter = mudconf.idle_interval + mudstate.nowmsec;
	mudstate.vattr_counter = mudconf.vattr_interval + mudstate.nowmsec;
	mudstate.rwho_counter = mudconf.rwho_interval + mudstate.nowmsec;
	mudstate.mstats_counter = 15.0 + mudstate.nowmsec;
        mudstate.alarm_triggered = 0;
	alarm_msec (next_timer());
}

void NDECL(dispatch)
{
char	*cmdsave;

	cmdsave = mudstate.debug_cmd;
	mudstate.debug_cmd = (char *)"< dispatch >";

	/* this routine can be used to poll from interface.c */

	if (!mudstate.alarm_triggered) {
//         STARTLOG(LOG_ALWAYS, "TIMER", "TRIGGERED");
//            log_text("Timer trigger event -- Alarm not found -- forcing alarm cycle.");
//         ENDLOG
           /* No alarm triggered, make sure we trigger next alarm */
           alarm_msec(next_timer());
           mudstate.alarm_triggered = 1;
           return;
        }
	mudstate.alarm_triggered = 0;
	mudstate.lastnowmsec = mudstate.nowmsec;
	mudstate.lastnow = mudstate.now;
	mudstate.nowmsec = time_ng(NULL);
	mudstate.now = (time_t) floor(mudstate.nowmsec);

	do_second();
	local_second();

	/* Free list reconstruction */

	if ((mudconf.control_flags & CF_DBCHECK) &&
	    (mudstate.check_counter <= mudstate.nowmsec)) {
		mudstate.check_counter = mudconf.check_interval + mudstate.nowmsec;
		mudstate.debug_cmd = (char *)"< dbck >";
		cache_reset(0);
		do_dbck (NOTHING, NOTHING, 0);
		cache_reset(0);
		pcache_trim();
	}

	/* Database dump routines */

	if ((mudconf.control_flags & CF_CHECKPOINT) &&
	    (mudstate.dump_counter <= mudstate.nowmsec)) {
		mudstate.dump_counter = mudconf.dump_interval + mudstate.nowmsec;
		mudstate.debug_cmd = (char *)"< dump >";
		fork_and_dump(0, (char *)NULL);
	}

	/* Idle user check */

	if ((mudconf.control_flags & CF_IDLECHECK) &&
	    (mudstate.idle_counter <= mudstate.nowmsec)) {
		mudstate.idle_counter = mudconf.idle_interval + mudstate.nowmsec;
		mudstate.debug_cmd = (char *)"< idlecheck >";
		cache_reset(0);
		check_idle();

	}

	if ((mudconf.control_flags & CF_VATTRCHECK) &&
	    (mudstate.vattr_counter <= mudstate.nowmsec)) {
		mudstate.vattr_counter = mudconf.vattr_interval + mudstate.nowmsec;
		mudstate.debug_cmd = (char *)"< vattrcheck >";
                do_recache_vattrs( GOD, 0, (char *)NULL, 1);
	}

#ifdef HAVE_GETRUSAGE
	/* Memory use stats */

	if (mudstate.mstats_counter <= mudstate.nowmsec) {

		int	curr;

		mudstate.mstats_counter = 15 + mudstate.nowmsec;
		curr = mudstate.mstat_curr;
		if ( (curr >= 0 ) && (mudstate.now > mudstate.mstat_secs[curr]) ) {

			struct rusage	usage;

			curr = 1-curr;
			getrusage(RUSAGE_SELF, &usage);
			mudstate.mstat_ixrss[curr] = usage.ru_ixrss;
			mudstate.mstat_idrss[curr] = usage.ru_idrss;
			mudstate.mstat_isrss[curr] = usage.ru_isrss;
			mudstate.mstat_secs[curr] = mudstate.now;
			mudstate.mstat_curr = curr;
		}
	}
#endif

#ifdef RWHO_IN_USE
	if ((mudconf.control_flags & CF_RWHO_XMIT) &&
	    (mudstate.rwho_counter <= mudstate.nowmsec)) {
		mudstate.rwho_counter = mudconf.rwho_interval + mudstate.nowmsec;
		mudstate.debug_cmd = (char *)"< rwho update >";
		rwho_update();
	}
#endif

	/* reset alarm */
        mudstate.alarm_triggered = 0;
	alarm_msec (next_timer());
	mudstate.debug_cmd = cmdsave;
}

/* ---------------------------------------------------------------------------
 * do_timewarp: Adjust various internal timers.
 */

void do_timewarp (dbref player, dbref cause, int key, char *arg)
{
int	secs;

	secs = atoi(arg);

	if ((key == 0) || (key & TWARP_QUEUE))		/* Sem/Wait queues */
		do_queue(player, cause, QUEUE_WARP, arg);
	if (key & TWARP_DUMP)
		mudstate.dump_counter -= secs;
	if (key & TWARP_CLEAN)
		mudstate.check_counter -= secs;
	if (key & TWARP_IDLE)
		mudstate.idle_counter -= secs;
	if (key & TWARP_RWHO)
		mudstate.rwho_counter -= secs;
}
