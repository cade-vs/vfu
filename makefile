
### MAKEMAKE STARTS HERE #######################################################


### Created by makemake.pl on Wed Dec 29 04:48:32 2004 #########################


### GLOBAL TARGETS #############################################################

default: all

re: rebuild

li: link

all: vfu 

clean: clean-vfu 

rebuild: rebuild-vfu 

link: link-vfu 

### GLOBAL (AND USER) DEFS #####################################################


AR = ar rv
CC = gcc
LD = gcc
MKDIR = mkdir -p
RANLIB = ranlib
RMDIR = rm -rf
RMFILE = rm -f
SRC = *.c *.cpp *.cc *.cxx


### TARGET 1: vfu ##############################################################

CC_1       = g++
LD_1       = g++
AR_1       = ar rv
RANLIB_1   = ranlib
CCFLAGS_1  = -I../vslib -I../vslib/pcre -I/usr/include/ncurses -O2 $(CCDEF) 
LDFLAGS_1  = -L../vslib -L../vslib/pcre -lvslib -lvscon -lpcre -lncurses $(LDDEF)
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

vfu:  .OBJ.vfu $(OBJ_1)
	$(LD_1) $(OBJ_1) $(LDFLAGS_1) -o $(TARGET_1)

clean-vfu: 
	$(RMFILE) $(TARGET_1)
	$(RMDIR) .OBJ.vfu

rebuild-vfu: clean-vfu vfu

link-vfu: .OBJ.vfu $(OBJ_1)
	$(RMFILE) vfu
	$(LD_1) $(OBJ_1) $(LDFLAGS_1) -o $(TARGET_1)


### TARGET OBJECTS FOR TARGET 1: vfu ###########################################

.OBJ.vfu/see.o: see.cpp  see.cpp see.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c see.cpp              -o .OBJ.vfu/see.o
.OBJ.vfu/vfu.o: vfu.cpp  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
  vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfu.cpp              -o .OBJ.vfu/vfu.o
.OBJ.vfu/vfuarc.o: vfuarc.cpp  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h vfuopt.h \
  see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuarc.cpp           -o .OBJ.vfu/vfuarc.o
.OBJ.vfu/vfucopy.o: vfucopy.cpp  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
  vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfucopy.cpp          -o .OBJ.vfu/vfucopy.o
.OBJ.vfu/vfudir.o: vfudir.cpp  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
  vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfudir.cpp           -o .OBJ.vfu/vfudir.o
.OBJ.vfu/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuopt.h \
  see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfufiles.cpp         -o .OBJ.vfu/vfufiles.o
.OBJ.vfu/vfumenu.o: vfumenu.cpp  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
  vfumenu.h vfuview.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfumenu.cpp          -o .OBJ.vfu/vfumenu.o
.OBJ.vfu/vfuopt.o: vfuopt.cpp  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
  vfudir.h vfuview.h vfumenu.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuopt.cpp           -o .OBJ.vfu/vfuopt.o
.OBJ.vfu/vfusys.o: vfusys.cpp  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfusys.cpp           -o .OBJ.vfu/vfusys.o
.OBJ.vfu/vfutools.o: vfutools.cpp  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
  vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfutools.cpp         -o .OBJ.vfu/vfutools.o
.OBJ.vfu/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h \
  vfudir.h vfuopt.h see.h vfuview.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuuti.cpp           -o .OBJ.vfu/vfuuti.o
.OBJ.vfu/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
  vfuopt.h see.h vfuuti.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuview.cpp          -o .OBJ.vfu/vfuview.o


### MAKEMAKE ENDS HERE #########################################################

