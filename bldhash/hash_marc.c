/*
 * hash_marc.c
 *
 * MARC (MAchine-Readable Cataloging) 21 authority codes/values from the Library of Congress initiative
 *
 * Copyright (c) Chris Putnam 2020-2021
 *
 * Source code released under the GPL version 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"
#include "uintlist.h"

static const char *marc_genre[] = {
	"abstract or summary",
	"art original",
	"art reproduction",
	"article",
	"atlas",
	"autobiography",
	"bibliography",
	"biography",
	"book",
	"calendar",
	"catalog",
	"chart",
	"comic or graphic novel",
	"comic strip",
	"conference publication",
	"database",
	"dictionary",
	"diorama",
	"directory",
	"discography",
	"drama",
	"encyclopedia",
	"essay",
	"festschrift",
	"fiction",
	"filmography",
	"filmstrip",
	"finding aid",
	"flash card",
	"folktale",
	"font",
	"game",
	"government publication",
	"graphic",
	"globe",
	"handbook",
	"history",
	"humor, satire",
	"hymnal",
	"index",
	"instruction",
	"interview",
	"issue",
	"journal",
	"kit",
	"language instruction",
	"law report or digest",
	"legal article",
	"legal case and case notes",
	"legislation",
	"letter",
	"loose-leaf",
	"map",
	"memoir",
	"microscope slide",
	"model",
	"motion picture",
	"multivolume monograph",
	"newspaper",
	"novel",
	"numeric data",
	"offprint",
	"online system or service",
	"patent",
	"periodical",
	"picture",
	"poetry",
	"programmed text",
	"realia",
	"rehearsal",
	"remote sensing image",
	"reporting",
	"review",
	"series",
	"short story",
	"slide",
	"sound",
	"speech",
	"standard or specification",
	"statistics",
	"survey of literature",
	"technical drawing",
	"technical report",
	"thesis",
	"toy",
	"transparency",
	"treaty",
	"videorecording",
	"web site",
	"yearbook",
};
static const int nmarc_genre = sizeof( marc_genre ) / sizeof( const char* );

static const char *marc_resource[] = {
	"cartographic",
	"kit",
	"mixed material",
	"moving image",
	"notated music",
	"software, multimedia",
	"sound recording",
	"sound recording - musical",
	"sound recording - nonmusical",
	"still image",
	"text",
	"three dimensional object"
};
static const int nmarc_resource = sizeof( marc_resource ) / sizeof( const char* );

typedef struct marc_trans {
	char *internal_name;
	char *abbreviation;
	char *comment;
} marc_trans;

static const marc_trans marc_country[] = {
	{ "Albania",                        "aa",  NULL },
	{ "Alberta",	                    "abc", NULL },
	{ "Ashmore and Cartier Islands",    "ac", "discontinued" },
	{ "Australian Capital Territory",   "aca", NULL },
	{ "Algeria",                        "ae",  NULL },
	{ "Afghanistan",                    "af",  NULL },
	{ "Argentina",                      "ag",  NULL },
	//{ "Anguilla",                     "-ai", "discontinued" },
	{ "Armenia (Republic)",             "ai",  NULL },
	{ "Armenian S.S.R.",                "air", "discontinued" },
	{ "Azerbaijan",                     "aj",  NULL },
	{ "Azerbaijan S.S.R.",              "ajr", "discontinued" },
	{ "Alaska",                         "aku", NULL },
	{ "Alabama",                        "alu", NULL },
	{ "Anguilla",                       "am",  NULL },
	{ "Andorra",                        "an",  NULL },
	{ "Angola",                         "ao",  NULL },
	{ "Antigua and Barbuda",            "aq",  NULL },
	{ "Arkansas",                       "aru", NULL },
	{ "American Samoa",                 "as",  NULL },
	{ "Australia",                      "at",  NULL },
	{ "Austria",                        "au",  NULL },
	{ "Aruba",                          "aw",  NULL },
	{ "Antarctica",                     "ay",  NULL },
	{ "Arizona",                        "azu", NULL },
	{ "Bahrain",                        "ba",  NULL },
	{ "Barbados",                       "bb",  NULL },
	{ "British Columbia",               "bcc", NULL },
	{ "Burundi",                        "bd",  NULL },
	{ "Belgium",                        "be",  NULL },
	{ "Bahamas",                        "bf",  NULL },
	{ "Bangladesh",                     "bg",  NULL },
	{ "Belize",                         "bh",  NULL },
	{ "British Indian Ocean Territory", "bi",  NULL },
	{ "Brazil",                         "bl",  NULL },
	{ "Bermuda Islands",                "bm",  NULL },
	{ "Bosnia and Herezegovina",        "bn",  NULL },
	{ "Bolivia",                        "bo",  NULL },
	{ "Solomon Islands",                "bp",  NULL },
	{ "Burma",                          "br",  NULL },
	{ "Botswana",                       "bs",  NULL },
	{ "Bhutan",                         "bt",  NULL },
	{ "Bulgaria",                       "bu",  NULL },
	{ "Bouvet Island",                  "bv",  NULL },
	{ "Belarus",                        "bw",  NULL },
	{ "Byelorussian S.S.R",             "bwr", "discontinued" },
	{ "Brunei",                         "bx",  NULL },
	{ "Caribbean Netherlands",          "ca",  NULL },
	{ "California",                     "cau", NULL },
	{ "Cambodia",                       "cb",  NULL },
	{ "China",                          "cc",  NULL },
	{ "Chad",                           "cd",  NULL },
	{ "Sri Lanka",                      "ce",  NULL },
	{ "Congo (Brazzaville)",            "cf",  NULL },
	{ "Congo (Democratic Republic)",    "cg",  NULL },
	{ "China (Republic : 1949- )",      "ch",  NULL },
	{ "Croatia",                        "ci",  NULL },
	{ "Cayman Islands",                 "cj",  NULL },
	{ "Colombia",                       "ck",  NULL },
	{ "Chile",                          "cl",  NULL },
	{ "Cameroon",                       "cm",  NULL },
	{ "Canada",                         "cn",  "discontinued" },
	{ "Curacao",                        "co",  NULL },
	{ "Colorado",                       "cou", NULL },
	{ "Canton and Enderbury Islands",   "cp",  "discontinued" },
	{ "Comoros",                        "cq",  NULL },
	{ "Costa Rica",                     "cr",  NULL },
	{ "Czechoslovakia",                 "cs",  "discontinued" },
	{ "Connecticut",                    "ctu", NULL },
	{ "Cuba",                           "cu",  NULL },
	{ "Cabo Verde",                     "cv",  NULL },
	{ "Cook Islands",                   "cw",  NULL },
	{ "Central African Republic",       "cx",  NULL },
	{ "Cyprus",                         "cy",  NULL },
	{ "Canal Zone",                     "cz",  "discontinued" },
	{ "District of Columbia",           "dcu", NULL },
	{ "Delaware",                       "deu", NULL },
	{ "Denmark",                        "dk",  NULL },
	{ "Benin",                          "dm",  NULL },
	{ "Dominica",                       "dq",  NULL },
	{ "Dominican Republic",             "dr",  NULL },
	{ "Eritrea",                        "ea",  NULL },
	{ "Ecuador",                        "ec",  NULL },
	{ "Equatorial Guinea",              "eg",  NULL },
	{ "Timor-Leste",                    "em",  NULL },
	{ "England",                        "enk", NULL },
	{ "Estonia",                        "er",  NULL },
	{ "Estonia",                        "err", "discontinued" },
	{ "El Salvador",                    "es",  NULL },
	{ "Ethiopia",                       "et",  NULL },
	{ "Faroe Islands",                  "fa",  NULL },
	{ "French Guiana",                  "fg",  NULL },
	{ "Finland",                        "fi",  NULL },
	{ "Fiji",                           "fj",  NULL },
	{ "Falkland Islands",               "fk",  NULL },
	{ "Florida",                        "flu", NULL },
	{ "Micronesia (Federated States)",  "fm",  NULL },
	{ "French Polynesia",               "fp",  NULL },
	{ "France",                         "fr",  NULL },
	{ "Terres australes et antarctiques francaises", "fs", NULL },
	{ "Djibouti",                       "ft",  NULL },
	{ "Georgia",                        "gau", NULL },
	{ "Kiribati",                       "gb",  NULL },
	{ "Grenada",                        "gd",  NULL },
	{ "East Germany",                   "ge",  "discontinued" },
	{ "Guernsey",                       "gg",  NULL },
	{ "Ghana",                          "gh",  NULL },
	{ "Gibraltar",                      "gi",  NULL },
	{ "Greenland",                      "gl",  NULL },
	{ "Gambia",                         "gm",  NULL },
	{ "Gilbert and Ellice Islands",     "gn",  "discontinued" },
	{ "Gabon",                          "go",  NULL },
	{ "Guadeloupe",                     "gp",  NULL },
	{ "Greece",                         "gr",  NULL },
	{ "Georgia (Republic)",             "gs",  NULL },
	{ "Georgian S.S.R",                 "gsr", "discontinued" },
	{ "Guatemala",                      "gt",  NULL },
	{ "Guam",                           "gu",  NULL },
	{ "Guinea",                         "gv",  NULL },
	{ "Germany",                        "gw",  NULL },
	{ "Guyana",                         "gy",  NULL },
	{ "Gaza Strip",                     "gz",  NULL },
	{ "Hawaii",                         "hiu", NULL },
	{ "Hong Kong",                      "hk",  "discontinued" },
	{ "Heard and McDonald Islands",     "hm",  NULL },
	{ "Honduras",                       "ho",  NULL },
	{ "Haiti",                          "ht",  NULL },
	{ "Hungary",                        "hu",  NULL },
	{ "Iowa",                           "iau", NULL },
	{ "Iceland",                        "ic",  NULL },
	{ "Idaho",                          "idu", NULL },
	{ "Ireland",                        "ie",  NULL },
	{ "India",                          "ii",  NULL },
	{ "Illinois",                       "ilu", NULL },
	{ "Isle of Man",                    "im",  NULL },
	{ "Indiana",                        "inu", NULL },
	{ "Indonesia",                      "io",  NULL },
	{ "Iraq",                           "iq",  NULL },
	{ "Iran",                           "ir",  NULL },
	{ "Israel",                         "is",  NULL },
	{ "Italy",                          "it",  NULL },
	{ "Israel-Syria Demilitarized Zones", "iu", "discontinued" },
	{ "Cote d'Ivoire",                  "iv",  NULL },
	{ "Isreal-Jordan Demilitarized Zones", "iw", "discontinued" },
	{ "Iraq-Saudi Arabia Neutral Zone",  "iy", NULL },
	{ "Japan",                           "ja", NULL },
	{ "Jersey",                          "je", NULL },
	{ "Johnston Atoll",                  "ji", NULL },
	{ "Jamaica",                         "jm", NULL },
	{ "Jan Mayen",                       "jn", "discontinued" },
	{ "Jordan",                          "jo", NULL },
	{ "Kenya",                           "ke", NULL },
	{ "Kyrgyzstan",                      "kg", NULL },
	{ "Kirghiz S.S.R.",                  "kgr", NULL },
	{ "North Korea",                     "kn", NULL },
	{ "South Korea",                     "ko", NULL },
	{ "Kansas",                          "ksu", NULL },
	{ "Kuwait",                          "ku",  NULL },
	{ "Kosovo",                          "kv",  NULL },
	{ "Kentucky",                        "kyu", NULL },
	{ "Kazakhstan",                      "kz",  NULL },
	{ "Kazakh S.S.R.",                   "kzr", "discontinued" },
	{ "Louisiana",                       "lau", NULL },
	{ "Liberia",                         "lb",  NULL },
	{ "Lebanon",                         "le",  NULL },
	{ "Liechtenstein",                   "lh",  NULL },
	{ "Lithuania",                       "li",  NULL },
	{ "Lithuania",                       "lir", "discontinued" },
	{ "Central and Southern Line Islands", "ln", "discontinued" },
	{ "Lesotho",                         "lo",  NULL },
	{ "Laos",                            "ls",  NULL },
	{ "Luxembourg",                      "lu",  NULL },
	{ "Latvia",                          "lv",  NULL },
	{ "Latvia",                          "lvr", "discontinued" },
	{ "Libya",                           "ly",  NULL },
	{ "Massachusetts",                   "mau", NULL },
	{ "Manitoba",                        "mbc", NULL },
	{ "Monaco",                          "mc",  NULL },
	{ "Maryland",                        "mdu", NULL },
	{ "Maine",                           "meu", NULL },
	{ "Mauritius",                       "mf",  NULL },
	{ "Madagascar",                      "mg",  NULL },
	{ "Macao",                           "mh",  "discontinued" },
	{ "Michigan",                        "miu", NULL },
	{ "Montserrat",                      "mj",  NULL },
	{ "Oman",                            "mk",  NULL },
	{ "Mali",                            "ml",  NULL },
	{ "Malta",                           "mm",  NULL },
	{ "Minnesota",                       "mnu", NULL },
	{ "Montenegro",                      "mo",  NULL },
	{ "Missouri",                        "mou", NULL },
	{ "Mongolia",                        "mp",  NULL },
	{ "Martinique",                      "mq",  NULL },
	{ "Morocco",                         "mr",  NULL },
	{ "Mississippi",                     "msu", NULL },
	{ "Montana",                         "mtu", NULL },
	{ "Mauritania",                      "mu",  NULL },
	{ "Moldova",                         "mv",  NULL },
	{ "Moldavian S.S.R.",                "mvr", "discontinued" },
	{ "Malawi",                          "mw",  NULL },
	{ "Mexico",                          "mx",  NULL },
	{ "Malaysia",                        "my",  NULL },
	{ "Mozambique",                      "mz",  NULL },
	{ "Netherlands Antilles",            "na",  "discontinued" },
	{ "Nebraska",                        "nbu", NULL },
	{ "North Carolina",                  "ncu", NULL },
	{ "North Dakota",                    "ndu", NULL },
	{ "Netherlands",                     "ne",  NULL },
	{ "Newfoundland and Labrador",       "nfc", NULL },
	{ "Niger",                           "ng",  NULL },
	{ "New Hampshire",                   "nhu", NULL },
	{ "Northern Ireland",                "nik", NULL },
	{ "New Jersey",                      "nju", NULL },
	{ "New Brunswick",                   "nkc", NULL },
	{ "New Caledonia",                   "nl",  NULL },
	{ "Northern Mariana Islands",        "nm",  "discontinued" },
	{ "New Mexico",                      "nmu", NULL },
	{ "Vanuatu",                         "nn",  NULL },
	{ "Norway",                          "no",  NULL },
	{ "Nepal",                           "np",  NULL },
	{ "Nicaragua",                       "nq",  NULL },
	{ "Nigeria",                         "nr",  NULL },
	{ "Nova Scotia",                     "nsc", NULL },
	{ "Northwest Territories",           "ntc", NULL },
	{ "Nauru",                           "nu",  NULL },
	{ "Nunavut",                         "nuc", NULL },
	{ "Nevada",                          "nvu", NULL },
	{ "Northern Mariana Islands",        "nw",  NULL },
	{ "Norfolk Island",                  "nx",  NULL },
	{ "New York",                        "nyu", NULL },
	{ "New Zealand",                     "nz",  NULL },
	{ "Ohio",                            "ohu", NULL },
	{ "Oklahoma",                        "oku", NULL },
	{ "Ontario",                         "onc", NULL },
	{ "Oregon",                          "oru", NULL },
	{ "Mayotte",                         "ot",  NULL },
	{ "Pennsylvania",                    "pau", NULL },
	{ "Pitcairn Island",                 "pc",  NULL },
	{ "Peru",                            "pe",  NULL },
	{ "Paracel Islands",                 "pf",  NULL },
	{ "Guinea-Bissau",                   "pg",  NULL },
	{ "Philippines",                     "ph",  NULL },
	{ "Prince Edward Island",            "pic", NULL },
	{ "Pakistan",                        "pk",  NULL },
	{ "Poland",                          "pl",  NULL },
	{ "Panama",                          "pn",  NULL },
	{ "Portugal",                        "po",  NULL },
	{ "Papua New Guinea",                "pp",  NULL },
	{ "Puerto Rico",                     "pr",  NULL },
	{ "Portuguese Timor",                "pt",  NULL },
	{ "Palau",                           "pw",  NULL },
	{ "Paraguay",                        "py",  NULL },
	{ "Qatar",                           "qa",  NULL },
	{ "Queensland",                      "qea", NULL },
	{ "Quebec",                          "quc", NULL },
	{ "Serbia",                          "rb",  NULL },
	{ "Reunion",                         "re",  NULL },
	{ "Zimbabwe",                        "rh",  NULL },
	{ "Romania",                         "rm",  NULL },
	{ "Russian Federation",              "ru",  NULL },
	{ "Russian S.F.S.R",                 "rur", "discontinued" },
	{ "Rwanda",                          "rw",  NULL },
	{ "Southern Ryukyu Islands",         "ry",  "discontinued" },
	{ "South Africa",                    "sa",  NULL },
	{ "Svalbard",                        "sb",  "discontinued" },
	{ "Saint_Barthelemy",                "sc",  NULL },
	{ "South Carolina",                  "scu", NULL },
	{ "South Sudan",                     "sd",  NULL },
	{ "Seychelles",                      "se",  NULL },
	{ "Sao Tome and Principe",           "sf",  NULL },
	{ "Senegal",                         "sg",  NULL },
	{ "Spanish North Africa",            "sh",  NULL },
	{ "Singapore",                       "si",  NULL },
	{ "Sudan",                           "sj",  NULL },
	{ "Sikkim",                          "sk",  "discontinued" },
	{ "Sierra Leone",                    "sl",  NULL },
	{ "San Marino",                      "sm",  NULL },
	{ "Sint Maarten",                    "sn",  NULL },
	{ "Saskatchewan",                    "snc", NULL },
	{ "Somalia",                         "so",  NULL },
	{ "Spain",                           "sp",  NULL },
	{ "Eswatini",                        "sq",  NULL },
	{ "Surinam",                         "sr",  NULL },
	{ "Western Sahara",                  "ss",  NULL },
	{ "Saint-Martin",                    "st",  NULL },
	{ "Scotland",                        "stk", NULL },
	{ "Saudi Arabia",                    "su",  NULL },
	{ "Swan Islands",                    "sv",  NULL },
	{ "Sweden",                          "sw",  NULL },
	{ "Namibia",                         "sx",  NULL },
	{ "Syria",                           "sy",  NULL },
	{ "Switzerland",                     "sz",  NULL },
	{ "Tajikistan",                      "ta",  NULL },
	{ "Tajik S.S.R",                     "tar", "discontinued" },
	{ "Turks and Caicos Islands",        "tc",  NULL },
	{ "Togo",                            "tg",  NULL },
	{ "Thailand",                        "th",  NULL },
	{ "Tunisia",                         "ti",  NULL },
	{ "Turkmenistan",                    "tk",  NULL },
	{ "Turkmen S.S.R.",                  "tkr", "discontinued" },
	{ "Tokelau",                         "tl",  NULL },
	{ "Tasmania",                        "tma", NULL },
	{ "Tennessee",                       "tnu", NULL },
	{ "Tonga",                           "to",  NULL },
	{ "Trinidad and Tobago",             "tr",  NULL },
	{ "United Arab Emirates",            "ts",  NULL },
	{ "Trust Territory of the Pacific Islands", "tt", "discontinued" },
	{ "Turkey",                          "tu",  NULL },
	{ "Tuvalu",                          "tv",  NULL },
	{ "Texas",                           "txu", NULL },
	{ "Tanzania",                        "tz",  NULL },
	{ "Egypt",                           "ua",  NULL },
	{ "United States Misc. Caribbean Islands", "uc", NULL },
	{ "Uganda",                          "ug",  NULL },
	{ "United Kingdom Misc. Islands",    "ui",  "discontinued" },
	{ "United Kingdom Misc. Islands",    "uik", "discontinued" },
	{ "United Kingdom",                  "uk",  "discontinued" },
	{ "Ukraine",                         "un",  NULL },
	{ "Ukraine",                         "unr", "discontinued" },
	{ "United States Misc. Pacific Islands", "up", NULL },
	{ "Soviet Union",                    "ur",  "discontinued" },
	{ "United States",                   "us",  "discontinued" },
	{ "Utah",                            "utu", NULL },
	{ "Burkina Faso",                    "uv",  NULL },
	{ "Uruguay",                         "uy",  NULL },
	{ "Uzbekistan",                      "uz",  NULL },
	{ "Uzbek S.S.R.",                    "uzr", "discontinued" },
	{ "Virginia",                        "vau", NULL },
	{ "British Virgin Islands",          "vb",  NULL },
	{ "Vatican City",                    "vc",  NULL },
	{ "Venezuela",                       "ve",  NULL },
	{ "Virgin Islands of the United States", "vi", NULL },
	{ "Vietnam",                         "vm",  NULL },
	{ "North Vietnam",                   "vn",  "discontinued" },
	{ "Various places",                  "vp",  NULL },
	{ "Victoria",                        "vra", NULL },
	{ "South Vietnam",                   "vs",  NULL },
	{ "Vermont",                         "vtu", NULL },
	{ "Washington",                      "wau", NULL },
	{ "West Berlin",                     "wb",  NULL },
	{ "Western Australia",               "wea", NULL },
	{ "Wallis and Futuna",               "wf",  NULL },
	{ "Wisconsin",                       "wiu", NULL },
	{ "West Bank of the Jordan River",   "wj",  NULL },
	{ "Wake Island",                     "wk",  NULL },
	{ "Wales",                           "wlk", NULL },
	{ "Samoa",                           "ws",  NULL },
	{ "West Virginia",                   "wvu", NULL },
	{ "Wyoming",                         "wyu", NULL },
	{ "Christmas Island (Indian Ocean)", "xa",  NULL },
	{ "Cocus (Keeling) Islands",         "xb",  NULL },
	{ "Maldives",                        "xc",  NULL },
	{ "Saint Kitts-Nevis",               "xd",  NULL },
	{ "Marshall Islands",                "xe",  NULL },
	{ "Midway Islands",                  "xf",  NULL },
	{ "Coral Sea Islands Territory",     "xga", NULL },
	{ "Niue",                            "xh",  NULL },
	{ "Saint Kitts-Nevis-Anguilla",      "xi",  "discontinued" },
	{ "Saint Helena",                    "xj",  NULL },
	{ "Saint Lucia",                     "xk",  NULL },
	{ "Saint Pierre and Miquelon",       "xl",  NULL },
	{ "Saint Vincent and the Grenadines","xm",  NULL },
	{ "North Macedonia",                 "xn",  NULL },
	{ "New South Wales",                 "xna", NULL },
	{ "Slovakia",                        "xo",  NULL },
	{ "Northern Territory",              "xoa", NULL },
	{ "Spratly Island",                  "xp",  NULL },
	{ "Czech Republic",                  "xr",  NULL },
	{ "South Australia",                 "xra", NULL },
	{ "South Georgia and the South Sandwich Islands", "xs", NULL },
	{ "Slovenia",                        "xv",  NULL },
	{ "No place, unknown, or undetermined", "xx", NULL },
	{ "Canada",                          "xxc", NULL },
	{ "United Kingdom",                  "xxk", NULL },
	{ "Soviet Union",                    "xxr", "discontinued" },
	{ "United States",                   "xxu", NULL },
	{ "Yemen",                           "ye",  NULL },
	{ "Yukon Territory",                 "ykc", NULL },
	{ "Yemen (People's Democratic Republic)", "ys", "discontinued" },
	{ "Serbia and Montenegro",           "yu",  "discontinued" },
	{ "Zambia",                          "za",  NULL },
};
static const int nmarc_country = sizeof( marc_country ) / sizeof( marc_country[0] );


/* www.loc.gov/marc/relators/relacode.html */
#if 0
typedef struct marc_trans {
	char *internal_name;
	char *abbreviation;
	char *comment;
} marc_trans;
#endif

