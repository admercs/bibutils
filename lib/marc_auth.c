/*
 * marc_auth.c - Identify genre and resources to be labeled with MARC authority/
 *
 * MARC (MAchine-Readable Cataloging) 21 authority codes/values from the Library of Congress initiative
 *
 * Copyright (c) Chris Putnam 2004-2021
 *
 * Source code released under the GPL version 2
 *
 * Because the string values belonging to the MARC authority
 * are constant, search for them in a pre-calculated hash to
 * reduce O(N) run-time linear searching of the list to O(1).
 * Note that hash size was set to ensure no collisions among
 * valid terms.
 */

#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "marc_auth.h"

typedef struct marc_trans {
	char *internal_name;
	char *abbreviation;
} marc_trans;

/*
 * MARC genre hash
 */
static const unsigned int marc_genre_hash_size = 360;
static const char *marc_genre[360] = {
	[ 0 ... 359 ] = NULL,
	[  47 ] = "abstract or summary",
	[ 104 ] = "art original",
	[  88 ] = "art reproduction",
	[  34 ] = "article",
	[ 330 ] = "atlas",
	[ 110 ] = "autobiography",
	[ 343 ] = "bibliography",
	[ 237 ] = "biography",
	[ 222 ] = "book",
	[ 140 ] = "calendar",
	[ 130 ] = "catalog",
	[  39 ] = "chart",
	[  51 ] = "comic or graphic novel",
	[ 192 ] = "comic strip",
	[  46 ] = "conference publication",
	[ 243 ] = "database",
	[ 318 ] = "dictionary",
	[  62 ] = "diorama",
	[ 164 ] = "directory",
	[   5 ] = "discography",
	[ 267 ] = "drama",
	[ 303 ] = "encyclopedia",
	[ 268 ] = "essay",
	[ 188 ] = "festschrift",
	[  92 ] = "fiction",
	[  28 ] = "filmography",
	[   1 ] = "filmstrip",
	[ 307 ] = "finding aid",
	[  11 ] = "flash card",
	[ 247 ] = "folktale",
	[ 321 ] = "font",
	[ 310 ] = "game",
	[  16 ] = "government publication",
	[ 167 ] = "graphic",
	[ 319 ] = "globe",
	[  99 ] = "handbook",
	[ 187 ] = "history",
	[ 230 ] = "humor, satire",
	[  41 ] = "hymnal",
	[  98 ] = "index",
	[ 116 ] = "instruction",
	[  27 ] = "interview",
	[  32 ] = "issue",
	[  18 ] = "journal",
	[ 326 ] = "kit",
	[ 285 ] = "language instruction",
	[ 251 ] = "law report or digest",
	[ 150 ] = "legal article",
	[ 159 ] = "legal case and case notes",
	[ 327 ] = "legislation",
	[ 220 ] = "letter",
	[ 229 ] = "loose-leaf",
	[  35 ] = "map",
	[  19 ] = "memoir",
	[  83 ] = "microscope slide",
	[ 123 ] = "model",
	[ 344 ] = "motion picture",
	[  93 ] = "multivolume monograph",
	[  81 ] = "newspaper",
	[ 113 ] = "novel",
	[ 119 ] = "numeric data",
	[ 314 ] = "offprint",
	[  31 ] = "online system or service",
	[ 288 ] = "patent",
	[  54 ] = "periodical",
	[ 322 ] = "picture",
	[  48 ] = "poetry",
	[ 114 ] = "programmed text",
	[  63 ] = "realia",
	[ 261 ] = "rehearsal",
	[ 329 ] = "remote sensing image",
	[ 126 ] = "reporting",
	[ 121 ] = "review",
	[  84 ] = "series",
	[   2 ] = "short story",
	[ 323 ] = "slide",
	[  36 ] = "sound",
	[ 196 ] = "speech",
	[ 265 ] = "standard or specification",
	[  20 ] = "statistics",
	[ 215 ] = "survey of literature",
	[ 165 ] = "technical drawing",
	[ 209 ] = "technical report",
	[  33 ] = "thesis",
	[ 266 ] = "toy",
	[ 195 ] = "transparency",
	[ 308 ] = "treaty",
	[  21 ] = "videorecording",
	[ 118 ] = "web site",
	[ 249 ] = "yearbook",
};

int
is_marc_genre( const char *query )
{
	unsigned int n;

	n = calculate_hash_char( query, marc_genre_hash_size );
	if ( marc_genre[n]==NULL ) return 0;
	if ( !strcmp( query, marc_genre[n] ) ) return 1;
	else if ( marc_genre[n+1] && !strcmp( query, marc_genre[n+1] ) ) return 1;
	else return 0;
}
/*
 * MARC resource hash
 */
static const unsigned int marc_resource_hash_size = 22;
static const char *marc_resource[22] = {
	[ 0 ... 21 ] = NULL,
	[  15 ] = "cartographic",
	[   1 ] = "kit",
	[  10 ] = "mixed material",
	[   9 ] = "moving image",
	[   2 ] = "notated music",
	[  16 ] = "software, multimedia",
	[  12 ] = "sound recording",
	[  20 ] = "sound recording - musical",
	[   0 ] = "sound recording - nonmusical",
	[   6 ] = "still image",
	[  13 ] = "text",
	[   3 ] = "three dimensional object",
};

int
is_marc_resource( const char *query )
{
	unsigned int n;

	n = calculate_hash_char( query, marc_resource_hash_size );
	if ( marc_resource[n]==NULL ) return 0;
	if ( !strcmp( query, marc_resource[n] ) ) return 1;
	else if ( marc_resource[n+1] && !strcmp( query, marc_resource[n+1] ) ) return 1;
	else return 0;
}
/*
 * MARC relators hash
 */

/* www.loc.gov/marc/relators/relacode.html */

