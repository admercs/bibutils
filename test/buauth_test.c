/*
 * buauth_test.c
 *
 * Copyright (c) 2020-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bu_auth.h"

char progname[] = "buauth_test";

typedef struct test_t {
	char *s;
	int expected;
} test_t;

int
test_is_bugenre( void )
{
	test_t tests[] = {

		{ "xxxxxxxx",             0 },
		{ "electronicXX",         0 },

		{ "academic journal",     1 },
		{ "airtel",               1 },
		{ "Airtel",               1 },
		{ "book chapter",         1 },
		{ "collection",           1 },
		{ "communication",        1 },
		{ "Diploma thesis",       1 },
		{ "Doctoral thesis",      1 },
		{ "electronic",           1 },
		{ "e-mail communication", 1 },
		{ "Habilitation thesis",  1 },
		{ "handwritten note",     1 },
		{ "hearing",              1 },
		{ "journal article",      1 },
		{ "Licentiate thesis",    1 },
		{ "magazine",             1 },
		{ "magazine article",     1 },
		{ "manuscript",           1 },
		{ "Masters thesis",       1 },
		{ "memo",                 1 },
		{ "miscellaneous",        1 },
		{ "newspaper article",    1 },
		{ "pamphlet",             1 },
		{ "Ph.D. thesis",         1 },
		{ "press release",        1 },
		{ "teletype",             1 },
		{ "television broadcast", 1 },
		{ "unpublished",          1 },
		{ "web page",             1 },

	};

	int ntests = sizeof( tests ) / sizeof( tests[0] );
	int failed = 0;
	int found;
	int i;

	for ( i=0; i<ntests; ++i ) {
		found = is_bu_genre( tests[i].s );
		if ( found != tests[i].expected ) {
			printf( "%s: Error is_bu_genre( '%s' ) returned %d, expected %d\n", progname, tests[i].s, found, tests[i].expected );
			failed++;
		}
	}
	return failed;
}

int
main( int argc, char *argv[] )
{
	int failed = 0;
	failed += test_is_bugenre();
	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}
}
