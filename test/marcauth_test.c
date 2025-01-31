/*
 * marcauth_test.c
 *
 * Copyright (c) 2020-2021
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "marc_auth.h"

char progname[] = "marcauth_test";

typedef struct test_t {
	char *s;
	int expected;
} test_t;

int
test_is_marcgenre( void )
{
	test_t tests[] = {

		{ "xxxxxxxx",             0 },
		{ "electronicXX",         0 },

		{ "abstract or summary",  1 },
		{ "art original",         1 },
		{ "art reproduction",     1 },
		{ "article",              1 },
		{ "atlas",                1 },
		{ "autobiography",        1 },
		{ "bibliography",         1 },
		{ "biography",            1 },
		{ "book",                 1 },
		{ "calendar",             1 },
		{ "catalog",              1 },
		{ "chart",                1 },
		{ "comic or graphic novel", 1 },
		{ "comic strip",            1 },
		{ "conference publication", 1 },
		{ "database",               1 },
		{ "dictionary",             1 },
		{ "diorama",                1 },
		{ "directory",              1 },
		{ "discography",            1 },
		{ "drama",                  1 },
		{ "encyclopedia",           1 },
		{ "essay",                  1 },
		{ "festschrift",            1 },
		{ "fiction",                1 },
		{ "filmography",            1 },
		{ "filmstrip",              1 },
		{ "finding aid",            1 },
		{ "flash card",             1 },
		{ "folktale",               1 },
		{ "font",                   1 },
		{ "game",                   1 },
		{ "government publication", 1 },
		{ "graphic",                1 },
		{ "globe",                  1 },
		{ "handbook",               1 },
		{ "history",                1 },
		{ "humor, satire",          1 },
		{ "hymnal",                 1 },
		{ "index",                  1 },
		{ "instruction",            1 },
		{ "interview",              1 },
		{ "issue",                  1 },
		{ "journal",                1 },
		{ "kit",                    1 },
		{ "language instruction",   1 },
		{ "law report or digest",   1 },
		{ "legal article",          1 },
		{ "legal case and case notes", 1 },
		{ "legislation",            1 },
		{ "letter",                 1 },
		{ "loose-leaf",             1 },
		{ "map",                    1 },
		{ "memoir",                 1 },
		{ "microscope slide",       1 },
		{ "model",                  1 },
		{ "motion picture",         1 },
		{ "multivolume monograph",  1 },
		{ "newspaper",              1 },
		{ "novel",                  1 },
		{ "numeric data",           1 },
		{ "offprint",               1 },
		{ "online system or service", 1 },
		{ "patent",                 1 },
		{ "periodical",             1 },
		{ "picture",                1 },
		{ "poetry",                 1 },
		{ "programmed text",        1 },
		{ "realia",                 1 },
		{ "rehearsal",              1 },
		{ "remote sensing image",   1 },
		{ "reporting",              1 },
		{ "review",                 1 },
		{ "series",                 1 },
		{ "short story",            1 },
		{ "slide",                  1 },
		{ "sound",                  1 },
		{ "speech",                 1 },
		{ "standard or specification", 1 },
		{ "statistics",             1 },
		{ "survey of literature",   1 },
		{ "technical drawing",      1 },
		{ "technical report",       1 },
		{ "thesis",                 1 },
		{ "toy",                    1 },
		{ "transparency",           1 },
		{ "treaty",                 1 },
		{ "videorecording",         1 },
		{ "web site",               1 },
		{ "yearbook",               1 },

	};

	int ntests = sizeof( tests ) / sizeof( tests[0] );
	int failed = 0;
	int found;
	int i;

	for ( i=0; i<ntests; ++i ) {
		found = is_marc_genre( tests[i].s );
		if ( found != tests[i].expected ) {
			printf( "%s: Error is_marc_genre( '%s' ) returned %d, expected %d\n", progname, tests[i].s, found, tests[i].expected );
			failed++;
		}
	}
	return failed;
}

int
main( int argc, char *argv[] )
{
	int failed = 0;
	failed += test_is_marcgenre();
	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}
}
