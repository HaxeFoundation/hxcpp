.PHONY : all build-ndlls

ifeq ("$(OSTYPE)","linux")
DLL_NAME = hxcpp.dso
BIN := Linux
LD_DYNAMIC :=  -shared
else
LD_DYNAMIC := -fpic -fPIC -dynamiclib -framework Cocoa
DLL_NAME = hxcpp.dylib
BIN := Mac
endif

FULL_DLL_NAME = ../bin/$(BIN)/$(DLL_NAME)

ifeq ("x$(HXCPP)","x")
export HXCPP := $(shell haxelib path hxcpp)
endif

all : obj $(FULL_DLL_NAME) build-ndlls

build-ndlls:
	cd ndlls/std; make --no-print-directory
	cd ndlls/regexp; make --no-print-directory
	cd ndlls/zlib; make --no-print-directory

CPP = g++
DEFINES = -DBUILDING_HXCPP_DLL -DNEKO_SOURCES

ifdef HXCPP_DEBUG
OPTIM := -O2
else
OPTIM := -g
endif

CPPFLAGS = -I"$(HXCPP)/include" -Ithirdparty/$(BIN)/gc/include -frtti -c $(OPTIM) $(DEFINES)
OBJ_FILES = obj/hxObject.o obj/hxNekoAPI.o obj/hxLib.o obj/hxHash.o obj/hxDate.o
HEADERS =  ../include/hxObject.h ../include/hxMacros.h ../include/hxMath.h
LD_FLAGS = -Lthirdparty/$(BIN)/gc/lib -lgc -ldl -Lthirdparty/$(BIN) -lz -lpthread $(LD_DYNAMIC) -o $(DLL_NAME)

$(FULL_DLL_NAME):obj

obj:
	mkdir obj

$(OBJ_FILES):$(HEADERS)

$(FULL_DLL_NAME) : $(OBJ_FILES)
	g++ $(GC_EXTRA) $(OBJ_FILES) $(LD_FLAGS)
	mv $(DLL_NAME) $(FULL_DLL_NAME)

obj/%.o:src/%.cpp
	$(CPP) $(CPPFLAGS) $< -o$@

clean: 
	rm -rf obj
	cd ndlls/std; make clean --no-print-directory
	cd ndlls/regexp; make clean --no-print-directory
	cd ndlls/zlib; make clean --no-print-directory

clobber: 
	rm -rf obj $(FULL_DLL_NAME)
	cd ndlls/std; make clobber --no-print-directory
	cd ndlls/regexp; make clobber --no-print-directory
	cd ndlls/zlib; make clobber --no-print-directory