static const marc_trans marc_relators[] = {
	{ "ABRIDGER",                          "abr"                                 ,NULL},
	{ "ART_COPYIST",                       "acp"                                 ,NULL},
	{ "ACTOR",                             "act"                                 ,NULL},
	{ "ART_DIRECTOR",                      "adi"                                 ,NULL},
	{ "ADAPTER",                           "adp"                                 ,NULL},
	{ "AFTERAUTHOR",                       "aft"                                 ,NULL},
	{ "ANALYST",                           "anl"                                 ,NULL},
	{ "ANIMATOR",                          "anm"                                 ,NULL},
	{ "ANNOTATOR",                         "ann"                                 ,NULL},
	{ "BIBLIOGRAPHIC_ANTECEDENT",          "ant"                                 ,NULL},
	{ "APPELLEE",                          "ape"                                 ,NULL},
	{ "APPELLANT",                         "apl"                                 ,NULL},
	{ "APPLICANT",                         "app"                                 ,NULL},
	{ "AUTHOR",                            "aqt"                                 ,"Author in quotations or text abstracts"},
	{ "ARCHITECT",                         "arc"                                 ,NULL},
	{ "ARTISTIC_DIRECTOR",                 "ard"                                ,NULL },
	{ "ARRANGER",                          "arr"                                 ,NULL},
	{ "ARTIST",                            "art"                                 ,NULL},
	{ "ASSIGNEE",                          "asg"                                 ,NULL},
	{ "ASSOCIATED_NAME",                   "asn"                                 ,NULL},
	{ "AUTOGRAPHER",                       "ato"                                 ,NULL},
	{ "ATTRIBUTED_NAME",                   "att"                                 ,NULL},
	{ "AUCTIONEER",                        "auc"                                 ,NULL},
	{ "AUTHOR",                            "aud"                                 ,"Author of dialog"},
	{ "INTROAUTHOR",                       "aui"                                 ,"Author of introduction, etc."},
	{ "AUTHOR",                            "aus"                                 ,"Screenwriter"},
	{ "AUTHOR",                            "aut"                                 ,NULL},
	{ "AUTHOR",                            "author"                              ,NULL},
	{ "AFTERAUTHOR",                       "author of afterword, colophon, etc." ,NULL},
	{ "INTROAUTHOR",                       "author of introduction, etc."        ,NULL},
	{ "BINDING_DESIGNER",                  "bdd"                                 ,NULL},
	{ "BOOKJACKET_DESIGNER",               "bjd"                                 ,NULL},
	{ "BOOK_DESIGNER",                     "bkd"                                 ,NULL},
	{ "BOOK_PRODUCER",                     "bkp"                                 ,NULL},
	{ "AUTHOR",                            "blw"                                 ,"Blurb writer"},
	{ "BINDER",                            "bnd"                                 ,NULL},
	{ "BOOKPLATE_DESIGNER",                "bpd"                                 ,NULL},
	{ "BROADCASTER",                       "brd"                                 ,NULL},
	{ "BRAILLE_EMBOSSER",                  "brl"                                 ,NULL},
	{ "BOOKSELLER",                        "bsl"                                 ,NULL},
	{ "CASTER",                            "cas"                                 ,NULL},
	{ "CONCEPTOR",                         "ccp"                                 ,NULL},
	{ "CHOREOGRAPHER",                     "chr"                                 ,NULL},
	{ "COLLABORATOR",                      "clb"                                 ,NULL},
	{ "CLIENT",                            "cli"                                 ,NULL},
	{ "CALLIGRAPHER",                      "cll"                                 ,NULL},
	{ "COLORIST",                          "clr"                                 ,NULL},
	{ "COLLOTYPER",                        "clt"                                 ,NULL},
	{ "COMMENTATOR",                       "cmm"                                 ,NULL},
	{ "COMPOSER",                          "cmp"                                 ,NULL},
	{ "COMPOSITOR",                        "cmt"                                 ,NULL},
	{ "CONDUCTOR",                         "cnd"                                 ,NULL},
	{ "CINEMATOGRAPHER",                   "cng"                                 ,NULL},
	{ "CENSOR",                            "cns"                                 ,NULL},
	{ "CONTESTANT-APPELLEE",               "coe"                                 ,NULL},
	{ "COLLECTOR",                         "col"                                 ,NULL},
	{ "COMPILER",                          "com"                                 ,NULL},
	{ "CONSERVATOR",                       "con"                                 ,NULL},
	{ "COLLECTION_REGISTRAR",              "cor"                                 ,NULL},
	{ "CONTESTANT",                        "cos"                                 ,NULL},
	{ "CONTESTANT-APPELLANT",              "cot"                                 ,NULL},
	{ "COURT_GOVERNED",                    "cou"                                 ,NULL},
	{ "COVER_DESIGNER",                    "cov"                                 ,NULL},
	{ "COPYRIGHT_CLAIMANT",                "cpc"                                 ,NULL},
	{ "COMPLAINANT-APPELLEE",              "cpe"                                 ,NULL},
	{ "COPYRIGHT_HOLDER",                  "cph"                                 ,NULL},
	{ "COMPLAINANT",                       "cpl"                                 ,NULL},
	{ "COMPLAINANT-APPELLANT",             "cpt"                                 ,NULL},
	{ "AUTHOR",                            "cre"                                 ,"Creator"},/* Creator */
	{ "AUTHOR",                            "creator"                             ,"Creator"},/* Creator */
	{ "CORRESPONDENT",                     "crp"                                 ,NULL},
	{ "CORRECTOR",                         "crr"                                 ,NULL},
	{ "COURT_REPORTER",                    "crt"                                 ,NULL},
	{ "CONSULTANT",                        "csl"                                 ,NULL},
	{ "CONSULTANT",                        "csp"                                 ,"Consultant to a project"},
	{ "COSTUME_DESIGNER",                  "cst"                                 ,NULL},
	{ "CONTRIBUTOR",                       "ctb"                                 ,NULL},
	{ "CONTESTEE-APPELLEE",                "cte"                                 ,NULL},
	{ "CARTOGRAPHER",                      "ctg"                                 ,NULL},
	{ "CONTRACTOR",                        "ctr"                                 ,NULL},
	{ "CONTESTEE",                         "cts"                                 ,NULL},
	{ "CONTESTEE-APPELLANT",               "ctt"                                 ,NULL},
	{ "CURATOR",                           "cur"                                 ,NULL},
	{ "COMMENTATOR",                       "cwt"                                 ,"Commentator for written text"},
	{ "DISTRIBUTION_PLACE",                "dbp"                                 ,NULL},
	{ "DEGREEGRANTOR",                     "degree grantor"                      ,"Degree granting institution"},
	{ "DEFENDANT",                         "dfd"                                 ,NULL},
	{ "DEFENDANT-APPELLEE",                "dfe"                                 ,NULL},
	{ "DEFENDANT-APPELLANT",               "dft"                                 ,NULL},
	{ "DEGREEGRANTOR",                     "dgg"                                 ,"Degree granting institution"},
	{ "DEGREE_SUPERVISOR",                 "dgs"                                 ,NULL},
	{ "DISSERTANT",                        "dis"                                 ,NULL},
	{ "DELINEATOR",                        "dln"                                 ,NULL},
	{ "DANCER",                            "dnc"                                 ,NULL},
	{ "DONOR",                             "dnr"                                 ,NULL},
	{ "DEPICTED",                          "dpc"                                 ,NULL},
	{ "DEPOSITOR",                         "dpt"                                 ,NULL},
	{ "DRAFTSMAN",                         "drm"                                 ,NULL},
	{ "DIRECTOR",                          "drt"                                 ,NULL},
	{ "DESIGNER",                          "dsr"                                 ,NULL},
	{ "DISTRIBUTOR",                       "dst"                                 ,NULL},
	{ "DATA_CONTRIBUTOR",                  "dtc"                                 ,NULL},
	{ "DEDICATEE",                         "dte"                                 ,NULL},
	{ "DATA_MANAGER",                      "dtm"                                 ,NULL},
	{ "DEDICATOR",                         "dto"                                 ,NULL},
	{ "AUTHOR",                            "dub"                                 ,"Dubious author"},
	{ "EDITOR",                            "edc"                                 ,"Editor of compilation"},
	{ "EDITOR",                            "edm"                                 ,"Editor of moving image work"},
	{ "EDITOR",                            "edt"                                 ,NULL},
	{ "ENGRAVER",                          "egr"                                 ,NULL},
	{ "ELECTRICIAN",                       "elg"                                 ,NULL},
	{ "ELECTROTYPER",                      "elt"                                 ,NULL},
	{ "ENGINEER",                          "eng"                                 ,NULL},
	{ "ENACTING_JURISDICTION",             "enj"                                 ,NULL},
	{ "ETCHER",                            "etr"                                 ,NULL},
	{ "EVENT_PLACE",                       "evp"                                 ,NULL},
	{ "EXPERT",                            "exp"                                 ,NULL},
	{ "FACSIMILIST",                       "fac"                                 ,NULL},
	{ "FILM_DISTRIBUTOR",                  "fds"                                 ,NULL},
	{ "FIELD_DIRECTOR",                    "fld"                                 ,NULL},
	{ "EDITOR",                            "flm"                                 ,"Film editor"},
	{ "DIRECTOR",                          "fmd"                                 ,"Film director"},
	{ "FILMMAKER",                         "fmk"                                 ,NULL},
	{ "FORMER_OWNER",                      "fmo"                                 ,NULL},
	{ "PRODUCER",                          "fmp"                                 ,"Film producer"},
	{ "FUNDER",                            "fnd"                                 ,NULL},
	{ "FIRST_PARTY",                       "fpy"                                 ,NULL},
	{ "FORGER",                            "frg"                                 ,NULL},
	{ "GEOGRAPHIC_INFORMATION_SPECIALIST", "gis"                                 ,NULL},
	{ "GRAPHIC_TECHNICIAN",                "grt"                                 ,NULL},
	{ "HOST_INSTITUTION",                  "his"                                 ,NULL},
	{ "HONOREE",                           "hnr"                                 ,NULL},
	{ "HOST",                              "hst"                                 ,NULL},
	{ "ILLUSTRATOR",                       "ill"                                 ,NULL},
	{ "ILLUMINATOR",                       "ilu"                                 ,NULL},
	{ "INSCRIBER",                         "ins"                                 ,NULL},
	{ "INVENTOR",                          "inv"                                 ,NULL},
	{ "ISSUING_BODY",                      "isb"                                 ,NULL},
	{ "MUSICIAN",                          "itr"                                 ,"Instrumentalist"},
	{ "INTERVIEWEE",                       "ive"                                 ,NULL},
	{ "INTERVIEWER",                       "ivr"                                 ,NULL},
	{ "JUDGE",                             "jud"                                 ,NULL},
	{ "JURISDICTION_GOVERNED",             "jug"                                 ,NULL},
	{ "LABORATORY",                        "lbr"                                 ,NULL},
	{ "AUTHOR",                            "lbt"                                 ,"Librettist"},
	{ "LABORATORY_DIRECTOR",               "ldr"                                 ,NULL},
	{ "LEAD",                              "led"                                 ,NULL},
	{ "LIBELEE-APPELLEE",                  "lee"                                 ,NULL},
	{ "LIBELEE",                           "lel"                                 ,NULL},
	{ "LENDER",                            "len"                                 ,NULL},
	{ "LIBELEE-APPELLANT",                 "let"                                 ,NULL},
	{ "LIGHTING_DESIGNER",                 "lgd"                                 ,NULL},
	{ "LIBELANT-APPELLEE",                 "lie"                                 ,NULL},
	{ "LIBELANT",                          "lil"                                 ,NULL},
	{ "LIBELANT-APPELLANT",                "lit"                                 ,NULL},
	{ "LANDSCAPE_ARCHITECT",               "lsa"                                 ,NULL},
	{ "LICENSEE",                          "lse"                                 ,NULL},
	{ "LICENSOR",                          "lso"                                 ,NULL},
	{ "LITHOGRAPHER",                      "ltg"                                 ,NULL},
	{ "AUTHOR",                            "lyr"                                 ,"Lyricist"},
	{ "MUSIC_COPYIST",                     "mcp"                                 ,NULL},
	{ "METADATA_CONTACT",                  "mdc"                                 ,NULL},
	{ "MEDIUM",                            "med"                                 ,NULL},
	{ "MANUFACTURE_PLACE",                 "mfp"                                 ,NULL},
	{ "MANUFACTURER",                      "mfr"                                 ,NULL},
	{ "MODERATOR",                         "mod"                                 ,NULL},
	{ "THESIS_EXAMINER",                   "mon"                                 ,"Monitor"},
	{ "MARBLER",                           "mrb"                                 ,NULL},
	{ "EDITOR",                            "mrk"                                 ,"Markup editor"},
	{ "MUSICAL_DIRECTOR",                  "msd"                                 ,NULL},
	{ "METAL-ENGRAVER",                    "mte"                                 ,NULL},
	{ "MINUTE_TAKER",                      "mtk"                                 ,NULL},
	{ "MUSICIAN",                          "mus"                                 ,NULL},
	{ "NARRATOR",                          "nrt"                                 ,NULL},
	{ "THESIS_OPPONENT",                   "opn"                                 ,"Opponent"},
	{ "ORIGINATOR",                        "org"                                 ,NULL},
	{ "ORGANIZER",                         "organizer of meeting"                ,NULL},
	{ "ORGANIZER",                         "orm" 	                             ,NULL},
	{ "ONSCREEN_PRESENTER",                "osp" 	                             ,NULL},
	{ "THESIS_OTHER",                      "oth" 	                             ,"Other"},
	{ "OWNER",                             "own" 	                             ,NULL},
	{ "PANELIST",                          "pan" 	                             ,NULL},
	{ "PATRON",                            "pat" 	                             ,NULL},
	{ "ASSIGNEE",                          "patent holder"                       ,NULL},
	{ "PUBLISHING_DIRECTOR",               "pbd" 	                             ,NULL},
	{ "PUBLISHER",                         "pbl"                                 ,NULL},
	{ "PROJECT_DIRECTOR",                  "pdr"                                 ,NULL},
	{ "PROOFREADER",                       "pfr"                                 ,NULL},
	{ "PHOTOGRAPHER",                      "pht"                                 ,NULL},
	{ "PLATEMAKER",                        "plt"                                 ,NULL},
	{ "PERMITTING_AGENCY",                 "pma"                                 ,NULL},
	{ "PRODUCTION_MANAGER",                "pmn"                                 ,NULL},
	{ "PRINTER_OF_PLATES",                 "pop"                                 ,NULL},
	{ "PAPERMAKER",                        "ppm"                                 ,NULL},
	{ "PUPPETEER",                         "ppt"                                 ,NULL},
	{ "PRAESES",                           "pra"                                 ,NULL},
	{ "PROCESS_CONTRACT",                  "prc"                                 ,NULL},
	{ "PRODUCTION_PERSONNEL",              "prd"                                 ,NULL},
	{ "PRESENTER",                         "pre"                                 ,NULL},
	{ "PERFORMER",                         "prf"                                 ,NULL},
	{ "AUTHOR",                            "prg"                                 ,"Programmer"},
	{ "PRINTMAKER",                        "prm"                                 ,NULL},
	{ "PRODUCTION_COMPANY",                "prn"                                 ,NULL},
	{ "PRODUCER",                          "pro"                                 ,NULL},
	{ "PRODUCTION_PLACE",                  "prp"                                 ,NULL},
	{ "PRODUCTION_DESIGNER",               "prs"                                 ,NULL},
	{ "PRINTER",                           "prt"                                 ,NULL},
	{ "PROVIDER",                          "prv"                                 ,NULL},
	{ "PATENT_APPLICANT",                  "pta"                                 ,NULL},
	{ "PLAINTIFF-APPELLEE",                "pte"                                 ,NULL},
	{ "PLAINTIFF",                         "ptf"                                 ,NULL},
	{ "ASSIGNEE",                          "pth"                                 ,"Patent holder"},
	{ "PLAINTIFF-APPELLANT",               "ptt"                                 ,NULL},
	{ "PUBLICATION_PLACE",                 "pup"                                 ,NULL},
	{ "RUBRICATOR",                        "rbr"                                 ,NULL},
	{ "RECORDIST",                         "rcd"                                 ,NULL},
	{ "RECORDING_ENGINEER",                "rce"                                 ,NULL},
	{ "ADDRESSEE",                         "rcp"                                 ,"Recipient"},
	{ "RADIO_DIRECTOR",                    "rdd"                                 ,NULL},
	{ "REDAKTOR",                          "red"                                 ,NULL},
	{ "RENDERER",                          "ren"                                 ,NULL},
	{ "RESEARCHER",                        "res"                                 ,NULL},
	{ "REVIEWER",                          "rev"                                 ,NULL},
	{ "RADIO_PRODUCER",                    "rpc"                                 ,NULL},
	{ "REPOSITORY",                        "rps"                                 ,NULL},
	{ "REPORTER",                          "rpt"                                 ,NULL},
	{ "RESPONSIBLE_PARTY",                 "rpy"                                 ,NULL},
	{ "RESPONDENT-APPELLEE",               "rse"                                 ,NULL},
	{ "RESTAGER",                          "rsg"                                 ,NULL},
	{ "RESPONDENT",                        "rsp"                                 ,NULL},
	{ "RESTORATIONIST",                    "rsr"                                 ,NULL},
	{ "RESPONDENT-APPELLANT",              "rst"                                 ,NULL},
	{ "RESEARCH_TEAM_HEAD",                "rth"                                 ,NULL},
	{ "RESEARCH_TEAM_MEMBER",              "rtm"                                 ,NULL},
	{ "SCIENTIFIC_ADVISOR",                "sad"                                 ,NULL},
	{ "SCENARIST",                         "sce"                                 ,NULL},
	{ "SCULPTOR",                          "scl"                                 ,NULL},
	{ "SCRIBE",                            "scr"                                 ,NULL},
	{ "SOUND_DESIGNER",                    "sds"                                 ,NULL},
	{ "SECRETARY",                         "sec"                                 ,NULL},
	{ "STAGE_DIRECTOR",                    "sgd"                                 ,NULL},
	{ "SIGNER",                            "sgn"                                 ,NULL},
	{ "SUPPORTING_HOST",                   "sht"                                 ,NULL},
	{ "SELLER",                            "sll"                                 ,NULL},
	{ "SINGER",                            "sng"                                 ,NULL},
	{ "SPEAKER",                           "spk"                                 ,NULL},
	{ "SPONSOR",                           "spn"                                 ,NULL},
	{ "SECOND_PARTY",                      "spy"                                 ,NULL},
	{ "SURVEYOR",                          "srv"                                 ,NULL},
	{ "SET_DESIGNER",                      "std"                                 ,NULL},
	{ "SETTING",                           "stg"                                 ,NULL},
	{ "STORYTELLER",                       "stl"                                 ,NULL},
	{ "STAGE_MANAGER",                     "stm"                                 ,NULL},
	{ "STANDARDS_BODY",                    "stn"                                 ,NULL},
	{ "STEREOTYPER",                       "str"                                 ,NULL},
	{ "TECHNICAL_DIRECTOR",                "tcd"                                 ,NULL},
	{ "TEACHER",                           "tch"                                 ,NULL},
	{ "THESIS_ADVISOR",                    "ths"                                 ,NULL},
	{ "TELEVISION_DIRECTOR",               "tld"                                 ,NULL},
	{ "TELEVISION_PRODUCER",               "tlp"                                 ,NULL},
	{ "TRANSCRIBER",                       "trc"                                 ,NULL},
	{ "TRANSLATOR",                        "translator"                          ,NULL},
	{ "TRANSLATOR",                        "trl"                                 ,NULL},
	{ "TYPE_DIRECTOR",                     "tyd"                                 ,NULL},
	{ "TYPOGRAPHER",                       "tyg"                                 ,NULL},
	{ "UNIVERSITY_PLACE",                  "uvp"                                 ,NULL},
	{ "VOICE_ACTOR",                       "vac"                                 ,NULL},
	{ "VIDEOGRAPHER",                      "vdg"                                 ,NULL},
	{ "VOCALIST",                          "voc"                                 ,NULL},
	{ "AUTHOR",                            "wac"                                 ,"Writer of added commentary"},
	{ "AUTHOR",                            "wal"                                 ,"Writer of added lyrics"},
	{ "AUTHOR",                            "wam"                                 ,"Writer of accompanying material"},
	{ "AUTHOR",                            "wat"                                 ,"Writer of added text"},
	{ "WOODCUTTER",                        "wdc"                                 ,NULL},
	{ "WOOD_ENGRAVER",                     "wde"                                 ,NULL},
	{ "INTROAUTHOR",                       "win"                                 ,"Writer of introduction"},
	{ "WITNESS",                           "wit"                                 ,NULL},
	{ "INTROAUTHOR",                       "wpr"                                 ,"Writer of preface"},
	{ "AUTHOR",                            "wst"                                 ,"Writer of supplementary textual content"}
};

