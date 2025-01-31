/*
 * modsin.c
 *
 * Copyright (c) Chris Putnam 2004-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "is_ws.h"
#include "str.h"
#include "str_conv.h"
#include "pages.h"
#include "xml.h"
#include "xml_encoding.h"
#include "fields.h"
#include "name.h"
#include "reftypes.h"
#include "modstypes.h"
#include "bu_auth.h"
#include "marc_auth.h"
#include "url.h"
#include "iso639_1.h"
#include "iso639_2.h"
#include "iso639_3.h"
#include "bibutils.h"
#include "bibformats.h"

static int modsin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset );
static int modsin_processf( fields *medin, const char *data, const char *filename, long nref, param *p );

/*****************************************************
 PUBLIC: void modsin_initparams()
*****************************************************/
int
modsin_initparams( param *pm, const char *progname )
{

	pm->readformat       = BIBL_MODSIN;
	pm->format_opts      = 0;
	pm->charsetin        = BIBL_CHARSET_UNICODE;
	pm->charsetin_src    = BIBL_SRC_DEFAULT;
	pm->latexin          = 0;
	pm->utf8in           = 1;
	pm->xmlin            = 1;
	pm->nosplittitle     = 0;
	pm->verbose          = 0;
	pm->addcount         = 0;
	pm->singlerefperfile = 0;
	pm->output_raw       = BIBL_RAW_WITHMAKEREFID |
	                      BIBL_RAW_WITHCHARCONVERT;

	pm->readf    = modsin_readf;
	pm->processf = modsin_processf;
	pm->cleanf   = NULL;
	pm->typef    = NULL;
	pm->convertf = NULL;
	pm->all      = NULL;
	pm->nall     = 0;

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
 PUBLIC: int modsin_processf()
*****************************************************/

static char modsns[]="mods";

/* Extract/expand language attributes from tags like:
 *
 * <node type="level" lang="swe">Självständigt arbete på avancerad nivå (masterexamen)</note>
 *
 * Returns NULL if attribute is missing
 */
static char *
modsin_get_lang_attribute( xml *node )
{
	char *lang, *expand;
	str *langtag;

	langtag = xml_attribute( node, "lang" );
	if ( !langtag ) return NULL;

	lang = str_cstr( langtag );

	expand = iso639_3_from_code( lang );
	if ( expand ) return expand;

	expand = iso639_2_from_code( lang );
	if ( expand ) return expand;

	expand = iso639_1_from_code( lang );
	if ( expand ) return expand;

	return lang;
}

static int
modsin_simple( xml *node, fields *info, const char *tag, const char *lang, int level )
{
	int fstatus;

	if ( !xml_has_value( node ) ) return BIBL_OK;

	fstatus = fields_add_lang( info, tag, xml_value_cstr( node ), lang, level );
	if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;

	return BIBL_OK;
}

/*
 * <detail></detail>
 * <detail><number></number></detail>
 */
static int
modsin_detail_value( xml *node, str *value )
{
	while ( node ) {

		if ( xml_has_value( node ) ) {
			if ( str_has_value( value ) ) str_addchar( value, ' ' );
			str_strcat( value, xml_value( node ) );
			if ( str_memerr( value ) ) return BIBL_ERR_MEMERR;
		}

		node = node->next;

	}

	return BIBL_OK;
}

static int
modsin_detail_type( xml *node, str *type )
{
	str *attribute;

	attribute = xml_attribute( node, "type" );
	if ( attribute ) {
		str_strcpy( type, attribute );
		str_toupper( type );
		if ( str_memerr( type ) ) return BIBL_ERR_MEMERR;
	}

	return BIBL_OK;
}

static int
modsin_detail( xml *node, fields *info, int level )
{
	int fstatus, status;
	str type, value;

	if ( !node->down ) return BIBL_OK;

	strs_init( &type, &value, NULL );

	status = modsin_detail_type( node, &type );
	if ( status!=BIBL_OK ) goto out;

	status = modsin_detail_value( node->down, &value );
	if ( status!=BIBL_OK ) goto out;

	if ( str_has_value( &type ) && !strcasecmp( str_cstr( &type ), "PAGE" ) ) {
		status = add_pages( info, &value, level );
	} else {
		fstatus = fields_add( info, str_cstr( &type ), str_cstr( &value ), level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

out:
	strs_free( &type, &value, NULL );
	return status;
}

/*
 * <date></date>
 * <dateIssued></dateIssued>
 */
static int
modsin_date_core( xml *node, fields *info, int level, int part )
{
	int fstatus, status = BIBL_OK;
	const char *tag, *p;
	str s;

	str_init( &s );

	p = xml_value_cstr( node );

	if ( p ) {

		p = str_cpytodelim( &s, skip_ws( p ), "-", 1 );
		if ( str_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }
		if ( str_has_value( &s ) ) {
			tag = ( part ) ? "PARTDATE:YEAR" : "DATE:YEAR";
			fstatus =  fields_add( info, tag, str_cstr( &s ), level );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}

		p = str_cpytodelim( &s, skip_ws( p ), "-", 1 );
		if ( str_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }
		if ( str_has_value( &s ) ) {
			tag = ( part ) ? "PARTDATE:MONTH" : "DATE:MONTH";
			fstatus =  fields_add( info, tag, str_cstr( &s ), level );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}

		(void) str_cpytodelim( &s, skip_ws( p ), "", 0 );
		if ( str_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }
		if ( str_has_value( &s ) ) {
			tag = ( part ) ? "PARTDATE:DAY" : "DATE:DAY";
			fstatus =  fields_add( info, tag, str_cstr( &s ), level );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}

	}

out:
	str_free( &s );
	return status;
}

static int
modsin_date( xml *node, fields *info, const char *lang, int level )
{
	return modsin_date_core( node, info, level, 0 );
}

/*
 * <extent unit="page"><start></start><end></end><total></total></extent>
 * <extent unit="pages"></extent>
 */
static int
modsin_page_get_values( xml *node, str *start_page, str *end_page, str *total_pages, str *list_pages )
{
	str *use = NULL;
	int status;

	if      ( xml_tag_matches_has_value( node, "start" ) ) use = start_page;
	else if ( xml_tag_matches_has_value( node, "end" ) )   use = end_page;
	else if ( xml_tag_matches_has_value( node, "total" ) ) use = total_pages;
	else if ( xml_tag_matches_has_value( node, "list" ) )  use = list_pages;

	if ( use ) {
		str_strcpy( use, xml_value( node ) );
		if ( str_memerr( use ) ) return BIBL_ERR_MEMERR;
	}

	if ( node->next ) {
		status = modsin_page_get_values( node->next, start_page, end_page, total_pages, list_pages );
		if ( status!=BIBL_OK ) return status;
	}

	return BIBL_OK;
}

static int
modsin_page_add_values( fields *info, str *start_page, str *end_page, str *total_pages, str *list_pages, int level )
{
	int fstatus;

	/*
	 * If start_page or end_page specified, add those, not list_pages.
	 * The list_pages value is typically redundant with start_page/end_page
	 */
	if ( str_has_value( start_page ) || str_has_value( end_page ) ) {
		if ( str_has_value( start_page ) ) {
			fstatus = fields_add( info, "PAGES:START", str_cstr( start_page ), level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
		if ( str_has_value( end_page ) ) {
			fstatus = fields_add( info, "PAGES:STOP", str_cstr( end_page ), level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
	}

	else if ( str_has_value( list_pages ) ) {
		fstatus = fields_add( info, "PAGES:START", str_cstr( list_pages ), level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	/* always add total pages, if available */
	if ( str_has_value( total_pages ) ) {
		fstatus = fields_add( info, "PAGES:TOTAL", str_cstr( total_pages ), level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	return BIBL_OK;
}

static int
modsin_page( xml *node, fields *info, int level )
{
	str start_page, end_page, total_pages, list_pages;
	int status;

	if ( !node->down ) return BIBL_OK;

	strs_init( &start_page, &end_page, &total_pages, &list_pages, NULL );

	status = modsin_page_get_values( node->down, &start_page, &end_page, &total_pages, &list_pages );
	if ( status!=BIBL_OK ) goto out;

	status = modsin_page_add_values( info, &start_page, &end_page, &total_pages, &list_pages, level );

out:
	strs_free( &start_page, &end_page, &total_pages, &list_pages, NULL );
	return status;
}

/*
 * <titleInfo>
 *     <nonSort></nonSort>
 *     <title></title>
 *     <subTitle></subTitle>
 * </titleInfo>
 */

static void
modsin_add_sep_if_necessary( str *s, char sep )
{
	char *p;

	if ( str_is_empty( s ) ) return;

	p = str_cstr( s );
	if ( p[s->len-1]!=sep ) str_addchar( s, sep );
}

static int
modsin_title_core( xml *node, str *title, str *subtitle )
{
	while ( node ) {

		if ( xml_tag_matches_has_value( node, "title" ) ) {
			modsin_add_sep_if_necessary( title, ' ' );
			str_strcat( title, xml_value( node ) );
			if ( str_memerr( title ) ) return BIBL_ERR_MEMERR;
		}

		else if ( xml_tag_matches_has_value( node, "subTitle" ) ) {
			str_strcat( subtitle, xml_value( node ) );
			if ( str_memerr( subtitle ) ) return BIBL_ERR_MEMERR;
		}

		else if ( xml_tag_matches_has_value( node, "nonSort" ) ) {
			modsin_add_sep_if_necessary( title, ' ' );
			str_strcat( title, xml_value( node ) );
			if ( str_memerr( title ) ) return BIBL_ERR_MEMERR;
		}

		node = node->next;

	}

	return BIBL_OK;
}

static int
modsin_title( xml *node, fields *info, const char *lang, int level )
{
	char *titletag[2][2] = {
		{ "TITLE",    "SHORTTITLE"    },
		{ "SUBTITLE", "SHORTSUBTITLE" },
	};
	int status, fstatus = FIELDS_OK;
	str title, subtitle;
	int abbr;

	if ( !node->down ) return BIBL_OK;

	strs_init( &title, &subtitle, NULL );

	abbr = xml_tag_has_attribute( node, "titleInfo", "type", "abbreviated" );

	status = modsin_title_core( node->down, &title, &subtitle );
	if ( status!=BIBL_OK ) goto out;

	if ( str_has_value( &title ) ) {
		fstatus = fields_add( info, titletag[0][abbr], str_cstr( &title ), level );
		if ( fstatus!=FIELDS_OK ) goto out;
	}

	if ( str_has_value( &subtitle ) ) {
		fstatus = fields_add( info, titletag[1][abbr], str_cstr( &subtitle ), level );
		if ( fstatus!=FIELDS_OK ) goto out;
	}

out:
	strs_free( &title, &subtitle, NULL );
	if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	return status;
}

/* modsin_marcrole_convert()
 *
 * Map MARC-authority roles for people or organizations associated
 * with a reference to internal roles.
 *
 * Take input strings with roles separated by '|' characters, e.g.
 * "author" or "author|creator" or "edt" or "editor|edt".
 */
static int
modsin_marcrole_convert( str *s, char *suffix, str *out )
{
	int i, sstatus, status = BIBL_OK;
	slist tokens;
	char *p;

	slist_init( &tokens );

	/* ...default to author on an empty string */
	if ( str_is_empty( s ) ) {
		str_strcpyc( out, "AUTHOR" );
	}

	else {
		sstatus = slist_tokenize( &tokens, s, "|", 1 );
		if ( sstatus!=SLIST_OK ) {
			status = BIBL_ERR_MEMERR;
			goto done;
		}
		/* ...take first match */
		for ( i=0; i<tokens.n; ++i ) {
			p = marc_convert_relators( slist_cstr( &tokens, i ) );
			if ( p ) {
				str_strcpyc( out, p );
				goto done;
			}
		}
		/* ...otherwise just copy input */
		str_strcpy( out, slist_str( &tokens, 0 ) );
		str_toupper( out );
	}

done:
	if ( suffix ) str_strcatc( out, suffix );
	slist_free( &tokens );
	if ( str_memerr( out ) ) return BIBL_ERR_MEMERR;
	return status;
}

static int
modsin_asis_corp_r( xml *node, str *name, str *role )
{
	int status = BIBL_OK;
	if ( xml_tag_matches_has_value( node, "namePart" ) ) {
		str_strcpy( name, xml_value( node ) );
		if ( str_memerr( name ) ) return BIBL_ERR_MEMERR;
	} else if ( xml_tag_matches_has_value( node, "roleTerm" ) ) {
		if ( role->len ) str_addchar( role, '|' );
		str_strcat( role, xml_value( node ) );
		if ( str_memerr( role ) ) return BIBL_ERR_MEMERR;
	}
	if ( node->down ) {
		status = modsin_asis_corp_r( node->down, name, role );
		if ( status!=BIBL_OK ) return status;
	}
	if ( node->next )
		status = modsin_asis_corp_r( node->next, name, role );
	return status;
}

static int
modsin_asis_corp( xml *node, fields *info, int level, char *suffix )
{
	int fstatus, status = BIBL_OK;
	str name, roles, role_out;
	xml *dnode = node->down;
	if ( dnode ) {
		strs_init( &name, &roles, &role_out, NULL );
		status = modsin_asis_corp_r( dnode, &name, &roles );
		if ( status!=BIBL_OK ) goto out;
		status = modsin_marcrole_convert( &roles, suffix, &role_out );
		if ( status!=BIBL_OK ) goto out;
		fstatus = fields_add( info, str_cstr( &role_out ), str_cstr( &name ), level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
out:
		strs_free( &name, &roles, &role_out, NULL );
	}
	return status;
}

static int
modsin_roler( xml *node, str *roles )
{
	if ( xml_has_value( node ) ) {
		if ( roles->len ) str_addchar( roles, '|' );
		str_strcat( roles, xml_value( node ) );
	}
	if ( str_memerr( roles ) ) return BIBL_ERR_MEMERR;
	else return BIBL_OK;
}

static int
modsin_personr( xml *node, str *familyname, str *givenname, str *suffix )
{
	int status = BIBL_OK;

	if ( !xml_has_value( node ) ) return status;

	if ( xml_tag_has_attribute( node, "namePart", "type", "family" ) ) {
		if ( str_has_value( familyname ) ) str_addchar( familyname, ' ' );
		str_strcat( familyname, xml_value( node ) );
		if ( str_memerr( familyname ) ) status = BIBL_ERR_MEMERR;
	}

	else if ( xml_tag_has_attribute( node, "namePart", "type", "suffix"         ) ||
	          xml_tag_has_attribute( node, "namePart", "type", "termsOfAddress" ) ) {
		if ( str_has_value( suffix ) ) str_addchar( suffix, ' ' );
		str_strcat( suffix, xml_value( node ) );
		if ( str_memerr( suffix ) ) status = BIBL_ERR_MEMERR;
	}

	else if ( xml_tag_has_attribute( node, "namePart", "type", "date" ) ) {
		/* no nothing */
	}

	else {
		if ( str_has_value( givenname ) ) str_addchar( givenname, '|' );
		str_strcat( givenname, xml_value( node ) );
		if ( str_memerr( givenname ) ) status = BIBL_ERR_MEMERR;
	}

	return status;
}

static int
modsin_person( xml *node, fields *info, const char *lang, int level )
{
	str familyname, givenname, name, suffix, roles, role_out;
	int fstatus, status = BIBL_OK;
	xml *dnode, *rnode;

	dnode = node->down;
	if ( !dnode ) return status;

	strs_init( &name, &familyname, &givenname, &suffix, &roles, &role_out, NULL );

	while ( dnode ) {

		if ( xml_tag_matches( dnode, "namePart" ) ) {
			status = modsin_personr( dnode, &familyname, &givenname, &suffix );
			if ( status!=BIBL_OK ) goto out;
		}

		else if ( xml_tag_matches( dnode, "role" ) ) {
			rnode = dnode->down;
			while ( rnode ) {
				if ( xml_tag_matches( rnode, "roleTerm" ) ) {
					status = modsin_roler( rnode, &roles );
					if ( status!=BIBL_OK ) goto out;
				}
				rnode = rnode->next;
			}
		}

		dnode = dnode->next;

	}

	/*
	 * Handle:
	 *          <namePart type='given'>Noah A.</namePart>
	 *          <namePart type='family'>Smith</namePart>
	 * without mangling the order of "Noah A."
	 */
	if ( str_has_value( &familyname ) ) {
		str_strcpy( &name, &familyname );
		if ( givenname.len ) {
			str_addchar( &name, '|' );
			str_strcat( &name, &givenname );
		}
	}

	/*
	 * Handle:
	 *          <namePart>Noah A. Smith</namePart>
	 * with name order mangling.
	 */
	else {
		if ( str_has_value( &givenname ) )
			name_parse( &name, &givenname, NULL, NULL );
	}

	if ( str_has_value( &suffix ) ) {
		str_strcatc( &name, "||" );
		str_strcat( &name, &suffix );
	}

	if ( str_memerr( &name ) ) {
		status=BIBL_ERR_MEMERR;
		goto out;
	}

	status = modsin_marcrole_convert( &roles, NULL, &role_out );
	if ( status!=BIBL_OK ) goto out;

	fstatus = fields_add_can_dup( info, str_cstr( &role_out ), str_cstr( &name ), level );
	if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;

out:
	strs_free( &name, &familyname, &givenname, &suffix, &roles, &role_out, NULL );
	return status;
}

static int
modsin_name( xml *node, fields *info, const char *lang, int level )
{
	int status = BIBL_OK;

	if ( xml_tag_has_attribute( node, "name", "type", "personal" ) )
		status = modsin_person( node, info, lang, level );

	else if ( xml_tag_has_attribute( node, "name", "type", "corporate" ) )
		status = modsin_asis_corp( node, info, level, ":CORP" );

	else if ( xml_tag_matches( node, "name" ) )
		status = modsin_asis_corp( node, info, level, ":ASIS" );

	return status;
}

/*
 * <place>
 *     <placeTerm></placeTerm>
 *     <placeTerm type="text"></placeTerm>
 *     <placeTerm type="code" authority="marccountry"></placeterm>
 * </place>
 */

static int
modsin_placeterm_code( xml *node, fields *info, int level, str *auth )
{
	char *value = NULL, *tag = "ADDRESS";
	int fstatus, status = BIBL_OK;
	str s;

	str_init( &s );

	if ( !str_strcmpc( auth, "marccountry" ) ) {
		value = marc_convert_country( xml_value_cstr( node ) );
	}

	if ( !value ) {
		str_strcpy( &s, auth );
		str_addchar( &s, '|' );
		str_strcat( &s, xml_value( node ) );
		if ( str_memerr( &s ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		value = str_cstr( &s );
		tag = "ADDRESS:CODED";
	}

	fstatus = fields_add( info, tag, value, level );
	if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;

out:
	str_free( &s );
	return status;
}

static int
modsin_place( xml *node, fields *info, const char *lang, int level )
{
	const char *tag[] = { "ADDRESS", "SCHOOL" };
	int place_is_school = 0, is_school;
	int status = BIBL_OK;
	str *type, *auth;

	if ( xml_tag_has_attribute( node, "place", "type", "school" ) )
		place_is_school = 1;

	node = node->down;

	while ( node ) {

		is_school = place_is_school;
		if ( !is_school ) {
			if ( xml_tag_has_attribute( node, "placeTerm", "type", "school" ) )
				is_school = 1;
		}

		type = xml_attribute( node, "type" );
		auth = xml_attribute( node, "authority" );

		if ( str_has_value( type ) || str_has_value( auth ) ) {
			if ( str_has_value( auth ) || !strcmp( str_cstr( type ), "code" ) )
				status = modsin_placeterm_code( node, info, level, auth );
			else /* if ( !strcmp( str_cstr( type ), "text" ) ) */
				status = modsin_simple( node, info, tag[is_school], lang, level );
		}
		else {
			status = modsin_simple( node, info, tag[is_school], lang, level );
		}

		if ( status!=BIBL_OK ) goto out;

		node = node->next;

	}

out:
	return status;
}

/*
 * <originInfo>
 *     <dateIssued></dateIssued>
 *     <place></place>
 *     <publisher></publisher>
 *     <edition></edition>
 *     <issuance></issuance>
 * </originInfo>
 */
static int
modsin_origininfo( xml *node, fields *info, const char *lang, int level )
{
	int status = BIBL_OK;

	node = node->down;

	while ( node ) {

		if ( xml_tag_matches( node, "dateIssued" ) )
			status = modsin_date_core( node, info, level, 0 );

		else if ( xml_tag_matches( node, "place" ) )
			status = modsin_place( node, info, lang, level );

		else if ( xml_tag_matches( node, "publisher" ) )
			status = modsin_simple( node, info, "PUBLISHER", lang, level );

		else if ( xml_tag_matches( node, "edition" ) )
			status = modsin_simple( node, info, "EDITION", lang, level );

		else if ( xml_tag_matches( node, "issuance" ) )
			status = modsin_simple( node, info, "ISSUANCE", lang, level );

		if ( status!=BIBL_OK ) goto out;

		node = node->next;

	}

out:

	return status;
}

/*
 * <subject>
 *     <topic></topic>
 *     <geographic></geographic>
 *     <topic class="primary"></topic>
 * </subject>
 */
static int
modsin_subject( xml *node, fields *info, const char *lang, int level )
{
	int status = BIBL_OK;

	node = node->down;

	while ( node ) {

		if ( xml_tag_has_attribute( node, "topic", "class", "primary" ) )
			status = modsin_simple( node, info, "EPRINTCLASS", lang, level );

		else if ( xml_tag_matches( node, "topic" ) )
			status = modsin_simple( node, info, "KEYWORD", lang, level );

		else if ( xml_tag_matches( node, "geographic" ) )
			status = modsin_simple( node, info, "KEYWORD", lang, level );

		if ( status!=BIBL_OK ) goto out;

		node = node->next;
	}

out:

	return status;
}

/* <genre></genre>
 *
 * MARC authority terms tagged with "GENRE:MARC"
 * bibutils authority terms tagged with "GENRE:BIBUTILS"
 * unknown terms tagged with "GENRE:UNKNOWN"
 */
static int
modsin_genre( xml *node, fields *info, const char *lang, int level )
{
	int fstatus;
	char *value;

	if ( !xml_has_value( node ) ) return BIBL_OK;

	value = xml_value_cstr( node );

	/* ...handle special genres in KTH DivA */
	if ( !strcmp( value, "conferenceProceedings" ) || !strcmp( value, "conferencePaper" ) )
		value = "conference publication";
	else if ( !strcmp( value, "artisticOutput" ) || !strcmp( value, "other" ) )
		value = "miscellaneous";
	else if ( !strcmp( value, "studentThesis" ) )
		value = "thesis";
	else if ( !strcmp( value, "monographDoctoralThesis" ) )
		value = "Ph.D. thesis";
	else if ( !strcmp( value, "comprehensiveDoctoralThesis" ) )
		value = "Ph.D. thesis";
	else if ( !strcmp( value, "monographLicentiateThesis" ) )
		value = "Licentiate thesis";
	else if ( !strcmp( value, "comprehensiveLicentiateThesis" ) )
		value = "Licentiate thesis";

	if ( is_marc_genre( value ) )
		fstatus = fields_add( info, "GENRE:MARC", value, level );
	else if ( is_bu_genre( value ) )
		fstatus = fields_add( info, "GENRE:BIBUTILS", value, level );
	else
		fstatus = fields_add( info, "GENRE:UNKNOWN", value, level );

	if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	else return BIBL_OK;
}

/*
 * in MODS version 3.5
 * <languageTerm type="text">....</languageTerm>
 * <languageTerm type="code" authority="xxx">...</languageTerm>
 * xxx = rfc3066
 * xxx = iso639-2b
 * xxx = iso639-3
 * xxx = rfc4646
 * xxx = rfc5646
 */
static int
modsin_languageterm( xml *node, fields *info, const char *outtag, int level )
{
	char *lang = NULL;
	str *authority;
	int fstatus;

	/* if language provided as a code, expand it */
	if ( xml_has_attribute( node, "type", "code" ) ) {
		authority = xml_attribute( node, "authority" );
		if ( authority ) {
			if ( !str_strcmpc( authority, "iso639-1" ) )
				lang = iso639_1_from_code( xml_value_cstr( node ) );
			else if ( !str_strcmpc( authority, "iso639-2b" ) )
				lang = iso639_2_from_code( xml_value_cstr( node ) );
			else if ( !str_strcmpc( authority, "iso639-3" ) )
				lang = iso639_3_from_code( xml_value_cstr( node ) );
		}
	}

	/* if not an identified code, just use the value directly */
	if ( !lang ) lang = xml_value_cstr( node );

	fstatus = fields_add( info, outtag, lang, level );
	if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;

	return BIBL_OK;
}

/*
 * <language><languageTerm></languageTerm></language>
 */
static int
modsin_language( xml *node, fields *info, const char *lang, int level )
{
	int status = BIBL_OK;

	/* Old versions of MODS had <language>English</language> */
	if ( xml_has_value( node ) ) {
		status = modsin_languageterm( node, info, "LANGUAGE", level );
	}

	/* New versions of MODS have <language><languageTerm>English</languageTerm></language> */
	node = node->down;
	while ( node ) {
		if ( xml_tag_matches( node, "languageTerm" ) && xml_has_value( node ) ) {
			status = modsin_languageterm( node, info, "LANGUAGE", level );
			if ( status!=BIBL_OK ) return status;
		}
		node = node->next;
	}

	return status;
}

static int
modsin_locationr( xml *node, fields *info, int level )
{
	int fstatus, status = BIBL_OK;
	char *url        = "URL";
	char *fileattach = "FILEATTACH";
	char *tag=NULL;

	if ( xml_tag_matches( node, "url" ) ) {
		if ( xml_has_attribute( node, "access", "raw object" ) )
			tag = fileattach;
		else
			tag = url;
	}
	else if ( xml_tag_matches( node, "physicalLocation" ) ) {
		if ( xml_has_attribute( node, "type", "school" ) )
			tag = "SCHOOL";
		else
			tag = "LOCATION";
	}

	if ( tag == url ) {
		status = urls_split_and_add( xml_value_cstr( node ), info, level );
		if ( status!=BIBL_OK ) return status;
	}
	else if ( tag ) {
		fstatus = fields_add( info, tag, xml_value_cstr( node ), level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	if ( node->down ) {
		status = modsin_locationr( node->down, info, level );
		if ( status!=BIBL_OK ) return status;
	}
	if ( node->next ) status = modsin_locationr( node->next, info, level );
	return status;
}

static int
modsin_location( xml *node, fields *info, const char *lang, int level )
{
	int status = BIBL_OK;
	if ( node->down ) status = modsin_locationr( node->down, info, level );
	return status;
}

static int
modsin_descriptionr( xml *node, str *s )
{
	int status = BIBL_OK;
	if ( xml_tag_matches( node, "extent" ) ||
	     xml_tag_matches( node, "note" ) ) {
		str_strcpy( s, &(node->value) );
		if ( str_memerr( s ) ) return BIBL_ERR_MEMERR;
	}
	if ( node->down ) {
		status = modsin_descriptionr( node->down, s );
		if ( status!=BIBL_OK ) return status;
	}
	if ( node->next ) status = modsin_descriptionr( node->next, s );
	return status;
}

static int
modsin_description( xml *node, fields *info, const char *lang, int level )
{
	int fstatus, status = BIBL_OK;
	str s;

	str_init( &s );

	if ( node->down ) {
		status = modsin_descriptionr( node->down, &s );
		if ( status!=BIBL_OK ) goto out;
	}

	else {
		if ( node->value.len > 0 )
			str_strcpy( &s, &(node->value) );
		if ( str_memerr( &s ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

	if ( str_has_value( &s ) ) {
		fstatus = fields_add( info, "DESCRIPTION", str_cstr( &s ), level );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

out:
	str_free( &s );
	return status;
}

/*
 * <part></part>
 */
static int
modsin_partr( xml *node, fields *info, int level )
{
	int status = BIBL_OK;

	if ( xml_tag_matches( node, "detail" ) )
		status = modsin_detail( node, info, level );

	else if ( xml_tag_has_attribute( node, "extent", "unit", "page" ) )
		status = modsin_page( node, info, level );

	else if ( xml_tag_has_attribute( node, "extent", "unit", "pages" ) )
		status = modsin_page( node, info, level );

	else if ( xml_tag_matches( node, "date" ) )
		status = modsin_date_core( node, info, level, 1 );

	if ( status!=BIBL_OK ) return status;

	if ( node->next ) status = modsin_partr( node->next, info, level );

	return status;
}

static int
modsin_part( xml *node, fields *info, const char *lang, int level )
{
	if ( node->down ) return modsin_partr( node->down, info, level );
	return BIBL_OK;
}

/*
 * <classification authority="lcc">Q3 .A65</classification>
 */
static int
modsin_classification( xml *node, fields *info, const char *lang, int level )
{
	int fstatus, status = BIBL_OK;
	char *tag;

	if ( xml_has_value( node ) ) {
		if ( xml_tag_has_attribute( node, "classification", "authority", "lcc" ) )
			tag = "LCC";
		else
			tag = "CLASSIFICATION";
		fstatus = fields_add( info, tag, xml_value_cstr( node ), level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	if ( node->down ) status = modsin_classification( node->down, info, lang, level );

	return status;
}

/*
 * <recordInfo>
 *     <recordIdentifier></recordIdentifier>
 *     <languageOfCataloging><languageTerm></languageTerm></languageofCataloging>
 * </recordInfo>
 */
static int
modsin_recordinfo( xml *node, fields *info, const char *lang, int level )
{
	int fstatus, status;
	xml *curr;

	/* extract recordIdentifier */
	curr = node->down;
	while ( curr ) {
		if ( xml_tag_matches_has_value( curr, "recordIdentifier" ) ) {
			fstatus = fields_add( info, "REFNUM", xml_value_cstr( curr ), level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
		else if ( xml_tag_matches( curr, "languageOfCataloging" ) ) {
			if ( curr->down ) {
				status = modsin_languageterm( curr->down, info, "LANGCATALOG", level );
				if ( status!=BIBL_OK ) return status;
			}
		}
		curr = curr->next;
	}
	return BIBL_OK;
}

static int
modsin_identifier( xml *node, fields *info, const char *lang, int level )
{
#if 0
	convert ids[] = {
		{ "citekey",       "REFNUM",      0, 0 },
		{ "issn",          "ISSN",        0, 0 },
		{ "coden",         "CODEN",       0, 0 },
		{ "isbn",          "ISBN",        0, 0 },
		{ "doi",           "DOI",         0, 0 },
		{ "url",           "URL",         0, 0 },
		{ "uri",           "URL",         0, 0 },
		{ "pmid",          "PMID",        0, 0 },
		{ "pubmed",        "PMID",        0, 0 },
		{ "medline",       "MEDLINE",     0, 0 },
		{ "pmc",           "PMC",         0, 0 },
		{ "arXiv",         "ARXIV",       0, 0 },
		{ "MRnumber",      "MRNUMBER",    0, 0 },
		{ "pii",           "PII",         0, 0 },
		{ "isi",           "ISIREFNUM",   0, 0 },
		{ "serial number", "SERIALNUMBER",0, 0 },
		{ "accessnum",     "ACCESSNUM",   0, 0 },
		{ "jstor",         "JSTOR",       0, 0 },
		{ "eid",           "EID",         0, 0 },
	};
#endif
	convert ids[] = {
		{ "citekey",       "REFNUM",      },
		{ "issn",          "ISSN",        },
		{ "coden",         "CODEN",       },
		{ "isbn",          "ISBN",        },
		{ "doi",           "DOI",         },
		{ "url",           "URL",         },
		{ "uri",           "URL",         },
		{ "pmid",          "PMID",        },
		{ "pubmed",        "PMID",        },
		{ "medline",       "MEDLINE",     },
		{ "pmc",           "PMC",         },
		{ "arXiv",         "ARXIV",       },
		{ "MRnumber",      "MRNUMBER",    },
		{ "pii",           "PII",         },
		{ "isi",           "ISIREFNUM",   },
		{ "serial number", "SERIALNUMBER",},
		{ "accessnum",     "ACCESSNUM",   },
		{ "jstor",         "JSTOR",       },
		{ "eid",           "EID",         },
	};
	int i, fstatus, n = sizeof( ids ) / sizeof( ids[0] );
	if ( node->value.len==0 ) return BIBL_OK;
	for ( i=0; i<n; ++i ) {
		if ( xml_tag_has_attribute( node, "identifier", "type", ids[i].mods ) ) {
			fstatus = fields_add( info, ids[i].internal, xml_value_cstr( node ), level );
			if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		}
	}
	return BIBL_OK;
}

static int
modsin_note( xml *node, fields *info, const char *lang, int level )
{
	if ( xml_has_attribute( node, "type", "annotation" ) )
		return modsin_simple( node, info, "ANNOTATION", lang, level );
	else
		return modsin_simple( node, info, "NOTES", lang, level );
}

static int
modsin_abstract( xml *node, fields *info, const char *lang, int level )
{
	return modsin_simple( node, info, "ABSTRACT", lang, level );
}

static int
modsin_resource( xml *node, fields *info, const char *lang, int level )
{
	return modsin_simple( node, info, "RESOURCE", lang, level );
}

static int
modsin_tablecontents( xml *node, fields *info, const char *lang, int level )
{
	return modsin_simple( node, info, "CONTENTS", lang, level );
}

static int
modsin_bibtexannote( xml *node, fields *info, const char *lang, int level )
{
	return modsin_simple( node, info, "ANNOTE", lang, level );
}

typedef struct {
	const char *tag;
	int ( *fn )( xml *, fields *, const char *, int );
} mods_vtable_t;

static int
modsin_mods( xml *node, fields *info, int level )
{
	const mods_vtable_t mods_vtable[] = {
		{ "titleInfo",           modsin_title          },
		{ "name",                modsin_name           },
		{ "recordInfo",          modsin_recordinfo     },
		{ "part",                modsin_part           },
		{ "identifier",          modsin_identifier     },
		{ "originInfo",          modsin_origininfo     },
		{ "language",            modsin_language       },
		{ "genre",               modsin_genre          },
		{ "date",                modsin_date           },
		{ "subject",             modsin_subject        },
		{ "classification",      modsin_classification },
		{ "location",            modsin_location       },
		{ "physicalDescription", modsin_description    },
		{ "note",                modsin_note           },
		{ "abstract",            modsin_abstract       },
		{ "typeOfResource",      modsin_resource       },
		{ "tableOfContents",     modsin_tablecontents  },
		{ "bibtex-annote",       modsin_bibtexannote   },
	};
	const int nmods_vtable = sizeof( mods_vtable ) / sizeof( mods_vtable[0] );

	int i, found=-1, status = BIBL_OK;
	char *lang;

	lang = modsin_get_lang_attribute( node );

	for ( i=0; i<nmods_vtable && found==-1; ++i ) {
		if ( !xml_tag_matches( node, mods_vtable[i].tag ) ) continue;
		found  = i;
		status = (*mods_vtable[i].fn)( node, info, lang, level );
	}

	if ( found==-1 ) {
		if ( xml_tag_has_attribute( node, "relatedItem", "type", "host" ) ||
		     xml_tag_has_attribute( node, "relatedItem", "type", "series" ) ) {
			if ( node->down ) status = modsin_mods( node->down, info, level+1 );
		}

		else if ( xml_tag_has_attribute( node, "relatedItem", "type", "original" ) ) {
			if ( node->down ) status = modsin_mods( node->down, info, LEVEL_ORIG );
		}
	}

	if ( status!=BIBL_OK ) return status;

	if ( node->next ) status = modsin_mods( node->next, info, level );

	return status;
}

static int
modsin_refid( xml *node, fields *info, int level )
{
	int fstatus;
	str *ns;

	ns = xml_attribute( node, "ID" );
	if ( str_has_value( ns ) ) {
		fstatus = fields_add( info, "REFNUM", str_cstr( ns ), level );
		if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	}

	return BIBL_OK;
}

static int
modsin_assembleref( xml *node, fields *info )
{
	int status = BIBL_OK;

	if ( xml_tag_matches( node, "mods" ) ) {
		status = modsin_refid( node, info, 0 );
		if ( status!=BIBL_OK ) return status;
		if ( node->down ) {
			status = modsin_mods( node->down, info, 0 );
			if ( status!=BIBL_OK ) return status;
		}
	}

	else if ( node->down ) {
		status = modsin_assembleref( node->down, info );
		if ( status!=BIBL_OK ) return status;
	}

	if ( node->next ) status = modsin_assembleref( node->next, info );

	return status;
}

static int
modsin_processf( fields *modsin, const char *data, const char *filename, long nref, param *p )
{
	int status;
	xml top;

	xml_init( &top );
	xml_parse( data, &top );
	status = modsin_assembleref( &top, modsin );
	xml_free( &top );

	if ( status==BIBL_OK ) return 1;
	else return 0;
}

/*****************************************************
 PUBLIC: int modsin_readf()
*****************************************************/

static char *
modsin_startptr( char *p, char **next )
{
	char *startptr;
	*next = NULL;
	startptr = xml_find_start( p, "mods:mods" );
	if ( startptr ) {
		/* set namespace if found */
		xml_pns = modsns;
		*next = startptr + 9;
	} else {
		startptr = xml_find_start( p, "mods" );
		if ( startptr ) {
			xml_pns = NULL;
			*next = startptr + 5;
		}
	}
	return startptr;
}

static char *
modsin_endptr( char *p )
{
	return xml_find_end( p, "mods" );
}

static int
modsin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset )
{
	str tmp;
	int m, file_charset = CHARSET_UNKNOWN;
	char *startptr = NULL, *nextptr, *endptr = NULL;

	str_init( &tmp );

	do {
		if ( line->data ) str_strcat( &tmp, line );
		if ( str_has_value( &tmp ) ) {
			m = xml_getencoding( &tmp );
			if ( m!=CHARSET_UNKNOWN ) file_charset = m;
			startptr = modsin_startptr( tmp.data, &nextptr );
			if ( nextptr ) endptr = modsin_endptr( nextptr );
		} else startptr = endptr = NULL;
		str_empty( line );
		if ( startptr && endptr ) {
			str_segcpy( reference, startptr, endptr );
			str_strcpyc( line, endptr );
		}
	} while ( !endptr && str_fget( fp, buf, bufsize, bufpos, line ) );

	str_free( &tmp );
	*fcharset = file_charset;
	return ( reference->len > 0 );
}

