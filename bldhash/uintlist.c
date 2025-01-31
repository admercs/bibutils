/*
 * uintlist.c
 *
 * Copyright (c) Chris Putnam 2007-2021
 *
 * Version 1/12/2017
 *
 * Source code released under the GPL version 2
 *
 * Implements a simple managed array of ints
 *
 */
#include <stdlib.h>
#include <assert.h>
#include "uintlist.h"

#define UINTLIST_MINALLOC (20)

static int
uintlist_validn( uintlist *il, int n )
{
	if ( n < 0 || n >= il->n ) return 0;
	return 1;
}

int
uintlist_wasfound( uintlist *il, int n )
{
	if ( n!=-1 ) return 1;
	else return 0;
}

int
uintlist_wasnotfound( uintlist *il, int n )
{
	if ( n==-1 ) return 1;
	else return 0;
}

static int
uintlist_alloc( uintlist *il, int alloc_size )
{
	il->data = ( unsigned int * ) calloc( alloc_size, sizeof( unsigned int ) );
	if ( !(il->data) ) return UINTLIST_MEMERR;
	il->max = alloc_size;
	il->n   = 0;
	return UINTLIST_OK;
}

static int
uintlist_realloc( uintlist *il, int alloc_size )
{
	unsigned int *more;
	int i;

	more = ( unsigned int * ) realloc( il->data, sizeof( unsigned int ) * alloc_size );
	if ( !more ) return UINTLIST_MEMERR;

	il->data = more;
	il->max  = alloc_size;

	for ( i=il->max; i<alloc_size; ++i )
		il->data[i] = 0;

	return UINTLIST_OK;
}

static int
uintlist_ensure_space( uintlist *il, int n )
{
	int alloc = n;

	if ( il->max == 0 ) {
		if ( alloc < UINTLIST_MINALLOC ) alloc = UINTLIST_MINALLOC;
		return uintlist_alloc( il, alloc );
	}

	else if ( il->max <= n ) {
		if ( alloc < il->max * 2 ) alloc = il->max * 2;
		return uintlist_realloc( il, alloc );
	}

	return UINTLIST_OK;
}

/* uintlist_add()
 *
 * Returns UINTLIST_OK/UINTLIST_MEMERR
 */
int
uintlist_add( uintlist *il, unsigned int value )
{
	int status;

	assert( il );

	status = uintlist_ensure_space( il, il->n+1 );

	if ( status == UINTLIST_OK ) {
		il->data[ il->n ] = value;
		il->n++;
	}

	return status;
}

/* uintlist_add_unique()
 *
 * Returns UINTLIST_OK/UINTLIST_MEMERR
 */
int
uintlist_add_unique( uintlist *il, unsigned int value )
{
	int n;

	assert( il );

	n = uintlist_find( il, value );
	if ( uintlist_wasnotfound( il, n ) )
		return uintlist_add( il, value );
	else
		return UINTLIST_OK;
}

int
uintlist_find_or_add( uintlist *il, unsigned int value )
{
	int n, status;

	n = uintlist_find( il, value );

	if ( uintlist_wasfound( il, n ) ) {
		return n;
	}

	else {
		status = uintlist_add( il, value );
		if ( status!=UINTLIST_OK ) return -1;
		else return il->n - 1;
	}
}

/* uintlist_find()
 *
 * Returns position of value in range [0,n), or -1 if
 * value cannot be found
 */
int
uintlist_find( uintlist *il, unsigned int value )
{
	int i;

	assert( il );

	for ( i=0; i<il->n; ++i )
		if ( il->data[i]==value ) return i;

	return -1;
}

static int
uintlist_remove_pos_core( uintlist *il, int pos )
{
	int i;

	assert( il );

	for ( i=pos; i<il->n-1; ++i )
		il->data[i] = il->data[i+1];
	il->n -= 1;

	return UINTLIST_OK;
}

/* uintlist_remove_pos()
 *
 * Returns UINTLIST_OK on success.
 */
int
uintlist_remove_pos( uintlist *il, int pos )
{
	assert( il );
	assert( uintlist_validn( il, pos ) );

	return uintlist_remove_pos_core( il, pos );
}

