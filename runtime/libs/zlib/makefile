NDLL_NAME := zlib


OBJ := o

ZLIB_DIR = ../../thirdparty/zlib-1.2.3

CPPFLAGS := -I$(ZLIB_DIR)

OBJ_FILES :=  \
  adler32.$(OBJ) \
  crc32.$(OBJ) \
  infback.$(OBJ) \
  inflate.$(OBJ) \
  uncompr.$(OBJ) \
  compress.$(OBJ) \
  deflate.$(OBJ) \
  gzio.$(OBJ) \
  inffast.$(OBJ) \
  inftrees.$(OBJ) \
  trees.$(OBJ) \
  zutil.$(OBJ)

include ../common.make


obj/%.$(OBJ):$(ZLIB_DIR)/%.c
	$(C) $(CFLAGS) $< -o$@


