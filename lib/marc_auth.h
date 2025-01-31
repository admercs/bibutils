/*
 * marc_auth.h
 *
 * Recognize the MARC authority vocabulary for genre and resource.
 *
 * Copyright (c) Chris Putnam 2008-2021
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef MARC_AUTH_H
#define MARC_AUTH_H

int is_marc_genre( const char *query );
int is_marc_resource( const char *query );

char *marc_convert_relators( const char *query );
char *marc_convert_country( const char *query );


#endif
