.PHONY : all build-libs

GC_FILES = \
pthread_stop_world pthread_support \
allchblk alloc backgraph blacklst checksums dbg_mlc dyn_load \
finalize gc_dlopen gcj_mlc gcname headers mach_dep malloc os_dep \
mallocx mark mark_rts misc new_hblk obj_map \
pcr_interface ptr_chck real_malloc reclaim specific stubborn \
thread_local_alloc typd_mlc



ifeq ("$(OSTYPE)","linux")

DLL_NAME = hxcpp.dso
BIN := Linux
LD_DYNAMIC :=  -shared -fpic -fPIX

else

LD_DYNAMIC := -fpic -fPIC -dynamiclib -framework Cocoa
DLL_NAME = hxcpp.dylib
BIN := Mac
GC_FILES += darwin_stop_world
GC_DEFINES += -DHAS_X86_THREAD_STATE32___EAX=1
endif

LIB_NAME = libstatic_hxcpp.a

FULL_DLL_NAME = ../bin/$(BIN)/$(DLL_NAME)
FULL_LIB_NAME = ../bin/$(BIN)/$(LIB_NAME)


GC_DEFINES += -DALL_INTERIOR_POINTERS -DGC_NOT_DLL -DMY_CPU -DX86 -DCPU -DMY_CPU -DGC_NO_THREAD_REDIRECTS -DGC_THREADS



ifeq ("x$(HXCPP)","x")
export HXCPP := $(shell haxelib path hxcpp)
endif

all : obj $(FULL_DLL_NAME) build-libs

lib : $(FULL_LIB_NAME)

build-libs:
	cd libs/std; make --no-print-directory
	cd libs/regexp; make --no-print-directory
	cd libs/zlib; make --no-print-directory

CPP = g++
C = gcc
DEFINES = -DBUILDING_HXCPP_DLL -DNEKO_SOURCES $(GC_DEFINES)

ifdef HXCPP_DEBUG
OPTIM := -O2
else
OPTIM := -g
endif


GC_DIR := thirdparty/gc-7.1
AO_VERSION := 1.2
GC_INCLUDES := -I$(GC_DIR) -I$(GC_DIR)/include -I$(GC_DIR)/include/private -I$(GC_DIR)/libatomic_ops-$(AO_VERSION)/src


CFLAGS = -I"../include" $(GC_INCLUDES) -c $(OPTIM) $(DEFINES)
CPPFLAGS = $(CFLAGS) -frtti

OBJ_FILES = obj/hxObject.o obj/hxNekoAPI.o obj/hxLib.o obj/hxHash.o \
             obj/hxDate.o \
            $(GC_FILES:%=obj/%.o)

HEADERS =  ../include/hxObject.h ../include/hxMacros.h ../include/hxMath.h

LD_FLAGS = -ldl -lpthread $(LD_DYNAMIC) -o $(DLL_NAME)

$(FULL_DLL_NAME):obj
$(FULL_LIB_NAME):obj

obj:
	mkdir obj

$(OBJ_FILES):$(HEADERS)

$(FULL_DLL_NAME) : $(OBJ_FILES)
	g++ $(GC_EXTRA) $(OBJ_FILES) $(LD_FLAGS)
	mv $(DLL_NAME) $(FULL_DLL_NAME)

$(FULL_LIB_NAME) : $(OBJ_FILES)
	ar -cr $(FULL_LIB_NAME) $(OBJ_FILES) 

obj/%.o:src/%.cpp
	$(CPP) $(CPPFLAGS) $< -o$@

obj/%.o:$(GC_DIR)/%.c
	$(C) $(CFLAGS) $< -o$@

clean: 
	rm -rf obj
	cd libs/std; make clean --no-print-directory
	cd libs/regexp; make clean --no-print-directory
	cd libs/zlib; make clean --no-print-directory

clobber: 
	rm -rf obj $(FULL_DLL_NAME) $(FULL_LIB_NAME)
	cd libs/std; make clobber --no-print-directory
	cd libs/regexp; make clobber --no-print-directory
	cd libs/zlib; make clobber --no-print-directory
