/*
 * hash.c - Bob Jenkin's one-at-a-time hash
 *
 * Copyright (c) Chris Putnam 2020-2021
 *
 * Source code released under the GPL version 2
 */

#include <stdlib.h>
#include <string.h>
#include "hash.h"

/*
 * Bob Jenkin's one-at-a-time hash
 */

static unsigned int
one_at_a_time_hash( const char *key, const unsigned int len, unsigned int HASH_SIZE )
{
	unsigned int hash = 0, i;
	for ( i=0; i<len; ++i ) {
		hash += key[i];
		hash += ( hash << 10 );
		hash ^= ( hash >> 6 );
	}
	hash += ( hash << 3 );
	hash ^= ( hash >> 11 );
	hash += ( hash << 15 );
	return hash % ( HASH_SIZE - 1 );
}

unsigned int
calculate_hash_char( const char *key, unsigned int HASH_SIZE )
{
	return one_at_a_time_hash( key, strlen( key ), HASH_SIZE );
}
