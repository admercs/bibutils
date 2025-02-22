/*
 * endout.c
 *
 * Copyright (c) Chris Putnam 2004-2021
 *
 * Program and source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utf8.h"
#include "str.h"
#include "fields.h"
#include "generic.h"
#include "append_easy.h"
#include "month.h"
#include "name.h"
#include "title.h"
#include "type.h"
#include "url.h"
#include "bibformats.h"

/*****************************************************
 PUBLIC: int endout_initparams()
*****************************************************/

static int endout_write( fields *in, FILE *fp, param *p, unsigned long refnum );
static int endout_assemble( fields *in, fields *out, param *pm, unsigned long refnum );

int
endout_initparams( param *pm, const char *progname )
{
	pm->writeformat      = BIBL_ENDNOTEOUT;
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
	pm->assemblef = endout_assemble;
	pm->writef    = endout_write;

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
 PUBLIC: int endout_assemble()
*****************************************************/

enum {
	TYPE_UNKNOWN = 0,
	TYPE_GENERIC,                     /* Generic */
	TYPE_ARTWORK,                     /* Artwork */
	TYPE_AUDIOVISUAL,                 /* Audiovisual Material */
	TYPE_BILL,                        /* Bill */
	TYPE_BOOK,                        /* Book */
	TYPE_INBOOK,                      /* Book Section */
	TYPE_CASE,                        /* Case */
	TYPE_CHARTTABLE,                  /* Chart or Table */
	TYPE_CLASSICALWORK,               /* Classical Work */
	TYPE_PROGRAM,                     /* Computer Program */
	TYPE_INPROCEEDINGS,               /* Conference Paper */
	TYPE_PROCEEDINGS,                 /* Conference Proceedings */
	TYPE_EDITEDBOOK,                  /* Edited Book */
	TYPE_EQUATION,                    /* Equation */
	TYPE_ELECTRONICARTICLE,           /* Electronic Article */
	TYPE_ELECTRONICBOOK,              /* Electronic Book */
	TYPE_ELECTRONIC,                  /* Electronic Source */
	TYPE_FIGURE,                      /* Figure */
	TYPE_FILMBROADCAST,               /* Film or Broadcast */
	TYPE_GOVERNMENT,                  /* Government Document */
	TYPE_HEARING,                     /* Hearing */
	TYPE_ARTICLE,                     /* Journal Article */
	TYPE_LEGALRULE,                   /* Legal Rule/Regulation */
	TYPE_MAGARTICLE,                  /* Magazine Article */
	TYPE_MANUSCRIPT,                  /* Manuscript */
	TYPE_MAP,                         /* Map */
	TYPE_NEWSARTICLE,                 /* Newspaper Article */
	TYPE_ONLINEDATABASE,              /* Online Database */
	TYPE_ONLINEMULTIMEDIA,            /* Online Multimedia */
	TYPE_PATENT,                      /* Patent */
	TYPE_COMMUNICATION,               /* Personal Communication */
	TYPE_REPORT,                      /* Report */
	TYPE_STATUTE,                     /* Statute */
	TYPE_THESIS,                      /* Thesis */
	TYPE_MASTERSTHESIS,               /* Thesis */
	TYPE_PHDTHESIS,                   /* Thesis */
	TYPE_DIPLOMATHESIS,               /* Thesis */
	TYPE_DOCTORALTHESIS,              /* Thesis */
	TYPE_HABILITATIONTHESIS,          /* Thesis */
	TYPE_LICENTIATETHESIS,            /* Thesis */
	TYPE_UNPUBLISHED,                 /* Unpublished Work */
};

static void
write_type( FILE *fp, int type )
{
	switch( type ) {
	case TYPE_UNKNOWN:           fprintf( fp, "TYPE_UNKNOWN" );            break;
	case TYPE_GENERIC:           fprintf( fp, "TYPE_GENERIC" );            break;
	case TYPE_ARTWORK:           fprintf( fp, "TYPE_ARTWORK" );            break;
	case TYPE_AUDIOVISUAL:       fprintf( fp, "TYPE_AUDIOVISUAL" );        break;
	case TYPE_BILL:              fprintf( fp, "TYPE_BILL" );               break;
	case TYPE_BOOK:              fprintf( fp, "TYPE_BOOK" );               break;
	case TYPE_INBOOK:            fprintf( fp, "TYPE_INBOOK" );             break;
	case TYPE_CASE:              fprintf( fp, "TYPE_CASE" );               break;
	case TYPE_CHARTTABLE:        fprintf( fp, "TYPE_CHARITABLE" );         break;
	case TYPE_CLASSICALWORK:     fprintf( fp, "TYPE_CLASSICALWORK" );      break;
	case TYPE_PROGRAM:           fprintf( fp, "TYPE_PROGRAM" );            break;
	case TYPE_INPROCEEDINGS:     fprintf( fp, "TYPE_INPROCEEDINGS" );      break;
	case TYPE_PROCEEDINGS:       fprintf( fp, "TYPE_PROCEEDINGS" );        break;
	case TYPE_EDITEDBOOK:        fprintf( fp, "TYPE_EDITEDBOOK" );         break;
	case TYPE_EQUATION:          fprintf( fp, "TYPE_EQUATION" );           break;
	case TYPE_ELECTRONICARTICLE: fprintf( fp, "TYPE_ELECTRONICARTICLE" );  break;
	case TYPE_ELECTRONICBOOK:    fprintf( fp, "TYPE_ELECTRONICBOOK" );     break;
	case TYPE_ELECTRONIC:        fprintf( fp, "TYPE_ELECTRONIC" );         break;
	case TYPE_FIGURE:            fprintf( fp, "TYPE_FIGURE" );             break;
	case TYPE_FILMBROADCAST:     fprintf( fp, "TYPE_FILMBROADCAST" );      break;
	case TYPE_GOVERNMENT:        fprintf( fp, "TYPE_GOVERNMENT" );         break;
	case TYPE_HEARING:           fprintf( fp, "TYPE_HEARING" );            break;
	case TYPE_ARTICLE:           fprintf( fp, "TYPE_ARTICLE" );            break;
	case TYPE_LEGALRULE:         fprintf( fp, "TYPE_LEGALRULE" );          break;
	case TYPE_MAGARTICLE:        fprintf( fp, "TYPE_MAGARTICLE" );         break;
	case TYPE_MANUSCRIPT:        fprintf( fp, "TYPE_MANUSCRIPT" );         break;
	case TYPE_MAP:               fprintf( fp, "TYPE_MAP" );                break;
	case TYPE_NEWSARTICLE:       fprintf( fp, "TYPE_NEWSARTICLE" );        break;
	case TYPE_ONLINEDATABASE:    fprintf( fp, "TYPE_ONLINEDATABASE" );     break;
	case TYPE_ONLINEMULTIMEDIA:  fprintf( fp, "TYPE_ONLINEMULTIMEDIA" );   break;
	case TYPE_PATENT:            fprintf( fp, "TYPE_PATENT" );             break;
	case TYPE_COMMUNICATION:     fprintf( fp, "TYPE_COMMUNICATION" );      break;
	case TYPE_REPORT:            fprintf( fp, "TYPE_REPORT" );             break;
	case TYPE_STATUTE:           fprintf( fp, "TYPE_STATUTE" );            break;
	case TYPE_THESIS:            fprintf( fp, "TYPE_THESIS" );             break;
	case TYPE_MASTERSTHESIS:     fprintf( fp, "TYPE_MASTERSTHESIS" );      break;
	case TYPE_PHDTHESIS:         fprintf( fp, "TYPE_PHDTHESIS" );          break;
	case TYPE_DIPLOMATHESIS:     fprintf( fp, "TYPE_DIPLOMATHESIS" );      break;
	case TYPE_DOCTORALTHESIS:    fprintf( fp, "TYPE_DOCTORALTHESIS" );     break;
	case TYPE_HABILITATIONTHESIS:fprintf( fp, "TYPE_HABILITATIONTHESIS" ); break;
	case TYPE_UNPUBLISHED:       fprintf( fp, "TYPE_UNPUBLISHED" );        break;
	default:                     fprintf( fp, "Error - type not in enum" );break;
	}
}

static void
type_report_progress( param *p, const char *element_type, int type, unsigned long refnum )
{
	if ( p->verbose ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Type from %s element in reference %lu: ", element_type, refnum+1 );
		write_type( stderr, type );
		fprintf( stderr, "\n" );
	}
}

static int
type_from_default( fields *in, param *p, unsigned long refnum )
{
	int n, type;

	/* default to chapter if host terms */
	if ( fields_maxlevel( in ) > 0 ) type = TYPE_INBOOK;

	/* default to generic if no host terms */
	else type = TYPE_GENERIC;


	if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
	fprintf( stderr, "Cannot identify TYPE in reference %lu ", refnum+1 );
	n = fields_find( in, "REFNUM", LEVEL_ANY );
	if ( n!=FIELDS_NOTFOUND )
		fprintf( stderr, " %s", (char *) fields_value( in, n, FIELDS_CHRP ) );
	if ( type==TYPE_INBOOK )
		fprintf( stderr, " (defaulting to book chapter)\n" );
	else
		fprintf( stderr, " (defaulting to generic)\n" );

	return type;
}

static int
get_type( fields *in, param *p, unsigned long refnum )
{
	/* Comment out TYPE_GENERIC entries as that is default, but
         * keep in source as record of mapping decision. */
	match_type genre_matches[] = {
		/* MARC Authority elements */
		{ "art original",              TYPE_ARTWORK,            LEVEL_ANY  },
		{ "art reproduction",          TYPE_ARTWORK,            LEVEL_ANY  },
		{ "article",                   TYPE_ARTICLE,            LEVEL_ANY  },
		{ "atlas",                     TYPE_MAP,                LEVEL_ANY  },
		{ "autobiography",             TYPE_BOOK,               LEVEL_ANY  },
/*		{ "bibliography",              TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "biography",                 TYPE_BOOK,               LEVEL_ANY  },
		{ "book",                      TYPE_BOOK,               LEVEL_MAIN },
		{ "book",                      TYPE_INBOOK,             LEVEL_ANY  },
/*		{ "calendar",                  TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "catalog",                   TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "chart",                     TYPE_CHARTTABLE,         LEVEL_ANY  },
/*		{ "comic or graphic novel",    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "comic strip",               TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "conference publication",    TYPE_PROCEEDINGS,        LEVEL_ANY  },
		{ "database",                  TYPE_ONLINEDATABASE,     LEVEL_ANY  },
/*		{ "dictionary",                TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "diorama",                   TYPE_ARTWORK,            LEVEL_ANY  },
/*		{ "directory",                 TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "discography",               TYPE_AUDIOVISUAL,        LEVEL_ANY  },
/*		{ "drama",                     TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "encyclopedia",              TYPE_BOOK,               LEVEL_ANY  },
/*		{ "essay",                     TYPE_GENERIC,            LEVEL_ANY  }, */
		{ "festschrift",               TYPE_BOOK,               LEVEL_MAIN },
		{ "festschrift",               TYPE_INBOOK,             LEVEL_ANY  },
		{ "fiction",                   TYPE_BOOK,               LEVEL_ANY  },
		{ "filmography",               TYPE_FILMBROADCAST,      LEVEL_ANY  },
		{ "filmstrip",                 TYPE_FILMBROADCAST,      LEVEL_ANY  },
/*		{ "finding aid",               TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "flash card",                TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "folktale",                  TYPE_CLASSICALWORK,      LEVEL_ANY  },
		{ "font",                      TYPE_ELECTRONIC,         LEVEL_ANY  },
/*		{ "game",                      TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "government publication",    TYPE_GOVERNMENT,         LEVEL_ANY  },
		{ "graphic",                   TYPE_FIGURE,             LEVEL_ANY  },
		{ "globe",                     TYPE_MAP,                LEVEL_ANY  },
/*		{ "handbook",                  TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "history",                   TYPE_BOOK,               LEVEL_ANY  },
		{ "hymnal",                    TYPE_BOOK,               LEVEL_MAIN },
		{ "hymnal",                    TYPE_INBOOK,             LEVEL_ANY  },
/*		{ "humor, satire",             TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "index",                     TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "instruction",               TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "interview",                 TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "issue",                     TYPE_ARTICLE,            LEVEL_ANY  },
		{ "journal",                   TYPE_ARTICLE,            LEVEL_ANY  },
/*		{ "kit",                       TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "language instruction",      TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "law report or digest",      TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "legal article",             TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "legal case and case notes", TYPE_CASE,               LEVEL_ANY  },
		{ "legislation",               TYPE_BILL,               LEVEL_ANY  },
		{ "letter",                    TYPE_COMMUNICATION,      LEVEL_ANY  },
		{ "loose-leaf",                TYPE_GENERIC,            LEVEL_ANY  },
		{ "map",                       TYPE_MAP,                LEVEL_ANY  },
/*		{ "memoir",                    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "microscope slide",          TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "model",                     TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "motion picture",            TYPE_AUDIOVISUAL,        LEVEL_ANY  },
		{ "multivolume monograph",     TYPE_BOOK,               LEVEL_ANY  },
		{ "newspaper",                 TYPE_NEWSARTICLE,        LEVEL_ANY  },
		{ "novel",                     TYPE_BOOK,               LEVEL_ANY  },
/*		{ "numeric data",              TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "offprint",                  TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "online system or service",  TYPE_ELECTRONIC,         LEVEL_ANY  },
		{ "patent",                    TYPE_PATENT,             LEVEL_ANY  },
		{ "picture",                   TYPE_ARTWORK,            LEVEL_ANY  },
/*		{ "poetry",                    TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "programmed text",           TYPE_PROGRAM,            LEVEL_ANY  },
/*		{ "realia",                    TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "rehearsal",                 TYPE_AUDIOVISUAL,        LEVEL_ANY  },
/*		{ "remote sensing image",      TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "reporting",                 TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "report",                    TYPE_REPORT,             LEVEL_ANY  },
/*		{ "review",                    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "script",                    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "series",                    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "short story",               TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "slide",                     TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "sound",                     TYPE_AUDIOVISUAL,        LEVEL_ANY  },
/*		{ "speech",                    TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "standard or specification", TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "statistics",                TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "survey of literature",      TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "technical drawing",         TYPE_ARTWORK,            LEVEL_ANY  },
		{ "technical report",          TYPE_REPORT,             LEVEL_ANY  },
/*		{ "toy",                       TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "transparency",              TYPE_GENERIC,            LEVEL_ANY  },*/
/*		{ "treaty",                    TYPE_GENERIC,            LEVEL_ANY  },*/
		{ "videorecording",            TYPE_AUDIOVISUAL,        LEVEL_ANY  },
		{ "web site",                  TYPE_ELECTRONIC,         LEVEL_ANY  },
		/* Non-MARC Authority elements */
		{ "academic journal",          TYPE_ARTICLE,            LEVEL_ANY  },
		{ "collection",                TYPE_BOOK,               LEVEL_MAIN },
		{ "collection",                TYPE_INBOOK,             LEVEL_ANY  },
		{ "magazine",                  TYPE_MAGARTICLE,         LEVEL_ANY  },
		{ "hearing",                   TYPE_HEARING,            LEVEL_ANY  },
		{ "Ph.D. thesis",              TYPE_PHDTHESIS,          LEVEL_ANY  },
		{ "Masters thesis",            TYPE_MASTERSTHESIS,      LEVEL_ANY  },
		{ "Diploma thesis",            TYPE_DIPLOMATHESIS,      LEVEL_ANY  },
		{ "Doctoral thesis",           TYPE_DOCTORALTHESIS,     LEVEL_ANY  },
		{ "Habilitation thesis",       TYPE_HABILITATIONTHESIS, LEVEL_ANY  },
		{ "Licentiate thesis",         TYPE_LICENTIATETHESIS,   LEVEL_ANY  },
		{ "communication",             TYPE_COMMUNICATION,      LEVEL_ANY  },
		{ "manuscript",                TYPE_MANUSCRIPT,         LEVEL_ANY  },
		{ "unpublished",               TYPE_UNPUBLISHED,        LEVEL_ANY  },
		/* Delayed MARC Authority elements */
		{ "thesis",                    TYPE_THESIS,             LEVEL_ANY  },
		{ "periodical",                TYPE_MAGARTICLE,         LEVEL_ANY  },
	};
	int ngenre_matches = sizeof( genre_matches ) / sizeof( genre_matches[0] );

	match_type resource_matches[] = {
		{ "moving image",              TYPE_FILMBROADCAST,      LEVEL_ANY  },
		{ "software, multimedia",      TYPE_PROGRAM,            LEVEL_ANY  },
	};
	int nresource_matches = sizeof( resource_matches ) / sizeof( resource_matches[0] );

	match_type issuance_matches[] = {
		{ "monographic",               TYPE_BOOK,               LEVEL_MAIN },
		{ "monographic",               TYPE_INBOOK,             LEVEL_ANY  },
	};
	int nissuance_matches = sizeof( issuance_matches ) / sizeof( issuance_matches[0] );

	int type;

	type = type_from_mods_hints( in, TYPE_FROM_GENRE, genre_matches, ngenre_matches, TYPE_UNKNOWN );
	type_report_progress( p, "genre", type, refnum );
	if ( type!=TYPE_UNKNOWN ) return type;

	type = type_from_mods_hints( in, TYPE_FROM_RESOURCE, resource_matches, nresource_matches, TYPE_UNKNOWN );
	type_report_progress( p, "resource", type, refnum );
	if ( type!=TYPE_UNKNOWN ) return type;

	type = type_from_mods_hints( in, TYPE_FROM_ISSUANCE, issuance_matches, nissuance_matches, TYPE_UNKNOWN );
	type_report_progress( p, "issuance", type, refnum );
	if ( type!=TYPE_UNKNOWN ) return type;

	return type_from_default( in, p, refnum );
}

static void
append_type( int type, fields *out, param *p, int *status )
{
	/* These are restricted to Endnote-defined types */
	match_type genrenames[] = {
		{ "Generic",                TYPE_GENERIC,            0 },
		{ "Artwork",                TYPE_ARTWORK,            0 },
		{ "Audiovisual Material",   TYPE_AUDIOVISUAL,        0 },
		{ "Bill",                   TYPE_BILL,               0 },
		{ "Book",                   TYPE_BOOK,               0 },
		{ "Book Section",           TYPE_INBOOK,             0 },
		{ "Case",                   TYPE_CASE,               0 },
		{ "Chart or Table",         TYPE_CHARTTABLE,         0 },
		{ "Classical Work",         TYPE_CLASSICALWORK,      0 },
		{ "Computer Program",       TYPE_PROGRAM,            0 },
		{ "Conference Paper",       TYPE_INPROCEEDINGS,      0 },
		{ "Conference Proceedings", TYPE_PROCEEDINGS,        0 },
		{ "Edited Book",            TYPE_EDITEDBOOK,         0 },
		{ "Equation",               TYPE_EQUATION,           0 },
		{ "Electronic Article",     TYPE_ELECTRONICARTICLE,  0 },
		{ "Electronic Book",        TYPE_ELECTRONICBOOK,     0 },
		{ "Electronic Source",      TYPE_ELECTRONIC,         0 },
		{ "Figure",                 TYPE_FIGURE,             0 },
		{ "Film or Broadcast",      TYPE_FILMBROADCAST,      0 },
		{ "Government Document",    TYPE_GOVERNMENT,         0 },
		{ "Hearing",                TYPE_HEARING,            0 },
		{ "Journal Article",        TYPE_ARTICLE,            0 },
		{ "Legal Rule/Regulation",  TYPE_LEGALRULE,          0 },
		{ "Magazine Article",       TYPE_MAGARTICLE,         0 },
		{ "Manuscript",             TYPE_MANUSCRIPT,         0 },
		{ "Map",                    TYPE_MAP,                0 },
		{ "Newspaper Article",      TYPE_NEWSARTICLE,        0 },
		{ "Online Database",        TYPE_ONLINEDATABASE,     0 },
		{ "Online Multimedia",      TYPE_ONLINEMULTIMEDIA,   0 },
		{ "Patent",                 TYPE_PATENT,             0 },
		{ "Personal Communication", TYPE_COMMUNICATION,      0 },
		{ "Report",                 TYPE_REPORT,             0 },
		{ "Statute",                TYPE_STATUTE,            0 },
		{ "Thesis",                 TYPE_THESIS,             0 },
		{ "Thesis",                 TYPE_PHDTHESIS,          0 },
		{ "Thesis",                 TYPE_MASTERSTHESIS,      0 },
		{ "Thesis",                 TYPE_DIPLOMATHESIS,      0 },
		{ "Thesis",                 TYPE_DOCTORALTHESIS,     0 },
		{ "Thesis",                 TYPE_HABILITATIONTHESIS, 0 },
		{ "Unpublished Work",       TYPE_UNPUBLISHED,        0 },
	};
	int ngenrenames = sizeof( genrenames ) / sizeof( genrenames[0] );
	int i, fstatus, found = 0;
	for ( i=0; i<ngenrenames && !found; ++i ) {
		if ( genrenames[i].type == type ) {
			fstatus = fields_add( out, "%0", genrenames[i].name, LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
			found = 1;
		}
	}
	if ( !found ) {
		fstatus = fields_add( out, "%0", "Generic", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot identify type %d\n", type );
	}
}

static int
append_title( fields *in, char *full, char *sub, int level, fields *out, char *endtag, int *status )
{
	str *mainttl = fields_findv( in, level, FIELDS_STRP, full );
	str *subttl  = fields_findv( in, level, FIELDS_STRP, sub );
	str fullttl;
	int fstatus;

	str_init( &fullttl );
	title_combine( &fullttl, mainttl, subttl );

	if ( str_memerr( &fullttl ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( str_has_value( &fullttl ) ) {
		fstatus = fields_add( out, endtag, str_cstr( &fullttl ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
out:
	str_free( &fullttl );
	return 1;
}

static void
append_people( fields *in, char *tag, char *entag, int level, fields *out, int *status )
{
	int i, n, flvl, fstatus;
	str oneperson;
	char *ftag;

	str_init( &oneperson );
	n = fields_num( in );
	for ( i=0; i<n; ++i ) {
		flvl = fields_level( in, i );
		if ( level!=LEVEL_ANY && flvl!=level ) continue;
		ftag = fields_tag( in, i, FIELDS_CHRP );
		if ( !strcasecmp( ftag, tag ) ) {
			name_build_withcomma( &oneperson, fields_value( in, i, FIELDS_CHRP ) );
			fstatus = fields_add_can_dup( out, entag, str_cstr( &oneperson ), LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		}
	}
	str_free( &oneperson );
}

static void
append_pages( fields *in, fields *out, int *status )
{
	str *sn, *en;
	int fstatus;
	str pages;
	char *ar;

	sn = fields_findv( in, LEVEL_ANY, FIELDS_STRP, "PAGES:START" );
	en = fields_findv( in, LEVEL_ANY, FIELDS_STRP, "PAGES:STOP" );
	if ( sn || en ) {
		str_init( &pages );
		if ( sn ) str_strcpy( &pages, sn );
		if ( sn && en ) str_strcatc( &pages, "-" );
		if ( en ) str_strcat( &pages, en );
		if ( str_memerr( &pages ) ) { *status = BIBL_ERR_MEMERR; str_free( &pages ); return; }
		fstatus = fields_add( out, "%P", str_cstr( &pages ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		str_free( &pages );
	} else {
		ar = fields_findv( in, LEVEL_ANY, FIELDS_CHRP, "ARTICLENUMBER" );
		if ( ar ) {
			fstatus = fields_add( out, "%P", ar, LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		}
	}
}

static void
append_urls( fields *in, fields *out, int *status )
{
	int lstatus;
	slist types;

	lstatus = slist_init_valuesc( &types, "URL", "DOI", "PMID", "PMC", "ARXIV", "JSTOR", "MRNUMBER", NULL );
	if ( lstatus!=SLIST_OK ) {
		*status = BIBL_ERR_MEMERR;
		return;
	}

	*status = urls_merge_and_add( in, LEVEL_ANY, out, "%U", LEVEL_MAIN, &types );

	slist_free( &types );
}

static void
append_year( fields *in, fields *out, int *status )
{
	int fstatus;
	char *year;

	year = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:YEAR", "PARTDATE:YEAR", NULL );
	if ( year ) {
		fstatus = fields_add( out, "%D", year, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
append_monthday( fields *in, fields *out, int *status )
{
	const char *month, *use, *day;
	str month_day;
	int fstatus;

	str_init( &month_day );
	month = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:MONTH", "PARTDATE:MONTH", NULL );
	day   = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:DAY",   "PARTDATE:DAY",   NULL );
	if ( month || day ) {
		if ( month ) {
			(void) number_to_full_month( month, &use );
			str_strcpyc( &month_day, use );
		}
		if ( month && day ) str_strcatc( &month_day, " " );
		if ( day ) str_strcatc( &month_day, day );
		fstatus = fields_add( out, "%8", str_cstr( &month_day ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	str_free( &month_day );
}

static void
append_genrehint_core( int type, fields *out, vplist *a, int *status )
{
	vplist_index i;
	int fstatus;
	char *g;

	for ( i=0; i<a->n; ++i ) {
		g = ( char * ) vplist_get( a, i );
		if ( !strcmp( g, "journal article" ) && type==TYPE_ARTICLE ) continue;
		if ( !strcmp( g, "academic journal" ) && type==TYPE_ARTICLE ) continue;
		if ( !strcmp( g, "collection" ) && type==TYPE_INBOOK ) continue;
		if ( !strcmp( g, "television broadcast" ) && type==TYPE_FILMBROADCAST ) continue;
		if ( !strcmp( g, "electronic" ) && type==TYPE_PROGRAM ) continue;
		if ( !strcmp( g, "magazine" ) && type==TYPE_MAGARTICLE ) continue;
		if ( !strcmp( g, "miscellaneous" ) && type==TYPE_GENERIC ) continue;
		if ( !strcmp( g, "hearing" ) && type==TYPE_HEARING ) continue;
		if ( !strcmp( g, "communication" ) && type==TYPE_COMMUNICATION ) continue;
		if ( !strcmp( g, "report" ) && type==TYPE_REPORT ) continue;
		if ( !strcmp( g, "book chapter" ) && type==TYPE_INBOOK ) continue;
		fstatus = fields_add( out, "%9", g, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			return;
		}
	}
}

static void
append_genrehint( int type, fields *in, fields *out, int *status )
{
	vplist a;

	vplist_init( &a );

	fields_findv_each( in, LEVEL_ANY, FIELDS_CHRP, &a, "GENRE:BIBUTILS" );
	append_genrehint_core( type, out, &a, status );

	vplist_empty( &a );

	fields_findv_each( in, LEVEL_ANY, FIELDS_CHRP, &a, "GENRE:UNKNOWN" );
	append_genrehint_core( type, out, &a, status );

	vplist_free( &a );
}

static void
append_thesishint( int type, fields *out, int *status )
{
	int fstatus;

	if ( type==TYPE_MASTERSTHESIS ) {
		fstatus = fields_add( out, "%9", "Masters thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_PHDTHESIS ) {
		fstatus = fields_add( out, "%9", "Ph.D. thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_DIPLOMATHESIS ) {
		fstatus = fields_add( out, "%9", "Diploma thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_DOCTORALTHESIS ) {
		fstatus = fields_add( out, "%9", "Doctoral thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_HABILITATIONTHESIS ) {
		fstatus = fields_add( out, "%9", "Habilitation thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_LICENTIATETHESIS ) {
		fstatus = fields_add( out, "%9", "Licentiate thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static int
endout_assemble( fields *in, fields *out, param *pm, unsigned long refnum )
{
	int added, type, status = BIBL_OK;

	fields_clear_used( in );

	type = get_type( in, pm, refnum );

	append_type( type, out, pm, &status );

	added = append_title( in, "TITLE",      "SUBTITLE",      LEVEL_MAIN, out, "%T", &status );
	if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", LEVEL_MAIN, out, "%T", &status );
	else            append_title( in, "SHORTTITLE", "SHORTSUBTITLE", LEVEL_MAIN, out, "%!", &status );

	append_people( in, "AUTHOR",     "%A", LEVEL_MAIN, out, &status );
	append_people( in, "EDITOR",     "%E", LEVEL_MAIN, out, &status );
	if ( type==TYPE_ARTICLE || type==TYPE_MAGARTICLE || type==TYPE_ELECTRONICARTICLE || type==TYPE_NEWSARTICLE )
		append_people( in, "EDITOR", "%E", LEVEL_HOST, out, &status );
	else if ( type==TYPE_INBOOK || type==TYPE_INPROCEEDINGS ) {
		append_people( in, "EDITOR", "%E", LEVEL_HOST, out, &status );
	} else {
		append_people( in, "EDITOR", "%Y", LEVEL_HOST, out, &status );
	}
	append_people( in, "TRANSLATOR", "%H", LEVEL_ANY,    out, &status  );

	append_people( in, "AUTHOR",     "%Y", LEVEL_SERIES, out, &status );
	append_people( in, "EDITOR",     "%Y", LEVEL_SERIES, out, &status );

	if ( type==TYPE_CASE ) {
		append_easy   ( in, "AUTHOR:CORP", LEVEL_MAIN, out, "%I", &status );
		append_easy   ( in, "AUTHOR:ASIS", LEVEL_MAIN, out, "%I", &status );
	}
	else if ( type==TYPE_HEARING ) {
		append_easyall( in, "AUTHOR:CORP", LEVEL_MAIN, out, "%S", &status );
		append_easyall( in, "AUTHOR:ASIS", LEVEL_MAIN, out, "%S", &status );
	}
	else if ( type==TYPE_NEWSARTICLE ) {
		append_people ( in, "REPORTER",        "%A", LEVEL_MAIN, out, &status );
		append_people ( in, "REPORTER:CORP",   "%A", LEVEL_MAIN, out, &status );
		append_people ( in, "REPORTER:ASIS",   "%A", LEVEL_MAIN, out, &status );
	}
	else if ( type==TYPE_COMMUNICATION ) {
		append_people ( in, "ADDRESSEE",       "%E", LEVEL_ANY,  out, &status  );
		append_people ( in, "ADDRESSEE:CORP",  "%E", LEVEL_ANY,  out, &status  );
		append_people ( in, "ADDRESSEE:ASIS",  "%E", LEVEL_ANY,  out, &status  );
	}
	else {
		append_easyall( in, "AUTHOR:CORP",     LEVEL_MAIN, out, "%A", &status );
		append_easyall( in, "AUTHOR:ASIS",     LEVEL_MAIN, out, "%A", &status );
		append_easyall( in, "EDITOR:CORP",     LEVEL_ANY,  out, "%E", &status  );
		append_easyall( in, "EDITOR:ASIS",     LEVEL_ANY,  out, "%E", &status  );
		append_easyall( in, "TRANSLATOR:CORP", LEVEL_ANY,  out, "%H", &status  );
		append_easyall( in, "TRANSLATOR:ASIS", LEVEL_ANY,  out, "%H", &status  );
	}

	if ( type==TYPE_ARTICLE || type==TYPE_MAGARTICLE || type==TYPE_ELECTRONICARTICLE || type==TYPE_NEWSARTICLE ) {
		added = append_title( in, "TITLE", "SUBTITLE", LEVEL_HOST, out, "%J", &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", LEVEL_HOST, out, "%J", &status );
	}

	else if ( type==TYPE_INBOOK || type==TYPE_INPROCEEDINGS ) {
		added = append_title( in, "TITLE", "SUBTITLE", LEVEL_HOST, out, "%B", &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", LEVEL_HOST, out, "%B", &status );
	}

	else {
		added = append_title( in, "TITLE", "SUBTITLE", LEVEL_HOST, out, "%S", &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", LEVEL_HOST, out, "%S", &status );
	}

	if ( type!=TYPE_CASE && type!=TYPE_HEARING ) {
		append_title( in, "TITLE", "SUBTITLE", LEVEL_SERIES, out, "%S", &status );
	}

	append_year    ( in, out, &status );
	append_monthday( in, out, &status );

	append_easy     ( in, "VOLUME",             LEVEL_ANY, out, "%V", &status );
	append_easy     ( in, "ISSUE",              LEVEL_ANY, out, "%N", &status );
	append_easy     ( in, "NUMBER",             LEVEL_ANY, out, "%N", &status );
	append_easy     ( in, "EDITION",            LEVEL_ANY, out, "%7", &status );
	append_easy     ( in, "PUBLISHER",          LEVEL_ANY, out, "%I", &status );
	append_easycombo( in, "ADDRESS",            LEVEL_ANY, out, "%C", "; ", &status );
	append_easy     ( in, "DEGREEGRANTOR",      LEVEL_ANY, out, "%C", &status );
	append_easy     ( in, "DEGREEGRANTOR:CORP", LEVEL_ANY, out, "%C", &status );
	append_easy     ( in, "DEGREEGRANTOR:ASIS", LEVEL_ANY, out, "%C", &status );
	append_easy     ( in, "SERIALNUMBER",       LEVEL_ANY, out, "%@", &status );
	append_easy     ( in, "ISSN",               LEVEL_ANY, out, "%@", &status );
	append_easy     ( in, "ISBN",               LEVEL_ANY, out, "%@", &status );
	append_easy     ( in, "LANGUAGE",           LEVEL_ANY, out, "%G", &status );
	append_easy     ( in, "REFNUM",             LEVEL_ANY, out, "%F", &status );
	append_easyall  ( in, "NOTES",              LEVEL_ANY, out, "%O", &status );
	append_easy     ( in, "ABSTRACT",           LEVEL_ANY, out, "%X", &status );
	append_easy     ( in, "CLASSIFICATION"   ,  LEVEL_ANY, out, "%L", &status );
	append_easyall  ( in, "KEYWORD",            LEVEL_ANY, out, "%K", &status );
	append_genrehint ( type, in, out, &status );
	append_thesishint( type, out, &status );
	append_easyall  ( in, "DOI",                LEVEL_ANY, out, "%R", &status );
	append_easyall  ( in, "URL",                LEVEL_ANY, out, "%U", &status );
	append_easyall  ( in, "FILEATTACH",         LEVEL_ANY, out, "%U", &status );
	append_urls     ( in, out, &status );
	append_pages    ( in, out, &status );

	return status;
}

/*****************************************************
 PUBLIC: int endout_write()
*****************************************************/

static int
endout_write( fields *out, FILE *fp, param *pm, unsigned long refnum )
{
	int i;

	for ( i=0; i<out->n; ++i ) {
		fprintf( fp, "%s %s\n",
			(char*) fields_tag( out, i, FIELDS_CHRP ),
			(char*) fields_value( out, i, FIELDS_CHRP )
		);
	}

	fprintf( fp, "\n" );
	fflush( fp );
	return BIBL_OK;
}
