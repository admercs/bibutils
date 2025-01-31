/*
 * title.h
 *
 * add titles as title/subtitle pairs for MODS
 *
 * Copyright (c) Chris Putnam 2004-2021
 *
 * Source code released under the GPL verison 2
 *
 */
#ifndef TITLE_H
#define TITLE_H

#include "str.h"
#include "fields.h"

int  add_title( fields *info, const char *tag, const char *value, int level, unsigned char nosplittitle );
void title_combine( str *fullttl, str *mainttl, str *subttl );

#endif
