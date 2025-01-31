/*
 * biblatexout.c
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
#include "str.h"
#include "strsearch.h"
#include "utf8.h"
#include "xml.h"
#include "fields.h"
#include "append_easy.h"
#include "generic.h"
#include "name.h"
#include "title.h"
#include "type.h"
#include "url.h"
#include "bibformats.h"

/*****************************************************
 PUBLIC: int biblatexout_initparams()
*****************************************************/

static int  biblatexout_write( fields *in, FILE *fp, param *p, unsigned long refnum );
static int  biblatexout_assemble( fields *in, fields *out, param *pm, unsigned long refnum );

int
biblatexout_initparams( param *pm, const char *progname )
{
	pm->writeformat      = BIBL_BIBLATEXOUT;
	pm->format_opts      = 0;
	pm->charsetout       = BIBL_CHARSET_DEFAULT;
	pm->charsetout_src   = BIBL_SRC_DEFAULT;
	pm->latexout         = 1;
	pm->utf8out          = BIBL_CHARSET_UTF8_DEFAULT;
	pm->utf8bom          = BIBL_CHARSET_BOM_DEFAULT;
	pm->xmlout           = BIBL_XMLOUT_FALSE;
	pm->nosplittitle     = 0;
	pm->verbose          = 0;
	pm->addcount         = 0;
	pm->singlerefperfile = 0;

	pm->headerf   = generic_writeheader;
	pm->footerf   = NULL;
	pm->assemblef = biblatexout_assemble;
	pm->writef    = biblatexout_write;

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
 PUBLIC: int biblatexout_assemble()
*****************************************************/

enum {
	TYPE_UNKNOWN = 0,
	TYPE_ARTICLE,
	TYPE_SUPPPERIODICAL,
	TYPE_INBOOK,
	TYPE_INPROCEEDINGS,
	TYPE_PROCEEDINGS,
	TYPE_CONFERENCE,       /* legacy */
	TYPE_INCOLLECTION,
	TYPE_COLLECTION,
	TYPE_SUPPCOLLECTION,
	TYPE_REFERENCE,
	TYPE_MVREFERENCE,
	TYPE_BOOK,
	TYPE_BOOKLET,
	TYPE_SUPPBOOK,
	TYPE_PHDTHESIS,        /* legacy */
	TYPE_MASTERSTHESIS,    /* legacy */
	TYPE_DIPLOMATHESIS,
	TYPE_REPORT,
	TYPE_TECHREPORT,
	TYPE_MANUAL,
	TYPE_UNPUBLISHED,
	TYPE_PATENT,
	TYPE_ELECTRONIC,       /* legacy */
	TYPE_ONLINE,
	TYPE_WWW,              /* jurabib compatibility */
	TYPE_MISC,
	NUM_TYPES
};

static int
biblatexout_type( fields *in, const char *progname, const char *filename, unsigned long refnum )
{
	match_type genre_matches[] = {
		{ "periodical",             TYPE_ARTICLE,       LEVEL_ANY  },
		{ "academic journal",       TYPE_ARTICLE,       LEVEL_ANY  },
		{ "magazine",               TYPE_ARTICLE,       LEVEL_ANY  },
		{ "newspaper",              TYPE_ARTICLE,       LEVEL_ANY  },
		{ "article",                TYPE_ARTICLE,       LEVEL_ANY  },
		{ "instruction",            TYPE_MANUAL,        LEVEL_ANY  },
		{ "book",                   TYPE_BOOK,          LEVEL_MAIN },
		{ "booklet",                TYPE_BOOKLET,       LEVEL_MAIN },
		{ "book",                   TYPE_INBOOK,        LEVEL_ANY  },
		{ "book chapter",           TYPE_INBOOK,        LEVEL_ANY  },
		{ "unpublished",            TYPE_UNPUBLISHED,   LEVEL_ANY  },
		{ "manuscript",             TYPE_UNPUBLISHED,   LEVEL_ANY  },
		{ "conference publication", TYPE_PROCEEDINGS,   LEVEL_MAIN },
		{ "conference publication", TYPE_INPROCEEDINGS, LEVEL_ANY  },
		{ "collection",             TYPE_COLLECTION,    LEVEL_MAIN },
		{ "collection",             TYPE_INCOLLECTION,  LEVEL_ANY  },
		{ "report",                 TYPE_REPORT,        LEVEL_ANY  },
		{ "technical report",       TYPE_TECHREPORT,    LEVEL_ANY  },
		{ "Masters thesis",         TYPE_MASTERSTHESIS, LEVEL_ANY  },
		{ "Diploma thesis",         TYPE_DIPLOMATHESIS, LEVEL_ANY  },
		{ "Ph.D. thesis",           TYPE_PHDTHESIS,     LEVEL_ANY  },
		{ "Licentiate thesis",      TYPE_PHDTHESIS,     LEVEL_ANY  },
		{ "thesis",                 TYPE_PHDTHESIS,     LEVEL_ANY  },
		{ "electronic",             TYPE_ELECTRONIC,    LEVEL_ANY  },
		{ "patent",                 TYPE_PATENT,        LEVEL_ANY  },
		{ "miscellaneous",          TYPE_MISC,          LEVEL_ANY  },
	};
	int ngenre_matches = sizeof( genre_matches ) / sizeof( genre_matches[0] );

	match_type resource_matches[] = {
		{ "moving image",           TYPE_ELECTRONIC,    LEVEL_ANY  },
		{ "software, multimedia",   TYPE_ELECTRONIC,    LEVEL_ANY  },
	};
	int nresource_matches = sizeof( resource_matches ) /sizeof( resource_matches[0] );

	match_type issuance_matches[] = {
		{ "monographic",            TYPE_BOOK,          LEVEL_MAIN },
		{ "monographic",            TYPE_INBOOK,        LEVEL_ANY  },
	};
	int nissuance_matches = sizeof( issuance_matches ) / sizeof( issuance_matches[0] );

	int type, maxlevel, n;

	type = type_from_mods_hints( in, TYPE_FROM_GENRE, genre_matches, ngenre_matches, TYPE_UNKNOWN );
	if ( type==TYPE_UNKNOWN ) type = type_from_mods_hints( in, TYPE_FROM_RESOURCE, resource_matches, nresource_matches, TYPE_UNKNOWN );
	if ( type==TYPE_UNKNOWN ) type = type_from_mods_hints( in, TYPE_FROM_ISSUANCE, issuance_matches, nissuance_matches, TYPE_UNKNOWN );

	/* default to TYPE_MISC */
	if ( type==TYPE_UNKNOWN ) {
		maxlevel = fields_maxlevel( in );
		if ( maxlevel > 0 ) type = TYPE_MISC;
		else {
			if ( progname ) fprintf( stderr, "%s: ", progname );
			fprintf( stderr, "Cannot identify TYPE in reference %lu ", refnum+1 );
			n = fields_find( in, "REFNUM", LEVEL_ANY );
			if ( n!=FIELDS_NOTFOUND ) 
				fprintf( stderr, " %s", (char*) fields_value( in, n, FIELDS_CHRP ) );
			fprintf( stderr, " (defaulting to @Misc)\n" );
			type = TYPE_MISC;
		}
	}
	return type;
}

static void
append_type( int type, fields *out, int *status )
{
	char *typenames[ NUM_TYPES ] = {
		[ TYPE_ARTICLE        ] = "Article",
		[ TYPE_SUPPPERIODICAL ] = "SuppPeriodical",
		[ TYPE_INBOOK         ] = "Inbook",
		[ TYPE_PROCEEDINGS    ] = "Proceedings",
		[ TYPE_INPROCEEDINGS  ] = "InProceedings",
		[ TYPE_CONFERENCE     ] = "Conference",
		[ TYPE_BOOK           ] = "Book",
		[ TYPE_BOOKLET        ] = "Booklet",
		[ TYPE_SUPPBOOK       ] = "SuppBook",
		[ TYPE_PHDTHESIS      ] = "PhdThesis",
		[ TYPE_MASTERSTHESIS  ] = "MastersThesis",
		[ TYPE_DIPLOMATHESIS  ] = "MastersThesis",
		[ TYPE_REPORT         ] = "Report",
		[ TYPE_TECHREPORT     ] = "TechReport",
		[ TYPE_REFERENCE      ] = "Reference",
		[ TYPE_MVREFERENCE    ] = "MvReference",
		[ TYPE_MANUAL         ] = "Manual",
		[ TYPE_COLLECTION     ] = "Collection",
		[ TYPE_SUPPCOLLECTION ] = "SuppCollection",
		[ TYPE_INCOLLECTION   ] = "InCollection",
		[ TYPE_UNPUBLISHED    ] = "Unpublished",
		[ TYPE_ELECTRONIC     ] = "Electronic",
		[ TYPE_ONLINE         ] = "Online",
		[ TYPE_WWW            ] = "WWW",
		[ TYPE_PATENT         ] = "Patent",
		[ TYPE_MISC           ] = "Misc",
	};
	int fstatus;
	char *s;

	if ( type < 0 || type >= NUM_TYPES ) type = TYPE_MISC;
	s = typenames[ type ];

	fstatus = fields_add( out, "TYPE", s, LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
}

static void
append_citekey( fields *in, fields *out, int format_opts, int *status )
{
	int n, fstatus;
	str s;
	char *p;

	n = fields_find( in, "REFNUM", LEVEL_ANY );
	if ( ( format_opts & BIBL_FORMAT_BIBOUT_DROPKEY ) || n==FIELDS_NOTFOUND ) {
		fstatus = fields_add( out, "REFNUM", "", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}

	else {
		str_init( &s );
		p = fields_value( in, n, FIELDS_CHRP );
		while ( p && *p && *p!='|' ) {
			if ( format_opts & BIBL_FORMAT_BIBOUT_STRICTKEY ) {
				if ( isdigit((unsigned char)*p) || (*p>='A' && *p<='Z') ||
				     (*p>='a' && *p<='z' ) ) {
					str_addchar( &s, *p );
				}
			}
			else {
				if ( *p!=' ' && *p!='\t' ) {
					str_addchar( &s, *p );
				}
			}
			p++;
		}
		if ( str_memerr( &s ) )  { *status = BIBL_ERR_MEMERR; str_free( &s ); return; }
		fstatus = fields_add( out, "REFNUM", str_cstr( &s ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		str_free( &s );
	}
}

static void
append_fileattach( fields *in, fields *out, int *status )
{
	char *tag, *value;
	int i, fstatus;
	str data;

	str_init( &data );

	for ( i=0; i<in->n; ++i ) {

		tag = fields_tag( in, i, FIELDS_CHRP );
		if ( strcasecmp( tag, "FILEATTACH" ) ) continue;

		value = fields_value( in, i, FIELDS_CHRP );
		str_strcpyc( &data, ":" );
		str_strcatc( &data, value );
		if ( strsearch( value, ".pdf" ) )
			str_strcatc( &data, ":PDF" );
		else if ( strsearch( value, ".html" ) )
			str_strcatc( &data, ":HTML" );
		else str_strcatc( &data, ":TYPE" );

		if ( str_memerr( &data ) ) {
			*status = BIBL_ERR_MEMERR;
			goto out;
		}

		fields_set_used( in, i );
		fstatus = fields_add( out, "file", str_cstr( &data ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			goto out;
		}

		str_empty( &data );
	}
out:
	str_free( &data );
}

static void
append_people( fields *in, char *tag, char *ctag, char *atag,
		char *bibtag, int level, fields *out, int format_opts, int latex_out, int *status )
{
	int i, npeople, person, corp, asis, fstatus;
	str allpeople, oneperson;

	strs_init( &allpeople, &oneperson, NULL );

	/* primary citation authors */
	npeople = 0;
	for ( i=0; i<in->n; ++i ) {
		if ( level!=LEVEL_ANY && fields_level( in, i )!=level ) continue;
		person = ( strcasecmp( fields_tag( in, i, FIELDS_CHRP ), tag  ) == 0 );
		corp   = ( strcasecmp( fields_tag( in, i, FIELDS_CHRP ), ctag ) == 0 );
		asis   = ( strcasecmp( fields_tag( in, i, FIELDS_CHRP ), atag ) == 0 );
		if ( person || corp || asis ) {
			if ( npeople>0 ) {
				if ( format_opts & BIBL_FORMAT_BIBOUT_WHITESPACE )
					str_strcatc( &allpeople, "\n\t\tand " );
				else str_strcatc( &allpeople, "\nand " );
			}
			if ( corp ) {
				if ( latex_out ) str_addchar( &allpeople, '{' );
				str_strcat( &allpeople, fields_value( in, i, FIELDS_STRP ) );
				if ( latex_out ) str_addchar( &allpeople, '}' );
			} else if ( asis ) {
				if ( latex_out ) str_addchar( &allpeople, '{' );
				str_strcat( &allpeople, fields_value( in, i, FIELDS_STRP ) );
				if ( latex_out ) str_addchar( &allpeople, '}' );
			} else {
				name_build_withcomma( &oneperson, fields_value( in, i, FIELDS_CHRP ) );
				str_strcat( &allpeople, &oneperson );
			}
			npeople++;
		}
	}
	if ( npeople ) {
		fstatus = fields_add( out, bibtag, allpeople.data, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}

	strs_free( &allpeople, &oneperson, NULL );
}

static int
append_title_chosen( fields *in, char *bibtag, fields *out, int nmainttl, int nsubttl )
{
	str fulltitle, *mainttl = NULL, *subttl = NULL;
	int status, ret = BIBL_OK;

	str_init( &fulltitle );

	if ( nmainttl!=-1 ) {
		mainttl = fields_value( in, nmainttl, FIELDS_STRP );
		fields_set_used( in, nmainttl );
	}

	if ( nsubttl!=-1 ) {
		subttl = fields_value( in, nsubttl, FIELDS_STRP );
		fields_set_used( in, nsubttl );
	}

	title_combine( &fulltitle, mainttl, subttl );

	if ( str_memerr( &fulltitle ) ) {
		ret = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( str_has_value( &fulltitle ) ) {
		status = fields_add( out, bibtag, str_cstr( &fulltitle ), LEVEL_MAIN );
		if ( status!=FIELDS_OK ) ret = BIBL_ERR_MEMERR;
	}

out:
	str_free( &fulltitle );
	return ret;
}

static int
append_title( fields *in, char *bibtag, char *shortbibtag, int level, fields *out, int format_opts )
{
	int title, short_title, subtitle, short_subtitle, use_title, use_subtitle;
	int status;

	title          = fields_find( in, "TITLE",         level );
	short_title    = fields_find( in, "SHORTTITLE",    level );
	subtitle       = fields_find( in, "SUBTITLE",      level );
	short_subtitle = fields_find( in, "SHORTSUBTITLE", level );

	if ( title==FIELDS_NOTFOUND || ( ( format_opts & BIBL_FORMAT_BIBOUT_SHORTTITLE ) && level==1 ) ) {
		use_title    = short_title;
		use_subtitle = short_subtitle;
	}

	else {
		use_title    = title;
		use_subtitle = subtitle;
	}

	status = append_title_chosen( in, bibtag, out, use_title, use_subtitle );
	if ( status!=BIBL_OK ) return status;

	/* add abbreviated title, if exists and tag is available */
	if ( use_title==title && short_title!=FIELDS_NOTFOUND && shortbibtag ) {
		status = append_title_chosen( in, shortbibtag, out, short_title, short_subtitle );
		if ( status!=BIBL_OK ) return status;
	}

	return BIBL_OK;
}

static void
append_titles( fields *in, int type, fields *out, int format_opts, int *status )
{
	/* item (main level) title */
	*status = append_title( in, "title", "shorttitle", LEVEL_MAIN, out, format_opts );
	if ( *status!=BIBL_OK ) return;

	/* host/series level titles */
	switch( type ) {

		case TYPE_ARTICLE:
		*status = append_title( in, "journal", "shortjournal", LEVEL_HOST, out, format_opts );
		break;

		case TYPE_INBOOK:
		*status = append_title( in, "booktitle", "shortbooktitle", LEVEL_HOST, out, format_opts );
		if ( *status!=BIBL_OK ) return;
		*status = append_title( in, "series", "shortseries", LEVEL_SERIES, out, format_opts );
		break;

		case TYPE_INCOLLECTION:
		case TYPE_INPROCEEDINGS:
		*status = append_title( in, "booktitle", "shortbooktitle", LEVEL_HOST, out, format_opts );
		if ( *status!=BIBL_OK ) return;
		*status = append_title( in, "series", "shortseries", LEVEL_SERIES,  out, format_opts );
		break;

		case TYPE_PHDTHESIS:
		case TYPE_MASTERSTHESIS:
		*status = append_title( in, "series", "shortseries", LEVEL_HOST, out, format_opts );
		break;

		case TYPE_BOOK:
		case TYPE_REPORT:
		case TYPE_COLLECTION:
		case TYPE_PROCEEDINGS:
		*status = append_title( in, "series", "shortseries", LEVEL_HOST, out, format_opts );
		if ( *status!=BIBL_OK ) return;
		*status = append_title( in, "series", "shortseries", LEVEL_SERIES, out, format_opts );
		break;

		default:
		/* do nothing */
		break;

	}
}

static int
find_date( fields *in, char *date_element )
{
	char date[100], partdate[100];
	int n;

	sprintf( date, "DATE:%s", date_element );
	n = fields_find( in, date, LEVEL_ANY );

	if ( n==FIELDS_NOTFOUND ) {
		sprintf( partdate, "PARTDATE:%s", date_element );
		n = fields_find( in, partdate, LEVEL_ANY );
	}

	return n;
}

static int
str_is_wholenumber( str *s )
{
	char *p;

	p = str_cstr( s );
	if ( !p ) return 1;

	while ( '0' <= *p && *p <= '9' ) p++;
	return (*p=='\0');
}


/* biblatex wants to use ISO 8601 dates:
 *
 * YYYY-MM-DD (or YYYYMMDD)
 * YYYY-DDD   (or YYYYDDD) for day of the year
 */
static int
is_valid_iso8601( str *year, str *month, str *day )
{
	if ( year ) {
		if ( str_strlen( year ) != 4 ) return 0;
		if ( !str_is_wholenumber( year ) ) return 0;
	}
	if ( month ) {
		if ( str_strlen( month ) != 2 ) return 0;
		if ( !str_is_wholenumber( month ) ) return 0;
	}
	if ( day ) {
		if ( !str_is_wholenumber( day ) ) return 0;
	}
	return 1;
}

static void
append_iso8601( str *year, str *month, str *day, fields *out, int *status )
{
	int fstatus;
	str date;

	str_init( &date );

	if ( year ) str_strcpy( &date, year );
	if ( month ) {
		str_addchar( &date, '-' );
		str_strcat( &date, month );
	}
	if ( day ) {
		str_addchar( &date, '-' );
		str_strcat( &date, day );
	}

	if ( str_memerr( &date ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	fstatus = fields_add( out, "date", str_cstr( &date ), LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;

out:
	str_free( &date );
}

static void
append_date_elements( str *year, str *month, str *day, fields *out, int *status )
{
	int fstatus;

	if ( year && str_has_value( year ) ) {
		fstatus = fields_add( out, "year", str_cstr( year ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	if ( month && str_has_value( month ) ) {
		fstatus = fields_add( out, "month", str_cstr( month ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	if ( day && str_has_value( day ) ) {
		fstatus = fields_add( out, "day", str_cstr( day ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
append_date( fields *in, fields *out, int *status )
{
	str *year = NULL, *month = NULL, *day = NULL;
	int nyear, nmonth, nday;

	nyear = find_date( in, "YEAR" );
	if ( nyear!=FIELDS_NOTFOUND ) {
		fields_set_used( in, nyear );
		year = ( str * ) fields_value( in, nyear, FIELDS_STRP );
	} else return;

	nmonth = find_date( in, "MONTH" );
	if ( nmonth!=FIELDS_NOTFOUND ) {
		fields_set_used( in, nmonth );
		month = ( str * ) fields_value( in, nmonth, FIELDS_STRP );
	}

	nday = find_date( in, "DAY" );
	if ( nday!=FIELDS_NOTFOUND ) {
		fields_set_used( in, nday );
		day = ( str* ) fields_value( in, nday, FIELDS_STRP );
	}

	if ( is_valid_iso8601( year, month, day ) )
		append_iso8601( year, month, day, out, status );
	else
		append_date_elements( year, month, day, out, status );
}

static void
append_arxiv( fields *in, fields *out, int *status )
{
	int n, fstatus1, fstatus2;
	str url;

	n = fields_find( in, "ARXIV", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return;

	fields_set_used( in, n );

	/* ...write:
	 *     archivePrefix = "arXiv",
	 *     eprint = "#####",
	 * ...for arXiv references
	 */
	fstatus1 = fields_add( out, "archivePrefix", "arXiv", LEVEL_MAIN );
	fstatus2 = fields_add( out, "eprint", fields_value( in, n, FIELDS_CHRP ), LEVEL_MAIN );
	if ( fstatus1!=FIELDS_OK || fstatus2!=FIELDS_OK ) {
		*status = BIBL_ERR_MEMERR;
		return;
	}

	/* ...also write:
	 *     url = "http://arxiv.org/abs/####",
	 * ...to maximize compatibility
	 */
	str_init( &url );
	arxiv_to_url( in, n, "URL", &url );
	if ( str_has_value( &url ) ) {
		fstatus1 = fields_add( out, "url", str_cstr( &url ), LEVEL_MAIN );
		if ( fstatus1!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	str_free( &url );
}

static void
append_urls( fields *in, fields *out, int *status )
{
	int lstatus;
	slist types;

	lstatus = slist_init_valuesc( &types, "URL", "DOI", "PMID", "PMC", "JSTOR", NULL );
	if ( lstatus!=SLIST_OK ) {
		*status = BIBL_ERR_MEMERR;
		return;
	}

	*status = urls_merge_and_add( in, LEVEL_ANY, out, "url", LEVEL_MAIN, &types );

	slist_free( &types );
}

static void
append_isi( fields *in, fields *out, int *status )
{
	int n, fstatus;

	n = fields_find( in, "ISIREFNUM", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return;

	fstatus = fields_add( out, "note", fields_value( in, n, FIELDS_CHRP ), LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
}

static void
append_articlenumber( fields *in, fields *out, int *status )
{
	int n, fstatus;

	n = fields_find( in, "ARTICLENUMBER", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return;

	fields_set_used( in, n );
	fstatus = fields_add( out, "pages", fields_value( in, n, FIELDS_CHRP ), LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
}

static int
pages_build_pagestr( str *pages, fields *in, int sn, int en, int format_opts )
{
	/* ...append if starting page number is defined */
	if ( sn!=-1 ) {
		str_strcat( pages, fields_value( in, sn, FIELDS_STRP ) );
		fields_set_used( in, sn );
	}

	/* ...append dashes if both starting and ending page numbers are defined */
	if ( sn!=-1 && en!=-1 ) {
		if ( format_opts & BIBL_FORMAT_BIBOUT_SINGLEDASH )
			str_strcatc( pages, "-" );
		else
			str_strcatc( pages, "--" );
	}

	/* ...append ending page number is defined */
	if ( en!=-1 ) {
		str_strcat( pages, fields_value( in, en, FIELDS_STRP ) );
		fields_set_used( in, en );
	}

	if ( str_memerr( pages ) ) return BIBL_ERR_MEMERR;
	else return BIBL_OK;
}

static int
pages_are_defined( fields *in, int *sn, int *en )
{
	*sn = fields_find( in, "PAGES:START", LEVEL_ANY );
	*en = fields_find( in, "PAGES:STOP",  LEVEL_ANY );
	if ( *sn==FIELDS_NOTFOUND && *en==FIELDS_NOTFOUND ) return 0;
	else return 1;
}

static void
append_pages( fields *in, fields *out, int format_opts, int *status )
{
	int sn, en, fstatus;
	str pages;

	if ( !pages_are_defined( in, &sn, &en ) ) {
		append_articlenumber( in, out, status );
		return;
	}

	str_init( &pages );
	*status = pages_build_pagestr( &pages, in, sn, en, format_opts );
	if ( *status==BIBL_OK ) {
		fstatus = fields_add( out, "pages", str_cstr( &pages ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	str_free( &pages );
}

/*
 * from Tim Hicks:
 * I'm no expert on bibtex, but those who know more than I on our mailing 
 * list suggest that 'issue' isn't a recognised key for bibtex and 
 * therefore that bibutils should be aliasing IS to number at some point in 
 * the conversion.
 *
 * Therefore prefer outputting issue/number as number and only keep
 * a distinction if both issue and number are present for a particular
 * reference.
 */

static void
append_issue_number( fields *in, fields *out, int *status )
{
	char issue[] = "issue", number[] = "number", *use_issue = number;
	int nissue  = fields_find( in, "ISSUE",  LEVEL_ANY );
	int nnumber = fields_find( in, "NUMBER", LEVEL_ANY );
	int fstatus;

	if ( nissue!=FIELDS_NOTFOUND && nnumber!=FIELDS_NOTFOUND ) use_issue = issue;

	if ( nissue!=FIELDS_NOTFOUND ) {
		fields_set_used( in, nissue );
		fstatus = fields_add( out, use_issue, fields_value( in, nissue, FIELDS_CHRP ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			return;
		}
	}

	if ( nnumber!=FIELDS_NOTFOUND ) {
		fields_set_used( in, nnumber );
		fstatus = fields_add( out, "number", fields_value( in, nnumber, FIELDS_CHRP ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			return;
		}
	}
}

static void
append_howpublished( fields *in, fields *out, int *status )
{
	int n, fstatus;
	char *d;

	n = fields_find( in, "GENRE:BIBUTILS", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return;

	d = fields_value( in, n, FIELDS_CHRP_NOUSE );
	if ( !strcmp( d, "Habilitation thesis" ) ) {
		fstatus = fields_add( out, "howpublised", d, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	if ( !strcmp( d, "Licentiate thesis" ) ) {
		fstatus = fields_add( out, "howpublised", d, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	if ( !strcmp( d, "Diploma thesis" ) ) {
		fstatus = fields_add( out, "howpublised", d, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static int
biblatexout_assemble( fields *in, fields *out, param *pm, unsigned long refnum )
{
	int type, status = BIBL_OK;

	type = biblatexout_type( in, pm->progname, "", refnum );

	append_type        ( type, out, &status );
	append_citekey     ( in, out, pm->format_opts, &status );
	append_people      ( in, "AUTHOR",     "AUTHOR:CORP",     "AUTHOR:ASIS",     "author",       LEVEL_MAIN, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "AUTHOR",     "AUTHOR:CORP",     "AUTHOR:ASIS",     "bookauthor",   LEVEL_HOST, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "EDITOR",     "EDITOR:CORP",     "EDITOR:ASIS",     "editor",       LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "ANNOTATOR",  "ANNOTATOR:CORP",  "ANNOTATOR:ASIS",  "annotator",    LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "TRANSLATOR", "TRANSLATOR:CORP", "TRANSLATOR:ASIS", "translator",   LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "REDACTOR",   "REDACTOR:CORP",   "REDACTOR:ASIS",   "redactor",     LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "COMMENTATOR","COMMENTATOR:CORP","COMMENTATOR:ASIS","commentator",  LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "INTROAUTHOR","INTROAUTHOR:CORP","INTROAUTHOR:ASIS","introduction", LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_people      ( in, "AFTERAUTHOR","AFTERAUTHOR:CORP","AFTERAUTHOR:ASIS","afterword",    LEVEL_ANY, out, pm->format_opts, pm->latexout, &status );
	append_titles      ( in, type, out, pm->format_opts, &status );
	append_date        ( in, out, &status );
	append_easy        ( in, "EDITION",            LEVEL_ANY,  out, "edition",      &status );
	append_easy        ( in, "PUBLISHER",          LEVEL_ANY,  out, "publisher",    &status );
	append_easycombo   ( in, "ADDRESS",            LEVEL_ANY,  out, "address",    "; ", &status );
	append_easy        ( in, "EDITION",            LEVEL_ANY,  out, "version",      &status );
	append_easy        ( in, "PART",               LEVEL_ANY,  out, "part",         &status );
	append_easy        ( in, "VOLUME",             LEVEL_ANY,  out, "volume",       &status );
	append_issue_number( in, out, &status );
	append_pages       ( in, out, pm->format_opts, &status );
	append_easycombo   ( in, "KEYWORD",            LEVEL_ANY,  out, "keywords",   "; ", &status );
	append_easy        ( in, "LANGCATALOG",        LEVEL_ANY,  out, "hyphenation",  &status );
	append_easy        ( in, "CONTENTS",           LEVEL_ANY,  out, "contents",     &status );
	append_easy        ( in, "ABSTRACT",           LEVEL_ANY,  out, "abstract",     &status );
	append_easy        ( in, "LOCATION",           LEVEL_ANY,  out, "location",     &status );
	append_easy        ( in, "DEGREEGRANTOR",      LEVEL_ANY,  out, "school",       &status );
	append_easy        ( in, "DEGREEGRANTOR:ASIS", LEVEL_ANY,  out, "school",       &status );
	append_easy        ( in, "DEGREEGRANTOR:CORP", LEVEL_ANY,  out, "school",       &status );
	append_easyall     ( in, "NOTES",              LEVEL_ANY,  out, "note",         &status );
	append_easyall     ( in, "ANNOTE",             LEVEL_ANY,  out, "annote",       &status );
	append_easyall     ( in, "ANNOTATION",         LEVEL_ANY,  out, "annotation",   &status );
	append_easy        ( in, "ISBN",               LEVEL_ANY,  out, "isbn",         &status );
	append_easy        ( in, "ISSN",               LEVEL_ANY,  out, "issn",         &status );
	append_easy        ( in, "MRNUMBER",           LEVEL_ANY,  out, "mrnumber",     &status );
	append_easy        ( in, "CODEN",              LEVEL_ANY,  out, "coden",        &status );
	append_easy        ( in, "DOI",                LEVEL_ANY,  out, "doi",          &status );
	append_easy        ( in, "EID",                LEVEL_ANY,  out, "eid",          &status );
	append_urls        ( in, out, &status );
	append_fileattach  ( in, out, &status );
	append_arxiv       ( in, out, &status );
	append_easy        ( in, "EPRINTCLASS",        LEVEL_ANY,  out, "primaryClass", &status );
	append_isi         ( in, out, &status );
	append_easy        ( in, "LANGUAGE",           LEVEL_ANY,  out, "language",     &status );
	append_howpublished( in, out, &status );

	return status;
}

/*****************************************************
 PUBLIC: int biblatexout_write()
*****************************************************/

static int
biblatexout_write( fields *out, FILE *fp, param *pm, unsigned long refnum )
{
	int i, j, len, nquotes, format_opts = pm->format_opts;
	char *tag, *value, ch;

	/* ...output type information "@article{" */
	value = ( char * ) fields_value( out, 0, FIELDS_CHRP );
	if ( !(format_opts & BIBL_FORMAT_BIBOUT_UPPERCASE) ) fprintf( fp, "@%s{", value );
	else {
		len = (value) ? strlen( value ) : 0;
		fprintf( fp, "@" );
		for ( i=0; i<len; ++i )
			fprintf( fp, "%c", toupper((unsigned char)value[i]) );
		fprintf( fp, "{" );
	}

	/* ...output refnum "Smith2001" */
	value = ( char * ) fields_value( out, 1, FIELDS_CHRP );
	fprintf( fp, "%s", value );

	/* ...rest of the references */
	for ( j=2; j<out->n; ++j ) {
		nquotes = 0;
		tag   = ( char * ) fields_tag( out, j, FIELDS_CHRP );
		value = ( char * ) fields_value( out, j, FIELDS_CHRP );
		fprintf( fp, ",\n" );
		if ( format_opts & BIBL_FORMAT_BIBOUT_WHITESPACE ) fprintf( fp, "  " );
		if ( !(format_opts & BIBL_FORMAT_BIBOUT_UPPERCASE ) ) fprintf( fp, "%s", tag );
		else {
			len = strlen( tag );
			for ( i=0; i<len; ++i )
				fprintf( fp, "%c", toupper((unsigned char)tag[i]) );
		}
		if ( format_opts & BIBL_FORMAT_BIBOUT_WHITESPACE ) fprintf( fp, " = \t" );
		else fprintf( fp, "=" );

		if ( format_opts & BIBL_FORMAT_BIBOUT_BRACKETS ) fprintf( fp, "{" );
		else fprintf( fp, "\"" );

		len = strlen( value );
		for ( i=0; i<len; ++i ) {
			ch = value[i];
			if ( ch!='\"' ) fprintf( fp, "%c", ch );
			else {
				if ( format_opts & BIBL_FORMAT_BIBOUT_BRACKETS || ( i>0 && value[i-1]=='\\' ) )
					fprintf( fp, "\"" );
				else {
					if ( nquotes % 2 == 0 )
						fprintf( fp, "``" );
					else    fprintf( fp, "\'\'" );
					nquotes++;
				}
			}
		}

		if ( format_opts & BIBL_FORMAT_BIBOUT_BRACKETS ) fprintf( fp, "}" );
		else fprintf( fp, "\"" );
	}

	/* ...finish reference */
	if ( format_opts & BIBL_FORMAT_BIBOUT_FINALCOMMA ) fprintf( fp, "," );
	fprintf( fp, "\n}\n\n" );

	fflush( fp );

	return BIBL_OK;
}