/* uintlist_remove()
 *
 * Removes first instance of value from the uintlist.
 * Returns UINTLIST_OK/UINTLIST_VALUE_MISSING
 */
int
uintlist_remove( uintlist *il, unsigned int value )
{
	int pos;

	assert( il );

	pos = uintlist_find( il, value );
	if ( pos==-1 ) return UINTLIST_VALUE_MISSING;

	return uintlist_remove_pos_core( il, pos );
}

/* don't actually free space, just reset counter */
void
uintlist_empty( uintlist *il )
{
	assert( il );

	il->n = 0;
}

void
uintlist_free( uintlist *il )
{
	assert( il );

	if ( il->data ) free( il->data );
	uintlist_init( il );
}

void
uintlist_delete( uintlist *il )
{
	assert( il );

	if ( il->data ) free( il->data );
	free( il );
}

void
uintlist_init( uintlist *il )
{
	assert( il );

	il->data = NULL;
	il->max = 0;
	il->n = 0;
}

/* Returns UINTLIST_OK/UINTLIST_MEMERR
 */
int
uintlist_init_fill( uintlist *il, int n, unsigned int v )
{
	uintlist_init( il );
	return uintlist_fill( il, n, v );
}

/* uintlist_init_range()
 *
 * Initializes uintlist to values from [low,high) with step step.
 * Returns UINTLIST_OK/UINTLIST_MEMERR.
 */
int
uintlist_init_range( uintlist *il, unsigned int low, unsigned int high, int step )
{
	uintlist_init( il );
	return uintlist_fill_range( il, low, high, step );
}

/* uintlist_new()
 *
 * Allocates an empty uintlist.
 * Returns pointer to uintlist on success, NULL on memory error.
 */
uintlist *
uintlist_new( void )
{
	uintlist *il;
	il = ( uintlist * ) malloc( sizeof( uintlist ) );
	if ( il ) uintlist_init( il );
	return il;
}

/* uintlist_new_range()
 *
 * Allocates a uintlist initialized to values from [low,high) in increments of step.
 * Returns pointer to uintlist on success, NULL on memory error.
 */
uintlist *
uintlist_new_range( unsigned int low, unsigned int high, int step )
{
	uintlist *il;
	int status;

	il = uintlist_new();
	if ( il ) {
		status = uintlist_fill_range( il, low, high, step );
		if ( status==UINTLIST_MEMERR ) {
			uintlist_free( il );
			free( il );
			il = NULL;
		}
	}
	return il;
}

/* uintlist_new_range()
 *
 * Allocates a uintlist initialized to n elements with value v.
 * Returns pointer to uintlist on success, NULL on memory error.
 */
uintlist *
uintlist_new_fill( int n, unsigned int v )
{
	uintlist *il;
	int status;

	il = uintlist_new();
	if ( il ) {
		status = uintlist_fill( il, n, v );
		if ( status==UINTLIST_MEMERR ) {
			uintlist_free( il );
			free( il );
			il = NULL;
		}
	}
	return il;
}

/* uintlist_fill()
 *
 * Fill an uintlist with n elements of value v.
 *
 * Returns UINTLIST_OK or UINTLIST_MEMERR.
 */
int
uintlist_fill( uintlist *il, int n, unsigned int v )
{
	int i, status;

	assert ( n > 0 );

	status = uintlist_ensure_space( il, n );

	if ( status==UINTLIST_OK ) {

		for ( i=0; i<n; ++i )
			il->data[i] = v;

		il->n = n;

	}

	return status;
}

/* uintlist_fill_range()
 *
 * Fill an uintlist with the values [low,high) in increments of step
 *
 * Returns UINTLIST_OK or UINTLIST_MEMERR.
 */
int
uintlist_fill_range( uintlist *il, unsigned int low, unsigned int high, int step )
{
	int i, n, status;

	n = ( high - low ) / step + 1;

	assert ( n > 0 );

	status = uintlist_ensure_space( il, n );

	if ( status==UINTLIST_OK ) {

		il->n = 0;

		/* ...fill uintlist with range */
		if ( step > 0 ) {
			for ( i=low; i<high; i+=step ) {
				il->data[il->n] = i;
				il->n += 1;
			}
		}
		else {
			for ( i=low; i>high; i+=step ) {
				il->data[il->n] = i;
				il->n += 1;
			}
		}

	}

	return status;
}

