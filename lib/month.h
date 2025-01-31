/*
 * month.h
 *
 * Copyright (c) Chris Putnam 2021
 *
 * Source code released under the GPL version 2
 */
#ifndef MONTH_H
#define MONTH_H

int month_to_number( const char *in, const char **out );
int month_is_number( const char *in );
int number_to_full_month( const char *in, const char **out );
int number_to_abbr_month( const char *in, const char **out );

#endif
