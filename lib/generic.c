/* generic.c
 *
 * Copyright (c) Chris Putnam 2016-2021
 *
 * Source code released under GPL version 2
 *
 * xxxx_convertf() stubs that can be shared.
 */
#include "bu_auth.h"
#include "marc_auth.h"
#include "name.h"
#include "notes.h"
#include "pages.h"
#include "serialno.h"
#include "title.h"
#include "url.h"
#include "utf8.h"
#include "generic.h"

/* stub for processtypes that aren't used, such as DEFAULT and ALWAYS handled by bibcore.c  */
int
generic_null( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	return BIBL_OK;
}

int
generic_url( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	return urls_split_and_add( str_cstr( invalue ), bibout, level );
}

int
generic_notes( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        return add_notes( bibout, invalue, level );
}

int
generic_pages( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	if ( is_doi( str_cstr( invalue ) )!=-1 ) return generic_url( bibin, n, intag, invalue, level, pm, outtag, bibout );
	else return add_pages( bibout, invalue, level );
}

int
generic_person( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        return add_name( bibout, outtag, str_cstr( invalue ), level, &(pm->asis), &(pm->corps) );
}

int
generic_serialno( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	return add_sn( bibout, str_cstr( invalue ), level );
}

/* SIMPLE = just copy */
int
generic_simple( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	if ( fields_add( bibout, outtag, str_cstr( invalue ), level ) == FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

/* just like generic_null(), but useful if we need one that isn't identical to generic_null() ala biblatexin.c */
int
generic_skip( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	fields_set_used( bibin, n );
	return BIBL_OK;
}

int
generic_title( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
        return add_title( bibout, outtag, str_cstr( invalue ), level, pm->nosplittitle );
}

int
generic_genre( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int status;

	if ( is_marc_genre( str_cstr( invalue ) ) )
		status = fields_add( bibout, "GENRE:MARC", str_cstr( invalue ), level );

	else if ( is_bu_genre( str_cstr( invalue ) ) )
		status = fields_add( bibout, "GENRE:BIBUTILS", str_cstr( invalue ), level );

	else
		status = fields_add( bibout, "GENRE:UNKNOWN", str_cstr( invalue ), level );

	if ( status == FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

void
generic_writeheader( FILE *outptr, param *pm )
{
	if ( pm->utf8bom ) utf8_writebom( outptr );
}
