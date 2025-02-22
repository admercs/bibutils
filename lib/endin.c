/*
 * endin.c
 *
 * Copyright (c) Chris Putnam 2003-2021
 *
 * Program and source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "is_ws.h"
#include "str.h"
#include "str_conv.h"
#include "bibformats.h"
#include "fields.h"
#include "generic.h"
#include "month.h"
#include "reftypes.h"
#include "url.h"

extern variants end_all[];
extern int end_nall;

/*****************************************************
 PUBLIC: void endin_initparams()
*****************************************************/

static int endin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset );
static int endin_processf( fields *endin, const char *p, const char *filename, long nref, param *pm );
int endin_typef( fields *endin, const char *filename, int nrefs, param *p );
int endin_convertf( fields *endin, fields *info, int reftype, param *p );
int endin_cleanf( bibl *bin, param *p );

int
endin_initparams( param *pm, const char *progname )
{
	pm->readformat       = BIBL_ENDNOTEIN;
	pm->charsetin        = BIBL_CHARSET_DEFAULT;
	pm->charsetin_src    = BIBL_SRC_DEFAULT;
	pm->latexin          = 0;
	pm->xmlin            = 0;
	pm->utf8in           = 0;
	pm->nosplittitle     = 0;
	pm->verbose          = 0;
	pm->addcount         = 0;
	pm->output_raw       = 0;

	pm->readf    = endin_readf;
	pm->processf = endin_processf;
	pm->cleanf   = endin_cleanf;
	pm->typef    = endin_typef;
	pm->convertf = endin_convertf;
	pm->all      = end_all;
	pm->nall     = end_nall;

	slist_init( &(pm->asis) );
	slist_init( &(pm->corps) );

	if ( !progname ) pm->progname = NULL;
	else {
		pm->progname = strdup( progname );
		if ( !pm->progname ) return BIBL_ERR_MEMERR;
	}

	return BIBL_OK;
}


/*****************************************************
 PUBLIC: int endin_readf()
*****************************************************/

/* Endnote tag definition:
    character 1 = '%'
    character 2 = alphabetic character or digit (or other characters)
    character 3 = space (ansi 32)
*/
static int
endin_istag( const char *buf )
{
	const char others[]="!@#$^&*()+=?[~>";
	if ( buf[0]!='%' ) return 0;
	if ( buf[2]!=' ' ) return 0;
	if ( isalpha( (unsigned char)buf[1] ) ) return 1;
	if ( isdigit( (unsigned char)buf[1] ) ) return 1;
	if ( strchr( others, buf[1] ) ) return 1;
	return 0;
}

static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, str *line )
{
	if ( line->len ) return 1;
	else return str_fget( fp, buf, bufsize, bufpos, line );
}

static int
endin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset )
{
	int haveref = 0, inref = 0;
	unsigned char *up;
	char *p;
	*fcharset = CHARSET_UNKNOWN;
	while ( !haveref && readmore( fp, buf, bufsize, bufpos, line ) ) {

		if ( !line->data ) continue;
		p = &(line->data[0]);

		/* Skip <feff> Unicode header information */
		/* <feff> = ef bb bf */
		up = (unsigned char* ) p;
		if ( line->len > 2 && up[0]==0xEF && up[1]==0xBB &&
				up[2]==0xBF ) {
			*fcharset = CHARSET_UNICODE;
			p += 3;
		}

		if ( !*p ) {
			if ( inref ) haveref = 1; /* blank line separates */
			else continue; /* blank line to ignore */
		}
		/* Each reference starts with a tag && ends with a blank line */
		if ( endin_istag( p ) ) {
			if ( reference->len ) str_addchar( reference, '\n' );
			str_strcatc( reference, p );
			inref = 1;
		} else if ( inref && *p ) {
			str_addchar( reference, '\n' );
			str_strcatc( reference, p );
		}
		str_empty( line );
	}
	if ( reference->len ) haveref = 1;
	return haveref;
}

/*****************************************************
 PUBLIC: int endin_processf()
*****************************************************/
static const char*
process_endline( str *tag, str *data, const char *p )
{
	int  i;

	i = 0;
	while ( i<2 && *p ) {
		str_addchar( tag, *p++);
		i++;
	}
	while ( *p==' ' || *p=='\t' ) p++;

	while ( *p && *p!='\r' && *p!='\n' )
		str_addchar( data, *p++ );
	str_trimendingws( data );

	while ( *p=='\r' || *p=='\n' ) p++;

	return p;
}

