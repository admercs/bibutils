/*
 * modstypes.c
 *
 * Copyright (c) Chris Putnam 2004-2021
 *
 * Source code released under the GPL version 2
 *      
 */
#include <stdio.h>
#include <string.h>
#include "modstypes.h"

char *
mods_get_id_from_internal( char *internal_name, convert *data, int ndata )
{
	int i;
	for ( i=0; i<ndata; ++i ) {
		if ( !strcasecmp( data[i].internal, internal_name ) )
			return data[i].mods;
	}
	return NULL;
}

char *
mods_get_id_from_mods( char *mods_name, convert *data, int ndata )
{
	int i;
	for ( i=0; i<ndata; ++i ) {
		if ( !strcasecmp( data[i].mods, mods_name ) )
			return data[i].internal;
	}
	return NULL;
}

/* convert2_findallfields()
 *
 *       Find the positions of all convert2.internal tags in the fields
 *       structure and store the locations in convert2.pos element.
 *
 *       Return number of the tags found.
 */
int
convert2_findallfields( fields *f, convert2 *parts, int nparts, int level )
{
        int i, n = 0;

        for ( i=0; i<nparts; ++i ) {
                parts[i].pos = fields_find( f, parts[i].internal, level );
                n += ( parts[i].pos!=FIELDS_NOTFOUND );
        }

        return n;
}
