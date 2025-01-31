/*
 * modsout.c
 *
 * Copyright (c) Chris Putnam 2003-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "is_ws.h"
#include "str.h"
#include "charsets.h"
#include "str_conv.h"
#include "fields.h"
#include "iso639_2.h"
#include "utf8.h"
#include "modstypes.h"
#include "bu_auth.h"
#include "marc_auth.h"
#include "bibformats.h"

/*****************************************************
 PUBLIC: int modsout_initparams()
*****************************************************/

static void modsout_writeheader( FILE *outptr, param *p );
static void modsout_writefooter( FILE *outptr );
static int  modsout_write( fields *info, FILE *outptr, param *p, unsigned long numrefs );

int
modsout_initparams( param *pm, const char *progname )
{
	pm->writeformat      = BIBL_MODSOUT;
	pm->format_opts      = 0;
	pm->charsetout       = BIBL_CHARSET_UNICODE;
	pm->charsetout_src   = BIBL_SRC_DEFAULT;
	pm->latexout         = 0;
	pm->utf8out          = 1;
	pm->utf8bom          = 1;
	pm->xmlout           = BIBL_XMLOUT_TRUE;
	pm->nosplittitle     = 0;
	pm->verbose          = 0;
	pm->addcount         = 0;
	pm->singlerefperfile = 0;

	pm->headerf   = modsout_writeheader;
	pm->footerf   = modsout_writefooter;
	pm->assemblef = NULL;
	pm->writef    = modsout_write;

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
 PUBLIC: int modsout_write()
*****************************************************/

/* output_tag()
 *
 * mode & TAG_OPEN,         "<tag>"
 * mode & TAG_CLOSE,        "</tag>"
 * mode & TAG_OPENCLOSE,    "<tag>data</tag>"
 * mode & TAG_SELFCLOSE,    "<tag/>"
 *
 * mode & TAG_NEWLINE,      "<tag>\n"
 *
 */
#define TAG_NONEWLINE (0)
#define TAG_OPEN      (1)
#define TAG_CLOSE     (2)
#define TAG_OPENCLOSE (4)
#define TAG_SELFCLOSE (8)
#define TAG_NEWLINE   (16)

static void
output_tag_core( FILE *outptr, int nindents, const char *tag, const char *data, unsigned char mode, va_list *attrs )
{
	const char *attr, *val;
	int i;

	for ( i=0; i<nindents; ++i ) fprintf( outptr, "    " );

	if ( mode & TAG_CLOSE ) fprintf( outptr, "</%s", tag );
	else                    fprintf( outptr, "<%s",  tag );

	do {
		attr = va_arg( *attrs, const char * );
		if ( attr ) val  = va_arg( *attrs, const char * );
		if ( attr && val ) fprintf( outptr, " %s=\"%s\"", attr, val );
	} while ( attr && val );

	if ( mode & TAG_SELFCLOSE ) fprintf( outptr, "/>" );
	else                        fprintf( outptr, ">" );

	if ( mode & TAG_OPENCLOSE ) fprintf( outptr, "%s</%s>", data, tag );

	if ( mode & TAG_NEWLINE   ) fprintf( outptr, "\n" );
}

/* output_tag()
 *
 *     output XML tag
 *
 * mode     = [ TAG_OPEN | TAG_CLOSE | TAG_OPENCLOSE | TAG_SELFCLOSE | TAG_NEWLINE ]
 *
 * for mode TAG_OPENCLOSE, ensure that value is non-NULL, as string pointed to by value
 * will be output in the tag
 */
static void
output_tag( FILE *outptr, int nindents, const char *tag, const char *value, unsigned char mode, ... )
{
	va_list attrs;

	va_start( attrs, mode );
	output_tag_core( outptr, nindents, tag, value, mode, &attrs );
	va_end( attrs );
}

/* output_fil()
 *
 *     output XML tag, but lookup data in fields struct
 *
 * mode     = [ TAG_OPEN | TAG_CLOSE | TAG_OPENCLOSE | TAG_SELFCLOSE | TAG_NEWLINE ]
 */
static void
output_fil( FILE *outptr, int nindents, const char *tag, fields *f, int n, unsigned char mode, ... )
{
	va_list attrs;
	char *value;

	if ( n==FIELDS_NOTFOUND ) return;

	value = (char *) fields_value( f, n, FIELDS_CHRP );
	va_start( attrs, mode );
	output_tag_core( outptr, nindents, tag, value, mode, &attrs );
	va_end( attrs );
}

/* output_vpl()
 *
 *       output XML tag for each element in the vplist
 *
 * mode     = [ TAG_OPEN | TAG_CLOSE | TAG_OPENCLOSE | TAG_SELFCLOSE | TAG_NEWLINE ]
 */

static void
output_vpl( FILE *outptr, int nindents, const char *tag, vplist *values, unsigned char mode, ... )
{
	vplist_index i;
	va_list attrs;
	char *value;

	/* need to reinitialize attrs for each loop */
	for ( i=0; i<values->n; ++i ) {
		va_start( attrs, mode );
		value = vplist_get( values, i );
		output_tag_core( outptr, nindents, tag, value, mode, &attrs );
		va_end( attrs );
	}
}

/*
 * lvl2indent()
 *
 * 	Since levels can be negative (source items), need to do a simple
 * 	calculation to determine the number of "tabs" to put before xml tag
 */
static inline int
lvl2indent( int level )
{
	if ( level < -1 ) return -level + 1;
	else return level + 1;
}

/*
 * incr_level()
 *
 * 	Increment positive levels (normal) or decrement negative levels (souce items)
 */
static inline int
incr_level( int level, int amount )
{
	if ( level > -1 ) return level + amount;
	else return level - amount;
}

static void
output_title( FILE *outptr, fields *f, int level )
{
	int ttl    = fields_find( f, "TITLE", level );
	int subttl = fields_find( f, "SUBTITLE", level );
	int shrttl = fields_find( f, "SHORTTITLE", level );
	int parttl = fields_find( f, "PARTTITLE", level );
	int indent1, indent2;
	char *val;


	indent1 = lvl2indent( level );
	indent2 = lvl2indent( incr_level( level, 1 ) );


	/* output main title */
	output_tag( outptr, indent1, "titleInfo", NULL,      TAG_OPEN      | TAG_NEWLINE, NULL );
	output_fil( outptr, indent2, "title",     f, ttl,    TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	output_fil( outptr, indent2, "subTitle",  f, subttl, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	output_fil( outptr, indent2, "partName",  f, parttl, TAG_OPENCLOSE | TAG_NEWLINE, NULL );

	/* MODS output doesn't verify if we don't at least have a <title/> element */
	if ( ttl==FIELDS_NOTFOUND && subttl==FIELDS_NOTFOUND && parttl==FIELDS_NOTFOUND )
		output_tag( outptr, indent2, "title", NULL,  TAG_SELFCLOSE | TAG_NEWLINE, NULL );

	output_tag( outptr, indent1, "titleInfo", NULL,      TAG_CLOSE     | TAG_NEWLINE, NULL );


	/* output shorttitle if it's different from normal title */
	if ( shrttl==FIELDS_NOTFOUND ) return;

	val = (char *) fields_value( f, shrttl, FIELDS_CHRP );
	if ( ttl==FIELDS_NOTFOUND || subttl!=FIELDS_NOTFOUND || strcmp(fields_value(f,ttl,FIELDS_CHRP),val) ) {
		output_tag( outptr, indent1, "titleInfo", NULL, TAG_OPEN      | TAG_NEWLINE, "type", "abbreviated", NULL );
		output_tag( outptr, indent2, "title",     val,  TAG_OPENCLOSE | TAG_NEWLINE, NULL );
		output_tag( outptr, indent1, "titleInfo", NULL, TAG_CLOSE     | TAG_NEWLINE, NULL );
	}
}

static void
output_name( FILE *outptr, char *p, int level )
{
	str family, part, suffix;
	int n=0;

	strs_init( &family, &part, &suffix, NULL );

	while ( *p && *p!='|' ) str_addchar( &family, *p++ );
	if ( *p=='|' ) p++;

	while ( *p ) {
		while ( *p && *p!='|' ) str_addchar( &part, *p++ );
		/* truncate periods from "A. B. Jones" names */
		if ( part.len ) {
			if ( part.len==2 && part.data[1]=='.' ) {
				part.len=1;
				part.data[1]='\0';
			}
			if ( n==0 )
				output_tag( outptr, lvl2indent(level), "name", NULL, TAG_OPEN | TAG_NEWLINE, "type", "personal", NULL );
			output_tag( outptr, lvl2indent(incr_level(level,1)), "namePart", part.data, TAG_OPENCLOSE | TAG_NEWLINE, "type", "given", NULL );
			n++;
		}
		if ( *p=='|' ) {
			p++;
			if ( *p=='|' ) {
				p++;
				while ( *p && *p!='|' ) str_addchar( &suffix, *p++ );
			}
			str_empty( &part );
		}
	}

	if ( family.len ) {
		if ( n==0 )
			output_tag( outptr, lvl2indent(level), "name", NULL, TAG_OPEN | TAG_NEWLINE, "type", "personal", NULL );
		output_tag( outptr, lvl2indent(incr_level(level,1)), "namePart", family.data, TAG_OPENCLOSE | TAG_NEWLINE, "type", "family", NULL );
		n++;
	}

	if ( suffix.len ) {
		if ( n==0 )
			output_tag( outptr, lvl2indent(level), "name", NULL, TAG_OPEN | TAG_NEWLINE, "type", "personal", NULL );
		output_tag( outptr, lvl2indent(incr_level(level,1)), "namePart", suffix.data, TAG_OPENCLOSE | TAG_NEWLINE, "type", "suffix", NULL );
	}

	strs_free( &part, &family, &suffix, NULL );
}


/* MODS v 3.4
 *
 * <name [type="corporation"/type="conference"]>
 *    <namePart></namePart>
 *    <displayForm></displayForm>
 *    <affiliation></affiliation>
 *    <role>
 *        <roleTerm [authority="marcrealtor"] type="text"></roleTerm>
 *    </role>
 *    <description></description>
 * </name>
 */

#define NO_AUTHORITY (0)
#define MARC_AUTHORITY (1)

static void
output_names( FILE *outptr, fields *f, int level )
{
	convert2   names[] = {
	  { "author",                              "AUTHOR",          0, MARC_AUTHORITY },
	  { "editor",                              "EDITOR",          0, MARC_AUTHORITY },
	  { "annotator",                           "ANNOTATOR",       0, MARC_AUTHORITY },
	  { "artist",                              "ARTIST",          0, MARC_AUTHORITY },
	  { "author",                              "2ND_AUTHOR",      0, MARC_AUTHORITY },
	  { "author",                              "3RD_AUTHOR",      0, MARC_AUTHORITY },
	  { "author",                              "SUB_AUTHOR",      0, MARC_AUTHORITY },
	  { "author",                              "COMMITTEE",       0, MARC_AUTHORITY },
	  { "author",                              "COURT",           0, MARC_AUTHORITY },
	  { "author",                              "LEGISLATIVEBODY", 0, MARC_AUTHORITY },
	  { "author of afterword, colophon, etc.", "AFTERAUTHOR",     0, MARC_AUTHORITY },
	  { "author of introduction, etc.",        "INTROAUTHOR",     0, MARC_AUTHORITY },
	  { "cartographer",                        "CARTOGRAPHER",    0, MARC_AUTHORITY },
	  { "collaborator",                        "COLLABORATOR",    0, MARC_AUTHORITY },
	  { "commentator",                         "COMMENTATOR",     0, MARC_AUTHORITY },
	  { "compiler",                            "COMPILER",        0, MARC_AUTHORITY },
	  { "degree grantor",                      "DEGREEGRANTOR",   0, MARC_AUTHORITY },
	  { "director",                            "DIRECTOR",        0, MARC_AUTHORITY },
	  { "event",                               "EVENT",           0, NO_AUTHORITY   },
	  { "inventor",                            "INVENTOR",        0, MARC_AUTHORITY },
	  { "organizer of meeting",                "ORGANIZER",       0, MARC_AUTHORITY },
	  { "patent holder",                       "ASSIGNEE",        0, MARC_AUTHORITY },
	  { "performer",                           "PERFORMER",       0, MARC_AUTHORITY },
	  { "producer",                            "PRODUCER",        0, MARC_AUTHORITY },
	  { "addressee",                           "ADDRESSEE",       0, MARC_AUTHORITY },
	  { "redactor",                            "REDACTOR",        0, MARC_AUTHORITY },
	  { "reporter",                            "REPORTER",        0, MARC_AUTHORITY },
	  { "sponsor",                             "SPONSOR",         0, MARC_AUTHORITY },
	  { "translator",                          "TRANSLATOR",      0, MARC_AUTHORITY },
	  { "writer",                              "WRITER",          0, MARC_AUTHORITY },
	};
	int ntypes = sizeof( names ) / sizeof( names[0] );

	int f_asis, f_corp, f_conf;
	int i, n, nfields;
	str role;

	str_init( &role );
	nfields = fields_num( f );
	for ( n=0; n<ntypes; ++n ) {
		for ( i=0; i<nfields; ++i ) {
			if ( fields_level( f, i )!=level ) continue;
			if ( fields_no_value( f, i ) ) continue;
			f_asis = f_corp = f_conf = 0;
			str_strcpy( &role, fields_tag( f, i, FIELDS_STRP ) );
			if ( str_findreplace( &role, ":ASIS", "" )) f_asis=1;
			if ( str_findreplace( &role, ":CORP", "" )) f_corp=1;
			if ( str_findreplace( &role, ":CONF", "" )) f_conf=1;
			if ( strcasecmp( role.data, names[n].internal ) )
				continue;
			if ( f_asis ) {
				output_tag( outptr, lvl2indent(level),               "name",     NULL, TAG_OPEN      | TAG_NEWLINE, NULL );
				output_fil( outptr, lvl2indent(incr_level(level,1)), "namePart", f, i, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
			} else if ( f_corp ) {
				output_tag( outptr, lvl2indent(level),               "name",     NULL, TAG_OPEN      | TAG_NEWLINE, "type", "corporate", NULL );
				output_fil( outptr, lvl2indent(incr_level(level,1)), "namePart", f, i, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
			} else if ( f_conf ) {
				output_tag( outptr, lvl2indent(level),               "name",     NULL, TAG_OPEN      |  TAG_NEWLINE, "type", "conference", NULL );
				output_fil( outptr, lvl2indent(incr_level(level,1)), "namePart", f, i, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
			} else {
				output_name(outptr, fields_value( f, i, FIELDS_CHRP ), level);
			}
			output_tag( outptr, lvl2indent(incr_level(level,1)), "role", NULL, TAG_OPEN | TAG_NEWLINE, NULL );
			if ( names[n].code & MARC_AUTHORITY )
				output_tag( outptr, lvl2indent(incr_level(level,2)), "roleTerm", names[n].mods, TAG_OPENCLOSE | TAG_NEWLINE, "authority", "marcrelator", "type", "text", NULL );
			else
				output_tag( outptr, lvl2indent(incr_level(level,2)), "roleTerm", names[n].mods, TAG_OPENCLOSE | TAG_NEWLINE, "type", "text", NULL );
			output_tag( outptr, lvl2indent(incr_level(level,1)), "role", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
			output_tag( outptr, lvl2indent(level),               "name", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
			fields_set_used( f, i );
		}
	}
	str_free( &role );
}

/* datepos[ NUM_DATE_TYPES ]
 *     use define to ensure that the array and loops don't get out of sync
 *     datepos[0] -> DATE:YEAR/PARTDATE:YEAR
 *     datepos[1] -> DATE:MONTH/PARTDATE:MONTH
 *     datepos[2] -> DATE:DAY/PARTDATE:DAY
 *     datepos[3] -> DATE/PARTDATE
 */
#define DATE_YEAR      (0)
#define DATE_MONTH     (1)
#define DATE_DAY       (2)
#define DATE_ALL       (3)
#define NUM_DATE_TYPES (4)

static int
find_datepos( fields *f, int level, unsigned char use_altnames, int datepos[NUM_DATE_TYPES] )
{
	char *src_names[] = { "DATE:YEAR",     "DATE:MONTH",     "DATE:DAY",     "DATE"     };
	char *alt_names[] = { "PARTDATE:YEAR", "PARTDATE:MONTH", "PARTDATE:DAY", "PARTDATE" };
	int  found = 0;
	int  i;

	for ( i=0; i<NUM_DATE_TYPES; ++i ) {
		if ( !use_altnames )
			datepos[i] = fields_find( f, src_names[i], level );
		else
			datepos[i] = fields_find( f, alt_names[i], level );
		if ( datepos[i]!=FIELDS_NOTFOUND ) found = 1;
	}

	return found;
}

/* find_dateinfo()
 *
 *      fill datepos[] array with position indexes to date information in fields *f
 *
 *      when generating dates for LEVEL_MAIN, first look at level=LEVEL_MAIN, but if that
 *      fails, use LEVEL_ANY (-1)
 *
 *      returns 1 if date information found, 0 otherwise
 */
static int
find_dateinfo( fields *f, int level, int datepos[ NUM_DATE_TYPES ] )
{
	int found;

	/* default to finding date information for the current level */
	found = find_datepos( f, level, 0, datepos );

	/* for LEVEL_MAIN, do whatever it takes to find a date */
	if ( !found && level == LEVEL_MAIN ) {
		found = find_datepos( f, -1, 0, datepos );
	}
	if ( !found && level == LEVEL_MAIN ) {
		found = find_datepos( f, -1, 1, datepos );
	}

	return found;
}

static void
output_datepieces( fields *f, FILE *outptr, int pos[ NUM_DATE_TYPES ] )
{
	str *s;
	int i;

	for ( i=0; i<3 && pos[i]!=-1; ++i ) {
		if ( i>0 ) fprintf( outptr, "-" );
		/* zero pad month or days written as "1", "2", "3" ... */
		if ( i==DATE_MONTH || i==DATE_DAY ) {
			s = fields_value( f, pos[i], FIELDS_STRP_NOUSE );
			if ( s->len==1 ) {
				fprintf( outptr, "0" );
			}
		}
		fprintf( outptr, "%s", (char *) fields_value( f, pos[i], FIELDS_CHRP ) );
	}
}

/*
 * <dateIssued>YYYY-MM-DD</dateIssued>
 *                  or
 * <dateIssued>DateAll</dateIssued>
 */
static void
output_dateissued( fields *f, FILE *outptr, int level, int pos[ NUM_DATE_TYPES ] )
{
	output_tag( outptr, lvl2indent(incr_level(level,1)), "dateIssued", NULL, TAG_OPEN, NULL );
	if ( pos[ DATE_YEAR ]!=-1 || pos[ DATE_MONTH ]!=-1 || pos[ DATE_DAY ]!=-1 ) {
		output_datepieces( f, outptr, pos );
	} else {
		fprintf( outptr, "%s", (char *) fields_value( f, pos[ DATE_ALL ], FIELDS_CHRP ) );
	}
	output_tag( outptr, 0, "dateIssued", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
}

static void
output_origin( FILE *outptr, fields *f, int level )
{
	convert2 parts[] = {
		{ "issuance",	  "ISSUANCE",          0, 0 },
		{ "publisher",	  "PUBLISHER",         0, 0 },
		{ "place",	  "ADDRESS",           0, 1 },
		{ "place",        "ADDRESS:PUBLISHER", 0, 1 },
		{ "place",	  "ADDRESS:AUTHOR",    0, 1 },
		{ "edition",	  "EDITION",           0, 0 },
		{ "dateCaptured", "URLDATE",           0, 0 }
	};
	int nparts = sizeof( parts ) / sizeof( parts[0] );

	int i, found, datefound, datepos[ NUM_DATE_TYPES ];
	int indent0, indent1, indent2;

	found     = convert2_findallfields( f, parts, nparts, level );
	datefound = find_dateinfo( f, level, datepos );
	if ( !found && !datefound ) return;

	indent0 = lvl2indent( level );
	indent1 = lvl2indent( incr_level( level, 1 ) );
	indent2 = lvl2indent( incr_level( level, 2 ) );

	output_tag( outptr, indent0, "originInfo", NULL, TAG_OPEN | TAG_NEWLINE, NULL );

	/* issuance must precede date */
	if ( parts[0].pos!=-1 )
		output_fil( outptr, indent1, "issuance", f, parts[0].pos, TAG_OPENCLOSE | TAG_NEWLINE, NULL );

	/* date */
	if ( datefound )
		output_dateissued( f, outptr, level, datepos );

	/* rest of the originInfo elements */
	for ( i=1; i<nparts; i++ ) {

		/* skip missing originInfo elements */
		if ( parts[i].pos==-1 ) continue;

		/* normal originInfo element */
		if ( parts[i].code==0 ) {
			output_fil( outptr, indent1, parts[i].mods, f, parts[i].pos, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
		}

		/* originInfo with placeTerm info */
		else {
			output_tag( outptr, indent1, parts[i].mods, NULL,            TAG_OPEN      | TAG_NEWLINE, NULL );
			output_fil( outptr, indent2, "placeTerm",   f, parts[i].pos, TAG_OPENCLOSE | TAG_NEWLINE, "type", "text", NULL );
			output_tag( outptr, indent1, parts[i].mods, NULL,            TAG_CLOSE     | TAG_NEWLINE, NULL );
		}
	}

	output_tag( outptr, indent0, "originInfo", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
}

/* output_language_core()
 *
 *      generates language output for tag="langauge" or tag="languageOfCataloging"
 *      if possible, outputs iso639-2b code for the language
 *
 * <language>
 *     <languageTerm type="text">xxx</languageTerm>
 * </language>
 *
 * <languageOfCataloging>
 *     <languageTerm type="text">xxx</languageTerm>
 * </languageOfCataloging>
 *
 * <language>
 *     <languageTerm type="text">xxx</languageTerm>
 *     <languageTerm type="code" authority="iso639-2b">xxx</languageTerm>
 * </language>
 *
 */
static void
output_language_core( fields *f, int n, FILE *outptr, const char *tag, int level )
{
	const char *term = "languageTerm";
	const char *lang, *code;
	int indent1, indent2;

	lang = (const char *) fields_value( f, n, FIELDS_CHRP );
	code = iso639_2_from_language( lang );

	indent1 = lvl2indent( level );
	indent2 = lvl2indent( incr_level( level, 1 ) );

	output_tag( outptr, indent1, tag,  NULL, TAG_OPEN      | TAG_NEWLINE, NULL );
	output_tag( outptr, indent2, term, lang, TAG_OPENCLOSE | TAG_NEWLINE, "type", "text", NULL );

	if ( code )
		output_tag( outptr, indent2, term, code, TAG_OPENCLOSE | TAG_NEWLINE, "type", "code", "authority", "iso639-2b", NULL );

	output_tag( outptr, indent1, tag,  NULL, TAG_CLOSE     | TAG_NEWLINE, NULL );
}

/*
 * output_language()
 *
 * <language>
 *     <languageTerm type="text">xxx</languageTerm>
 *     <languageTerm type="code" authority="iso639-2b">xxx</languageTerm>
 * </language>
 */
static inline void
output_language( FILE *outptr, fields *f, int level )
{
	int n;
	
	n = fields_find( f, "LANGUAGE", level );
	if ( n==FIELDS_NOTFOUND ) return;

	output_language_core( f, n, outptr, "language", level );
}

/* output_description()
 *
 * <physicalDescription>
 *      <note>XXXX</note>
 * </physicalDescription>
 */
static void
output_description( FILE *outptr, fields *f, int level )
{
	int n, indent1, indent2;
	const char *val;

	n = fields_find( f, "DESCRIPTION", level );
	if ( n==FIELDS_NOTFOUND ) return;

	val = ( const char * ) fields_value( f, n, FIELDS_CHRP );

	indent1 = lvl2indent( level );
	indent2 = lvl2indent( incr_level( level, 1 ) );

	output_tag( outptr, indent1, "physicalDescription", NULL, TAG_OPEN      | TAG_NEWLINE, NULL );
	output_tag( outptr, indent2, "note",                val,  TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	output_tag( outptr, indent1, "physicalDescription", NULL, TAG_CLOSE     | TAG_NEWLINE, NULL );
}

/* output_toc()
 *
 * <tableOfContents>XXXX</tableOfContents>
 */
static void
output_toc( FILE *outptr, fields *f, int level )
{
	int n, indent;
	char *val;

	n = fields_find( f, "CONTENTS", level );
	if ( n==FIELDS_NOTFOUND ) return;

	val = (char *) fields_value( f, n, FIELDS_CHRP );

	indent = lvl2indent( level );

	output_tag( outptr, indent, "tableOfContents", val, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
}

/* detail output
 *
 * for example:
 *
 * <detail type="volume"><number>xxx</number></detail>
 */
static void
output_detail( FILE *outptr, fields *f, int n, char *item_name, int level )
{
	int indent;

	if ( n==FIELDS_NOTFOUND ) return;

	indent = lvl2indent( incr_level( level, 1 ) );

	output_tag( outptr, indent, "detail", NULL,  TAG_OPEN, "type", item_name, NULL );
	output_fil( outptr, 0,      "number", f, n,  TAG_OPENCLOSE, NULL );
	output_tag( outptr, 0,      "detail", NULL,  TAG_CLOSE | TAG_NEWLINE, NULL );
}

/* extents output
 *
 * <extent unit="page">
 * 	<start>xxx</start>
 * 	<end>xxx</end>
 * </extent>
 */
static void
output_extents( FILE *outptr, fields *f, int start, int end, int total, const char *type, int level )
{
	int indent1, indent2;
	char *val;

	indent1 = lvl2indent( incr_level( level, 1 ) );
	indent2 = lvl2indent( incr_level( level, 2 ) );

	output_tag( outptr, indent1, "extent", NULL, TAG_OPEN | TAG_NEWLINE, "unit", type, NULL );
	if ( start!=FIELDS_NOTFOUND ) {
		val = (char *) fields_value( f, start, FIELDS_CHRP );
		output_tag( outptr, indent2, "start", val, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	}
	if ( end!=FIELDS_NOTFOUND ) {
		val = (char *) fields_value( f, end, FIELDS_CHRP );
		output_tag( outptr, indent2, "end",   val, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	}
	if ( total!=FIELDS_NOTFOUND ) {
		val = (char *) fields_value( f, total, FIELDS_CHRP );
		output_tag( outptr, indent2, "total", val, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	}
	output_tag( outptr, indent1, "extent", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
}

static void
try_output_partheader( FILE *outptr, int wrote_header, int level )
{
	if ( !wrote_header )
		output_tag( outptr, lvl2indent(level), "part", NULL, TAG_OPEN | TAG_NEWLINE, NULL );
}

static void
try_output_partfooter( FILE *outptr, int wrote_header, int level )
{
	if ( wrote_header )
		output_tag( outptr, lvl2indent(level), "part", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
}

/* part date output
 *
 * <date>xxxx-xx-xx</date>
 *
 */
static int
output_partdate( FILE *outptr, fields *f, int level, int wrote_header )
{
	convert2 parts[] = {
		{ "",	"PARTDATE:YEAR",           0, 0 },
		{ "",	"PARTDATE:MONTH",          0, 0 },
		{ "",	"PARTDATE:DAY",            0, 0 },
	};
	int nparts = sizeof(parts)/sizeof(parts[0]);

	if ( !convert2_findallfields( f, parts, nparts, level ) ) return 0;

	try_output_partheader( outptr, wrote_header, level );

	output_tag( outptr, lvl2indent(incr_level(level,1)), "date", NULL, TAG_OPEN, NULL );

	if ( parts[0].pos!=-1 ) {
		fprintf( outptr, "%s", (char *) fields_value( f, parts[0].pos, FIELDS_CHRP ) );
	} else fprintf( outptr, "XXXX" );

	if ( parts[1].pos!=-1 ) {
		fprintf( outptr, "-%s", (char *) fields_value( f, parts[1].pos, FIELDS_CHRP ) );
	}

	if ( parts[2].pos!=-1 ) {
		if ( parts[1].pos==-1 )
			fprintf( outptr, "-XX" );
		fprintf( outptr, "-%s", (char *) fields_value( f, parts[2].pos, FIELDS_CHRP ) );
	}

	fprintf( outptr,"</date>\n");

	return 1;
}

static int
output_partpages( FILE *outptr, fields *f, int level, int wrote_header )
{
	convert2 parts[] = {
		{ "",  "PAGES:START",              0, 0 },
		{ "",  "PAGES:STOP",               0, 0 },
		{ "",  "PAGES",                    0, 0 },
		{ "",  "PAGES:TOTAL",              0, 0 }
	};
	int nparts = sizeof(parts)/sizeof(parts[0]);

	if ( !convert2_findallfields( f, parts, nparts, level ) ) return 0;

	try_output_partheader( outptr, wrote_header, level );

	/* If PAGES:START or PAGES:STOP are undefined */
	if ( parts[0].pos==-1 || parts[1].pos==-1 ) {
		if ( parts[0].pos!=-1 )
			output_detail ( outptr, f, parts[0].pos, "page", level );
		if ( parts[1].pos!=-1 )
			output_detail ( outptr, f, parts[1].pos, "page", level );
		if ( parts[2].pos!=-1 )
			output_detail ( outptr, f, parts[2].pos, "page", level );
		if ( parts[3].pos!=-1 )
			output_extents( outptr, f, FIELDS_NOTFOUND, FIELDS_NOTFOUND, parts[3].pos, "page", level );
	}
	/* If both PAGES:START and PAGES:STOP are defined */
	else {
		output_extents( outptr, f, parts[0].pos, parts[1].pos, parts[3].pos, "page", level );
	}

	return 1;
}

static int
output_partelement( FILE *outptr, fields *f, int level, int wrote_header )
{
	convert2 parts[] = {
		{ "",                "NUMVOLUMES",      0, 0 },
		{ "volume",          "VOLUME",          0, 0 },
		{ "section",         "SECTION",         0, 0 },
		{ "issue",           "ISSUE",           0, 0 },
		{ "number",          "NUMBER",          0, 0 },
		{ "publiclawnumber", "PUBLICLAWNUMBER", 0, 0 },
		{ "session",         "SESSION",         0, 0 },
		{ "articlenumber",   "ARTICLENUMBER",   0, 0 },
		{ "part",            "PART",            0, 0 },
		{ "chapter",         "CHAPTER",         0, 0 },
		{ "report number",   "REPORTNUMBER",    0, 0 },
	};
	int i, nparts = sizeof( parts ) / sizeof( parts[0] );

	if ( !convert2_findallfields( f, parts, nparts, level ) ) return 0;

	try_output_partheader( outptr, wrote_header, level );

	/* start loop at 1 to skip NUMVOLUMES */
	for ( i=1; i<nparts; ++i ) {
		if ( parts[i].pos==FIELDS_NOTFOUND ) continue;
		output_detail( outptr, f, parts[i].pos, parts[i].mods, level );
	}

	if ( parts[0].pos!=FIELDS_NOTFOUND )
		output_extents( outptr, f, FIELDS_NOTFOUND, FIELDS_NOTFOUND, parts[0].pos, "volumes", level );

	return 1;
}

static void
output_part( FILE *outptr, fields *f, int level )
{
	int wrote_hdr;
	wrote_hdr  = output_partdate( outptr, f, level, 0 );
	wrote_hdr += output_partelement( outptr, f, level, wrote_hdr );
	wrote_hdr += output_partpages( outptr, f, level, wrote_hdr );
	try_output_partfooter( outptr, wrote_hdr, level );
}

/* output_recordInfo()
 *
 * <recordInfo>
 *     <languageOfCataloging>
 *         <languageTerm type="text">xxx</languageTerm>
 *         <languageTerm type="code" authority="iso639-2b">xxx</languageTerm>
 *     </languageOfCataloging>
 * </recordInfo>
 */
static void
output_recordInfo( FILE *outptr, fields *f, int level )
{
	int n, indent;

	n = fields_find( f, "LANGCATALOG", level );
	if ( n==FIELDS_NOTFOUND ) return;

	indent = lvl2indent( level );

	output_tag( outptr, indent, "recordInfo", NULL, TAG_OPEN  | TAG_NEWLINE, NULL );
	output_language_core( f, n, outptr, "languageOfCataloging", incr_level(level,1) );
	output_tag( outptr, indent, "recordInfo", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
}

/* output_genre()
 *
 * <genre authority="marcgt">thesis</genre>
 * <genre authority="bibutilsgt">Diploma thesis</genre>
 */
static void
output_genre( FILE *outptr, fields *f, int level )
{
	const char *authority="authority", *marcauth="marcgt", *buauth="bibutilsgt";
	const char *attr = NULL, *attrvalue = NULL;
	const char *tag, *value;
	int i, n;

	n = fields_num( f );

	for ( i=0; i<n; ++i ) {

		if ( fields_level( f, i ) != level ) continue;

		tag = ( const char * ) fields_tag( f, i, FIELDS_CHRP );

		if ( !strcmp( tag, "GENRE:MARC" ) ) {
			attr      = authority;
			attrvalue = marcauth;
		}
		else if ( !strcmp( tag, "GENRE:BIBUTILS" ) ) {
			attr      = authority;
			attrvalue = buauth;
		}
		else if ( !strcmp( tag, "GENRE:UNKNOWN" ) || !strcmp( tag, "GENRE" ) ) {
			/* do nothing */
		}
		else continue;

		value = ( const char * ) fields_value( f, i, FIELDS_CHRP );

		/* if the internal tag hasn't told us, try to look up genre tag */
		if ( !attr ) {
			if ( is_marc_genre( value ) ) {
				attr      = authority;
				attrvalue = marcauth;
			}
			else if ( is_bu_genre( value ) ) {
				attr      = authority;
				attrvalue = buauth;
			}
		}

		output_tag( outptr, lvl2indent(level), "genre", value, TAG_OPENCLOSE | TAG_NEWLINE, attr, attrvalue, NULL );

	}
}

/* output_resource()
 *
 * <typeOfResource>text</typeOfResource>
 *
 * Only output typeOfResources defined by MARC authority
 */
static void
output_resource( FILE *outptr, fields *f, int level )
{
	const char *value;
	int n;

	n = fields_find( f, "RESOURCE", level );
	if ( n==FIELDS_NOTFOUND ) return;

	value = ( const char * ) fields_value( f, n, FIELDS_CHRP );
	if ( is_marc_resource( value ) ) {
		output_fil( outptr, lvl2indent(level), "typeOfResource", f, n, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	} else {
		fprintf( stderr, "Illegal typeofResource = '%s'\n", value );
	}
}

static void
output_type( FILE *outptr, fields *f, int level )
{
	int n;

	/* silence warnings about INTERNAL_TYPE being unused */
	n = fields_find( f, "INTERNAL_TYPE", LEVEL_MAIN );
	if ( n!=FIELDS_NOTFOUND ) fields_set_used( f, n );

	output_resource( outptr, f, level );
	output_genre( outptr, f, level );
}

/* output_abs()
 *
 * <abstract>xxxx</abstract>
 */
static void
output_abs( FILE *outptr, fields *f, int level )
{
	int n;

	n = fields_find( f, "ABSTRACT", level );
	output_fil( outptr, lvl2indent(level), "abstract", f, n, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
}

static void
output_notes( FILE *outptr, fields *f, int level )
{
	int i, n;
	char *t;

	n = fields_num( f );
	for ( i=0; i<n; ++i ) {
		if ( fields_level( f, i ) != level ) continue;
		t = fields_tag( f, i, FIELDS_CHRP_NOUSE );
		if ( !strcasecmp( t, "NOTES" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
		else if ( !strcasecmp( t, "PUBSTATE" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, "type", "publication status", NULL );
		else if ( !strcasecmp( t, "ANNOTE" ) )
			output_fil( outptr, lvl2indent(level), "bibtex-annote", f, i, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
		else if ( !strcasecmp( t, "TIMESCITED" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, "type", "times cited", NULL );
		else if ( !strcasecmp( t, "ANNOTATION" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, "type", "annotation", NULL );
		else if ( !strcasecmp( t, "ADDENDUM" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, "type", "addendum", NULL );
		else if ( !strcasecmp( t, "BIBKEY" ) )
			output_fil( outptr, lvl2indent(level), "note", f, i, TAG_OPENCLOSE | TAG_NEWLINE, "type", "bibliography key", NULL );
	}
}

/* output_key()
 *
 * <subject>
 *    <topic>KEYWORD/topic>
 * </subject>
 * <subject>
 *    <topic class="primary">EPRINTCLASS</topic>
 * </subject>
 */
static void
output_key( FILE *outptr, fields *f, int level )
{
	int indent1, indent2;
	vplist_index i;
	char *value;
	vplist keys;
	int status;

	vplist_init( &keys );

	indent1 = lvl2indent( level );
	indent2 = lvl2indent( incr_level( level, 1 ) );

	/* output KEYWORDs */

	status = fields_findv_each( f, level, FIELDS_CHRP, &keys, "KEYWORD" );
	if ( status!=FIELDS_OK ) goto out;

	for ( i=0; i<keys.n; ++i ) {
		value = vplist_get( &keys, i );
		output_tag( outptr, indent1, "subject", NULL,  TAG_OPEN      | TAG_NEWLINE, NULL );
		output_tag( outptr, indent2, "topic",   value, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
		output_tag( outptr, indent1, "subject", NULL,  TAG_CLOSE     | TAG_NEWLINE, NULL );
	}

	vplist_empty( &keys );

	/* output EPRINTCLASSes */

	status = fields_findv_each( f, level, FIELDS_CHRP, &keys, "EPRINTCLASS" );
	if ( status!=FIELDS_OK ) goto out;

	for ( i=0; i<keys.n; ++i ) {
		value = vplist_get( &keys, i );
		output_tag( outptr, indent1, "subject", NULL,  TAG_OPEN      | TAG_NEWLINE, NULL );
		output_tag( outptr, indent2, "topic",   value, TAG_OPENCLOSE | TAG_NEWLINE,
				"class", "primary", NULL );
		output_tag( outptr, indent1, "subject", NULL,  TAG_CLOSE     | TAG_NEWLINE, NULL );
	}

out:
	vplist_free( &keys );
}

static void
output_sn( FILE *outptr, fields *f, int level )
{
	convert sn_types[] = {
		{ "isbn",      "ISBN",      },
		{ "isbn",      "ISBN13",    },
		{ "lccn",      "LCCN",      },
		{ "issn",      "ISSN",      },
		{ "coden",     "CODEN",     },
		{ "citekey",   "REFNUM",    },
		{ "doi",       "DOI",       },
		{ "eid",       "EID",       },
		{ "eprint",    "EPRINT",    },
		{ "eprinttype","EPRINTTYPE",},
		{ "pubmed",    "PMID",      },
		{ "MRnumber",  "MRNUMBER",  },
		{ "medline",   "MEDLINE",   },
		{ "pii",       "PII",       },
		{ "pmc",       "PMC",       },
		{ "arXiv",     "ARXIV",     },
		{ "isi",       "ISIREFNUM", },
		{ "accessnum", "ACCESSNUM", },
		{ "jstor",     "JSTOR",     },
		{ "isrn",      "ISRN",      },
		{ "serial number", "SERIALNUMBER", },
	};
	int ntypes = sizeof( sn_types ) / sizeof( sn_types[0] );
	int i, n, status, indent;
	vplist serialno;

	vplist_init( &serialno );

	indent = lvl2indent( level );

	/* output call number */
	n = fields_find( f, "CALLNUMBER", level );
	output_fil( outptr, indent, "classification", f, n, TAG_OPENCLOSE | TAG_NEWLINE, NULL );

	/* output all types of serial numbers */
	for ( i=0; i<ntypes; ++i ) {

		status = fields_findv_each( f, level, FIELDS_CHRP, &serialno, sn_types[i].internal );
		if ( status!=FIELDS_OK ) goto out;

		output_vpl( outptr, indent, "identifier", &serialno, TAG_OPENCLOSE | TAG_NEWLINE,
				"type", sn_types[i].mods, NULL );

		vplist_empty( &serialno );
	}

out:
	vplist_free( &serialno );
}

/* output_url()
 *
 * <location>
 *     <url>URL</url>
 *     <url urlType="pdf">PDFLINK</url>
 *     <url displayLabel="Electronic full text" access="raw object">PDFLINK</url>
 *     <physicalLocation>LOCATION</physicalLocation>
 * </location>
 */
static void
output_url( FILE *outptr, fields *f, int level )
{
	vplist fileattach, location, pdflink, url;
	int indent1, indent2, status;

	vplist_init( &fileattach );
	vplist_init( &location );
	vplist_init( &pdflink );
	vplist_init( &url );

	status = fields_findv_each( f, level, FIELDS_CHRP, &fileattach, "FILEATTACH" );
	if ( status!=FIELDS_OK ) goto out;

	status = fields_findv_each( f, level, FIELDS_CHRP, &location,   "LOCATION"   );
	if ( status!=FIELDS_OK ) goto out;

	status = fields_findv_each( f, level, FIELDS_CHRP, &pdflink,    "PDFLINK"    );
	if ( status!=FIELDS_OK ) goto out;

	status = fields_findv_each( f, level, FIELDS_CHRP, &url,        "URL"        );
	if ( status!=FIELDS_OK ) goto out;


	/* do not write location tag if no elements found */
	if ( fileattach.n + location.n + pdflink.n + url.n == 0 ) goto out;


	/* output all of the found elements surrounded by <location>...</location>*/

	indent1 = lvl2indent( level );
	indent2 = lvl2indent( incr_level( level, 1 ) );

	output_tag( outptr, indent1, "location", NULL, TAG_OPEN | TAG_NEWLINE, NULL );

	output_vpl( outptr, indent2, "url", &url, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	output_vpl( outptr, indent2, "url", &pdflink, TAG_OPENCLOSE | TAG_NEWLINE, NULL );
	output_vpl( outptr, indent2, "url", &fileattach, TAG_OPENCLOSE | TAG_NEWLINE,
			"displayLabel", "Electronic full text",
			"access",       "raw object",
			NULL );
	output_vpl( outptr, indent2, "physicalLocation", &location, TAG_OPENCLOSE | TAG_NEWLINE, NULL );

	output_tag( outptr, indent1, "location", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );

out:
	vplist_free( &fileattach );
	vplist_free( &location );
	vplist_free( &pdflink );
	vplist_free( &url );
}

static int
original_items( fields *f, int level )
{
	int i, targetlevel, n;
	if ( level < 0 ) return 0;
	targetlevel = -( level + 2 );
	n = fields_num( f );
	for ( i=0; i<n; ++i ) {
		if ( fields_level( f, i ) == targetlevel )
			return targetlevel;
	}
	return 0;
}

static void
output_citeparts( FILE *outptr, fields *f, int level, int max )
{
	int orig_level;

	output_title      ( outptr, f, level );
	output_names      ( outptr, f, level );
	output_origin     ( outptr, f, level );
	output_type       ( outptr, f, level );
	output_language   ( outptr, f, level );
	output_description( outptr, f, level );

	/* Recursively output relatedItems, which are host items like series for a book */
	if ( level >= 0 && level < max ) {
		output_tag( outptr, lvl2indent(level), "relatedItem", NULL, TAG_OPEN  | TAG_NEWLINE, "type", "host", NULL );
		output_citeparts( outptr, f, incr_level(level,1), max );
		output_tag( outptr, lvl2indent(level), "relatedItem", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
	}

	/* Recursively output relatedItems that are original item, if they exist;
	 * these are the first publication of an item 
	 */
	orig_level = original_items( f, level );
	if ( orig_level ) {
		output_tag( outptr, lvl2indent(level), "relatedItem", NULL, TAG_OPEN  | TAG_NEWLINE, "type", "original", NULL );
		output_citeparts( outptr, f, orig_level, max );
		output_tag( outptr, lvl2indent(level), "relatedItem", NULL, TAG_CLOSE | TAG_NEWLINE, NULL );
	}

	output_abs       ( outptr, f, level );
	output_notes     ( outptr, f, level );
	output_toc       ( outptr, f, level );
	output_key       ( outptr, f, level );
	output_sn        ( outptr, f, level );
	output_url       ( outptr, f, level );
	output_part      ( outptr, f, level );
	output_recordInfo( outptr, f, level );
}

static int
no_unused_tags( fields *f )
{
	int i, n;

	n = fields_num( f );
	for ( i=0; i<n; ++i )
		if ( fields_used( f, i ) == 0 ) return 0;

	return 1;
}

static void
report_unused_tags( FILE *outptr, fields *f, param *p, unsigned long refnum )
{
	char *tag, *value, *prefix;
	int i, n, nwritten, level;

	if ( no_unused_tags( f ) ) return;

	n = fields_num( f );

	if ( p->progname ) prefix = p->progname;
	else               prefix = "modsout";

	fprintf( outptr, "%s: Reference %lu has unused tags.\n", prefix, refnum+1 );

	/* Report authors from level LEVEL_MAIN */
	nwritten = 0;
	for ( i=0; i<n; ++i ) {
		if ( fields_level( f, i ) != LEVEL_MAIN ) continue;
		tag = fields_tag( f, i, FIELDS_CHRP_NOUSE );
		if ( strcasecmp( tag, "AUTHOR" ) && strcasecmp( tag, "AUTHOR:ASIS" ) && strcasecmp( tag, "AUTHOR:CORP" ) ) continue;
		value = fields_value( f, i, FIELDS_CHRP_NOUSE );
		if ( nwritten==0 ) fprintf( outptr, "%s:    Author(s): %s\n", prefix, value );
		else               fprintf( outptr, "%s:               %s\n", prefix, value );
		nwritten++;
	}

	/* Report date from level LEVEL_MAIN */
	for ( i=0; i<n; ++i ) {
		if ( fields_level( f, i ) != LEVEL_MAIN ) continue;
		tag = fields_tag( f, i, FIELDS_CHRP_NOUSE );
		if ( strcasecmp( tag, "DATE:YEAR" ) && strcasecmp( tag, "PARTDATE:YEAR" ) ) continue;
		value = fields_value( f, i, FIELDS_CHRP_NOUSE );
		fprintf( outptr, "%s:    Year: %s\n", prefix, value );
		break;
	}

	/* Report title from level LEVEL_MAIN */
	for ( i=0; i<n; ++i ) {
		if ( fields_level( f, i ) != LEVEL_MAIN ) continue;
		tag = fields_tag( f, i, FIELDS_CHRP_NOUSE );
		if ( strncasecmp( tag, "TITLE", 5 ) ) continue;
		value = fields_value( f, i, FIELDS_CHRP_NOUSE );
		fprintf( outptr, "%s:    Title: %s\n", prefix, value );
		break;
	}
	
	fprintf( outptr, "%s:    Unused entries: tag, value, level\n", prefix );
	for ( i=0; i<n; ++i ) {
		if ( fields_used( f, i ) ) continue;
		tag   = fields_tag(   f, i, FIELDS_CHRP_NOUSE );
		value = fields_value( f, i, FIELDS_CHRP_NOUSE );
		level = fields_level( f, i );
		fprintf( outptr, "%s:        '%s', '%s', %d\n", prefix, tag, value, level );
	}

}

/* refnum should start with a non-number and not include spaces -- ignore this */
static void
output_refnum( FILE *outptr, fields *f, int n )
{
	char *p = fields_value( f, n, FIELDS_CHRP_NOUSE );
	while ( p && *p ) {
		if ( !is_ws(*p) ) fprintf( outptr, "%c", *p );
		p++;
	}
}

static void
output_head( FILE *outptr, fields *f, int dropkey )
{
	int n;
	fprintf( outptr, "<mods");
	if ( !dropkey ) {
		n = fields_find( f, "REFNUM", LEVEL_MAIN );
		if ( n!=FIELDS_NOTFOUND ) {
			fprintf( outptr, " ID=\"");
			output_refnum( outptr, f, n );
			fprintf( outptr, "\"");
		}
	}
	fprintf( outptr, ">\n" );
}

static inline void
output_tail( FILE *outptr )
{
	fprintf( outptr, "</mods>\n" );
}

static int
modsout_write( fields *f, FILE *outptr, param *p, unsigned long refnum )
{
	int max, dropkey;

	max = fields_maxlevel( f );
	dropkey = ( p->format_opts & BIBL_FORMAT_MODSOUT_DROPKEY );

	output_head( outptr, f, dropkey );
	output_citeparts( outptr, f, LEVEL_MAIN, max );
	output_tail( outptr );
	fflush( outptr );

	report_unused_tags( stderr, f, p, refnum );

	return BIBL_OK;
}

/*****************************************************
 PUBLIC: int modsout_writeheader()
*****************************************************/

static void
modsout_writeheader( FILE *outptr, param *p )
{
	if ( p->utf8bom ) utf8_writebom( outptr );
	fprintf(outptr,"<?xml version=\"1.0\" encoding=\"%s\"?>\n",
			charset_get_xmlname( p->charsetout ) );
	fprintf(outptr,"<modsCollection xmlns=\"http://www.loc.gov/mods/v3\">\n");
}

/*****************************************************
 PUBLIC: int modsout_writefooter()
*****************************************************/

static void
modsout_writefooter( FILE *outptr )
{
	fprintf(outptr,"</modsCollection>\n");
	fflush( outptr );
}
