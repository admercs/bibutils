/*
 * charsets.h
 *
 * Copyright (c) Chris Putnam 2003-2021
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef CHARSETS_H
#define CHARSETS_H

#define CHARSET_UNKNOWN      (-1)
#define CHARSET_UNICODE      (-2)
#define CHARSET_GB18030      (-3)
#define CHARSET_DEFAULT      CHARSET_UNICODE
#define CHARSET_UTF8_DEFAULT (1)
#define CHARSET_BOM_DEFAULT  (1)

char * charset_get_xmlname( int n );
int    charset_find( char *name );
void   charset_list_all( FILE *fp );
unsigned int charset_lookupchar( int charsetin, char c );
unsigned int charset_lookupuni( int charsetout, unsigned int unicode );

#endif
