/*
 * month.c
 *
 * Copyright (c) Chris Putnam 2021
 *
 * Source code released under the GPL version 2
 */
#include <string.h>
#include "month.h"

typedef struct {
	const char *text, *number;
} month_convert;

static month_convert full_months[] = {
	{ "January",   "01" },
	{ "February",  "02" },
	{ "March",     "03" },
	{ "April",     "04" },
	{ "May",       "05" },
	{ "June",      "06" },
	{ "July",      "07" },
	{ "August",    "08" },
	{ "September", "09" },
	{ "October",   "10" },
	{ "November",  "11" },
	{ "December",  "12" },
};
static int nfull_months = sizeof( full_months ) / sizeof( full_months[0] );

static month_convert abbr_months[] = {
	{ "Jan", "01" }, { "Jan.", "01" },
	{ "Feb", "02" }, { "Feb.", "02" },
	{ "Mar", "03" }, { "Mar.", "03" },
	{ "Apr", "04" }, { "Apr.", "04" },
	{ "May", "05" },
	{ "Jun", "06" }, { "Jun.", "06" },
	{ "Jul", "07" }, { "Jul.", "07" },
	{ "Aug", "08" }, { "Aug.", "08" },
	{ "Sep", "09" }, { "Sep.", "09" },
	{ "Oct", "10" }, { "Oct.", "10" },
	{ "Nov", "11" }, { "Nov.", "11" },
	{ "Dec", "12" }, { "Dec.", "12" },
};
static int nabbr_months = sizeof( abbr_months ) / sizeof( abbr_months[0] );


/* month_to_number()
 * convert month name to number in format MM, e.g. "January" -> "01"
 * if output is the format "01"->"12", return 1
 * otherwise return 0
 */

int
month_to_number( const char *in, const char **out )
{
	int i;

	for ( i=0; i<nfull_months; ++i ) {
		if ( !strcasecmp( in, full_months[i].text ) ) {
			*out = full_months[i].number;
			return 1;
		}
	}

	for ( i=0; i<nabbr_months; ++i ) {
		if ( !strcasecmp( in, abbr_months[i].text ) ) {
			*out = abbr_months[i].number;
			return 1;
		}
	}

	*out = in;

	/* check to see if it's already in the format "01"->"12" */
	for ( i=0; i<nfull_months; ++i ) {
		if ( !strcmp( in, full_months[i].number ) ) return 1;
	}

	return 0;
}

int
number_to_full_month( const char *in, const char **out )
{
	int i;

	for ( i=0; i<nfull_months; ++i ) {
		if ( !strcasecmp( in, full_months[i].number ) ) {
			*out = full_months[i].text;
			return 1;
		}
	}

	*out = in;

	return 0;
}

int
number_to_abbr_month( const char *in, const char **out )
{
	int i;

	for ( i=0; i<nabbr_months; ++i ) {
		if ( !strcasecmp( in, abbr_months[i].number ) ) {
			*out = abbr_months[i].text;
			return 1;
		}
	}

	*out = in;

	return 0;
}

int
month_is_number( const char *in )
{
	int i;
	for ( i=0; i<nfull_months; ++i )
		if ( !strcmp( in, full_months[i].number ) ) return 1;
	return 0;
}
