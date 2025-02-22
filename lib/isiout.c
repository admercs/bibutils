/*
 * isiout.c
 *
 * Copyright (c) Chris Putnam 2008-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "append_easy.h"
#include "bibformats.h"
#include "bibutils.h"
#include "fields.h"
#include "generic.h"
#include "month.h"
#include "str.h"
#include "title.h"
#include "type.h"
#include "utf8.h"

/*****************************************************
 PUBLIC: int isiout_initparams()
*****************************************************/

static int  isiout_write( fields *info, FILE *fp, param *p, unsigned long refnum );
static int  isiout_assemble( fields *in, fields *out, param *pm, unsigned long refnum );

int
isiout_initparams( param *pm, const char *progname )
{
	pm->writeformat      = BIBL_ISIOUT;
	pm->format_opts      = 0;
	pm->charsetout       = BIBL_CHARSET_DEFAULT;
	pm->charsetout_src   = BIBL_SRC_DEFAULT;
	pm->latexout         = 0;
	pm->utf8out          = BIBL_CHARSET_UTF8_DEFAULT;
	pm->utf8bom          = BIBL_CHARSET_BOM_DEFAULT;
	pm->xmlout           = BIBL_XMLOUT_FALSE;
	pm->nosplittitle     = 0;
	pm->verbose          = 0;
	pm->addcount         = 0;
	pm->singlerefperfile = 0;

	if ( pm->charsetout == BIBL_CHARSET_UNICODE ) {
		pm->utf8out = pm->utf8bom = 1;
	}

	pm->headerf   = generic_writeheader;
	pm->footerf   = NULL;
	pm->assemblef = isiout_assemble;
	pm->writef    = isiout_write;

	if ( !pm->progname ) {
		if ( !progname ) pm->progname = NULL;
		else {
			pm->progname = strdup( progname );
			if ( !pm->progname ) return BIBL_ERR_MEMERR;
		}
	}

	return BIBL_OK;
}

/*****************************************************
 PUBLIC: int isiout_assemble()
*****************************************************/

enum {
        TYPE_UNKNOWN = 0,
        TYPE_ARTICLE = 1,
        TYPE_INBOOK  = 2,
        TYPE_BOOK    = 3,
};

static int 
get_type( fields *in )
{
	match_type genre_matches[] = {
		{ "periodical",         TYPE_ARTICLE,        LEVEL_ANY  },
		{ "academic journal",   TYPE_ARTICLE,        LEVEL_ANY  },
		{ "journal article",    TYPE_ARTICLE,        LEVEL_ANY  },
		{ "book",               TYPE_BOOK,           LEVEL_MAIN },
		{ "book",               TYPE_INBOOK,         LEVEL_ANY  },
		{ "book chapter",       TYPE_INBOOK,         LEVEL_ANY  },
		{ "collection",         TYPE_BOOK,           LEVEL_MAIN },
		{ "collection",         TYPE_INBOOK,         LEVEL_ANY  },
	};

	int ngenre_matches = sizeof( genre_matches ) / sizeof( genre_matches[0] );

	match_type issuance_matches[] = {
		{ "monographic",        TYPE_BOOK,           LEVEL_MAIN },
		{ "monographic",        TYPE_INBOOK,         LEVEL_ANY  },
	};
	int nissuance_matches = sizeof( issuance_matches ) / sizeof( issuance_matches[0] );

	int type;

	type = type_from_mods_hints( in, TYPE_FROM_GENRE, genre_matches, ngenre_matches, TYPE_UNKNOWN );
	if ( type!=TYPE_UNKNOWN ) return type;

	return type_from_mods_hints( in, TYPE_FROM_ISSUANCE, issuance_matches, nissuance_matches, TYPE_UNKNOWN );
}

