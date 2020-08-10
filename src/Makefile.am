bin_PROGRAMS = aplvis

aplvis_SOURCES = aplvis.c aplvis.h \
                 curves.c curves.h

#BUILT_SOURCES = xml-kwds.h

DISTCLEANFILES = .deps/*.P

APL_LIB = -L `apl --show_lib_dir` -lapl
#APL_LIB = -L /usr/local/lib/apl -lapl


aplvis_CFLAGS = -std=c99 -Wall -Werror \
              $(GTK_CFLAGS) $(CAIRO_CFLAGS) $(MAGICKWAND_CFLAGS)  \
              $(FONTCONFIG_CFLAGS) $(PLPLOT_CFLAGS)

aplvis_LDFLAGS = -lm $(GTK_LIBS) $(CAIRO_LIBS) $(MAGICKWAND_LIBS) \
              $(FONTCONFIG_LIBS)  $(PLPLOT_LIBS) $(APL_LIB) -lstdc++

#xml-kwds.h : xml-kwds.m4
#	m4 $< >$@


# -leak-check=full
#  algrind --tool=memcheck --leak-check=full --leak-resolution=high -
#                -num-callers=20 --log-file=vgdump ./gfig
#  
#VALGRIND_FLAGS = --gen-suppressions=all --run-libc-freeres=yes \
#    --suppressions=valgrind.supp
#VALGRIND_FLAGS = --gen-suppressions=all --run-libc-freeres=yes

valgrind.log : aplvis
	valgrind $(VALGRIND_FLAGS) --log-file=$@ ./vis

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -rf vis-parse.c vis-parse.h vis-lex.c