static int
intcomp( const void *v1, const void *v2 )
{
	int *i1 = ( int * ) v1;
	int *i2 = ( int * ) v2;
	if ( *i1 < *i2 ) return -1;
	else if ( *i1 > *i2 ) return 1;
	return 0;
}

void
uintlist_sort( uintlist *il )
{
	assert( il );

	qsort( il->data, il->n, sizeof( int ), intcomp );
}

/* Returns random integer in the range [floor,ceil) */
static int
randomint( int floor, int ceil )
{
	int len = ceil - floor;
	return floor + rand() % len;
}

static void
swap( unsigned int *a, unsigned int *b )
{
	unsigned int tmp;
	tmp = *a;
	*a = *b;
	*b = tmp;
}

void
uintlist_randomize( uintlist *il )
{
	unsigned int i, j;

	assert( il );

	if ( il->n < 2 ) return;
	for ( i=0; i<il->n; ++i ) {
		j = randomint( i, il->n );
		if ( i==j ) continue;
		swap( &(il->data[i]), &(il->data[j]) );
	}
}

/* Returns UINTLIST_OK/UINTLIST_MEMERR */
int
uintlist_copy( uintlist *to, uintlist *from )
{
	int i, status;

	assert( to );
	assert( from );

	status = uintlist_ensure_space( to, from->n );

	if ( status==UINTLIST_OK ) {

		to->n = from->n;

		for ( i=0; i<from->n; ++i )
			to->data[i] = from->data[i];

	}

	return status;
}

/* Returns pointer on success, NULL on error */
uintlist *
uintlist_dup( uintlist *il )
{
	uintlist *l;
	int status;

	assert( il );

	l = uintlist_new();
	if ( l ) {
		status = uintlist_copy( l, il );
		if ( status==UINTLIST_MEMERR ) {
			uintlist_delete( l );
			l = NULL;
		}
	}

	return l;
}

int
uintlist_append( uintlist *to, uintlist *from )
{
	int i, status;

	assert( to );
	assert( from );

	status = uintlist_ensure_space( to, to->n + from->n );

	if ( status == UINTLIST_OK ) {

		for ( i=0; i<from->n; ++i )
			to->data[ to->n + i ] = from->data[ i ];

		to->n += from->n;
	}

	return status;
}

int
uintlist_append_unique( uintlist *to, uintlist *from )
{
	int i, nsave, status = UINTLIST_OK;

	assert( to );
	assert( from );

	nsave = to->n;
	for ( i=0; i<from->n; ++i ) {
		if ( uintlist_find( to, from->data[i] )!=-1 ) continue;
		status = uintlist_add( to, from->data[i] );
		if ( status==UINTLIST_MEMERR ) {
			to->n = nsave;
		}
	}
	return status;
}

int
uintlist_get( uintlist *il, int pos )
{
	assert( il );
	assert( uintlist_validn( il, pos ) );

	return il->data[pos];
}

/* uintlist_set()
 *
 *   Returns UINTLIST_OK
 */
int
uintlist_set( uintlist *il, int pos, unsigned int value )
{
	assert( il );
	assert( uintlist_validn( il, pos ) );

	il->data[pos] = value;
	return UINTLIST_OK;
}

float
uintlist_median( uintlist *il )
{
	uintlist *tmp;
	float median;
	int m1, m2;

	assert( il );

	if ( il->n==0 ) return 0.0;

	tmp = uintlist_dup( il );
	if ( !tmp ) return 0.0;

	uintlist_sort( tmp );

	if ( tmp->n % 2 == 1 ) {
		median = uintlist_get( tmp, tmp->n / 2 );
	} else {
		m1 = uintlist_get( tmp, tmp->n / 2 );
		m2 = uintlist_get( tmp, tmp->n / 2 - 1);
		median = ( m1 + m2 ) / 2.0;
	}

	uintlist_delete( tmp );

	return median;
}

float
uintlist_mean( uintlist *il )
{
	float sum = 0.0;
	int i;

	assert( il );

	if ( il->n==0 ) return 0.0;

	for ( i=0; i<il->n; ++i )
		sum += uintlist_get( il, i );

	return sum / il->n;
}
