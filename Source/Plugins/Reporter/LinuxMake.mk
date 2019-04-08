
#compiler flags

#basic primer on how to build linux libraries:
#	http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html

#if clang doesnt work, try gcc, g++, or any other c++ compiler you have installed

#increment version and revision as necessary here
VERS_MAJOR=0
VERS_MINOR=0
RELEASE=1

BASENAME=libReporter.so
BASENAME_DEV=libReporter-dev.so

LIBNAME=${BASENAME}.${VERS_MAJOR}.${VERS_MINOR}.${RELEASE}
LIBNAME_DEV=${BASENAME_DEV}.${VERS_MAJOR}.${VERS_MINOR}.${RELEASE}
INSTALL_DIR=/usr/local/lib/envision/

CC = clang
CFLAGS =  -Wall -std=c++11 -fopenmp -shared -fPIC -Wl,-soname,${BASENAME}.${VERS_MAJOR} 
CFLAGS_DEV =  -Wall -std=c++11 -fopenmp -shared -fPIC -Wl,-soname,${BASENAME_DEV}.${VERS_MAJOR} 
#-Wl,-rpath,/home/pwingo/Envision/src/Libs/


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

DEFINES= -D __EXPORT__= -D LIBSAPI= -D AFX_INLINE= -D AFXAPI= -D AFX_EXT_CLASS= -D PASCAL= -D CALLBACK= \
-D NO_MFC -D NO_MFC_COMPAT_TEMPLATE_CALLS


#arguments for linker.
LNK_ARGS=  -L/usr/local/lib/envision -L../Libs -lstdc++ -lgdal -lm -ldl -lLibs
LNK_ARGS_DEV=  -L/usr/local/lib/envision -L../Libs -lstdc++ -lgdal -lm -ldl -lLibs-dev

#Warnings to suppress
NO_WARNS= -Wno-unknown-pragmas -Wno-reorder -Wno-unused-value -Wno-unused-variable -Wno-switch -Wno-c++11-compat-deprecated-writable-strings \
-Wno-comment -Wno-format -Wno-parentheses -Wno-unneeded-internal-declaration -Wno-logical-op-parentheses


#files to compile
CPP_FILES= LinuxEntries.cpp \
Reporter.cpp \
../EnvExtension.cpp

#NOTE: we'll eventually need to set rpath at some point

#build configs
all: release

debug:
	$(CC) $(CFLAGS_DEV) -g -D _DEBUG -D DEBUG_NEW=new -o ${LIBNAME_DEV} $(INCLUDE) $(LNK_ARGS_DEV) $(CPP_FILES) $(DEFINES) $(NO_WARNS)

release:
	$(CC) $(CFLAGS) -o ${LIBNAME} $(INCLUDE) $(LNK_ARGS) $(CPP_FILES) $(DEFINES) $(NO_WARNS)
clean:
	rm *o

install:
	echo Installing ${LIBNAME} in ${INSTALL_DIR}
	#create directory if needed
	mkdir -p ${INSTALL_DIR}
	#Move product
	mv ${LIBNAME} ${INSTALL_DIR}${LIBNAME}
	#build symlinks for versioning
	ln -fs ${INSTALL_DIR}${LIBNAME} ${INSTALL_DIR}${BASENAME} 
	ln -fs ${INSTALL_DIR}${LIBNAME} ${INSTALL_DIR}${BASENAME}.${VERS_MAJOR} 

	#update system lib cache
	sudo ldconfig

install_dbg:
	echo Installing ${LIBNAME_DEV} in ${INSTALL_DIR}
	#create directory if needed
	mkdir -p ${INSTALL_DIR}
	#Move product
	mv ${LIBNAME_DEV} ${INSTALL_DIR}${LIBNAME_DEV}
	#build symlinks for versioning
	ln -fs ${INSTALL_DIR}${LIBNAME_DEV} ${INSTALL_DIR}${BASENAME_DEV} 
	ln -fs ${INSTALL_DIR}${LIBNAME_DEV} ${INSTALL_DIR}${BASENAME_DEV}.${VERS_MAJOR} 

	#update system lib cache
	sudo ldconfig

release_and_install: | release install

debug_and_install: | debug install_dbg
