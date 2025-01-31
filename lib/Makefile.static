CFLAGS  = $(CFLAGSIN)

SIMPLE_OBJS   = charsets.o \
                is_ws.o \
                strsearch.o

NEWSTR_OBJS   = entities.o \
                gb18030.o \
                latex.o \
		latex_parse.o \
                str.o \
                str_conv.o \
                unicode.o \
                utf8.o

CONTAIN_OBJS  = fields.o \
                intlist.o \
                slist.o \
                vplist.o \
                xml.o \
                xml_encoding.o

BIBL_OBJS     = bibl.o \
                bu_auth.o \
		hash.o \
                iso639_1.o \
                iso639_2.o \
                iso639_3.o \
                marc_auth.o \
		month.o \
                name.o \
                notes.o \
                pages.o \
                reftypes.o \
                serialno.o \
                title.o \
		type.o \
                url.o

INPUT_OBJS    = append_easy.o \
		bibtexin.o \
                bibtextypes.o \
		biblatexin.o \
                blxtypes.o \
		copacin.o \
                copactypes.o \
                ebiin.o \
		endin.o \
                endtypes.o \
		endxmlin.o \
		generic.o \
		isiin.o \
                isitypes.o \
                marc_auth.o \
		medin.o \
		modsin.o \
                modstypes.o \
		nbibin.o \
		nbibtypes.o \
		risin.o \
                ristypes.o \
		wordin.o

OUTPUT_OBJS   = adsout.o \
		biblatexout.o \
                bibtexout.o \
                endout.o \
                isiout.o \
                modsout.o \
		nbibout.o \
                risout.o \
                wordout.o

BIBCORE_OBJS  = $(SIMPLE_OBJS) \
                $(NEWSTR_OBJS) \
                $(CONTAIN_OBJS) \
                $(BIBL_OBJS) \
                bibcore.o

BIBUTILS_OBJS = $(INPUT_OBJS) \
                $(OUTPUT_OBJS) \
                bibutils.o

all: libbibcore.a libbibutils.a

libbibcore.a: $(BIBCORE_OBJS)
	ar -cr $@ $^
	$(RANLIB) $@

libbibutils.a: $(BIBUTILS_OBJS)
	ar -cr $@ $^
	$(RANLIB) $@

clean: 
	rm -f *.o *.plist core

realclean:
	rm -f *.o *.a *.plist core

install:

test:

clang:
	clang --analyze charsets.c
	clang --analyze is_ws.c
	clang --analyze strsearch.c
	clang --analyze entities.c
	clang --analyze gb18030.c
	clang --analyze latex.c
	clang --analyze latex_parse.c
	clang --analyze str.c
	clang --analyze str_conv.c
	clang --analyze unicode.c
	clang --analyze utf8.c
	clang --analyze fields.c
	clang --analyze intlist.c
	clang --analyze slist.c
	clang --analyze vplist.c
	clang --analyze xml.c
	clang --analyze xml_encoding.c
	clang --analyze bibl.c
	clang --analyze bu_auth.c
	clang --analyze iso639_1.c
	clang --analyze iso639_2.c
	clang --analyze iso639_3.c
	clang --analyze marc_auth.c
	clang --analyze name.c
	clang --analyze notes.c
	clang --analyze pages.c
	clang --analyze reftypes.c
	clang --analyze serialno.c
	clang --analyze title.c
	clang --analyze type.c
	clang --analyze url.c
	clang --analyze bibtexin.c
	clang --analyze bibtextypes.c
	clang --analyze biblatexin.c
	clang --analyze blxtypes.c
	clang --analyze copacin.c
	clang --analyze copactypes.c
	clang --analyze ebiin.c
	clang --analyze endin.c
	clang --analyze endtypes.c
	clang --analyze endxmlin.c
	clang --analyze generic.c
	clang --analyze isiin.c
	clang --analyze isitypes.c
	clang --analyze marc_auth.c
	clang --analyze medin.c
	clang --analyze modsin.c
	clang --analyze modstypes.c
	clang --analyze nbibin.c
	clang --analyze nbibtypes.c
	clang --analyze risin.c
	clang --analyze ristypes.c
	clang --analyze wordin.c
	clang --analyze adsout.c
	clang --analyze biblatexout.c
	clang --analyze bibtexout.c
	clang --analyze endout.c
	clang --analyze isiout.c
	clang --analyze modsout.c
	clang --analyze nbibout.c
	clang --analyze risout.c
	clang --analyze wordout.c
	clang --analyze bibcore.c
	clang --analyze bibutils.c