static const int nmarc_relators = sizeof( marc_relators ) / sizeof( marc_relators[0] );

void
memerr( const char *fn )
{
	fprintf( stderr, "Memory error in %s()\n", fn );
	exit( EXIT_FAILURE );
}

int
hashify_marc_test_size( const char *list[], int nlist, unsigned int HASH_SIZE, uintlist *u )
{
	unsigned int n;
	int i, pos;

	uintlist_empty( u );

	for ( i=0; i<nlist; ++i ) {

		n = calculate_hash_char( list[i], HASH_SIZE );

		pos = uintlist_find( u, n );

		/* position taken, try next bucket */
		if ( pos!=-1 ) {
			n++;
			pos = uintlist_find( u, n );
		}

		/* hash is too full, exit */
		if ( pos!=-1 ) return 0;

		uintlist_add( u, n );

	}

	return 1;
}

void
hashify_marc_write( FILE *fp, const char *list[], int nlist, const char *type, unsigned int HASH_SIZE, uintlist *u )
{
	unsigned int n;
	int i;

#if 0
	/* write starting list to document hash info */
	fprintf( fp, "#if 0\n\n" );
	fprintf( fp, "/* starting MARC %s list */\n\n", type );
	fprintf( fp, "static const char *marc_%s[] = {\n", type );
	for ( i=0; i<nlist; ++i ) {
		fprintf( fp, "\t\"%s\",\n", list[i] );
	}
	fprintf( fp, "};\n" );
	fprintf( fp, "static const int nmarc_%s = sizeof( marc_%s ) / sizeof( const char* );\n", type, type );
	fprintf( fp, "\n#endif\n\n" );
#endif

	/* write hash */
	fprintf( fp, "/*\n" );
	fprintf( fp, " * MARC %s hash\n", type );
	fprintf( fp, " */\n" );
	fprintf( fp, "static const unsigned int marc_%s_hash_size = %u;\n", type, HASH_SIZE );
	fprintf( fp, "static const char *marc_%s[%u] = {\n", type, HASH_SIZE );
	fprintf( fp, "\t[ 0 ... %u ] = NULL,\n", HASH_SIZE-1 );
	for ( i=0; i<nlist; ++i ) {
		n = uintlist_get( u, i );
		fprintf( fp, "\t[ %3u ] = \"%s\",\n", n, list[i] );
	}
	fprintf( fp, "};\n\n" );

	fprintf( fp, "int\n" );
	fprintf( fp, "is_marc_%s( const char *query )\n", type );
	fprintf( fp, "{\n" );
	fprintf( fp, "\tunsigned int n;\n\n" );
	fprintf( fp, "\tn = calculate_hash_char( query, marc_%s_hash_size );\n", type );
	fprintf( fp, "\tif ( marc_%s[n]==NULL ) return 0;\n", type );
	fprintf( fp, "\tif ( !strcmp( query, marc_%s[n] ) ) return 1;\n", type );
	fprintf( fp, "\telse if ( marc_%s[n+1] && !strcmp( query, marc_%s[n+1] ) ) return 1;\n", type, type );
	fprintf( fp, "\telse return 0;\n" );
	fprintf( fp, "}\n" );
}

