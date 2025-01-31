/*
 * uintlist.h
 *
 * Copyright (c) Chris Putnam 2007-2021
 *
 * Version 01/12/2020
 *
 * Source code released under the GPL version 2
 *
 */

#ifndef UINTLIST_H
#define UINTLIST_H

#define UINTLIST_OK            (0)
#define UINTLIST_MEMERR        (-1)
#define UINTLIST_VALUE_MISSING (-2)

typedef struct uintlist {
	int n, max;
	unsigned int *data;
} uintlist;

void      uintlist_init( uintlist *il );
int       uintlist_init_fill( uintlist *il, int n, unsigned int value );
int       uintlist_init_range( uintlist *il, unsigned int low, unsigned int high, int step );
uintlist * uintlist_new( void );
uintlist * uintlist_new_fill( int n, unsigned int value );
uintlist * uintlist_new_range( unsigned int low, unsigned int high, int step );
void      uintlist_delete( uintlist *il );
void      uintlist_sort( uintlist *il );
void      uintlist_randomize( uintlist *il );
int       uintlist_add( uintlist *il, unsigned int value );
int       uintlist_add_unique( uintlist *il, unsigned int value );
int       uintlist_fill( uintlist *il, int n, unsigned int value );
int       uintlist_fill_range( uintlist *il, unsigned int low, unsigned int high, int step );
int       uintlist_find( uintlist *il, unsigned int searchvalue );
int       uintlist_find_or_add( uintlist *il, unsigned int searchvalue );
void      uintlist_empty( uintlist *il );
void      uintlist_free( uintlist *il );
int       uintlist_copy( uintlist *to, uintlist *from );
uintlist * uintlist_dup( uintlist *from );
int       uintlist_get( uintlist *il, int pos );
int       uintlist_set( uintlist *il, int pos, unsigned int value );
int       uintlist_remove( uintlist *il, unsigned int searchvalue );
int       uintlist_remove_pos( uintlist *il, int pos );
int       uintlist_append( uintlist *to, uintlist *from );
int       uintlist_append_unique( uintlist *to, uintlist *from );
float     uintlist_median( uintlist *il );
float     uintlist_mean( uintlist *il );

#endif
