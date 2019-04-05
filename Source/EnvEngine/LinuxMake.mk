
#compiler flags

#if clang doesnt work, try gcc, g++, or any other c++ compiler you have installed
CC = clang
CFLAGS =  -Wall -std=c++11 -fopenmp -Wl,-rpath,/usr/local/lib/envision


#header include paths
INCLUDE= -I./ -I../ -I../Libs -I../Libs/AlgLib -I../Libs/mtparser -I../Libs/randgen 

#preprocessor defines
#	The first line are MACROS that are needed for the WINDOWS build, but 
#	Need to be removed for the linux build
#
#	The second line contains macros to enable Linux building. They are as follows:
#	+ NO_MFC Switch used to root around MFC based code, usually for interface stuff
#	+ NO_MFC_COMPAT_TEMPLATE_CALLS Flag to assist with eventual migration away from MFC.
#	  If defined, the drop in replacements will use signatures compatible with their MFC 
#	  counterparts; otherwise, the extra ARG_* template declarations are removed. For example: 
#	  CArray has a template signature of <VALUE,ARG_VALUE> when this flag is enabled, which 
#	  matches MFC. However, when the flag is absent, the CArray template args become <VALUE>.

DEFINES= -D __EXPORT__= -D LIBSAPI= -D AFX_INLINE= -D AFXAPI= -D AFX_EXT_CLASS= -D PASCAL= -D __ENV_ENGINE__ \
-D NO_MFC -D NO_MFC_COMPAT_TEMPLATE_CALLS



#arguments for linker
LNK_ARGS= -L/usr/local/lib/envision -L../Libs -lstdc++ -lgdal -lm -ldl -lLibs
LNK_ARGS_DEV= -L/usr/local/lib/envision -L../Libs -lstdc++ -lgdal -lm -ldl -lLibs-dev

#Warnings to suppress
NO_WARNS= -Wno-unknown-pragmas -Wno-reorder -Wno-unused-value -Wno-unused-variable -Wno-switch -Wno-c++11-compat-deprecated-writable-strings \
-Wno-comment -Wno-format -Wno-parentheses -Wno-unneeded-internal-declaration -Wno-logical-op-parentheses

#files to compile
CPP_FILES= LinuxMain.cpp \
Actor.cpp     \
ColumnTrace.cpp          \
DataManager.cpp          \
DeltaArray.cpp          \
Delta.cpp                \
EnvException.cpp         \
EnvLoader.cpp            \
EnvMisc.cpp              \
EnvModel.cpp             \
Policy.cpp               \
PolicyOutcomeLexer.cpp   \
PolicyOutcomeParser.cpp  \
Scenario.cpp             \
SocialNetwork.cpp 	 \
../no_mfc_files/CArray_compat.cpp

#build configs
all: release

build_depends:
	make -C ../Libs/ -f ../Libs/LinuxMake.mk release_and_install
	make -C ../SpatialAllocator/ -f ../Libs/LinuxMake.mk release_and_install

build_depends_dbg:
	make -C ../Libs/ -f ../Libs/LinuxMake.mk debug_and_install
	make -C ../SpatialAllocator/ -f ../Libs/LinuxMake.mk debug_and_install
debug:
	$(CC) $(CFLAGS) -g -D _DEBUG -D DEBUG_NEW=new -o "EnvEngine-dev" $(INCLUDE) $(LNK_ARGS_DEV) $(CPP_FILES) $(DEFINES) $(NO_WARNS)

release:
	$(CC) $(CFLAGS) -o "EnvEngine" $(INCLUDE) $(LNK_ARGS) $(CPP_FILES) $(DEFINES) $(NO_WARNS)

rebuild_all: | build_depends release


rebuild_all_dbg: | build_depends_dbg debug

clean:
	rm *o
