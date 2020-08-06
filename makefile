
### MAKEMAKE STARTS HERE #######################################################


### Created by makemake.pl on Thu Aug  6 03:46:08 2020 #########################


### GLOBAL TARGETS #############################################################

default: mm_update all

re: mm_update rebuild

li: mm_update link

all: mm_update vfu 

clean: mm_update clean-vfu 

rebuild: mm_update rebuild-vfu 

link: mm_update link-vfu 

### GLOBAL (AND USER) DEFS #####################################################


AR ?= ar
LD = $(CXX)
MKDIR = mkdir -p
RANLIB ?= ranlib
RMDIR = rm -rf
RMFILE = rm -f
SRC = *.c *.cpp *.cc *.cxx


### TARGET 1: vfu ##############################################################

CC_1       = $(CXX)
LD_1       = $(LD)
AR_1       = $(AR) rv
RANLIB_1   = $(RANLIB)
CCFLAGS_1  = -I../vstring -I../vslib -I/usr/include/ncurses -O2 $(CFLAGS) $(CPPFLAGS) $(CCDEF)  
LDFLAGS_1  = -L../vstring -L../vslib -lvstring -lvslib -lvscon -lpcre -lncurses $(LDFLAGS) $(LDDEF) 
DEPFLAGS_1 = 
ARFLAGS_1  = 
TARGET_1   = vfu

### SOURCES FOR TARGET 1: vfu ##################################################

SRC_1= \
     see.cpp \
     vfu.cpp \
     vfuarc.cpp \
     vfucopy.cpp \
     vfudir.cpp \
     vfufiles.cpp \
     vfumenu.cpp \
     vfuopt.cpp \
     vfusys.cpp \
     vfutools.cpp \
     vfuuti.cpp \
     vfuview.cpp \

#### OBJECTS FOR TARGET 1: vfu #################################################

OBJ_1= \
     .OBJ.vfu/see.o \
     .OBJ.vfu/vfu.o \
     .OBJ.vfu/vfuarc.o \
     .OBJ.vfu/vfucopy.o \
     .OBJ.vfu/vfudir.o \
     .OBJ.vfu/vfufiles.o \
     .OBJ.vfu/vfumenu.o \
     .OBJ.vfu/vfuopt.o \
     .OBJ.vfu/vfusys.o \
     .OBJ.vfu/vfutools.o \
     .OBJ.vfu/vfuuti.o \
     .OBJ.vfu/vfuview.o \

### TARGET DEFINITION FOR TARGET 1: vfu ########################################

.OBJ.vfu: 
	$(MKDIR) .OBJ.vfu

vfu:   .OBJ.vfu $(OBJ_1)
	$(LD_1) $(OBJ_1) $(LDFLAGS_1) -o $(TARGET_1)

clean-vfu: 
	$(RMFILE) $(TARGET_1)
	$(RMDIR) .OBJ.vfu

rebuild-vfu: clean-vfu vfu

re-vfu: rebuild-vfu

link-vfu: .OBJ.vfu $(OBJ_1)
	$(RMFILE) vfu
	$(LD_1) $(OBJ_1) $(LDFLAGS_1) -o $(TARGET_1)


### TARGET OBJECTS FOR TARGET 1: vfu ###########################################

.OBJ.vfu/see.o: see.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c see.cpp              -o .OBJ.vfu/see.o
.OBJ.vfu/vfu.o: vfu.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfu.cpp              -o .OBJ.vfu/vfu.o
.OBJ.vfu/vfuarc.o: vfuarc.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuarc.cpp           -o .OBJ.vfu/vfuarc.o
.OBJ.vfu/vfucopy.o: vfucopy.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfucopy.cpp          -o .OBJ.vfu/vfucopy.o
.OBJ.vfu/vfudir.o: vfudir.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfudir.cpp           -o .OBJ.vfu/vfudir.o
.OBJ.vfu/vfufiles.o: vfufiles.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfufiles.cpp         -o .OBJ.vfu/vfufiles.o
.OBJ.vfu/vfumenu.o: vfumenu.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfumenu.cpp          -o .OBJ.vfu/vfumenu.o
.OBJ.vfu/vfuopt.o: vfuopt.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuopt.cpp           -o .OBJ.vfu/vfuopt.o
.OBJ.vfu/vfusys.o: vfusys.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfusys.cpp           -o .OBJ.vfu/vfusys.o
.OBJ.vfu/vfutools.o: vfutools.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfutools.cpp         -o .OBJ.vfu/vfutools.o
.OBJ.vfu/vfuuti.o: vfuuti.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuuti.cpp           -o .OBJ.vfu/vfuuti.o
.OBJ.vfu/vfuview.o: vfuview.cpp 
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuview.cpp          -o .OBJ.vfu/vfuview.o


mm_update:
	


### MAKEMAKE ENDS HERE #########################################################

