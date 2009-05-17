NDLL_NAME = zlib
ZLIB_DIR = ../../thirdparty/zlib-1.2.3
HX_INCLUDE = -I$(ZLIB_DIR)

ZLIB_OBJS = \
 obj\adler32.obj \
 obj\crc32.obj \
 obj\infback.obj \
 obj\inflate.obj \
 obj\uncompr.obj \
 obj\compress.obj \
 obj\deflate.obj \
 obj\gzio.obj \
 obj\inffast.obj \
 obj\inftrees.obj \
 obj\trees.obj \
 obj\zutil.obj \

OBJ_FILES = \
   obj\ZLib.obj $(ZLIB_OBJS) \

include ../common.mak

{$(ZLIB_DIR)}.c{obj}.obj:
	$(CPP) $(CPPFLAGS) $< -Fo$@




