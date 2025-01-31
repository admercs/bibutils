/*
 * fields_test.c
 *
 * Copyright (c) 2020-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "fields.h"

char progname[] = "fields_test";
char version[] = "0.1";

void
memerr( const char *fn )
{
	fprintf( stderr, "%s: Memory error in %s()\n", progname, fn );
	exit( EXIT_FAILURE );
}

#define check( a, b ) { \
	if ( !(a) ) { \
		fprintf( stderr, "Failed %s (%s) in %s() line %d\n", #a, b, __FUNCTION__, __LINE__ );\
		return 1; \
	} \
}

#define check_len( a, b ) if ( !_check_len( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_len( fields *f, int expected, const char *fn, int line )
{
	if ( f->n == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected fields length of %d, found %d\n", fn, line, expected, f->n );
	return 0;
}

#define check_tag( a, b, c ) if ( !_check_entry( a, b, c, "tag", 0, __FUNCTION__, __LINE__ ) ) return 1;
#define check_value( a, b, c ) if ( !_check_entry( a, b, c, "value", 1,  __FUNCTION__, __LINE__ ) ) return 1;
int
_check_entry( fields *f, int n, const char *expected, const char *type, int typenum, const char *fn, int line )
{
	char *s;

	if ( typenum==0 ) s = fields_tag( f, n, FIELDS_CHRP_NOUSE );
	else              s = fields_value( f, n, FIELDS_CHRP_NOUSE );

	if ( s==NULL && expected==NULL ) return 1;
	if ( s!=NULL && expected==NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected fields %s %d to be NULL, found '%s'\n",
			fn, line, type, n, s );
		return 0;
	}
	if ( s==NULL && expected!=NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected fields %s %d to be '%s', found NULL\n",
			fn, line, type, n, expected );
		return 0;
	}
	if ( !strcmp( s, expected ) ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected fields %s %d to be '%s', found '%s'\n",
		fn, line, type, n, expected, s );
	return 0;
}

#define check_level( a, b, c ) if ( !_check_level( a, b, c, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_level( fields *f, int n, int expected, const char *fn, int line )
{
	int lvl;

	lvl = fields_level( f, n );
	if ( lvl==expected ) return 1;

	fprintf( stderr, "Failed: %s() line %d: Expected fields level %d to be %d, found %d\n",
		fn, line, n, expected, lvl );

	return 0;
}

#define check_entry_empty( a, b ) { check_tag( a, b, NULL ); check_value( a, b, NULL ); check_level( a, b, 0 ); }

#define check_entry( a, b, c, d, e ) { check_tag( a, b, c ); check_value( a, b, d ); check_level( a, b, e ); }

int
make_fields_empty( fields *f )
{
	fields_init( f );
	return FIELDS_OK;
}

int
make_fields_with_unique_content( fields *f, ... )
{
	int level, status = FIELDS_OK;
	const char *tag, *value;
	va_list args;

	va_start( args, f );

	fields_init( f );

	while ( ( tag = va_arg( args, const char * ) ) ) {
		value = va_arg( args, const char * );
		level = va_arg( args, int );
		status = fields_add( f, tag, value, level );
		if ( status!=FIELDS_OK ) goto out;
	}
out:
	va_end( args );
	return status;
}

int
make_fields_with_unique_content_suffix( fields *f, ... )
{
	int level, status = FIELDS_OK;
	const char *tag, *value, *suffix;
	va_list args;

	va_start( args, f );

	fields_init( f );

	while ( ( tag = va_arg( args, const char * ) ) ) {
		suffix = va_arg( args, const char * );
		value = va_arg( args, const char * );
		level = va_arg( args, int );
		status = fields_add_suffix( f, tag, suffix, value, level );
		if ( status!=FIELDS_OK ) goto out;
	}
out:
	va_end( args );
	return status;
}

int
make_fields_with_dup_content( fields *f, ... )
{
	int level, status = FIELDS_OK;
	const char *tag, *value;
	va_list args;

	va_start( args, f );

	fields_init( f );

	while ( ( tag = va_arg( args, const char * ) ) ) {
		value = va_arg( args, const char * );
		level = va_arg( args, int );
		status = fields_add_can_dup( f, tag, value, level );
		if ( status!=FIELDS_OK ) goto out;
	}
out:
	va_end( args );
	return status;
}

int
make_fields_with_dup_content_suffix( fields *f, ... )
{
	int level, status = FIELDS_OK;
	const char *tag, *value, *suffix;
	va_list args;

	va_start( args, f );

	fields_init( f );

	while ( ( tag = va_arg( args, const char * ) ) ) {
		suffix = va_arg( args, const char * );
		value = va_arg( args, const char * );
		level = va_arg( args, int );
		status = fields_add_suffix_can_dup( f, tag, suffix, value, level );
		if ( status!=FIELDS_OK ) goto out;
	}
out:
	va_end( args );
	return status;
}

/*
 * fields_init() should initialize an empty structure
 */