static void
append_type( int type, fields *out, int *status )
{
	int fstatus;
	char *s;

	switch( type ) {
		case TYPE_ARTICLE: s = "Journal"; break;
		case TYPE_INBOOK:  s = "Chapter"; break;
		case TYPE_BOOK:    s = "Book";    break;
		default:           s = "Unknown"; break;
	}

	fstatus = fields_add( out, "PT", s, LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
}

static void
append_titlecore( fields *in, char *isitag, int level, fields *out, char *maintag, char *subtag, int *status )
{
	str *mainttl = fields_findv( in, level, FIELDS_STRP, maintag );
	str *subttl  = fields_findv( in, level, FIELDS_STRP, subtag );
	str fullttl;
	int fstatus;

	str_init( &fullttl );
	title_combine( &fullttl, mainttl, subttl );

	if ( str_memerr( &fullttl ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( str_has_value( &fullttl ) ) {
		fstatus = fields_add( out, isitag, str_cstr( &fullttl ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
out:
	str_free( &fullttl );
}

static void
append_title( fields *in, char *isitag, int level, fields *out, int *status )
{
	append_titlecore( in, isitag, level, out, "TITLE", "SUBTITLE", status );
}

static void
append_abbrtitle( fields *in, char *isitag, int level, fields *out, int *status )
{
	append_titlecore( in, isitag, level, out, "SHORTTITLE", "SHORTSUBTITLE", status );
}

static void
process_person( str *person, char *name )
{
	str family, given, suffix;
	char *p = name;

	str_empty( person );

	strs_init( &family, &given, &suffix, NULL );

	while ( *p && *p!='|' )
		str_addchar( &family, *p++ );

	while ( *p=='|' && *(p+1)!='|' ) {
		p++;
		if ( *p!='|' ) str_addchar( &given, *p++ );
		while ( *p && *p!='|' ) p++;
	}

	if ( *p=='|' && *(p+1)=='|' ) {
		p += 2;
		while ( *p && *p!='|' ) str_addchar( &suffix, *p++ );
	}

	if ( str_has_value( &family ) ) str_strcat( person, &family );
	if ( str_has_value( &suffix ) ) {
		if ( str_has_value( &family ) ) str_strcatc( person, " " );
		str_strcat( person, &suffix );
	}
	if ( str_has_value( &given ) ) {
		if ( str_has_value( person ) ) str_strcatc( person, ", " );
		str_strcat( person, &given );
	}

	strs_free( &family, &given, &suffix, NULL );
}

static void
append_people( fields *f, char *tag, int level, fields *out, char *isitag, int *status )
{
	vplist_index i;
	vplist people;
	str person;
	int fstatus;

	str_init( &person );
	vplist_init( &people );

	fields_findv_each( f, level, FIELDS_CHRP, &people, tag );
	for ( i=0; i<people.n; ++i ) {
		process_person( &person, (char *)vplist_get( &people, i ) );
		if ( str_memerr( &person ) ) { *status = BIBL_ERR_MEMERR; goto out; }
		if ( i==0 ) fstatus = fields_add_can_dup( out, isitag, str_cstr( &person ), LEVEL_MAIN );
		else        fstatus = fields_add_can_dup( out, "  ",   str_cstr( &person ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) { *status = BIBL_ERR_MEMERR; goto out; }
	}

out:
	vplist_free( &people );
	str_free( &person );
}

/* convert any em-dash or en-dash to a simple dash */
static void
append_page( fields *in, const char *intag, int levelin, fields* out, const char *outtag, int *status )
{
	char *value;

	value = fields_findv( in, levelin, FIELDS_CHRP, intag );
	if ( value ) {
		*status = append_easypage( out, outtag, value, LEVEL_MAIN );
	}
}

static void
append_date( fields *in, fields *out, int *status )
{
	const char *month, *year, *use;
	int fstatus;

	month = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "PARTDATE:MONTH", "DATE:MONTH", NULL );
	if ( month ) {
		(void) number_to_full_month( month, &use );
		fstatus = fields_add( out, "PD", use, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	year  = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "PARTDATE:YEAR",  "DATE:YEAR",  NULL );
	if ( year ) {
		fstatus = fields_add( out, "PY", year, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static int
isiout_assemble( fields *in, fields *out, param *pm, unsigned long refnum )
{
	int type, status = BIBL_OK;

	type = get_type( in );

	append_type     ( type, out, &status );
	append_people   ( in, "AUTHOR",            LEVEL_MAIN, out, "AU", &status );
	append_easyall  ( in, "AUTHOR:CORP",       LEVEL_MAIN, out, "AU", &status );
	append_easyall  ( in, "AUTHOR:ASIS",       LEVEL_MAIN, out, "AU", &status );

	append_title  ( in, "TI", LEVEL_MAIN, out, &status );
	if ( type==TYPE_ARTICLE ) {
		append_title    ( in, "SO", LEVEL_HOST,   out, &status );
		append_abbrtitle( in, "JI", LEVEL_HOST,   out, &status );
		append_title    ( in, "SE", LEVEL_SERIES, out, &status );
	} else if ( type==TYPE_INBOOK ) {
		append_title    ( in, "BT", LEVEL_HOST,   out, &status );
		append_title    ( in, "SE", LEVEL_SERIES, out, &status );
	} else { /* type==BOOK */
		append_title    ( in, "SE", LEVEL_HOST,   out, &status );
	}

	append_date     ( in, out, &status );

	append_page     ( in, "PAGES:START",       LEVEL_ANY, out, "BP", &status );
	append_page     ( in, "PAGES:STOP",        LEVEL_ANY, out, "EP", &status );
	append_page     ( in, "ARTICLENUMBER",     LEVEL_ANY, out, "AR", &status );
	append_page     ( in, "PAGES:TOTAL",       LEVEL_ANY, out, "PG", &status );

	append_easy     ( in, "VOLUME",            LEVEL_ANY, out, "VL", &status );
	append_easy     ( in, "ISSUE",             LEVEL_ANY, out, "IS", &status );
	append_easy     ( in, "NUMBER",            LEVEL_ANY, out, "IS", &status );
	append_easy     ( in, "PUBLISHER",         LEVEL_ANY, out, "PU", &status );
	append_easycombo( in, "ADDRESS:PUBLISHER", LEVEL_ANY, out, "PA", "; ", &status );
	append_easy     ( in, "DOI",               LEVEL_ANY, out, "DI", &status );
	append_easy     ( in, "URL",               LEVEL_ANY, out, "WP", &status );
	append_easy     ( in, "ISIREFNUM",         LEVEL_ANY, out, "UT", &status );
	append_easy     ( in, "LANGUAGE",          LEVEL_ANY, out, "LA", &status );
	append_easy     ( in, "ISIDELIVERNUM",     LEVEL_ANY, out, "GA", &status );
	append_easycombo( in, "KEYWORD",           LEVEL_ANY, out, "DE", "; ", &status );
	append_easy     ( in, "ISBN",              LEVEL_ANY, out, "SN", &status );
	append_easy     ( in, "ISSN",              LEVEL_ANY, out, "SN", &status );
	append_easy     ( in, "ABSTRACT",          LEVEL_ANY, out, "AB", &status );
	append_easy     ( in, "TIMESCITED",        LEVEL_ANY, out, "TC", &status );
	append_easy     ( in, "NUMBERREFS",        LEVEL_ANY, out, "NR", &status );
	append_easy     ( in, "CITEDREFS",         LEVEL_ANY, out, "CR", &status );
	append_easycombo( in, "ADDRESS",           LEVEL_ANY, out, "PI", "; ", &status );

	return status;
}

/*****************************************************
 PUBLIC: int isiout_write()
*****************************************************/

static int
isiout_write( fields *out, FILE *fp, param *p, unsigned long refnum )
{
	int i;

	for ( i=0; i<out->n; ++i ) {
		fprintf( fp, "%s %s\n",
			( char * ) fields_tag  ( out, i, FIELDS_CHRP ),
			( char * ) fields_value( out, i, FIELDS_CHRP )
		);
	}
        fprintf( fp, "ER\n\n" );
        fflush( fp );
	return BIBL_OK;
}
