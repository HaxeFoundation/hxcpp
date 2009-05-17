NEKO_SOURCE_DIR = e:/Hugh/gpl/neko-1.8.0
PCRE_DIR = ../../thirdparty/pcre-7.8

HX_INCLUDE = -I$(PCRE_DIR)
NDLL_NAME = regexp
DEFINES = -DPCRE_STATIC
OBJ_FILES = obj/RegExp.obj

PCRE_OBJS =  \
   obj/pcre_get.obj         obj/pcre_study.obj        \
   obj/pcre_chartables.obj  obj/pcre_globals.obj     obj/pcre_tables.obj        \
   obj/pcre_compile.obj     obj/pcre_info.obj        obj/pcre_try_flipped.obj   \
   obj/pcre_config.obj      obj/pcre_maketables.obj  obj/pcre_ucd.obj           \
   obj/pcre_dfa_exec.obj    obj/pcre_newline.obj     obj/pcre_valid_utf8.obj \
   obj/pcre_exec.obj        obj/pcre_ord2utf8.obj    obj/pcre_version.obj \
   obj/pcre_fullinfo.obj    obj/pcre_refcount.obj    obj/pcre_xclass.obj

OBJ_FILES = $(OBJ_FILES) $(PCRE_OBJS)

include ../common.mak

{$(PCRE_DIR)}.c.obj:
	$(CPP) $(CPPFLAGS) $< -Fo$@