static const char *
process_endline2( str *tag, str *data, const char *p )
{
	while ( *p==' ' || *p=='\t' ) p++;
	while ( *p && *p!='\r' && *p!='\n' )
		str_addchar( data, *p++ );
	str_trimendingws( data );
	while ( *p=='\r' || *p=='\n' ) p++;
	return p;
}

static int
endin_processf( fields *endin, const char *p, const char *filename, long nref, param *pm )
{
	str tag, value, *oldvalue;
	int status, n;
	char *oldtag;

	strs_init( &tag, &value, NULL );

	while ( *p ) {

		strs_empty( &tag, &value, NULL );

		if ( endin_istag( p ) ) {

			p = process_endline( &tag, &value, p );
			if ( str_is_empty( &value ) ) continue;

			status = fields_add( endin, str_cstr( &tag ), str_cstr( &value ), LEVEL_MAIN );
			if ( status!=FIELDS_OK ) return 0;

		}

		/* endnote puts %K only on 1st line of keywords */
		else {

			p = process_endline2( &tag, &value, p );
			if ( str_is_empty( &value ) ) continue;

			n = fields_num( endin );
			if ( n==0 ) continue; /* no previous line to append to */

			oldtag = fields_tag( endin, n-1, FIELDS_CHRP_NOUSE );

			/* last line was a keyword, so this is a new keyword */
			if ( !strncmp( oldtag, "%K", 2 ) ) {
				status = fields_add( endin, "%K", str_cstr( &value ), LEVEL_MAIN );
				if ( status!=FIELDS_OK ) return 0;
			}

			/* last line wasn't a keyword, so this line should just be appended */
			else {
				oldvalue = fields_value( endin, n-1, FIELDS_STRP_NOUSE );
				str_addchar( oldvalue, ' ' );
				str_strcat( oldvalue, &value );
				if ( str_memerr( oldvalue ) ) return 0;
			}
		}
	}
	strs_free( &tag, &value, NULL );
	return 1;
}

/*****************************************************
 PUBLIC: int endin_typef()
*****************************************************/

/* Endnote defaults if no %0 tag
 *
 * if %J & %V - journal article
 * if %B - book section
 * if %R & !%T - report
 * if %I & !%B & !%J & !%R - book
 * if !%B & !%J & !%R & !%I - journal article
 */
int
endin_typef( fields *endin, const char *filename, int nrefs, param *p )
{
	int ntypename, nrefname, is_default, nj, nv, nb, nr, nt, ni;
	char *refname = "", *typename="";

	ntypename = fields_find( endin, "%0", LEVEL_MAIN );
	nrefname  = fields_find( endin, "%F", LEVEL_MAIN );
	if ( nrefname!=-1  ) refname  = fields_value( endin, nrefname,  FIELDS_CHRP_NOUSE );
	if ( ntypename!=-1 ) typename = fields_value( endin, ntypename, FIELDS_CHRP_NOUSE );
	else {
		nj = fields_find( endin, "%J", 0 );
		nv = fields_find( endin, "%V", 0 );
		nb = fields_find( endin, "%B", 0 );
		nr = fields_find( endin, "%R", 0 );
		nt = fields_find( endin, "%T", 0 );
		ni = fields_find( endin, "%I", 0 );
		if ( nj!=-1 && nv!=-1 ) {
			typename = "Journal Article";
		} else if ( nb!=-1 ) {
			typename = "Book Section";
		} else if ( nr!=-1 && nt==-1 ) {
			typename = "Report";
		} else if ( ni!=-1 && nb==-1 && nj==-1 && nr==-1 ) {
			typename = "Book";
		} else if ( nb==-1 && nj==-1 && nr==-1 && ni==-1 ) {
			typename = "Journal Article";
		}
	}

	return get_reftype( typename, nrefs, p->progname, p->all, p->nall, refname, &is_default, REFTYPE_CHATTY );
}

/*****************************************************
 PUBLIC: void endin_cleanf()
*****************************************************/

/* Wiley puts multiple authors separated by commas on the %A lines.
 * We can detect this by finding the terminal comma in the value
 * from the tag/value pair.
 *
 * "%A" "Author A. X. Last, Author N. B. Next,"
 */
static int
is_wiley_author( fields *endin, int n )
{
	str *t, *v;
	t = fields_tag( endin, n, FIELDS_STRP_NOUSE );
	if ( str_is_empty( t ) || strcmp( str_cstr( t ), "%A" ) ) return 0;
	v = fields_value( endin, n, FIELDS_STRP_NOUSE );
	if ( str_is_empty( v ) ) return 0;
	if ( v->data[v->len-1]!=',' ) return 0;
	return 1;
}