unsigned int
hashify_size( const char *list[], int nlist, unsigned int max, uintlist *u )
{
	unsigned int HASH_SIZE = (unsigned int ) nlist;
	int ok;

	do {
		ok = hashify_marc_test_size( list, nlist, HASH_SIZE, u );
		if ( ok ) break;
		HASH_SIZE += 1;
		if ( HASH_SIZE > max ) break;
	} while ( 1 );

	if ( !ok ) return 0;
	else return HASH_SIZE;
}

void
hashify_marc( const char *list[], int nlist, const char *type )
{
	unsigned int HASH_SIZE;
	uintlist u;

	uintlist_init( &u );

	HASH_SIZE = hashify_size( list, nlist, 10000, &u );
	if ( HASH_SIZE > 0 ) {
		hashify_marc_write( stdout, list, nlist, type, HASH_SIZE, &u );
	}
	else {
		printf( "/* No valid HASH_SIZE for marc_%s up to %u */\n", type, 10000 );
	}

	uintlist_free( &u );
}

void
hashify_marc_trans_write( FILE *fp, unsigned int HASH_SIZE, const marc_trans *trans, int ntrans, const char *label, const char *comment, uintlist *u )
{
	unsigned int n;
	int i, j, len;

	fprintf( fp, "/*\n" );
	fprintf( fp, " * MARC %s hash\n", label );
	fprintf( fp, " */\n\n" );

	if ( comment ) fprintf( fp, "/* %s */\n\n", comment );
#if 0
	fprintf( fp, "typedef struct marc_trans {\n" );
	fprintf( fp, "\tchar *internal_name;\n" );
	fprintf( fp, "\tchar *abbreviation;\n" );
	fprintf( fp, "} marc_trans;\n\n" );
#endif

	fprintf( fp, "static const unsigned int marc_%s_hash_size = %u;\n", label, HASH_SIZE );
	fprintf( fp, "static const marc_trans marc_%s[%u] = {\n", label, HASH_SIZE );
	fprintf( fp, "\t[ 0 ... %u ] = { NULL, NULL },\n", HASH_SIZE-1 );

	for ( i=0; i<ntrans; ++i ) {
		n = uintlist_get( u, i );
		fprintf( fp, "\t[ %4u ] = { \"%s\", ", n, trans[i].internal_name );
		len = strlen( trans[i].internal_name );
		for ( j=len; j<35; ++j ) fprintf( fp, " " );
		fprintf( fp, "\"%s\"", trans[i].abbreviation );
		len = strlen( trans[i].abbreviation );
		for ( j=len; j<36; ++j ) fprintf( fp, " " );
		fprintf( fp, "}," );
		if ( trans[i].comment ) fprintf( fp, "/* %s */", trans[i].comment );
		fprintf( fp, "\n" );
	}

	fprintf( fp, "};\n\n" );

	fprintf( fp, "char *\n" );
	fprintf( fp, "marc_convert_%s( const char *query )\n", label );
	fprintf( fp, "{\n" );
	fprintf( fp, "\tunsigned int n;\n\n" );

	fprintf( fp, "\tn = calculate_hash_char( query, marc_%s_hash_size );\n", label );
	fprintf( fp, "\tif ( marc_%s[n].abbreviation==NULL ) return NULL;\n", label );
	fprintf( fp, "\tif ( !strcmp( query, marc_%s[n].abbreviation ) ) return marc_%s[n].internal_name;\n", label, label );
	fprintf( fp, "\telse if ( marc_%s[n+1].abbreviation && !strcmp( query, marc_%s[n+1].abbreviation ) ) return marc_%s[n+1].internal_name;\n", label, label, label );
	fprintf( fp, "\telse return NULL;\n" );
	fprintf( fp, "}\n" );
}

