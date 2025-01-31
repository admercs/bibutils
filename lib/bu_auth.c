/*
 * bu_auth.c - Identify genre to be labeled with Bibutils authority
 *
 * Copyright (c) Chris Putnam 2017-2021
 *
 * Source code released under the GPL version 2
 */

#include <stdlib.h>
#include <string.h>
#include "bu_auth.h"
#include "hash.h"

/*
 * Bibutils genre hash
 */
static const unsigned int bu_genre_hash_size = 50;
static const char *bu_genre[50] = {
	[ 0 ... 49 ] = NULL,
	[  11 ] = "academic journal",
	[   6 ] = "airtel",
	[  37 ] = "Airtel",
	[  21 ] = "book chapter",
	[  29 ] = "collection",
	[   9 ] = "communication",
	[  34 ] = "Diploma thesis",
	[   8 ] = "Doctoral thesis",
	[   7 ] = "electronic",
	[   0 ] = "e-mail communication",
	[  48 ] = "Habilitation thesis",
	[  22 ] = "handwritten note",
	[  10 ] = "hearing",
	[  14 ] = "journal article",
	[  17 ] = "Licentiate thesis",
	[   3 ] = "magazine",
	[  31 ] = "magazine article",
	[  28 ] = "manuscript",
	[  35 ] = "Masters thesis",
	[  18 ] = "memo",
	[  36 ] = "miscellaneous",
	[  42 ] = "newspaper article",
	[  19 ] = "pamphlet",
	[   4 ] = "Ph.D. thesis",
	[  13 ] = "press release",
	[  16 ] = "teletype",
	[  26 ] = "television broadcast",
	[  45 ] = "unpublished",
	[  12 ] = "web page",
};

int
is_bu_genre( const char *query )
{
	unsigned int n;

	n = calculate_hash_char( query, bu_genre_hash_size );
	if ( bu_genre[n]==NULL ) return 0;
	if ( !strcmp( query, bu_genre[n] ) ) return 1;
	else if ( bu_genre[n+1] && !strcmp( query, bu_genre[n+1] ) ) return 1;
	else return 0;
}