static int
add_wiley_author( fields *endin, char *intag, str *instring, int inlevel, str *name, int authornum )
{
	int fstatus;

	/* if first author, just replace the data string in the field */
	if ( authornum==0 ) {
		str_strcpy( instring, name );
		if ( str_memerr( instring ) ) return BIBL_ERR_MEMERR;
	}

	/* otherwise, append the author */
	else {
		fstatus = fields_add( endin, intag, str_cstr( name ), inlevel );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	return BIBL_OK;
}

static int
cleanup_wiley_author( fields *endin, int n )
{	
	int status=BIBL_OK, inlevel, authornum = 0;
	str *instring, copy, name;
	char *p, *intag;

	strs_init( &copy, &name, NULL );

	intag    = fields_tag  ( endin, n, FIELDS_CHRP_NOUSE );
	instring = fields_value( endin, n, FIELDS_STRP_NOUSE );
	inlevel  = fields_level( endin, n );

	str_strcpy( &copy, instring );
	p = str_cstr( &copy );

	while ( *p ) {

		if ( *p==',' ) {
			if ( str_memerr( &name ) ) {
				status = BIBL_ERR_MEMERR;
				goto out;
			}

			status = add_wiley_author( endin, intag, instring, inlevel, &name, authornum );
			if ( status!=BIBL_OK ) goto out;

			str_empty( &name );
			authornum++;

			p++;
			while ( is_ws( *p ) ) p++;
		}

		else {
			str_addchar( &name, *p );
			p++;
		}
	}

	if ( str_has_value( &name ) )
		status = add_wiley_author( endin, intag, instring, inlevel, &name, authornum );
out:
	strs_free( &copy, &name, NULL );
	return status;
}

static int
endin_cleanref( fields *endin )
{
	int i, n, status;
	n = fields_num( endin );
	for ( i=0; i<n; ++i ) {
		if ( is_wiley_author( endin, i ) ) {
			status = cleanup_wiley_author( endin, i );
			if ( status!=BIBL_OK ) return status;
		}
	}
	return BIBL_OK;
}

int
endin_cleanf( bibl *bin, param *p )
{
	int status;
        long i;
        for ( i=0; i<bin->n; ++i ) {
                status = endin_cleanref( bin->ref[i] );
		if ( status!=BIBL_OK ) return status;
	}
	return BIBL_OK;
}

/*****************************************************
 PUBLIC: int endin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

static int
endin_date( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	char *tags[3][2] = {
		{ "DATE:YEAR",  "PARTDATE:YEAR" },
		{ "DATE:MONTH", "PARTDATE:MONTH" },
		{ "DATE:DAY",   "PARTDATE:DAY" }
	};
	const char *p = invalue->data;
	int part, status;
	const char *use;
	str date;

	str_init( &date );

	if ( !strncasecmp( outtag, "PART", 4 ) ) part = 1;
	else part = 0;

	/* %D YEAR */
	if ( !strcasecmp( intag->data, "%D" ) ) {
		str_cpytodelim( &date, skip_ws( p ), "", 0 );
		if ( str_memerr( &date ) ) return BIBL_ERR_MEMERR;
		if ( str_has_value( &date ) ) {
			status = fields_add( bibout, tags[0][part], date.data, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
	}

	/* %8 MONTH DAY, YEAR */
	/* %8 MONTH, YEAR */
	/* %8 MONTH YEAR */
	else if ( !strcasecmp( intag->data, "%8" ) ) {

		/* ...get month */
		p = str_cpytodelim( &date, skip_ws( p ), " ,\n", 0 );
		if ( str_memerr( &date ) ) return BIBL_ERR_MEMERR;
		if ( str_has_value( &date ) ) {
			(void) month_to_number( str_cstr( &date ), &use );
			status = fields_add( bibout, tags[1][part], use, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}

		p = skip_ws( p );
		if ( *p==',' ) p++;

		/* ...get days */
		p = str_cpytodelim( &date, skip_ws( p ), ",\n", 0 );
		if ( str_memerr( &date ) ) return BIBL_ERR_MEMERR;
		if ( date.len>0 && date.len<3 ) {
			status = fields_add( bibout, tags[2][part], date.data, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		} else if ( date.len==4 ) {
			status = fields_add( bibout, tags[0][part], date.data, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}

		p = skip_ws( p );
		if ( *p==',' ) p++;

		/* ...get year */
		(void) str_cpytodelim( &date, skip_ws( p ), " \t\n\r", 0 );
		if ( str_memerr( &date ) ) return BIBL_ERR_MEMERR;
		if ( str_has_value( &date ) ) {
			status = fields_add( bibout, tags[0][part], date.data, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
	}
	str_free( &date );
	return BIBL_OK;
}

static int
endin_type( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	lookups types[] = {
		{ "GENERIC",                "ARTICLE",       0, 0 },
		{ "BOOK",                   "BOOK",          0, 0 },
		{ "MANUSCRIPT",             "MANUSCRIPT",    0, 0 },
		{ "CONFERENCE PROCEEDINGS", "INPROCEEDINGS", 0, 0 },
		{ "REPORT",                 "REPORT",        0, 0 },
		{ "COMPUTER PROGRAM",       "BOOK",          0, 0 },
		{ "AUDIOVISUAL MATERIAL",   "AUDIOVISUAL",   0, 0 },
		{ "ARTWORK",                "BOOK",          0, 0 },
		{ "PATENT",                 "BOOK",          0, 0 },
		{ "BILL",                   "BILL",          0, 0 },
		{ "CASE",                   "CASE",          0, 0 },
		{ "JOURNAL ARTICLE",        "ARTICLE",       0, 0 },
		{ "MAGAZINE ARTICLE",       "ARTICLE",       0, 0 },
		{ "BOOK SECTION",           "INBOOK",        0, 0 },
		{ "EDITED BOOK",            "BOOK",          0, 0 },
		{ "NEWSPAPER ARTICLE",      "NEWSARTICLE",   0, 0 },
		{ "THESIS",                 "PHDTHESIS",     0, 0 },
		{ "PERSONAL COMMUNICATION", "COMMUNICATION", 0, 0 },
		{ "ELECTRONIC SOURCE",      "TEXT",          0, 0 },
		{ "FILM OR BROADCAST",      "AUDIOVISUAL",   0, 0 },
		{ "MAP",                    "MAP",           0, 0 },
		{ "HEARING",                "HEARING",       0, 0 },
		{ "STATUTE",                "STATUTE",       0, 0 },
		{ "CHART OR TABLE",         "CHART",         0, 0 },
		{ "WEB PAGE",               "WEBPAGE",       0, 0 },
	};
	int  ntypes = sizeof( types ) / sizeof( lookups );
	int  i, status, found=0;
	for ( i=0; i<ntypes; ++i ) {
		if ( !strcasecmp( types[i].oldstr, invalue->data ) ) {
			found = 1;
			status = fields_add( bibout, "INTERNAL_TYPE", types[i].newstr, level );
			if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
	}
	if ( !found ) {
		fprintf( stderr, "Did not identify reference type '%s'\n", invalue->data );
		fprintf( stderr, "Defaulting to journal article type\n");
		status = fields_add( bibout, "INTERNAL_TYPE", types[0].newstr, level );
		if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

static void
endin_notag( param *p, char *tag, char *data )
{
	if ( p->verbose ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot find tag '%s'='%s'\n", tag, data );
	}
}

int
endin_convertf( fields *bibin, fields *bibout, int reftype, param *p )
{
	static int (*convertfns[NUM_REFTYPES])(fields *, int, str *, str *, int, param *, char *, fields *) = {
		[ 0 ... NUM_REFTYPES-1 ] = generic_null,
		[ SIMPLE       ] = generic_simple,
		[ TITLE        ] = generic_title,
		[ PERSON       ] = generic_person,
		[ SERIALNO     ] = generic_serialno,
		[ PAGES        ] = generic_pages,
		[ NOTES        ] = generic_notes,
		[ URL          ] = generic_url,
		[ GENRE        ] = generic_genre,
		[ TYPE         ] = endin_type,
		[ DATE         ] = endin_date,
        };

	int i, level, process, nfields, fstatus, status = BIBL_OK;
	char *outtag;
	str *intag, *invalue;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		/* Ensure we have data */
		if ( fields_no_value( bibin, i ) ) {
			fields_set_used( bibin, i );
			continue;
		}

		intag = fields_tag( bibin, i, FIELDS_STRP );
		invalue = fields_value( bibin, i, FIELDS_STRP );

		/*
		 * Refer format tags start with '%'.  If we have one
		 * that doesn't, assume that it comes from endx2xml
		 * and just copy and paste to output
		 */
		if ( str_has_value( intag ) && intag->data[0]!='%' ) {
			fstatus = fields_add( bibout, str_cstr( intag ), str_cstr( invalue ), fields_level( bibin, i ) );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
			continue;
		}

		if ( !translate_oldtag( str_cstr( intag ), reftype, p->all, p->nall, &process, &level, &outtag ) ) {
			endin_notag( p, str_cstr( intag ), str_cstr( invalue ) );
			continue;
		}

		fields_set_used( bibin, i );

		status = convertfns[ process ]( bibin, i, intag, invalue, level, p, outtag, bibout );
		if ( status!=BIBL_OK ) return status;

	}

	return status;
}
