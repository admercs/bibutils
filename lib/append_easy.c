/*
 * append_easy.c
 *
 * Most output formats have identical requirements for outputting
 * easy tag/value pairs, so centralize code.
 *
 * append_easy()      - output first match
 * append_easyall()   - output all matches as separate tag/value pairs
 * append_easycombo() - output all matches as a single tag/value pair, where
 *                      values are separated by the sep string
 *
 * Copyright (c) Chris Putnam 2020-2021
 *
 * Source code released under the GPL version 2
 */
#include <stdio.h>
#include <stdlib.h>
#include "append_easy.h"
#include "bibdefs.h"
#include "utf8.h"
#include "vplist.h"


void
append_easy( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, int *status )
{
	char *value;
	int fstatus;

	value = fields_findv( in, inlevel, FIELDS_CHRP, intag );
	if ( value ) {
		fstatus = fields_add( out, outtag, value, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
unique_entries_only( vplist *a )
{
	vplist_index i, j;
	int match;
	str *curr;

	i = 0;
	while ( i < a->n ) {

		curr = ( str * ) vplist_get( a, i );
		match = 0;

		for ( j=0; j<i; ++j ) {
			if ( !str_strcmp( curr, (str*) vplist_get( a, j ) ) ) {
				match = 1;
				break;
			}
		}

		if ( match ) vplist_remove( a, i );
		else i++;

	}
}


void
append_easyall( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, int *status )
{
	vplist_index i;
	int fstatus;
	str *value;
	vplist a;

	vplist_init( &a );

	fields_findv_each( in, inlevel, FIELDS_STRP, &a, intag );
	unique_entries_only( &a );

	for ( i=0; i<a.n; ++i ) {
		value = ( str * ) vplist_get( &a, i );
		fstatus = fields_add( out, outtag, str_cstr( value ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

out:
	vplist_free( &a );
}


void
append_easyallpre( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, const char *prefix, int *status )
{
	vplist_index i;
	int fstatus;
	str value;
	vplist a;

	str_init( &value );
	vplist_init( &a );

	fields_findv_each( in, inlevel, FIELDS_STRP, &a, intag );
	unique_entries_only( &a );

	for ( i=0; i<a.n; ++i ) {
		str_strcpyc( &value, prefix );
		str_strcat( &value, ( str * ) vplist_get( &a, i ) );
		fstatus = fields_add( out, outtag, str_cstr( &value ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

out:
	str_free( &value );
	vplist_free( &a );
}


void
append_easycombo( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, const char *sep, int *status )
{
	vplist_index i;
	int fstatus;
	str value;
	vplist a;

	str_init( &value );
	vplist_init( &a );

	fields_findv_each( in, inlevel, FIELDS_STRP, &a, intag );
	unique_entries_only( &a );

	for ( i=0; i<a.n; ++i ) {
		if ( i>0 ) str_strcatc( &value, sep );
		str_strcat( &value, (str *) vplist_get( &a, i ) );
	}

	if ( str_memerr( &value ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	fstatus = fields_add( out, outtag, str_cstr( &value ), LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;

out:
	str_free( &value );
	vplist_free( &a );
}

/* output page value while converting em-dash and en-dash to a simple dash */
int
append_easypage( fields *out, const char *outtag, const char *value, int level )
{
	int fstatus, status = BIBL_OK;
	const char *p;
	str use;

	str_init( &use );

	p = value;
	while ( *p ) {
		/* -30 is the first character of a UTF8 em-dash and en-dash */
		if ( *p==-30 && ( utf8_is_emdash( p ) || utf8_is_endash( p ) ) ) {
			str_addchar( &use, '-' );
			p+=3;
		} else {
			str_addchar( &use, *p );
			p+=1;
		}
	}

	fstatus = fields_add( out, outtag, str_cstr( &use ), level );
	if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;

	str_free( &use );
	return status;
}