void
hashify_marc_trans( const marc_trans *trans, int ntrans, const char *label, const char *comment )
{
	unsigned int HASH_SIZE;
	const char **list;
	uintlist u;
	int i;

	list = (const char **) malloc( sizeof( const char * ) * ntrans );
	if ( !list ) memerr( __FUNCTION__ );

	uintlist_init( &u );

	for ( i=0; i<ntrans; ++i )
		list[i] = (const char*) trans[i].abbreviation;

	HASH_SIZE = hashify_size( list, ntrans, 10000, &u );
	if ( HASH_SIZE > 0 ) {
		hashify_marc_trans_write( stdout, HASH_SIZE, trans, ntrans, label, comment, &u );
	}
	else {
		printf( "/* No valid HASH_SIZE for marc_%s up to %u */\n", label, 10000 );
	}

	uintlist_free( &u );

	free( list );
}

void
write_header( FILE *fp )
{
	fprintf( fp, "/*\n" );
	fprintf( fp, " * marc_auth.c - Identify genre and resources to be labeled with MARC authority/\n" );
	fprintf( fp, " *\n" );
	fprintf( fp, " * MARC (MAchine-Readable Cataloging) 21 authority codes/values from the Library of Congress initiative\n" );
 	fprintf( fp, " *\n" );
	fprintf( fp, " * Copyright (c) Chris Putnam 2004-2020\n" );
	fprintf( fp, " *\n" );
 	fprintf( fp, " * Source code released under the GPL version 2\n" );
	fprintf( fp, " *\n" );
	fprintf( fp, " * Because the string values belonging to the MARC authority\n" );
	fprintf( fp, " * are constant, search for them in a pre-calculated hash to\n" );
	fprintf( fp, " * reduce O(N) run-time linear searching of the list to O(1).\n" );
	fprintf( fp, " * Note that hash size was set to ensure no collisions among\n" );
	fprintf( fp, " * valid terms.\n" );
	fprintf( fp, " */\n" );
	fprintf( fp, "\n" );

	fprintf( fp, "#include <stdlib.h>\n" );
	fprintf( fp, "#include <string.h>\n" );
	fprintf( fp, "#include \"hash.h\"\n" );
	fprintf( fp, "#include \"marc_auth.h\"\n" );
	fprintf( fp, "\n" );

	fprintf( fp, "typedef struct marc_trans {\n" );
	fprintf( fp, "\tchar *internal_name;\n" );
	fprintf( fp, "\tchar *abbreviation;\n" );
	fprintf( fp, "} marc_trans;\n\n" );

#if 0
	fprintf( fp, "/*\n" );
	fprintf( fp, " * Bob Jenkin's one-at-a-time hash\n" );
	fprintf( fp, " */\n" );
	fprintf( fp, "\n" );

	fprintf( fp, "static unsigned int\n" );
	fprintf( fp, "one_at_a_time_hash( const char *key, const unsigned int len, unsigned int HASH_SIZE )\n" );
	fprintf( fp, "{\n" );
	fprintf( fp, "\tunsigned int hash = 0, i;\n" );
	fprintf( fp, "\tfor ( i=0; i<len; ++i ) {\n" );
	fprintf( fp, "\t\thash += key[i];\n" );
	fprintf( fp, "\t\thash += ( hash << 10 );\n" );
	fprintf( fp, "\t\thash ^= ( hash >> 6 );\n" );
	fprintf( fp, "\t}\n" );
	fprintf( fp, "\thash += ( hash << 3 );\n" );
	fprintf( fp, "\thash ^= ( hash >> 11 );\n" );
	fprintf( fp, "\thash += ( hash << 15 );\n" );
	fprintf( fp, "\treturn hash % ( HASH_SIZE - 1 );\n" );
	fprintf( fp, "}\n\n" );

	fprintf( fp, "static unsigned int\n" );
	fprintf( fp, "calculate_hash_char( const char *key, unsigned int HASH_SIZE )\n" );
	fprintf( fp, "{\n" );
	fprintf( fp, "\treturn one_at_a_time_hash( key, strlen( key ), HASH_SIZE );\n" );
	fprintf( fp, "}\n\n" );
#endif
}

int
main( int argc, char *argv[] )
{
	write_header( stdout );
	fflush( NULL );
	hashify_marc( marc_genre, nmarc_genre, "genre" );
	fflush( NULL );
	hashify_marc( marc_resource, nmarc_resource, "resource" );
	fflush( NULL );
	hashify_marc_trans( marc_relators, nmarc_relators, "relators", "www.loc.gov/marc/relators/relacode.html" );
	fflush( NULL );
	hashify_marc_trans( marc_country, nmarc_country, "country", "www.loc.gov/marc/countries/countries_code.html" );
	fflush( NULL );
}