int
test_init( void )
{
	int status;
	fields f;

	status = make_fields_empty( &f );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	check_entry_empty( &f, -1 );
	check_entry_empty( &f, 0 );
	check_entry_empty( &f, 1 );

	fields_free( &f );

	return 0;
}

/*
 * test fields_new(); should allocate an empty structure
 */
int
test_new( void )
{
	fields *f;

	f = fields_new();
	if ( f==NULL ) memerr( __FUNCTION__ );

	check_len( f, 0 );

	check_entry_empty( f, -1 );
	check_entry_empty( f, 0 );
	check_entry_empty( f, 1 );

	fields_delete( f );

	return 0;
}

/*
 * test fields_add(); should add a single entry if valid strings are provided
 *                    should add mutliple entries if valid strings are provided
 *                    should not add entries with a NULL tag
 *                    should not add entries with a NULL value
 *                    should add a single unique entry and ignore duplicates
 */
int
test_add_single( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 1 );

	check_entry_empty( &f, -1 );
	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry_empty( &f, 1 );

	fields_free( &f );

	return 0;
}

int
test_add_badtag( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			NULL, "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_badvalue( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", NULL, LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_multiple( void )
{
	char tag[512], value[512];
	int i, n=100, status;
	fields f;

	fields_init( &f );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add( &f, tag, value, i );
		check( status==FIELDS_OK, "fields_add() did not return FIELDS_OK" );
	}

	check_len( &f, n );

	check_entry_empty( &f, -1 );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		check_entry( &f, i, tag, value, i );
	}

	check_entry_empty( &f, n );

	fields_free( &f );

	return 0;
}

int
test_add_check_dup( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG1", "VALUE1", LEVEL_MAIN, /* same as first */
			"TAG1", "VALUE1", LEVEL_HOST, /* valid; different level */
			"TAG1", "VALUE2", LEVEL_MAIN, /* valid; different value */
			"TAG2", "VALUE1", LEVEL_MAIN, /* valid; different tag */
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 4 );

	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG1", "VALUE1", LEVEL_HOST );
	check_entry( &f, 2, "TAG1", "VALUE2", LEVEL_MAIN );
	check_entry( &f, 3, "TAG2", "VALUE1", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_add( void )
{
	int err = 0;
	err += test_add_single();
	err += test_add_badtag();
	err += test_add_badvalue();
	err += test_add_multiple();
	err += test_add_check_dup();
	return err;
}

/*
 * test fields_add_can_dup(); should add a single entry if valid strings are provided
 *                            should not add entries with a NULL tag
 *                            should not add entries with a NULL value
 *                            should add an entry for each valid combination
 *                            should add all copies of duplicates
 */
int
test_add_can_dup_single( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 1 );

	check_entry_empty( &f, -1 );
	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry_empty( &f, 1 );

	fields_free( &f );

	return 0;
}

int
test_add_can_dup_badtag( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content( &f,
			NULL, "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_can_dup_badvalue( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content( &f,
			"TAG1", NULL, LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_can_dup_multiple( void )
{
	char tag[512], value[512];
	int i, n=100, status;
	fields f;

	fields_init( &f );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add_can_dup( &f, tag, value, i );
		check( status==FIELDS_OK, "fields_add_can_dup() did not return FIELDS_OK" );
	}

	check_len( &f, n );

	check_entry_empty( &f, -1 );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		check_entry( &f, i, tag, value, i );
	}

	check_entry_empty( &f, n );

	fields_free( &f );

	return 0;
}

