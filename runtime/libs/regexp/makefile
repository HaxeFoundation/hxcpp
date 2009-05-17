NDLL_NAME := regexp

OBJ := o

PCRE_DIR = ../../thirdparty/$(BIN)/pcre-7.8

CPPFLAGS := -I$(PCRE_DIR)

OBJ_FILES :=  \
   pcre_get.$(OBJ)         pcre_study.$(OBJ)        \
   pcre_chartables.$(OBJ)  pcre_globals.$(OBJ)     pcre_tables.$(OBJ)        \
   pcre_compile.$(OBJ)     pcre_info.$(OBJ)        pcre_try_flipped.$(OBJ)   \
   pcre_config.$(OBJ)      pcre_maketables.$(OBJ)  pcre_ucd.$(OBJ)           \
   pcre_dfa_exec.$(OBJ)    pcre_newline.$(OBJ)     pcre_valid_utf8.$(OBJ) \
   pcre_exec.$(OBJ)        pcre_ord2utf8.$(OBJ)    pcre_version.$(OBJ) \
   pcre_fullinfo.$(OBJ)    pcre_refcount.$(OBJ)    pcre_xclass.$(OBJ)


include ../common.make



obj/%.$(OBJ):$(PCRE_DIR)/%.c
	$(CPP) $(CPPFLAGS) $< -o$@


