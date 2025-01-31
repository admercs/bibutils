/*
 * hash_bu.c
 *
 * Copyright (c) Chris Putnam 2020-2021
 *
 * Source code released under the GPL version 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "uintlist.h"

const char *bu_genre[] = {
	"academic journal",
	"airtel",
	"Airtel",
	"book chapter",
	"collection",
	"communication",
	"Diploma thesis",
	"Doctoral thesis",
	"electronic",
	"e-mail communication",
	"Habilitation thesis",
	"handwritten note",
	"hearing",
	"journal article",
	"Licentiate thesis",
	"magazine",
	"magazine article",
	"manuscript",
	"Masters thesis",
	"memo",
	"miscellaneous",
	"newspaper article",
	"pamphlet",
	"Ph.D. thesis",
	"press release",
	"teletype",
	"television broadcast",
	"unpublished",
	"web page"
};
const int nbu_genre = sizeof( bu_genre ) / sizeof( const char *);

void
memerr( const char *fn )
{
	fprintf( stderr, "Memory error in %s()\n", fn );
	exit( EXIT_FAILURE );
}

int
hashify_test_size( const char *list[], int nlist, unsigned int HASH_SIZE, uintlist *u )
{
	unsigned int n;
	int i, pos;
//	int i, ok, pos;

//	ok = 1;

	uintlist_empty( u );

	for ( i=0; i<nlist; ++i ) {

		n = calculate_hash_char( list[i], HASH_SIZE );

		pos = uintlist_find( u, n );

		/* if position occupied, try next bucket */
		if ( pos!=-1 ) {
			n++;
			pos = uintlist_find( u, n );
		}

		/* if both n and n+1 occupied, bail */
		if ( pos!=-1 ) return 0;

		uintlist_add( u, n );
#if 0
		fprintf( stderr, "i=%d nlist=%d\n",i,nlist );
		{ int j;
		for ( j=0; j<u.n; ++j ) fprintf( stderr, " %u", uintlist_get( &u, j ) );
		fprintf( stderr, "\n" );
		}
#endif

	}

	return 1;
}

void
hashify_write( FILE *fp, const char *list[], int nlist, const char *type, unsigned int HASH_SIZE, uintlist *u )
{
	unsigned n;
	int i;

	/* write hash */
	fprintf( fp, "/*\n" );
	fprintf( fp, " * Bibutils %s hash\n", type );
	fprintf( fp, " */\n" );
	fprintf( fp, "static const unsigned int bu_%s_hash_size = %u;\n", type, HASH_SIZE );
	fprintf( fp, "static const char *bu_%s[%u] = {\n", type, HASH_SIZE );
	fprintf( fp, "\t[ 0 ... %u ] = NULL,\n", HASH_SIZE-1 );
	for ( i=0; i<nlist; ++i ) {
		n = uintlist_get( u, i );
//		n = calculate_hash_char( list[i], HASH_SIZE );
		fprintf( fp, "\t[ %3u ] = \"%s\",\n", n, list[i] );
	}
	fprintf( fp, "};\n\n" );

	fprintf( fp, "int\n" );
	fprintf( fp, "is_bu_%s( const char *query )\n", type );
	fprintf( fp, "{\n" );
	fprintf( fp, "\tunsigned int n;\n\n" );
	fprintf( fp, "\tn = calculate_hash_char( query, bu_%s_hash_size );\n", type );
	fprintf( fp, "\tif ( bu_%s[n]==NULL ) return 0;\n", type );
	fprintf( fp, "\tif ( !strcmp( query, bu_%s[n] ) ) return 1;\n", type );
	fprintf( fp, "\telse if ( bu_%s[n+1] && !strcmp( query, bu_%s[n+1] ) ) return 1;\n", type, type );
	fprintf( fp, "\telse return 0;\n" );
	fprintf( fp, "}\n" );
}

unsigned int
hashify_size( const char *list[], int nlist, unsigned int max, uintlist *u )
{
	unsigned int HASH_SIZE = (unsigned int ) nlist;
	int ok;

	do {
		ok = hashify_test_size( list, nlist, HASH_SIZE, u );
		if ( ok ) break;
		HASH_SIZE += 1;
		if ( HASH_SIZE > max ) break;
	} while ( 1 );

	if ( !ok ) return 0;
	else return HASH_SIZE;
}

void
hashify( const char *list[], int nlist, const char *type )
{
	unsigned int HASH_SIZE;
	uintlist u;

	uintlist_init( &u );

	HASH_SIZE = hashify_size( list, nlist, 10000, &u );
	if ( HASH_SIZE > 0 ) {
		hashify_write( stdout, list, nlist, type, HASH_SIZE, &u );
	}
	else {
		printf( "/* No valid HASH_SIZE for bu_%s up to %u */\n", type, 10000 );
	}

	uintlist_free( &u );
}

void
write_header( FILE *fp )
{
	fprintf( fp, "/*\n" );
	fprintf( fp, " * bu_auth.c - Identify genre to be labeled with Bibutils authority/\n" );
	fprintf( fp, " *\n" );
 	fprintf( fp, " * Copyright (c) Chris Putnam 2017-2020\n" );
	fprintf( fp, " *\n" );
	fprintf( fp, " * Source code released under the GPL version 2\n" );
	fprintf( fp, " */\n\n" );

	fprintf( fp, "#include <stdlib.h>\n" );
	fprintf( fp, "#include <string.h>\n" );
	fprintf( fp, "#include \"bu_auth.h\"\n" );
	fprintf( fp, "#include \"hash.h\"\n" );
	fprintf( fp, "\n" );

#if 0
	fprintf( fp, "/*\n" );
	fprintf( fp, " * Bob Jenkin's one-at-a-time hash\n" );
	fprintf( fp, " */\n" );
	fprintf( fp, "\n" );

	fprintf( fp, "static unsigned int\n" );
	fprintf( fp, "one_at_a_time_hash( const char *key, const unsigned int len, unsigned int HASH_SIZE )\n" );
	fprintf( fp, "{\n" );
	fprintf( fp, "\tunsigned int hash = 0, i;\n" );
	fprintf( fp, "\tfor ( i=0; i<len; ++i ) {\n" );
	fprintf( fp, "\t\thash += key[i];\n" );
	fprintf( fp, "\t\thash += ( hash << 10 );\n" );
	fprintf( fp, "\t\thash ^= ( hash >> 6 );\n" );
	fprintf( fp, "\t}\n" );
	fprintf( fp, "\thash += ( hash << 3 );\n" );
	fprintf( fp, "\thash ^= ( hash >> 11 );\n" );
	fprintf( fp, "\thash += ( hash << 15 );\n" );
	fprintf( fp, "\treturn hash % ( HASH_SIZE - 1 );\n" );
	fprintf( fp, "}\n\n" );

	fprintf( fp, "static unsigned int\n" );
	fprintf( fp, "calculate_hash_char( const char *key, unsigned int HASH_SIZE )\n" );
	fprintf( fp, "{\n" );
	fprintf( fp, "\treturn one_at_a_time_hash( key, strlen( key ), HASH_SIZE );\n" );
	fprintf( fp, "}\n\n" );
#endif
}

int
main( int argc, char *argv[] )
{
	write_header( stdout );
	hashify( bu_genre, nbu_genre, "genre" );
}
