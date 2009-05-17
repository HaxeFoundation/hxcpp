NEKO_SOURCE_DIR = ../neko
NDLL_NAME = std
LD_FLAGS = wsock32.lib

OBJ_FILES = \
   obj/Init.obj \
   obj/Misc.obj \
   obj/File.obj \
   obj/Process.obj \
   obj/Random.obj \
   obj/Socket.obj \
   obj/String.obj \
   obj/Sys.obj \
   obj/Thread.obj \
   obj/Xml.obj

include ../common.mak


