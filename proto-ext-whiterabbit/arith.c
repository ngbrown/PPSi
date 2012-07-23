/*
 * Aurelio Colosimo for CERN, 2011 -- GNU LGPL v2.1 or later
 * Based on PTPd project v. 2.1.0 (see AUTHORS for details)
 */

#include <limits.h>
#include <ppsi/ppsi.h>
#include <ppsi/diag.h>

void int64_to_TimeInternal(Integer64 bigint, TimeInternal *internal)
{
	uint64_t bigint_val;

	if (bigint.msb < 0)
		pp_printf("BUG: %s doesn't support negatives\n", __func__);

	bigint_val = bigint.lsb;
	bigint_val += ((int64_t)bigint.msb) << 32;

	/* Use __div64_32 from library, to avoid libgcc on small targets */
	internal->nanoseconds = __div64_32(&bigint_val, PP_NSEC_PER_SEC);
	internal->seconds = bigint_val;
}

int from_TimeInternal(TimeInternal *internal, Timestamp *external)
{
	/*
	 * fromInternalTime is only used to convert time given by the system
	 * to a timestamp As a consequence, no negative value can normally
	 * be found in (internal)
	 *
	 * Note that offsets are also represented with TimeInternal structure,
	 * and can be negative, but offset are never convert into Timestamp
	 * so there is no problem here.
	 */

	if ((internal->seconds & ~INT_MAX) ||
	    (internal->nanoseconds & ~INT_MAX)) {
		PP_PRINTF("Error: Negative value cannot be converted into "
			  "timestamp\n");
		return -1;
	} else {
		external->secondsField.lsb = internal->seconds;
		external->nanosecondsField = internal->nanoseconds;
		external->secondsField.msb = 0;
	}
	return 0;
}

int to_TimeInternal(TimeInternal *internal, Timestamp *external)
{
	/* Program will not run after 2038... */
	if (external->secondsField.lsb < INT_MAX) {
		internal->seconds = external->secondsField.lsb;
		internal->nanoseconds = external->nanosecondsField;
		return 0;
	} else {
		PP_PRINTF("to_TimeInternal: "
		    "seconds field is higher than signed integer (32bits)\n");
		return -1;
	}
}

static void normalize_TimeInternal(TimeInternal *r)
{
	r->seconds += r->nanoseconds / PP_NSEC_PER_SEC;
	r->nanoseconds -= r->nanoseconds / PP_NSEC_PER_SEC * PP_NSEC_PER_SEC;

	if (r->seconds > 0 && r->nanoseconds < 0) {
		r->seconds -= 1;
		r->nanoseconds += PP_NSEC_PER_SEC;
	} else if (r->seconds < 0 && r->nanoseconds > 0) {
		r->seconds += 1;
		r->nanoseconds -= PP_NSEC_PER_SEC;
	}
}

void add_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y)
{
	r->seconds = x->seconds + y->seconds;
	r->nanoseconds = x->nanoseconds + y->nanoseconds;

	normalize_TimeInternal(r);
}

void sub_TimeInternal(TimeInternal *r, TimeInternal *x, TimeInternal *y)
{
	r->seconds = x->seconds - y->seconds;
	r->nanoseconds = x->nanoseconds - y->nanoseconds;

	normalize_TimeInternal(r);
}

void div2_TimeInternal(TimeInternal *r)
{
    r->nanoseconds += r->seconds % 2 * 1000000000;
    r->seconds /= 2;
    r->nanoseconds /= 2;

    normalize_TimeInternal(r);
}

void set_TimeInternal(TimeInternal *t, Integer32 s, Integer32 ns)
{
	t->seconds = s;
	t->nanoseconds = ns;
}

void assign_TimeInternal(TimeInternal *to, TimeInternal *from)
{
	to->seconds = from->seconds;
	to->nanoseconds = from->nanoseconds;
	to->phase = from->phase;
	to->correct = from->correct;
	to->raw_phase = from->raw_phase;
	to->raw_nsec = from->raw_nsec;
	to->raw_ahead = from->raw_ahead;
}

void display_TimeInternal(const char *label, TimeInternal *t)
{
	PP_VPRINTF("%s: %s%d.%09d \n", label,
		(t->seconds < 0 || (t->seconds == 0 && t->nanoseconds < 0)) ? "-" : " ",
		abs(t->seconds), abs(t->nanoseconds));
}