static const unsigned int marc_relators_hash_size = 1295;
static const marc_trans marc_relators[1295] = {
	[ 0 ... 1294 ] = { NULL, NULL },
	[  424 ] = { "ABRIDGER",                            "abr"                                 },
	[   23 ] = { "ART_COPYIST",                         "acp"                                 },
	[  133 ] = { "ACTOR",                               "act"                                 },
	[  404 ] = { "ART_DIRECTOR",                        "adi"                                 },
	[ 1292 ] = { "ADAPTER",                             "adp"                                 },
	[ 1138 ] = { "AFTERAUTHOR",                         "aft"                                 },
	[ 1067 ] = { "ANALYST",                             "anl"                                 },
	[  260 ] = { "ANIMATOR",                            "anm"                                 },
	[ 1031 ] = { "ANNOTATOR",                           "ann"                                 },
	[ 1045 ] = { "BIBLIOGRAPHIC_ANTECEDENT",            "ant"                                 },
	[  813 ] = { "APPELLEE",                            "ape"                                 },
	[  944 ] = { "APPELLANT",                           "apl"                                 },
	[  147 ] = { "APPLICANT",                           "app"                                 },
	[  923 ] = { "AUTHOR",                              "aqt"                                 },/* Author in quotations or text abstracts */
	[  353 ] = { "ARCHITECT",                           "arc"                                 },
	[  196 ] = { "ARTISTIC_DIRECTOR",                   "ard"                                 },
	[  951 ] = { "ARRANGER",                            "arr"                                 },
	[  286 ] = { "ARTIST",                              "art"                                 },
	[  428 ] = { "ASSIGNEE",                            "asg"                                 },
	[  734 ] = { "ASSOCIATED_NAME",                     "asn"                                 },
	[ 1112 ] = { "AUTOGRAPHER",                         "ato"                                 },
	[  699 ] = { "ATTRIBUTED_NAME",                     "att"                                 },
	[  382 ] = { "AUCTIONEER",                          "auc"                                 },
	[ 1249 ] = { "AUTHOR",                              "aud"                                 },/* Author of dialog */
	[  478 ] = { "INTROAUTHOR",                         "aui"                                 },/* Author of introduction, etc. */
	[  895 ] = { "AUTHOR",                              "aus"                                 },/* Screenwriter */
	[  332 ] = { "AUTHOR",                              "aut"                                 },
	[  354 ] = { "AUTHOR",                              "author"                              },
	[  492 ] = { "AFTERAUTHOR",                         "author of afterword, colophon, etc." },
	[  691 ] = { "INTROAUTHOR",                         "author of introduction, etc."        },
	[  784 ] = { "BINDING_DESIGNER",                    "bdd"                                 },
	[  570 ] = { "BOOKJACKET_DESIGNER",                 "bjd"                                 },
	[  778 ] = { "BOOK_DESIGNER",                       "bkd"                                 },
	[  836 ] = { "BOOK_PRODUCER",                       "bkp"                                 },
	[  997 ] = { "AUTHOR",                              "blw"                                 },/* Blurb writer */
	[  387 ] = { "BINDER",                              "bnd"                                 },
	[ 1182 ] = { "BOOKPLATE_DESIGNER",                  "bpd"                                 },
	[  437 ] = { "BROADCASTER",                         "brd"                                 },
	[ 1226 ] = { "BRAILLE_EMBOSSER",                    "brl"                                 },
	[ 1121 ] = { "BOOKSELLER",                          "bsl"                                 },
	[  194 ] = { "CASTER",                              "cas"                                 },
	[ 1168 ] = { "CONCEPTOR",                           "ccp"                                 },
	[  831 ] = { "CHOREOGRAPHER",                       "chr"                                 },
	[ 1282 ] = { "COLLABORATOR",                        "clb"                                 },
	[  453 ] = { "CLIENT",                              "cli"                                 },
	[  372 ] = { "CALLIGRAPHER",                        "cll"                                 },
	[ 1287 ] = { "COLORIST",                            "clr"                                 },
	[  758 ] = { "COLLOTYPER",                          "clt"                                 },
	[ 1010 ] = { "COMMENTATOR",                         "cmm"                                 },
	[   67 ] = { "COMPOSER",                            "cmp"                                 },
	[ 1179 ] = { "COMPOSITOR",                          "cmt"                                 },
	[  785 ] = { "CONDUCTOR",                           "cnd"                                 },
	[  979 ] = { "CINEMATOGRAPHER",                     "cng"                                 },
	[  283 ] = { "CENSOR",                              "cns"                                 },
	[  379 ] = { "CONTESTANT-APPELLEE",                 "coe"                                 },
	[ 1281 ] = { "COLLECTOR",                           "col"                                 },
	[  955 ] = { "COMPILER",                            "com"                                 },
	[  602 ] = { "CONSERVATOR",                         "con"                                 },
	[  657 ] = { "COLLECTION_REGISTRAR",                "cor"                                 },
	[  107 ] = { "CONTESTANT",                          "cos"                                 },
	[ 1058 ] = { "CONTESTANT-APPELLANT",                "cot"                                 },
	[  538 ] = { "COURT_GOVERNED",                      "cou"                                 },
	[  202 ] = { "COVER_DESIGNER",                      "cov"                                 },
	[ 1222 ] = { "COPYRIGHT_CLAIMANT",                  "cpc"                                 },
	[ 1033 ] = { "COMPLAINANT-APPELLEE",                "cpe"                                 },
	[  522 ] = { "COPYRIGHT_HOLDER",                    "cph"                                 },
	[  926 ] = { "COMPLAINANT",                         "cpl"                                 },
	[  251 ] = { "COMPLAINANT-APPELLANT",               "cpt"                                 },
	[  746 ] = { "AUTHOR",                              "cre"                                 },/* Creator */
	[  780 ] = { "AUTHOR",                              "creator"                             },/* Creator */
	[  461 ] = { "CORRESPONDENT",                       "crp"                                 },
	[  266 ] = { "CORRECTOR",                           "crr"                                 },
	[ 1269 ] = { "COURT_REPORTER",                      "crt"                                 },
	[  533 ] = { "CONSULTANT",                          "csl"                                 },
	[ 1291 ] = { "CONSULTANT",                          "csp"                                 },/* Consultant to a project */
	[  761 ] = { "COSTUME_DESIGNER",                    "cst"                                 },
	[ 1205 ] = { "CONTRIBUTOR",                         "ctb"                                 },
	[ 1110 ] = { "CONTESTEE-APPELLEE",                  "cte"                                 },
	[  516 ] = { "CARTOGRAPHER",                        "ctg"                                 },
	[  273 ] = { "CONTRACTOR",                          "ctr"                                 },
	[  695 ] = { "CONTESTEE",                           "cts"                                 },
	[  487 ] = { "CONTESTEE-APPELLANT",                 "ctt"                                 },
	[  851 ] = { "CURATOR",                             "cur"                                 },
	[  704 ] = { "COMMENTATOR",                         "cwt"                                 },/* Commentator for written text */
	[  683 ] = { "DISTRIBUTION_PLACE",                  "dbp"                                 },
	[  481 ] = { "DEGREEGRANTOR",                       "degree grantor"                      },/* Degree granting institution */
	[  999 ] = { "DEFENDANT",                           "dfd"                                 },
	[  197 ] = { "DEFENDANT-APPELLEE",                  "dfe"                                 },
	[  553 ] = { "DEFENDANT-APPELLANT",                 "dft"                                 },
	[  662 ] = { "DEGREEGRANTOR",                       "dgg"                                 },/* Degree granting institution */
	[  894 ] = { "DEGREE_SUPERVISOR",                   "dgs"                                 },
	[  534 ] = { "DISSERTANT",                          "dis"                                 },
	[    6 ] = { "DELINEATOR",                          "dln"                                 },
	[ 1113 ] = { "DANCER",                              "dnc"                                 },
	[  771 ] = { "DONOR",                               "dnr"                                 },
	[   46 ] = { "DEPICTED",                            "dpc"                                 },
	[ 1210 ] = { "DEPOSITOR",                           "dpt"                                 },
	[  762 ] = { "DRAFTSMAN",                           "drm"                                 },
	[  203 ] = { "DIRECTOR",                            "drt"                                 },
	[  953 ] = { "DESIGNER",                            "dsr"                                 },
	[ 1254 ] = { "DISTRIBUTOR",                         "dst"                                 },
	[  366 ] = { "DATA_CONTRIBUTOR",                    "dtc"                                 },
	[  613 ] = { "DEDICATEE",                           "dte"                                 },
	[  978 ] = { "DATA_MANAGER",                        "dtm"                                 },
	[  383 ] = { "DEDICATOR",                           "dto"                                 },
	[  738 ] = { "AUTHOR",                              "dub"                                 },/* Dubious author */
	[ 1143 ] = { "EDITOR",                              "edc"                                 },/* Editor of compilation */
	[  567 ] = { "EDITOR",                              "edm"                                 },/* Editor of moving image work */
	[  956 ] = { "EDITOR",                              "edt"                                 },
	[  945 ] = { "ENGRAVER",                            "egr"                                 },
	[  874 ] = { "ELECTRICIAN",                         "elg"                                 },
	[  682 ] = { "ELECTROTYPER",                        "elt"                                 },
	[  703 ] = { "ENGINEER",                            "eng"                                 },
	[  129 ] = { "ENACTING_JURISDICTION",               "enj"                                 },
	[  433 ] = { "ETCHER",                              "etr"                                 },
	[  940 ] = { "EVENT_PLACE",                         "evp"                                 },
	[  493 ] = { "EXPERT",                              "exp"                                 },
	[ 1169 ] = { "FACSIMILIST",                         "fac"                                 },
	[  191 ] = { "FILM_DISTRIBUTOR",                    "fds"                                 },
	[  314 ] = { "FIELD_DIRECTOR",                      "fld"                                 },
	[  794 ] = { "EDITOR",                              "flm"                                 },/* Film editor */
	[ 1184 ] = { "DIRECTOR",                            "fmd"                                 },/* Film director */
	[  906 ] = { "FILMMAKER",                           "fmk"                                 },
	[    1 ] = { "FORMER_OWNER",                        "fmo"                                 },
	[  614 ] = { "PRODUCER",                            "fmp"                                 },/* Film producer */
	[  288 ] = { "FUNDER",                              "fnd"                                 },
	[  430 ] = { "FIRST_PARTY",                         "fpy"                                 },
	[   81 ] = { "FORGER",                              "frg"                                 },
	[ 1087 ] = { "GEOGRAPHIC_INFORMATION_SPECIALIST",   "gis"                                 },
	[  275 ] = { "GRAPHIC_TECHNICIAN",                  "grt"                                 },
	[ 1152 ] = { "HOST_INSTITUTION",                    "his"                                 },
	[  580 ] = { "HONOREE",                             "hnr"                                 },
	[  113 ] = { "HOST",                                "hst"                                 },
	[  603 ] = { "ILLUSTRATOR",                         "ill"                                 },
	[  882 ] = { "ILLUMINATOR",                         "ilu"                                 },
	[  620 ] = { "INSCRIBER",                           "ins"                                 },
	[  590 ] = { "INVENTOR",                            "inv"                                 },
	[  814 ] = { "ISSUING_BODY",                        "isb"                                 },
	[  508 ] = { "MUSICIAN",                            "itr"                                 },/* Instrumentalist */
	[  578 ] = { "INTERVIEWEE",                         "ive"                                 },
	[ 1195 ] = { "INTERVIEWER",                         "ivr"                                 },
	[  716 ] = { "JUDGE",                               "jud"                                 },
	[ 1225 ] = { "JURISDICTION_GOVERNED",               "jug"                                 },
	[  873 ] = { "LABORATORY",                          "lbr"                                 },
	[  395 ] = { "AUTHOR",                              "lbt"                                 },/* Librettist */
	[  299 ] = { "LABORATORY_DIRECTOR",                 "ldr"                                 },
	[  927 ] = { "LEAD",                                "led"                                 },
	[  455 ] = { "LIBELEE-APPELLEE",                    "lee"                                 },
	[  865 ] = { "LIBELEE",                             "lel"                                 },
	[  552 ] = { "LENDER",                              "len"                                 },
	[   66 ] = { "LIBELEE-APPELLANT",                   "let"                                 },
	[  847 ] = { "LIGHTING_DESIGNER",                   "lgd"                                 },
	[  625 ] = { "LIBELANT-APPELLEE",                   "lie"                                 },
	[  722 ] = { "LIBELANT",                            "lil"                                 },
	[  308 ] = { "LIBELANT-APPELLANT",                  "lit"                                 },
	[  143 ] = { "LANDSCAPE_ARCHITECT",                 "lsa"                                 },
	[   11 ] = { "LICENSEE",                            "lse"                                 },
	[  700 ] = { "LICENSOR",                            "lso"                                 },
	[  165 ] = { "LITHOGRAPHER",                        "ltg"                                 },
	[  788 ] = { "AUTHOR",                              "lyr"                                 },/* Lyricist */
	[  514 ] = { "MUSIC_COPYIST",                       "mcp"                                 },
	[  131 ] = { "METADATA_CONTACT",                    "mdc"                                 },
	[   29 ] = { "MEDIUM",                              "med"                                 },
	[  645 ] = { "MANUFACTURE_PLACE",                   "mfp"                                 },
	[  164 ] = { "MANUFACTURER",                        "mfr"                                 },
	[  173 ] = { "MODERATOR",                           "mod"                                 },
	[ 1132 ] = { "THESIS_EXAMINER",                     "mon"                                 },/* Monitor */
	[ 1146 ] = { "MARBLER",                             "mrb"                                 },
	[ 1141 ] = { "EDITOR",                              "mrk"                                 },/* Markup editor */
	[  715 ] = { "MUSICAL_DIRECTOR",                    "msd"                                 },
	[  498 ] = { "METAL-ENGRAVER",                      "mte"                                 },
	[  518 ] = { "MINUTE_TAKER",                        "mtk"                                 },
	[  103 ] = { "MUSICIAN",                            "mus"                                 },
	[  348 ] = { "NARRATOR",                            "nrt"                                 },
	[  112 ] = { "THESIS_OPPONENT",                     "opn"                                 },/* Opponent */
	[  420 ] = { "ORIGINATOR",                          "org"                                 },
	[  804 ] = { "ORGANIZER",                           "organizer of meeting"                },
	[   12 ] = { "ORGANIZER",                           "orm"                                 },
	[   34 ] = { "ONSCREEN_PRESENTER",                  "osp"                                 },
	[  148 ] = { "THESIS_OTHER",                        "oth"                                 },/* Other */
	[  411 ] = { "OWNER",                               "own"                                 },
	[ 1038 ] = { "PANELIST",                            "pan"                                 },
	[  144 ] = { "PATRON",                              "pat"                                 },
	[ 1001 ] = { "ASSIGNEE",                            "patent holder"                       },
	[  386 ] = { "PUBLISHING_DIRECTOR",                 "pbd"                                 },
	[  828 ] = { "PUBLISHER",                           "pbl"                                 },
	[  334 ] = { "PROJECT_DIRECTOR",                    "pdr"                                 },
	[  811 ] = { "PROOFREADER",                         "pfr"                                 },
	[  496 ] = { "PHOTOGRAPHER",                        "pht"                                 },
	[  597 ] = { "PLATEMAKER",                          "plt"                                 },
	[  124 ] = { "PERMITTING_AGENCY",                   "pma"                                 },
	[ 1092 ] = { "PRODUCTION_MANAGER",                  "pmn"                                 },
	[  792 ] = { "PRINTER_OF_PLATES",                   "pop"                                 },
	[  924 ] = { "PAPERMAKER",                          "ppm"                                 },
	[  104 ] = { "PUPPETEER",                           "ppt"                                 },
	[  958 ] = { "PRAESES",                             "pra"                                 },
	[   31 ] = { "PROCESS_CONTRACT",                    "prc"                                 },
	[  901 ] = { "PRODUCTION_PERSONNEL",                "prd"                                 },
	[  450 ] = { "PRESENTER",                           "pre"                                 },
	[  936 ] = { "PERFORMER",                           "prf"                                 },
	[   58 ] = { "AUTHOR",                              "prg"                                 },/* Programmer */
	[  859 ] = { "PRINTMAKER",                          "prm"                                 },
	[ 1042 ] = { "PRODUCTION_COMPANY",                  "prn"                                 },
	[  872 ] = { "PRODUCER",                            "pro"                                 },
	[  867 ] = { "PRODUCTION_PLACE",                    "prp"                                 },
	[  988 ] = { "PRODUCTION_DESIGNER",                 "prs"                                 },
	[  678 ] = { "PRINTER",                             "prt"                                 },
	[  701 ] = { "PROVIDER",                            "prv"                                 },
	[  264 ] = { "PATENT_APPLICANT",                    "pta"                                 },
	[   68 ] = { "PLAINTIFF-APPELLEE",                  "pte"                                 },
	[ 1202 ] = { "PLAINTIFF",                           "ptf"                                 },
	[  581 ] = { "ASSIGNEE",                            "pth"                                 },/* Patent holder */
	[  840 ] = { "PLAINTIFF-APPELLANT",                 "ptt"                                 },
	[ 1061 ] = { "PUBLICATION_PLACE",                   "pup"                                 },
	[  254 ] = { "RUBRICATOR",                          "rbr"                                 },
	[  838 ] = { "RECORDIST",                           "rcd"                                 },
	[ 1274 ] = { "RECORDING_ENGINEER",                  "rce"                                 },
	[  421 ] = { "ADDRESSEE",                           "rcp"                                 },/* Recipient */
	[  339 ] = { "RADIO_DIRECTOR",                      "rdd"                                 },
	[   50 ] = { "REDAKTOR",                            "red"                                 },
	[  375 ] = { "RENDERER",                            "ren"                                 },
	[   75 ] = { "RESEARCHER",                          "res"                                 },
	[  504 ] = { "REVIEWER",                            "rev"                                 },
	[  400 ] = { "RADIO_PRODUCER",                      "rpc"                                 },
	[ 1070 ] = { "REPOSITORY",                          "rps"                                 },
	[ 1260 ] = { "REPORTER",                            "rpt"                                 },
	[  497 ] = { "RESPONSIBLE_PARTY",                   "rpy"                                 },
	[ 1129 ] = { "RESPONDENT-APPELLEE",                 "rse"                                 },
	[  427 ] = { "RESTAGER",                            "rsg"                                 },
	[  290 ] = { "RESPONDENT",                          "rsp"                                 },
	[  269 ] = { "RESTORATIONIST",                      "rsr"                                 },
	[  766 ] = { "RESPONDENT-APPELLANT",                "rst"                                 },
	[  243 ] = { "RESEARCH_TEAM_HEAD",                  "rth"                                 },
	[  573 ] = { "RESEARCH_TEAM_MEMBER",                "rtm"                                 },
	[  937 ] = { "SCIENTIFIC_ADVISOR",                  "sad"                                 },
	[  623 ] = { "SCENARIST",                           "sce"                                 },
	[  376 ] = { "SCULPTOR",                            "scl"                                 },
	[  892 ] = { "SCRIBE",                              "scr"                                 },
	[  747 ] = { "SOUND_DESIGNER",                      "sds"                                 },
	[  914 ] = { "SECRETARY",                           "sec"                                 },
	[  806 ] = { "STAGE_DIRECTOR",                      "sgd"                                 },
	[  919 ] = { "SIGNER",                              "sgn"                                 },
	[  913 ] = { "SUPPORTING_HOST",                     "sht"                                 },
	[  816 ] = { "SELLER",                              "sll"                                 },
	[  377 ] = { "SINGER",                              "sng"                                 },
	[  696 ] = { "SPEAKER",                             "spk"                                 },
	[  756 ] = { "SPONSOR",                             "spn"                                 },
	[ 1026 ] = { "SECOND_PARTY",                        "spy"                                 },
	[  510 ] = { "SURVEYOR",                            "srv"                                 },
	[  330 ] = { "SET_DESIGNER",                        "std"                                 },
	[  486 ] = { "SETTING",                             "stg"                                 },
	[  109 ] = { "STORYTELLER",                         "stl"                                 },
	[  893 ] = { "STAGE_MANAGER",                       "stm"                                 },
	[  802 ] = { "STANDARDS_BODY",                      "stn"                                 },
	[  462 ] = { "STEREOTYPER",                         "str"                                 },
	[  908 ] = { "TECHNICAL_DIRECTOR",                  "tcd"                                 },
	[  929 ] = { "TEACHER",                             "tch"                                 },
	[  130 ] = { "THESIS_ADVISOR",                      "ths"                                 },
	[  883 ] = { "TELEVISION_DIRECTOR",                 "tld"                                 },
	[   24 ] = { "TELEVISION_PRODUCER",                 "tlp"                                 },
	[ 1204 ] = { "TRANSCRIBER",                         "trc"                                 },
	[ 1153 ] = { "TRANSLATOR",                          "translator"                          },
	[  195 ] = { "TRANSLATOR",                          "trl"                                 },
	[  748 ] = { "TYPE_DIRECTOR",                       "tyd"                                 },
	[  284 ] = { "TYPOGRAPHER",                         "tyg"                                 },
	[   40 ] = { "UNIVERSITY_PLACE",                    "uvp"                                 },
	[   25 ] = { "VOICE_ACTOR",                         "vac"                                 },
	[  336 ] = { "VIDEOGRAPHER",                        "vdg"                                 },
	[ 1283 ] = { "VOCALIST",                            "voc"                                 },
	[ 1007 ] = { "AUTHOR",                              "wac"                                 },/* Writer of added commentary */
	[  436 ] = { "AUTHOR",                              "wal"                                 },/* Writer of added lyrics */
	[ 1123 ] = { "AUTHOR",                              "wam"                                 },/* Writer of accompanying material */
	[  960 ] = { "AUTHOR",                              "wat"                                 },/* Writer of added text */
	[  852 ] = { "WOODCUTTER",                          "wdc"                                 },
	[  550 ] = { "WOOD_ENGRAVER",                       "wde"                                 },
	[  974 ] = { "INTROAUTHOR",                         "win"                                 },/* Writer of introduction */
	[  134 ] = { "WITNESS",                             "wit"                                 },
	[  825 ] = { "INTROAUTHOR",                         "wpr"                                 },/* Writer of preface */
	[  105 ] = { "AUTHOR",                              "wst"                                 },/* Writer of supplementary textual content */
};

