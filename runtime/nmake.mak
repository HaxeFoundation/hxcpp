DLL_NAME = ../bin/Windows/hxcpp.dll

all : dll build-libs

dll : $(DLL_NAME)

GC_DIR = thirdparty/gc-7.1

CPP = cl
DEFINES = -DBUILDING_HXCPP_DLL -DNEKO_SOURCES -D_CRT_SECURE_NO_DEPRECATE

AO_VERSION=1.2
INCLUDES = -I"$(HXCPP)/include" -I$(GC_DIR) -I$(GC_DIR)\include -I$(GC_DIR)\include\private \
 -I$(GC_DIR)/libatomic_ops-$(AO_VERSION)/src

!ifdef HXCPP_DEBUG
CPPFLAGS = -nologo $(INCLUDES) /EHsc -c -Zi -Od $(DEFINES)
DEBUG_LINK = -debug
!else
CPPFLAGS = -nologo $(INCLUDES) /EHsc -c -Zi -O2 $(DEFINES)
DEBUG_LINK = -debug
!endif

GC_OBJS = \
 obj\win32_threads.obj \
 obj\allchblk.obj \
 obj\alloc.obj \
 obj\backgraph.obj \
 obj\blacklst.obj \
 obj\checksums.obj \
 obj\dbg_mlc.obj \
 obj\dyn_load.obj \
 obj\finalize.obj \
 obj\gc_dlopen.obj \
 obj\gcj_mlc.obj \
 obj\gcname.obj \
 obj\headers.obj \
 obj\mach_dep.obj \
 obj\malloc.obj \
 obj\mallocx.obj \
 obj\mark.obj \
 obj\mark_rts.obj \
 obj\misc.obj \
 obj\msvc_dbg.obj \
 obj\new_hblk.obj \
 obj\obj_map.obj \
 obj\os_dep.obj \
 obj\pcr_interface.obj \
 obj\ptr_chck.obj \
 obj\real_malloc.obj \
 obj\reclaim.obj \
 obj\specific.obj \
 obj\stubborn.obj \
 obj\thread_local_alloc.obj \
 obj\typd_mlc.obj


OBJ_FILES = obj\hxObject.obj obj\hxNekoAPI.obj obj\hxLib.obj obj\hxHash.obj obj\hxDate.obj obj\hxGC.obj $(GC_OBJS)

HEADERS =  ../include/hxObject.h ../include/hxMacros.h ../include/hxMath.h
LD_FLAGS = $(DEBUG_LINK) -dll -out:$(DLL_NAME)
GC_EXTRA = user32.lib

$(DLL_NAME):obj

obj:
	- mkdir obj

build-libs:
	cd libs/std && nmake -nologo -f nmake.mak
	cd libs/regexp && nmake -nologo -f nmake.mak
	cd libs/zlib && nmake -nologo -f nmake.mak

$(OBJ_FILES):$(HEADERS)

$(DLL_NAME) : $(OBJ_FILES)
	link $(LD_FLAGS) $(GC_EXTRA) $(OBJ_FILES)

{src}.cpp{obj}.obj:
	$(CPP) $(CPPFLAGS) $< -Fo$@

GC_DEFINES = -DALL_INTERIOR_POINTERS -DGC_NOT_DLL -DGC_WIN32_THREADS -DMY_CPU -DX86 -DCPU -DMY_CPU

{thirdparty\gc-7.1}.c{obj}.obj:
	$(CPP) $(CPPFLAGS) $(GC_DEFINES) $< -Fo$@

clean: 
	-rm -rf obj vc80.pdb ../bin/Windows/hxcpp.exp  ../bin/Windows/hxcpp.pdb  ../bin/Windows/vc80.pdb  ../bin/Windows/hxcpp.ilk
	cd libs/std && nmake -nologo -f nmake.mak clean
	cd libs/regexp && nmake -nologo -f nmake.mak clean
	cd libs/zlib && nmake -nologo -f nmake.mak clean
