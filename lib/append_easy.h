/*
 * append_easy.h
 *
 * Most output formats have identical requirements for outputting
 * easy tag/value pairs, so centralize code.
 *
 * append_easy()      - output first match
 * append_easyall()   - output all matches as separate tag/value pairs
 * append_easycombo() - output all matches as a single tag/value pair, where
 *                      values are separated by the sep string
 *
 * Copyright (c) Chris Putnam 2020-2021
 *
 * Source code released under the GPL version 2
 */
#ifndef APPEND_EASY_H
#define APPEND_EASY_H

#include "fields.h"

void append_easy      ( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, int *status );
void append_easyall   ( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, int *status );
void append_easyallpre( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, const char *prefix, int *status );
void append_easycombo ( fields *in, const char *intag, int inlevel, fields *out, const char *outtag, const char *sep,    int *status );
int  append_easypage  ( fields *out, const char *outtag, const char *value, int level );


#endif
