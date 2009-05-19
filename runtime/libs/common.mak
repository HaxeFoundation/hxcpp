.default:all
.phony:clean

DLL_NAME = ../../../bin/Windows/$(NDLL_NAME).dll
CPP = cl
DEFINES = $(DEFINES) -DNEKO_SOURCES -D_CRT_SECURE_NO_DEPRECATE

HX_INCLUDE = $(HX_INCLUDE) -I../../../include
!ifdef HXCPP_DEBUG
CPPFLAGS = -nologo $(HX_INCLUDE) /EHsc -c -Zi -Od $(DEFINES) 
!else
CPPFLAGS = -nologo $(HX_INCLUDE) /EHsc -c -Zi -O2 $(DEFINES) 
!endif


all : obj $(DLL_NAME)

obj:
	-mkdir obj

LD_FLAGS = $(LD_FLAGS) -debug -dll -libpath:../../../bin/Windows hxcpp.lib -out:$(DLL_NAME) wsock32.lib

{.\}.cpp{obj}.obj:
	$(CPP) $(CPPFLAGS) $< -Fo$@


$(DLL_NAME) : $(OBJ_FILES)
	link $(LD_FLAGS) $(OBJ_FILES)
	-rm -f ../../../bin/Windows/$(NDLL_NAME).exp
	-rm -f ../../../bin/Windows/$(NDLL_NAME).lib
	-rm -f ../../../bin/Windows/$(NDLL_NAME).ilk

clean:
	-rm -rf obj vc80.pdb
	-rm -f ../../../bin/Windows/$(NDLL_NAME).pdb
	-rm -f ../../../bin/Windows/$(NDLL_NAME).exp
	-rm -f ../../../bin/Windows/$(NDLL_NAME).lib
	-rm -f ../../../bin/Windows/$(NDLL_NAME).ilk