int
test_add_can_dup_check_dup( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG1", "VALUE1", LEVEL_MAIN, /* duplicate */
			"TAG1", "VALUE1", LEVEL_HOST, /* level differs */
			"TAG1", "VALUE2", LEVEL_MAIN, /* value differs */
			"TAG2", "VALUE1", LEVEL_MAIN, /* tag differs */
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 2, "TAG1", "VALUE1", LEVEL_HOST );
	check_entry( &f, 3, "TAG1", "VALUE2", LEVEL_MAIN );
	check_entry( &f, 4, "TAG2", "VALUE1", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_add_can_dup( void )
{
	int err = 0;
	err += test_add_can_dup_single();
	err += test_add_can_dup_badtag();
	err += test_add_can_dup_badvalue();
	err += test_add_can_dup_multiple();
	err += test_add_can_dup_check_dup();
	return err;
}

/*
 * test fields_add_suffix(); should add a single entry if valid strings are provided
 *                           should not add entries with a NULL tag
 *                           should not add entries with a NULL value
 *                           should add an entry for each valid combination
 *                           should add a single unique entry and ignore duplicates
 */
int
test_add_suffix_single( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content_suffix( &f,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 1 );

	check_entry_empty( &f, -1 );
	check_entry( &f, 0, "TAG1:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry_empty( &f, 1 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_badtag( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content_suffix( &f,
			NULL, ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_badvalue( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content_suffix( &f,
			"TAG1", ":SUFFIX1", NULL, LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_multiple( void )
{
	char tag[512], value[512];
	int i, n=100, status;
	fields f;

	fields_init( &f );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add_suffix( &f, tag, ":SUFFIX", value, i );
		check( status==FIELDS_OK, "fields_add_suffix() did not return FIELDS_OK" );
	}

	check_len( &f, n );

	check_entry_empty( &f, -1 );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d:SUFFIX", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		check_entry( &f, i, tag, value, i );
	}

	check_entry_empty( &f, n );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_check_dup( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content_suffix( &f,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN, /* duplicate */
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_HOST, /* level  differs */
			"TAG1", ":SUFFIX1", "VALUE2", LEVEL_MAIN, /* value  differs */
			"TAG2", ":SUFFIX1", "VALUE1", LEVEL_MAIN, /* tag    differs */
			"TAG1", ":SUFFIX2", "VALUE1", LEVEL_MAIN, /* suffix differs */
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	check_entry( &f, 0, "TAG1:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG1:SUFFIX1", "VALUE1", LEVEL_HOST );
	check_entry( &f, 2, "TAG1:SUFFIX1", "VALUE2", LEVEL_MAIN );
	check_entry( &f, 3, "TAG2:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 4, "TAG1:SUFFIX2", "VALUE1", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_add_suffix( void )
{
	int err = 0;
	err += test_add_suffix_single();
	err += test_add_suffix_badtag();
	err += test_add_suffix_badvalue();
	err += test_add_suffix_multiple();
	err += test_add_suffix_check_dup();
	return err;
}

/*
 * test fields_add_can_dup_suffix(); should add a single entry if valid strings are provided
 *                                   should not add entries with a NULL tag
 *                                   should not add entries with a NULL value
 *                                   should add an entry for each valid combination
 *                                   should add all copies of duplicates
 */
int
test_add_suffix_can_dup_single( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content_suffix( &f,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 1 );

	check_entry_empty( &f, -1 );
	check_entry( &f, 0, "TAG1:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry_empty( &f, 1 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_can_dup_badtag( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content_suffix( &f,
			NULL, ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_can_dup_badvalue( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content_suffix( &f,
			"TAG1", ":SUFFIX1", NULL, LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 0 );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_can_dup_multiple( void )
{
	char tag[512], value[512];
	int i, n=100, status;
	fields f;

	fields_init( &f );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add_suffix_can_dup( &f, tag, ":SUFFIX", value, i );
		check( status==FIELDS_OK, "fields_add_suffix_can_dup() did not return FIELDS_OK" );
	}

	check_len( &f, n );

	check_entry_empty( &f, -1 );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d:SUFFIX", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		check_entry( &f, i, tag, value, i );
	}

	check_entry_empty( &f, n );

	fields_free( &f );

	return 0;
}

/*
 * fields_add() should add a single unique entry and ignore duplicates
 */
int
test_add_suffix_can_dup_check_dup( void )
{
	int status;
	fields f;

	status = make_fields_with_dup_content_suffix( &f,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN,
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_MAIN, /* duplicate */
			"TAG1", ":SUFFIX1", "VALUE1", LEVEL_HOST, /* level  differs */
			"TAG1", ":SUFFIX1", "VALUE2", LEVEL_MAIN, /* value  differs */
			"TAG2", ":SUFFIX1", "VALUE1", LEVEL_MAIN, /* tag    differs */
			"TAG1", ":SUFFIX2", "VALUE1", LEVEL_MAIN, /* suffix differs */
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 6 );

	check_entry( &f, 0, "TAG1:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG1:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 2, "TAG1:SUFFIX1", "VALUE1", LEVEL_HOST );
	check_entry( &f, 3, "TAG1:SUFFIX1", "VALUE2", LEVEL_MAIN );
	check_entry( &f, 4, "TAG2:SUFFIX1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 5, "TAG1:SUFFIX2", "VALUE1", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_add_suffix_can_dup( void )
{
	int err = 0;
	err += test_add_suffix_can_dup_single();
	err += test_add_suffix_can_dup_badtag();
	err += test_add_suffix_can_dup_badvalue();
	err += test_add_suffix_can_dup_multiple();
	err += test_add_suffix_can_dup_check_dup();
	return err;
}

/*
 * fields_replace_or_add() replaces the value of a matching tag/value if exists, adds otherwise
 */
int
test_replace_or_add( void )
{
	char tag[512], value[512];
	int i, n=40, status;
	fields f;

	fields_init( &f );

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add( &f, tag, value, LEVEL_MAIN );
		check( status==FIELDS_OK, "fields_add() did not return FIELDS_OK" );
	}

	check_len( &f, n );
	check_value( &f, 1, "VALUE2" );

	status = fields_replace_or_add( &f, "TAG2", "VALUE2NEW", LEVEL_MAIN );
	check( status==FIELDS_OK, "fields_replace_or_add() did not return FIELDS_OK" );

	check_len( &f, n );
	check_value( &f, 1, "VALUE2NEW" );

	status = fields_replace_or_add( &f, "NNN", "VALUENNN", LEVEL_MAIN );
	check( status==FIELDS_OK, "fields_replace_or_add() did not return FIELDS_OK" );

	check_len( &f, n+1 );
	check_tag  ( &f, n, "NNN" );
	check_value( &f, n, "VALUENNN" );

	fields_free( &f );

	return 0;
}

/*
 * test fields_remove(); should eliminate specific entries and return FIELDS_OK
 *                       should return FIELDS_ERR_NOTFOUND with invalid entries
 */
int
test_remove_valid_first( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	status = fields_remove( &f, 0 );
	check( status==FIELDS_OK, "fields_remove() did not return FIELDS_OK" );

	check_len( &f, 4 );

	check_entry( &f, 0, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( &f, 1, "TAG3", "VALUE3", LEVEL_SERIES );
	check_entry( &f, 2, "TAG4", "VALUE4", LEVEL_ORIG );
	check_entry( &f, 3, "TAG5", "VALUE5", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_remove_valid_middle( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	status = fields_remove( &f, 2 );
	check( status==FIELDS_OK, "fields_remove() did not return FIELDS_OK" );

	check_len( &f, 4 );

	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( &f, 2, "TAG4", "VALUE4", LEVEL_ORIG );
	check_entry( &f, 3, "TAG5", "VALUE5", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_remove_valid_last( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	status = fields_remove( &f, 4 );
	check( status==FIELDS_OK, "fields_remove() did not return FIELDS_OK" );

	check_len( &f, 4 );

	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( &f, 2, "TAG3", "VALUE3", LEVEL_SERIES );
	check_entry( &f, 3, "TAG4", "VALUE4", LEVEL_ORIG );

	fields_free( &f );

	return 0;
}

int
test_remove_invalid( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	status = fields_remove( &f, 5 );
	check( status==FIELDS_ERR_NOTFOUND, "ERR" );

	check_len( &f, 5 );

	check_entry( &f, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( &f, 1, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( &f, 2, "TAG3", "VALUE3", LEVEL_SERIES );
	check_entry( &f, 3, "TAG4", "VALUE4", LEVEL_ORIG );
	check_entry( &f, 4, "TAG5", "VALUE5", LEVEL_MAIN );

	fields_free( &f );

	return 0;
}

int
test_remove( void )
{
	int err = 0;
	err += test_remove_valid_first();
	err += test_remove_valid_middle();
	err += test_remove_valid_last();
	err += test_remove_invalid();
	return err;
}

/*
 * fields_maxlevel() finds the maximum level in the fields structure
 */
int
test_maxlevel( void )
{
	char tag[512], value[512];
	int i, n=100, status;
	fields f;

	fields_init( &f );

	if ( fields_maxlevel( &f ) != 0 ) {
		fprintf( stderr, "Failed %s in %s() line %d\n", "fields_maxlevel( &f ) == 0", __FUNCTION__, __LINE__ );
		return 1;
	}

	for ( i=0; i<n; ++i ) {
		sprintf( tag, "TAG%d", i+1 );
		sprintf( value, "VALUE%d", i+1 );
		status = fields_add( &f, tag, value, i );
		check( status==FIELDS_OK, "fields_add() did not return FIELDS_OK" );
	}

	if ( fields_maxlevel( &f ) != n-1 ) {
		fprintf( stderr, "Failed %s in %s() line %d\n", "fields_maxlevel( &f ) == n-1", __FUNCTION__, __LINE__ );
		return 1;
	}

	fields_free( &f );

	return 0;
}

/*
 * fields_dupl() allocates a new fields structure and copies all values
 */
int
test_dupl_basic( void )
{
	int status;
	fields f, *dup;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	dup = fields_dupl( &f );
	if ( !dup ) memerr( __FUNCTION__ );

	check_len( dup, 5 );

	check_entry( dup, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( dup, 1, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( dup, 2, "TAG3", "VALUE3", LEVEL_SERIES );
	check_entry( dup, 3, "TAG4", "VALUE4", LEVEL_ORIG );
	check_entry( dup, 4, "TAG5", "VALUE5", LEVEL_MAIN );

	fields_free( &f );
	fields_delete( dup );

	return 0;
}

int
test_dupl_independent( void )
{
	int status;
	fields f, *dup;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check_len( &f, 5 );

	dup = fields_dupl( &f );
	if ( !dup ) memerr( __FUNCTION__ );

	check_len( dup, 5 );

	/* alter values in f */
	status = fields_remove( &f, 3 );
	check( status==FIELDS_OK, "fields_remove() did not return FIELDS_OK" );

	status = fields_replace_or_add( &f, "TAG1", "VALUE11", LEVEL_MAIN );
	check( status==FIELDS_OK, "fields_replace_or_add() did not return FIELDS_OK" );

	/* ensure that values are unchanged in dup */
	check_entry( dup, 0, "TAG1", "VALUE1", LEVEL_MAIN );
	check_entry( dup, 1, "TAG2", "VALUE2", LEVEL_HOST );
	check_entry( dup, 2, "TAG3", "VALUE3", LEVEL_SERIES );
	check_entry( dup, 3, "TAG4", "VALUE4", LEVEL_ORIG );
	check_entry( dup, 4, "TAG5", "VALUE5", LEVEL_MAIN );

	fields_free( &f );
	fields_delete( dup );

	return 0;
}

int
test_dupl( void )
{
	int err = 0;
	err += test_dupl_basic();
	err += test_dupl_independent();
	return err;
}

/* test_used() check used flag for entries */
int
test_used_set( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	fields_set_used( &f, 0 );
	fields_set_used( &f, 2 );
	fields_set_used( &f, 4 );

	check( fields_used( &f, 0 )==1, "ERR" );
	check( fields_used( &f, 1 )==0, "ERR" );
	check( fields_used( &f, 2 )==1, "ERR" );
	check( fields_used( &f, 3 )==0, "ERR" );
	check( fields_used( &f, 4 )==1, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_used_clear( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	fields_set_used( &f, 0 );
	fields_set_used( &f, 2 );
	fields_set_used( &f, 4 );

	fields_clear_used( &f );

	check( fields_used( &f, 0 )==0, "ERR" );
	check( fields_used( &f, 1 )==0, "ERR" );
	check( fields_used( &f, 2 )==0, "ERR" );
	check( fields_used( &f, 3 )==0, "ERR" );
	check( fields_used( &f, 4 )==0, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_used( void )
{
	int err = 0;
	err += test_used_set();
	err += test_used_clear();
	return err;
}

int
test_match_level( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check( fields_match_level( &f, 0, LEVEL_MAIN )==1, "ERR" );
	check( fields_match_level( &f, 0, LEVEL_HOST )==0, "ERR" );
	check( fields_match_level( &f, 0, LEVEL_SERIES )==0, "ERR" );
	check( fields_match_level( &f, 0, LEVEL_ORIG )==0, "ERR" );
	check( fields_match_level( &f, 0, LEVEL_ANY  )==1, "ERR" );

	check( fields_match_level( &f, 1, LEVEL_MAIN )==0, "ERR" );
	check( fields_match_level( &f, 1, LEVEL_HOST )==1, "ERR" );
	check( fields_match_level( &f, 1, LEVEL_SERIES )==0, "ERR" );
	check( fields_match_level( &f, 1, LEVEL_ORIG )==0, "ERR" );
	check( fields_match_level( &f, 1, LEVEL_ANY  )==1, "ERR" );

	check( fields_match_level( &f, 2, LEVEL_MAIN )==0, "ERR" );
	check( fields_match_level( &f, 2, LEVEL_HOST )==0, "ERR" );
	check( fields_match_level( &f, 2, LEVEL_SERIES )==1, "ERR" );
	check( fields_match_level( &f, 2, LEVEL_ORIG )==0, "ERR" );
	check( fields_match_level( &f, 2, LEVEL_ANY  )==1, "ERR" );

	check( fields_match_level( &f, 3, LEVEL_MAIN )==0, "ERR" );
	check( fields_match_level( &f, 3, LEVEL_HOST )==0, "ERR" );
	check( fields_match_level( &f, 3, LEVEL_SERIES )==0, "ERR" );
	check( fields_match_level( &f, 3, LEVEL_ORIG )==1, "ERR" );
	check( fields_match_level( &f, 3, LEVEL_ANY  )==1, "ERR" );

	check( fields_match_level( &f, 4, LEVEL_MAIN )==1, "ERR" );
	check( fields_match_level( &f, 4, LEVEL_HOST )==0, "ERR" );
	check( fields_match_level( &f, 4, LEVEL_SERIES )==0, "ERR" );
	check( fields_match_level( &f, 4, LEVEL_ORIG )==0, "ERR" );
	check( fields_match_level( &f, 4, LEVEL_ANY  )==1, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_match_tag( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check( fields_match_tag( &f, 0, "TAG1" )==1, "ERR" );
	check( fields_match_tag( &f, 0, "TAG2" )==0, "ERR" );
	check( fields_match_tag( &f, 0, "tag1" )==0, "ERR" );

	check( fields_match_tag( &f, 1, "TAG1" )==0, "ERR" );
	check( fields_match_tag( &f, 1, "TAG2" )==1, "ERR" );
	check( fields_match_tag( &f, 1, "tag2" )==0, "ERR" );

	check( fields_match_tag( &f, 2, "TAG1" )==0, "ERR" );
	check( fields_match_tag( &f, 2, "TAG3" )==1, "ERR" );
	check( fields_match_tag( &f, 2, "tag3" )==0, "ERR" );

	check( fields_match_tag( &f, 3, "TAG1" )==0, "ERR" );
	check( fields_match_tag( &f, 3, "TAG4" )==1, "ERR" );
	check( fields_match_tag( &f, 3, "tag4" )==0, "ERR" );

	check( fields_match_tag( &f, 4, "TAG1" )==0, "ERR" );
	check( fields_match_tag( &f, 4, "TAG5" )==1, "ERR" );
	check( fields_match_tag( &f, 4, "tag5" )==0, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_match_casetag( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check( fields_match_casetag( &f, 0, "TAG1" )==1, "ERR" );
	check( fields_match_casetag( &f, 0, "TAG2" )==0, "ERR" );
	check( fields_match_casetag( &f, 0, "tag1" )==1, "ERR" );

	check( fields_match_casetag( &f, 1, "TAG1" )==0, "ERR" );
	check( fields_match_casetag( &f, 1, "TAG2" )==1, "ERR" );
	check( fields_match_casetag( &f, 1, "tag2" )==1, "ERR" );

	check( fields_match_casetag( &f, 2, "TAG1" )==0, "ERR" );
	check( fields_match_casetag( &f, 2, "TAG3" )==1, "ERR" );
	check( fields_match_casetag( &f, 2, "tag3" )==1, "ERR" );

	check( fields_match_casetag( &f, 3, "TAG1" )==0, "ERR" );
	check( fields_match_casetag( &f, 3, "TAG4" )==1, "ERR" );
	check( fields_match_casetag( &f, 3, "tag4" )==1, "ERR" );

	check( fields_match_casetag( &f, 4, "TAG1" )==0, "ERR" );
	check( fields_match_casetag( &f, 4, "TAG5" )==1, "ERR" );
	check( fields_match_casetag( &f, 4, "tag5" )==1, "ERR" );
	
	fields_free( &f );

	return 0;
}

int
test_match_tag_level( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check( fields_match_tag_level( &f, 0, "TAG1", LEVEL_MAIN )==1, "ERR" );
	check( fields_match_tag_level( &f, 0, "TAG2", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_tag_level( &f, 0, "TAG1", LEVEL_HOST )==0, "ERR" );
	check( fields_match_tag_level( &f, 0, "TAG1", LEVEL_SERIES )==0, "ERR" );
	check( fields_match_tag_level( &f, 0, "TAG1", LEVEL_ANY  )==1, "ERR" );
	check( fields_match_tag_level( &f, 0, "TAG2", LEVEL_ANY  )==0, "ERR" );

	check( fields_match_tag_level( &f, 1, "TAG1", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_tag_level( &f, 1, "TAG2", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_tag_level( &f, 1, "TAG2", LEVEL_HOST )==1, "ERR" );
	check( fields_match_tag_level( &f, 1, "TAG2", LEVEL_SERIES )==0, "ERR" );
	check( fields_match_tag_level( &f, 1, "TAG1", LEVEL_ANY  )==0, "ERR" );
	check( fields_match_tag_level( &f, 1, "TAG2", LEVEL_ANY  )==1, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_match_casetag_level( void )
{
	int status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	check( fields_match_casetag_level( &f, 0, "tag1", LEVEL_MAIN )==1, "ERR" );
	check( fields_match_casetag_level( &f, 0, "TAG1", LEVEL_MAIN )==1, "ERR" );
	check( fields_match_casetag_level( &f, 0, "tag2", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_casetag_level( &f, 0, "tag1", LEVEL_HOST )==0, "ERR" );
	check( fields_match_casetag_level( &f, 0, "tag1", LEVEL_SERIES )==0, "ERR" );
	check( fields_match_casetag_level( &f, 0, "tag1", LEVEL_ANY  )==1, "ERR" );
	check( fields_match_casetag_level( &f, 0, "TAG1", LEVEL_ANY  )==1, "ERR" );
	check( fields_match_casetag_level( &f, 0, "tag2", LEVEL_ANY  )==0, "ERR" );

	check( fields_match_casetag_level( &f, 1, "tag1", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_casetag_level( &f, 1, "tag2", LEVEL_MAIN )==0, "ERR" );
	check( fields_match_casetag_level( &f, 1, "tag2", LEVEL_HOST )==1, "ERR" );
	check( fields_match_casetag_level( &f, 1, "TAG2", LEVEL_HOST )==1, "ERR" );
	check( fields_match_casetag_level( &f, 1, "tag2", LEVEL_SERIES )==0, "ERR" );
	check( fields_match_casetag_level( &f, 1, "tag1", LEVEL_ANY  )==0, "ERR" );
	check( fields_match_casetag_level( &f, 1, "tag2", LEVEL_ANY  )==1, "ERR" );
	check( fields_match_casetag_level( &f, 1, "TAG2", LEVEL_ANY  )==1, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_find( void )
{
	int n, status;
	fields f;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "VALUE4", LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	n = fields_find( &f, "TAG1", LEVEL_MAIN );
	check( n==0, "ERR" );

	n = fields_find( &f, "TAG1", LEVEL_ANY );
	check( n==0, "ERR" );

	n = fields_find( &f, "TAG1", LEVEL_HOST );
	check( n==FIELDS_NOTFOUND, "ERR" );

	n = fields_find( &f, "TAG4", LEVEL_ORIG );
	check( n==3, "ERR" );

	n = fields_find( &f, "TAG4", LEVEL_ANY );
	check( n==3, "ERR" );

	n = fields_find( &f, "TAG4", LEVEL_MAIN );
	check( n==FIELDS_NOTFOUND, "ERR" );

	n = fields_find( &f, "NOT_A_TAG", LEVEL_ANY );
	check( n==FIELDS_NOTFOUND, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_findv_chrp_use( void )
{
	int n, status;
	fields f;
	char *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_MAIN, FIELDS_CHRP, "TAG1" );
	check( !strcmp( p, "VALUE1" ), "ERR" );
	check( fields_used( &f, 0 )==1, "ERR" );

	p = fields_findv( &f, LEVEL_ANY,  FIELDS_CHRP, "TAG1" );
	check( !strcmp( p, "VALUE1" ), "ERR" );

	p = fields_findv( &f, LEVEL_HOST, FIELDS_CHRP, "TAG1" );
	check( p==NULL, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_findv_chrp_nouse( void )
{
	int n, status;
	fields f;
	char *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_MAIN, FIELDS_CHRP_NOUSE, "TAG5" );
	check( !strcmp( p, "VALUE5" ), "ERR" );
	check( fields_used( &f, 4 )==0, "ERR" );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_CHRP_NOUSE, "TAG5" );
	check( !strcmp( p, "VALUE5" ), "ERR" );
	check( fields_used( &f, 4 )==0, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_findv_chrp_nolen( void )
{
	int n, status;
	fields f;
	char *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_CHRP, "TAG4" );
	check( p==NULL, "Unless FIELDS_NOLENOK_FLAG set, values with zero length aren't found" );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_CHRP_NOLEN, "TAG4" );
	check( !strcmp( p, "" ), "With FIELDS_NOLENOK_FLAG set, should get entries with zero length" );

	fields_free( &f );

	return 0;
}

int
test_findv_chrp( void )
{
	int err = 0;
	err += test_findv_chrp_use();
	err += test_findv_chrp_nouse();
	err += test_findv_chrp_nolen();
	return err;
}

int
test_findv_strp_use( void )
{
	int n, status;
	fields f;
	str *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_MAIN, FIELDS_STRP, "TAG1" );
	check( !strcmp( str_cstr( p ), "VALUE1" ), "ERR" );
	check( fields_used( &f, 0 )==1, "ERR" );

	p = fields_findv( &f, LEVEL_ANY,  FIELDS_STRP, "TAG1" );
	check( !strcmp( str_cstr( p ), "VALUE1" ), "ERR" );

	p = fields_findv( &f, LEVEL_HOST, FIELDS_STRP, "TAG1" );
	check( p==NULL, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_findv_strp_nouse( void )
{
	int n, status;
	fields f;
	str *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_MAIN, FIELDS_STRP_NOUSE, "TAG5" );
	check( !strcmp( str_cstr( p ), "VALUE5" ), "ERR" );
	check( fields_used( &f, 4 )==0, "ERR" );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_STRP_NOUSE, "TAG5" );
	check( !strcmp( str_cstr( p ), "VALUE5" ), "ERR" );
	check( fields_used( &f, 4 )==0, "ERR" );

	fields_free( &f );

	return 0;
}

int
test_findv_strp_nolen( void )
{
	int n, status;
	fields f;
	str *p;

	status = make_fields_with_unique_content( &f,
			"TAG1", "VALUE1", LEVEL_MAIN,
			"TAG2", "VALUE2", LEVEL_HOST,
			"TAG3", "VALUE3", LEVEL_SERIES,
			"TAG4", "",       LEVEL_ORIG,
			"TAG5", "VALUE5", LEVEL_MAIN,
			NULL );
	if ( status!=FIELDS_OK ) memerr( __FUNCTION__ );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_STRP, "TAG4" );
	check( p==NULL, "Unless FIELDS_NOLENOK_FLAG set, values with zero length aren't found" );

	p = fields_findv( &f, LEVEL_ANY, FIELDS_STRP_NOLEN, "TAG4" );
	check( str_is_empty( p )==1, "With FIELDS_NOLENOK_FLAG set, should get entries with zero length" );

	fields_free( &f );

	return 0;
}

int
test_findv_strp( void )
{
	int err = 0;
	err += test_findv_strp_use();
	err += test_findv_strp_nouse();
	err += test_findv_strp_nolen();
	return err;
}

int
test_findv( void )
{
	int err = 0;
	err += test_findv_chrp();
	err += test_findv_strp();
	return err;
}

int
main( int argc, char *argv[] )
{
	int failed = 0;

	failed += test_init();
	failed += test_new();

	failed += test_add();
	failed += test_add_can_dup();
	failed += test_add_suffix();
	failed += test_add_suffix_can_dup();

	failed += test_replace_or_add();

	failed += test_remove();

	failed += test_maxlevel();

	failed += test_dupl();

	failed += test_used();

	failed += test_match_level();
	failed += test_match_tag();
	failed += test_match_casetag();
	failed += test_match_tag_level();
	failed += test_match_casetag_level();

	failed += test_find();
	failed += test_findv();

	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