char *
marc_convert_relators( const char *query )
{
	unsigned int n;

	n = calculate_hash_char( query, marc_relators_hash_size );
	if ( marc_relators[n].abbreviation==NULL ) return NULL;
	if ( !strcmp( query, marc_relators[n].abbreviation ) ) return marc_relators[n].internal_name;
	else if ( marc_relators[n+1].abbreviation && !strcmp( query, marc_relators[n+1].abbreviation ) ) return marc_relators[n+1].internal_name;
	else return NULL;
}
/*
 * MARC country hash
 */

/* www.loc.gov/marc/countries/countries_code.html */

static const unsigned int marc_country_hash_size = 2018;
static const marc_trans marc_country[2018] = {
	[ 0 ... 2017 ] = { NULL, NULL },
	[ 1131 ] = { "Albania",                             "aa"                                  },
	[ 2015 ] = { "Alberta",                             "abc"                                 },
	[ 1252 ] = { "Ashmore and Cartier Islands",         "ac"                                  },/* discontinued */
	[ 1697 ] = { "Australian Capital Territory",        "aca"                                 },
	[ 1514 ] = { "Algeria",                             "ae"                                  },
	[ 1807 ] = { "Afghanistan",                         "af"                                  },
	[ 1677 ] = { "Argentina",                           "ag"                                  },
	[  737 ] = { "Armenia (Republic)",                  "ai"                                  },
	[  579 ] = { "Armenian S.S.R.",                     "air"                                 },/* discontinued */
	[ 1477 ] = { "Azerbaijan",                          "aj"                                  },
	[  350 ] = { "Azerbaijan S.S.R.",                   "ajr"                                 },/* discontinued */
	[ 1069 ] = { "Alaska",                              "aku"                                 },
	[  702 ] = { "Alabama",                             "alu"                                 },
	[  747 ] = { "Anguilla",                            "am"                                  },
	[ 1438 ] = { "Andorra",                             "an"                                  },
	[  529 ] = { "Angola",                              "ao"                                  },
	[  816 ] = { "Antigua and Barbuda",                 "aq"                                  },
	[ 1755 ] = { "Arkansas",                            "aru"                                 },
	[ 1661 ] = { "American Samoa",                      "as"                                  },
	[ 1321 ] = { "Australia",                           "at"                                  },
	[  722 ] = { "Austria",                             "au"                                  },
	[ 1865 ] = { "Aruba",                               "aw"                                  },
	[ 1395 ] = { "Antarctica",                          "ay"                                  },
	[  742 ] = { "Arizona",                             "azu"                                 },
	[ 1314 ] = { "Bahrain",                             "ba"                                  },
	[ 1346 ] = { "Barbados",                            "bb"                                  },
	[   88 ] = { "British Columbia",                    "bcc"                                 },
	[ 1976 ] = { "Burundi",                             "bd"                                  },
	[ 1427 ] = { "Belgium",                             "be"                                  },
	[  238 ] = { "Bahamas",                             "bf"                                  },
	[ 1201 ] = { "Bangladesh",                          "bg"                                  },
	[ 1315 ] = { "Belize",                              "bh"                                  },
	[  527 ] = { "British Indian Ocean Territory",      "bi"                                  },
	[  178 ] = { "Brazil",                              "bl"                                  },
	[  980 ] = { "Bermuda Islands",                     "bm"                                  },
	[ 1761 ] = { "Bosnia and Herezegovina",             "bn"                                  },
	[ 1333 ] = { "Bolivia",                             "bo"                                  },
	[ 1032 ] = { "Solomon Islands",                     "bp"                                  },
	[ 1635 ] = { "Burma",                               "br"                                  },
	[  808 ] = { "Botswana",                            "bs"                                  },
	[ 1730 ] = { "Bhutan",                              "bt"                                  },
	[ 1125 ] = { "Bulgaria",                            "bu"                                  },
	[ 1036 ] = { "Bouvet Island",                       "bv"                                  },
	[  367 ] = { "Belarus",                             "bw"                                  },
	[ 1493 ] = { "Byelorussian S.S.R",                  "bwr"                                 },/* discontinued */
	[ 1373 ] = { "Brunei",                              "bx"                                  },
	[  395 ] = { "Caribbean Netherlands",               "ca"                                  },
	[ 1318 ] = { "California",                          "cau"                                 },
	[ 1329 ] = { "Cambodia",                            "cb"                                  },
	[ 1418 ] = { "China",                               "cc"                                  },
	[  778 ] = { "Chad",                                "cd"                                  },
	[ 1844 ] = { "Sri Lanka",                           "ce"                                  },
	[  885 ] = { "Congo (Brazzaville)",                 "cf"                                  },
	[ 1749 ] = { "Congo (Democratic Republic)",         "cg"                                  },
	[  218 ] = { "China (Republic : 1949- )",           "ch"                                  },
	[ 1335 ] = { "Croatia",                             "ci"                                  },
	[  720 ] = { "Cayman Islands",                      "cj"                                  },
	[  399 ] = { "Colombia",                            "ck"                                  },
	[  584 ] = { "Chile",                               "cl"                                  },
	[  817 ] = { "Cameroon",                            "cm"                                  },
	[  710 ] = { "Canada",                              "cn"                                  },/* discontinued */
	[  580 ] = { "Curacao",                             "co"                                  },
	[ 1989 ] = { "Colorado",                            "cou"                                 },
	[  404 ] = { "Canton and Enderbury Islands",        "cp"                                  },/* discontinued */
	[  650 ] = { "Comoros",                             "cq"                                  },
	[  944 ] = { "Costa Rica",                          "cr"                                  },
	[ 1485 ] = { "Czechoslovakia",                      "cs"                                  },/* discontinued */
	[ 1960 ] = { "Connecticut",                         "ctu"                                 },
	[  275 ] = { "Cuba",                                "cu"                                  },
	[ 1876 ] = { "Cabo Verde",                          "cv"                                  },
	[ 1056 ] = { "Cook Islands",                        "cw"                                  },
	[ 1642 ] = { "Central African Republic",            "cx"                                  },
	[ 1512 ] = { "Cyprus",                              "cy"                                  },
	[  242 ] = { "Canal Zone",                          "cz"                                  },/* discontinued */
	[  613 ] = { "District of Columbia",                "dcu"                                 },
	[ 1070 ] = { "Delaware",                            "deu"                                 },
	[ 1124 ] = { "Denmark",                             "dk"                                  },
	[  368 ] = { "Benin",                               "dm"                                  },
	[ 1286 ] = { "Dominica",                            "dq"                                  },
	[ 1614 ] = { "Dominican Republic",                  "dr"                                  },
	[  792 ] = { "Eritrea",                             "ea"                                  },
	[ 1988 ] = { "Ecuador",                             "ec"                                  },
	[  462 ] = { "Equatorial Guinea",                   "eg"                                  },
	[  469 ] = { "Timor-Leste",                         "em"                                  },
	[ 1695 ] = { "England",                             "enk"                                 },
	[ 1482 ] = { "Estonia",                             "er"                                  },
	[  859 ] = { "Estonia",                             "err"                                 },/* discontinued */
	[ 1419 ] = { "El Salvador",                         "es"                                  },
	[ 1473 ] = { "Ethiopia",                            "et"                                  },
	[  820 ] = { "Faroe Islands",                       "fa"                                  },
	[  840 ] = { "French Guiana",                       "fg"                                  },
	[ 1412 ] = { "Finland",                             "fi"                                  },
	[  562 ] = { "Fiji",                                "fj"                                  },
	[ 1857 ] = { "Falkland Islands",                    "fk"                                  },
	[ 1927 ] = { "Florida",                             "flu"                                 },
	[ 1543 ] = { "Micronesia (Federated States)",       "fm"                                  },
	[  188 ] = { "French Polynesia",                    "fp"                                  },
	[ 1302 ] = { "France",                              "fr"                                  },
	[  149 ] = { "Terres australes et antarctiques francaises", "fs"                                  },
	[  632 ] = { "Djibouti",                            "ft"                                  },
	[  818 ] = { "Georgia",                             "gau"                                 },
	[   39 ] = { "Kiribati",                            "gb"                                  },
	[ 1859 ] = { "Grenada",                             "gd"                                  },
	[ 1159 ] = { "East Germany",                        "ge"                                  },/* discontinued */
	[  658 ] = { "Guernsey",                            "gg"                                  },
	[ 1996 ] = { "Ghana",                               "gh"                                  },
	[  753 ] = { "Gibraltar",                           "gi"                                  },
	[ 1376 ] = { "Greenland",                           "gl"                                  },
	[ 1962 ] = { "Gambia",                              "gm"                                  },
	[ 1445 ] = { "Gilbert and Ellice Islands",          "gn"                                  },/* discontinued */
	[ 1066 ] = { "Gabon",                               "go"                                  },
	[  193 ] = { "Guadeloupe",                          "gp"                                  },
	[ 1290 ] = { "Greece",                              "gr"                                  },
	[ 1826 ] = { "Georgia (Republic)",                  "gs"                                  },
	[  714 ] = { "Georgian S.S.R",                      "gsr"                                 },/* discontinued */
	[  546 ] = { "Guatemala",                           "gt"                                  },
	[  559 ] = { "Guam",                                "gu"                                  },
	[ 1862 ] = { "Guinea",                              "gv"                                  },
	[  150 ] = { "Germany",                             "gw"                                  },
	[ 1941 ] = { "Guyana",                              "gy"                                  },
	[  973 ] = { "Gaza Strip",                          "gz"                                  },
	[  221 ] = { "Hawaii",                              "hiu"                                 },
	[ 1692 ] = { "Hong Kong",                           "hk"                                  },/* discontinued */
	[  883 ] = { "Heard and McDonald Islands",          "hm"                                  },
	[ 1577 ] = { "Honduras",                            "ho"                                  },
	[ 1298 ] = { "Haiti",                               "ht"                                  },
	[ 1306 ] = { "Hungary",                             "hu"                                  },
	[ 1609 ] = { "Iowa",                                "iau"                                 },
	[  987 ] = { "Iceland",                             "ic"                                  },
	[ 1379 ] = { "Idaho",                               "idu"                                 },
	[ 1987 ] = { "Ireland",                             "ie"                                  },
	[ 1643 ] = { "India",                               "ii"                                  },
	[    3 ] = { "Illinois",                            "ilu"                                 },
	[ 1149 ] = { "Isle of Man",                         "im"                                  },
	[ 1089 ] = { "Indiana",                             "inu"                                 },
	[  796 ] = { "Indonesia",                           "io"                                  },
	[ 1883 ] = { "Iraq",                                "iq"                                  },
	[  312 ] = { "Iran",                                "ir"                                  },
	[ 1884 ] = { "Israel",                              "is"                                  },
	[  699 ] = { "Italy",                               "it"                                  },
	[  829 ] = { "Israel-Syria Demilitarized Zones",    "iu"                                  },/* discontinued */
	[ 1838 ] = { "Cote d'Ivoire",                       "iv"                                  },
	[ 1535 ] = { "Isreal-Jordan Demilitarized Zones",   "iw"                                  },/* discontinued */
	[ 1456 ] = { "Iraq-Saudi Arabia Neutral Zone",      "iy"                                  },
	[ 1037 ] = { "Japan",                               "ja"                                  },
	[  746 ] = { "Jersey",                              "je"                                  },
	[ 1190 ] = { "Johnston Atoll",                      "ji"                                  },
	[ 1580 ] = { "Jamaica",                             "jm"                                  },
	[  283 ] = { "Jan Mayen",                           "jn"                                  },/* discontinued */
	[ 1011 ] = { "Jordan",                              "jo"                                  },
	[  607 ] = { "Kenya",                               "ke"                                  },
	[  349 ] = { "Kyrgyzstan",                          "kg"                                  },
	[ 1926 ] = { "Kirghiz S.S.R.",                      "kgr"                                 },
	[ 1389 ] = { "North Korea",                         "kn"                                  },
	[ 1068 ] = { "South Korea",                         "ko"                                  },
	[ 1155 ] = { "Kansas",                              "ksu"                                 },
	[ 1496 ] = { "Kuwait",                              "ku"                                  },
	[ 1917 ] = { "Kosovo",                              "kv"                                  },
	[ 1429 ] = { "Kentucky",                            "kyu"                                 },
	[ 1038 ] = { "Kazakhstan",                          "kz"                                  },
	[  266 ] = { "Kazakh S.S.R.",                       "kzr"                                 },/* discontinued */
	[  672 ] = { "Louisiana",                           "lau"                                 },
	[ 1898 ] = { "Liberia",                             "lb"                                  },
	[   25 ] = { "Lebanon",                             "le"                                  },
	[ 1925 ] = { "Liechtenstein",                       "lh"                                  },
	[ 1803 ] = { "Lithuania",                           "li"                                  },
	[ 1603 ] = { "Lithuania",                           "lir"                                 },/* discontinued */
	[ 1768 ] = { "Central and Southern Line Islands",   "ln"                                  },/* discontinued */
	[ 1735 ] = { "Lesotho",                             "lo"                                  },
	[  227 ] = { "Laos",                                "ls"                                  },
	[ 1780 ] = { "Luxembourg",                          "lu"                                  },
	[ 1355 ] = { "Latvia",                              "lv"                                  },
	[  444 ] = { "Latvia",                              "lvr"                                 },/* discontinued */
	[   17 ] = { "Libya",                               "ly"                                  },
	[  854 ] = { "Massachusetts",                       "mau"                                 },
	[ 1639 ] = { "Manitoba",                            "mbc"                                 },
	[  646 ] = { "Monaco",                              "mc"                                  },
	[  287 ] = { "Maryland",                            "mdu"                                 },
	[ 1230 ] = { "Maine",                               "meu"                                 },
	[  922 ] = { "Mauritius",                           "mf"                                  },
	[  298 ] = { "Madagascar",                          "mg"                                  },
	[  422 ] = { "Macao",                               "mh"                                  },/* discontinued */
	[ 1819 ] = { "Michigan",                            "miu"                                 },
	[  862 ] = { "Montserrat",                          "mj"                                  },
	[  219 ] = { "Oman",                                "mk"                                  },
	[ 1717 ] = { "Mali",                                "ml"                                  },
	[  774 ] = { "Malta",                               "mm"                                  },
	[   41 ] = { "Minnesota",                           "mnu"                                 },
	[ 1541 ] = { "Montenegro",                          "mo"                                  },
	[ 1553 ] = { "Missouri",                            "mou"                                 },
	[  112 ] = { "Mongolia",                            "mp"                                  },
	[  954 ] = { "Martinique",                          "mq"                                  },
	[  411 ] = { "Morocco",                             "mr"                                  },
	[ 1504 ] = { "Mississippi",                         "msu"                                 },
	[ 1820 ] = { "Montana",                             "mtu"                                 },
	[  568 ] = { "Mauritania",                          "mu"                                  },
	[   51 ] = { "Moldova",                             "mv"                                  },
	[ 1231 ] = { "Moldavian S.S.R.",                    "mvr"                                 },/* discontinued */
	[  869 ] = { "Malawi",                              "mw"                                  },
	[ 1364 ] = { "Mexico",                              "mx"                                  },
	[  327 ] = { "Malaysia",                            "my"                                  },
	[ 1720 ] = { "Mozambique",                          "mz"                                  },
	[  893 ] = { "Netherlands Antilles",                "na"                                  },/* discontinued */
	[ 1830 ] = { "Nebraska",                            "nbu"                                 },
	[  950 ] = { "North Carolina",                      "ncu"                                 },
	[ 1168 ] = { "North Dakota",                        "ndu"                                 },
	[ 1200 ] = { "Netherlands",                         "ne"                                  },
	[ 1912 ] = { "Newfoundland and Labrador",           "nfc"                                 },
	[  262 ] = { "Niger",                               "ng"                                  },
	[  222 ] = { "New Hampshire",                       "nhu"                                 },
	[ 1679 ] = { "Northern Ireland",                    "nik"                                 },
	[ 1872 ] = { "New Jersey",                          "nju"                                 },
	[ 1751 ] = { "New Brunswick",                       "nkc"                                 },
	[ 1959 ] = { "New Caledonia",                       "nl"                                  },
	[ 1825 ] = { "Northern Mariana Islands",            "nm"                                  },/* discontinued */
	[ 1742 ] = { "New Mexico",                          "nmu"                                 },
	[  752 ] = { "Vanuatu",                             "nn"                                  },
	[ 1467 ] = { "Norway",                              "no"                                  },
	[ 1775 ] = { "Nepal",                               "np"                                  },
	[ 1086 ] = { "Nicaragua",                           "nq"                                  },
	[   42 ] = { "Nigeria",                             "nr"                                  },
	[ 1262 ] = { "Nova Scotia",                         "nsc"                                 },
	[ 1956 ] = { "Northwest Territories",               "ntc"                                 },
	[  934 ] = { "Nauru",                               "nu"                                  },
	[ 1928 ] = { "Nunavut",                             "nuc"                                 },
	[  270 ] = { "Nevada",                              "nvu"                                 },
	[ 1509 ] = { "Northern Mariana Islands",            "nw"                                  },
	[  999 ] = { "Norfolk Island",                      "nx"                                  },
	[  200 ] = { "New York",                            "nyu"                                 },
	[  530 ] = { "New Zealand",                         "nz"                                  },
	[  220 ] = { "Ohio",                                "ohu"                                 },
	[ 1980 ] = { "Oklahoma",                            "oku"                                 },
	[  427 ] = { "Ontario",                             "onc"                                 },
	[ 1021 ] = { "Oregon",                              "oru"                                 },
	[  970 ] = { "Mayotte",                             "ot"                                  },
	[ 1180 ] = { "Pennsylvania",                        "pau"                                 },
	[  606 ] = { "Pitcairn Island",                     "pc"                                  },
	[ 1693 ] = { "Peru",                                "pe"                                  },
	[  482 ] = { "Paracel Islands",                     "pf"                                  },
	[ 1186 ] = { "Guinea-Bissau",                       "pg"                                  },
	[ 1331 ] = { "Philippines",                         "ph"                                  },
	[  515 ] = { "Prince Edward Island",                "pic"                                 },
	[ 1823 ] = { "Pakistan",                            "pk"                                  },
	[  564 ] = { "Poland",                              "pl"                                  },
	[ 1958 ] = { "Panama",                              "pn"                                  },
	[ 2016 ] = { "Portugal",                            "po"                                  },
	[  184 ] = { "Papua New Guinea",                    "pp"                                  },
	[ 1529 ] = { "Puerto Rico",                         "pr"                                  },
	[  138 ] = { "Portuguese Timor",                    "pt"                                  },
	[   78 ] = { "Palau",                               "pw"                                  },
	[  381 ] = { "Paraguay",                            "py"                                  },
	[   10 ] = { "Qatar",                               "qa"                                  },
	[  359 ] = { "Queensland",                          "qea"                                 },
	[ 1289 ] = { "Quebec",                              "quc"                                 },
	[ 1071 ] = { "Serbia",                              "rb"                                  },
	[ 1450 ] = { "Reunion",                             "re"                                  },
	[ 1330 ] = { "Zimbabwe",                            "rh"                                  },
	[   97 ] = { "Romania",                             "rm"                                  },
	[  864 ] = { "Russian Federation",                  "ru"                                  },
	[ 1387 ] = { "Russian S.F.S.R",                     "rur"                                 },/* discontinued */
	[  228 ] = { "Rwanda",                              "rw"                                  },
	[  377 ] = { "Southern Ryukyu Islands",             "ry"                                  },/* discontinued */
	[ 1558 ] = { "South Africa",                        "sa"                                  },
	[ 1770 ] = { "Svalbard",                            "sb"                                  },/* discontinued */
	[ 1694 ] = { "Saint_Barthelemy",                    "sc"                                  },
	[ 1198 ] = { "South Carolina",                      "scu"                                 },
	[  586 ] = { "South Sudan",                         "sd"                                  },
	[ 1397 ] = { "Seychelles",                          "se"                                  },
	[   27 ] = { "Sao Tome and Principe",               "sf"                                  },
	[  335 ] = { "Senegal",                             "sg"                                  },
	[ 1861 ] = { "Spanish North Africa",                "sh"                                  },
	[    5 ] = { "Singapore",                           "si"                                  },
	[  126 ] = { "Sudan",                               "sj"                                  },
	[  430 ] = { "Sikkim",                              "sk"                                  },/* discontinued */
	[  667 ] = { "Sierra Leone",                        "sl"                                  },
	[ 1129 ] = { "San Marino",                          "sm"                                  },
	[  647 ] = { "Sint Maarten",                        "sn"                                  },
	[  107 ] = { "Saskatchewan",                        "snc"                                 },
	[ 1567 ] = { "Somalia",                             "so"                                  },
	[   65 ] = { "Spain",                               "sp"                                  },
	[ 1019 ] = { "Eswatini",                            "sq"                                  },
	[  656 ] = { "Surinam",                             "sr"                                  },
	[ 1022 ] = { "Western Sahara",                      "ss"                                  },
	[  609 ] = { "Saint-Martin",                        "st"                                  },
	[ 1478 ] = { "Scotland",                            "stk"                                 },
	[ 1323 ] = { "Saudi Arabia",                        "su"                                  },
	[   47 ] = { "Swan Islands",                        "sv"                                  },
	[ 1233 ] = { "Sweden",                              "sw"                                  },
	[  338 ] = { "Namibia",                             "sx"                                  },
	[  961 ] = { "Syria",                               "sy"                                  },
	[ 1719 ] = { "Switzerland",                         "sz"                                  },
	[   66 ] = { "Tajikistan",                          "ta"                                  },
	[  259 ] = { "Tajik S.S.R",                         "tar"                                 },/* discontinued */
	[  948 ] = { "Turks and Caicos Islands",            "tc"                                  },
	[ 1790 ] = { "Togo",                                "tg"                                  },
	[   46 ] = { "Thailand",                            "th"                                  },
	[  804 ] = { "Tunisia",                             "ti"                                  },
	[  572 ] = { "Turkmenistan",                        "tk"                                  },
	[  425 ] = { "Turkmen S.S.R.",                      "tkr"                                 },/* discontinued */
	[ 1877 ] = { "Tokelau",                             "tl"                                  },
	[  909 ] = { "Tasmania",                            "tma"                                 },
	[ 1501 ] = { "Tennessee",                           "tnu"                                 },
	[  578 ] = { "Tonga",                               "to"                                  },
	[  373 ] = { "Trinidad and Tobago",                 "tr"                                  },
	[ 1061 ] = { "United Arab Emirates",                "ts"                                  },
	[  120 ] = { "Trust Territory of the Pacific Islands", "tt"                                  },/* discontinued */
	[  375 ] = { "Turkey",                              "tu"                                  },
	[ 1359 ] = { "Tuvalu",                              "tv"                                  },
	[  489 ] = { "Texas",                               "txu"                                 },
	[ 1117 ] = { "Tanzania",                            "tz"                                  },
	[  151 ] = { "Egypt",                               "ua"                                  },
	[ 1885 ] = { "United States Misc. Caribbean Islands", "uc"                                  },
	[  307 ] = { "Uganda",                              "ug"                                  },
	[   87 ] = { "United Kingdom Misc. Islands",        "ui"                                  },/* discontinued */
	[ 1671 ] = { "United Kingdom Misc. Islands",        "uik"                                 },/* discontinued */
	[  438 ] = { "United Kingdom",                      "uk"                                  },/* discontinued */
	[  723 ] = { "Ukraine",                             "un"                                  },
	[ 1593 ] = { "Ukraine",                             "unr"                                 },/* discontinued */
	[ 1878 ] = { "United States Misc. Pacific Islands", "up"                                  },
	[   58 ] = { "Soviet Union",                        "ur"                                  },/* discontinued */
	[ 1713 ] = { "United States",                       "us"                                  },/* discontinued */
	[  985 ] = { "Utah",                                "utu"                                 },
	[ 1637 ] = { "Burkina Faso",                        "uv"                                  },
	[ 1040 ] = { "Uruguay",                             "uy"                                  },
	[  156 ] = { "Uzbekistan",                          "uz"                                  },
	[ 1067 ] = { "Uzbek S.S.R.",                        "uzr"                                 },/* discontinued */
	[ 1325 ] = { "Virginia",                            "vau"                                 },
	[   63 ] = { "British Virgin Islands",              "vb"                                  },
	[ 1970 ] = { "Vatican City",                        "vc"                                  },
	[ 1963 ] = { "Venezuela",                           "ve"                                  },
	[ 1234 ] = { "Virgin Islands of the United States", "vi"                                  },
	[   59 ] = { "Vietnam",                             "vm"                                  },
	[ 1984 ] = { "North Vietnam",                       "vn"                                  },/* discontinued */
	[ 1080 ] = { "Various places",                      "vp"                                  },
	[ 1400 ] = { "Victoria",                            "vra"                                 },
	[ 1199 ] = { "South Vietnam",                       "vs"                                  },
	[ 1815 ] = { "Vermont",                             "vtu"                                 },
	[  994 ] = { "Washington",                          "wau"                                 },
	[ 1858 ] = { "West Berlin",                         "wb"                                  },
	[ 1341 ] = { "Western Australia",                   "wea"                                 },
	[ 1457 ] = { "Wallis and Futuna",                   "wf"                                  },
	[   29 ] = { "Wisconsin",                           "wiu"                                 },
	[  655 ] = { "West Bank of the Jordan River",       "wj"                                  },
	[  173 ] = { "Wake Island",                         "wk"                                  },
	[ 1312 ] = { "Wales",                               "wlk"                                 },
	[ 1481 ] = { "Samoa",                               "ws"                                  },
	[ 1402 ] = { "West Virginia",                       "wvu"                                 },
	[  265 ] = { "Wyoming",                             "wyu"                                 },
	[  936 ] = { "Christmas Island (Indian Ocean)",     "xa"                                  },
	[ 1039 ] = { "Cocus (Keeling) Islands",             "xb"                                  },
	[  346 ] = { "Maldives",                            "xc"                                  },
	[ 1938 ] = { "Saint Kitts-Nevis",                   "xd"                                  },
	[ 1736 ] = { "Marshall Islands",                    "xe"                                  },
	[ 1337 ] = { "Midway Islands",                      "xf"                                  },
	[  243 ] = { "Coral Sea Islands Territory",         "xga"                                 },
	[  860 ] = { "Niue",                                "xh"                                  },
	[ 1320 ] = { "Saint Kitts-Nevis-Anguilla",          "xi"                                  },/* discontinued */
	[  593 ] = { "Saint Helena",                        "xj"                                  },
	[  705 ] = { "Saint Lucia",                         "xk"                                  },
	[  211 ] = { "Saint Pierre and Miquelon",           "xl"                                  },
	[  684 ] = { "Saint Vincent and the Grenadines",    "xm"                                  },
	[  129 ] = { "North Macedonia",                     "xn"                                  },
	[ 1800 ] = { "New South Wales",                     "xna"                                 },
	[  249 ] = { "Slovakia",                            "xo"                                  },
	[ 1235 ] = { "Northern Territory",                  "xoa"                                 },
	[  657 ] = { "Spratly Island",                      "xp"                                  },
	[ 1299 ] = { "Czech Republic",                      "xr"                                  },
	[  279 ] = { "South Australia",                     "xra"                                 },
	[  284 ] = { "South Georgia and the South Sandwich Islands", "xs"                                  },
	[ 1340 ] = { "Slovenia",                            "xv"                                  },
	[ 1971 ] = { "No place, unknown, or undetermined",  "xx"                                  },
	[ 1568 ] = { "Canada",                              "xxc"                                 },
	[  668 ] = { "United Kingdom",                      "xxk"                                 },
	[ 1483 ] = { "Soviet Union",                        "xxr"                                 },/* discontinued */
	[ 1875 ] = { "United States",                       "xxu"                                 },
	[  697 ] = { "Yemen",                               "ye"                                  },
	[  902 ] = { "Yukon Territory",                     "ykc"                                 },
	[ 1569 ] = { "Yemen (People's Democratic Republic)", "ys"                                  },/* discontinued */
	[ 1447 ] = { "Serbia and Montenegro",               "yu"                                  },/* discontinued */
	[ 1827 ] = { "Zambia",                              "za"                                  },
};

char *
marc_convert_country( const char *query )
{
	unsigned int n;

	n = calculate_hash_char( query, marc_country_hash_size );
	if ( marc_country[n].abbreviation==NULL ) return NULL;
	if ( !strcmp( query, marc_country[n].abbreviation ) ) return marc_country[n].internal_name;
	else if ( marc_country[n+1].abbreviation && !strcmp( query, marc_country[n+1].abbreviation ) ) return marc_country[n+1].internal_name;
	else return NULL;
}
