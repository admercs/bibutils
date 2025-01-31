/*
 * pages.c
 *
 * Copyright (c) Chris Putnam 2016-2021
 *
 * Program and source code released under GPL verison 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bibdefs.h"
#include "is_ws.h"
#include "utf8.h"
#include "pages.h"

/* extract_range()
 *
 * Handle input strings like:
 *
 * "1-15"
 * " 1 - 15 "
 * " 1000--- 1500"
 * " 1 <<em-dash>> 10"
 * " 107 111"
 */
static void
extract_range( str *input, str *begin, str *end )
{
	/* -30 is the first character of a UTF8 em-dash and en-dash */
	const char terminators[] = { ' ', '-', '\t', '\r', '\n', -30, '\0' };
	const char *p;

	str_empty( begin );
	str_empty( end );

	if ( input->len==0 ) return;

	p = skip_ws( str_cstr( input ) );
	while ( *p && !strchr( terminators, *p ) )
		str_addchar( begin, *p++ );

	p = skip_ws( p );

	while ( *p=='-' ) p++;
	while ( utf8_is_emdash( p ) ) p+=3;
	while ( utf8_is_endash( p ) ) p+=3;

	p = skip_ws( p );

	while ( *p )
		str_addchar( end, *p++ );

	str_trimendingws( end );
}

/* Expand ranges like 100-15 to 100 115 */

static int
str_iswholenumber( str *s )
{
	const char *p = str_cstr( s );
	while ( '0' <= *p && *p <= '9' ) p++;
	return ( *p=='\0' );
}

static void
complete_range( str *start, str *stop )
{
	unsigned long i, n;
	const char *p;
	str newstop;

	if ( str_strlen( start ) == 0 || str_strlen( stop ) == 0 ) return;
	if ( !str_iswholenumber( start ) || !str_iswholenumber( stop ) ) return;
	if ( str_strlen( start ) <= str_strlen( stop ) ) return;

	/* copy first part of "start" and combine with all of "stop" */
	n = str_strlen( start ) - str_strlen( stop );

	str_init( &newstop );

	p = str_cstr( start );
	for ( i=0; i<n; ++i ) {
		str_addchar( &newstop, *p );
		p++;
	}

	str_strcat( &newstop, stop );

	str_strcpy( stop, &newstop );

	str_free( &newstop );
}

int
add_pages( fields *bibout, str *value, int level )
{
	int fstatus, status = BIBL_OK;
	str start, stop;

	str_init( &start );
	str_init( &stop );

	extract_range( value, &start, &stop );

	complete_range( &start, &stop );

	if ( str_memerr( &start ) || str_memerr( &stop ) ) {
		status = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( start.len>0 ) {
		fstatus = fields_add( bibout, "PAGES:START", str_cstr( &start ), level );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

	if ( stop.len>0 ) {
		fstatus = fields_add( bibout, "PAGES:STOP", str_cstr( &stop ), level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

out:
	str_free( &start );
	str_free( &stop );
	return status;
}

