
### MAKEMAKE STARTS HERE #######################################################


### Created by makemake.pl on Mon Aug 24 02:59:06 2015 #########################


### GLOBAL TARGETS #############################################################

default: mm_update all

re: mm_update rebuild

li: mm_update link

all: mm_update vfu vfu-yas 

clean: mm_update clean-vfu clean-vfu-yas 

rebuild: mm_update rebuild-vfu rebuild-vfu-yas 

link: mm_update link-vfu link-vfu-yas 

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
CCFLAGS_1  = -I../vslib -I/usr/include/ncurses -O2 $(CCDEF)  
LDFLAGS_1  = -L../vslib -lvslib -lvscon -lpcre -lncurses $(LDDEF) 
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


### TARGET 2: vfu-yas ##########################################################

CC_2       = g++
LD_2       = g++
AR_2       = ar rv
RANLIB_2   = ranlib
CCFLAGS_2  = -I../vslib -I../yascreen -DUSE_YASCREEN -O2        $(CCDEF)  
LDFLAGS_2  = -L../vslib -L../yascreen -lvslib -lvscony -lpcre -lyascreen -lrt $(LDDEF) 
DEPFLAGS_2 = 
ARFLAGS_2  = 
TARGET_2   = vfu-yas

### SOURCES FOR TARGET 2: vfu-yas ##############################################

SRC_2= \
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

#### OBJECTS FOR TARGET 2: vfu-yas #############################################

OBJ_2= \
     .OBJ.vfu-yas/see.o \
     .OBJ.vfu-yas/vfu.o \
     .OBJ.vfu-yas/vfuarc.o \
     .OBJ.vfu-yas/vfucopy.o \
     .OBJ.vfu-yas/vfudir.o \
     .OBJ.vfu-yas/vfufiles.o \
     .OBJ.vfu-yas/vfumenu.o \
     .OBJ.vfu-yas/vfuopt.o \
     .OBJ.vfu-yas/vfusys.o \
     .OBJ.vfu-yas/vfutools.o \
     .OBJ.vfu-yas/vfuuti.o \
     .OBJ.vfu-yas/vfuview.o \

### TARGET DEFINITION FOR TARGET 2: vfu-yas ####################################

.OBJ.vfu-yas: 
	$(MKDIR) .OBJ.vfu-yas

vfu-yas:   .OBJ.vfu-yas $(OBJ_2)
	$(LD_2) $(OBJ_2) $(LDFLAGS_2) -o $(TARGET_2)

clean-vfu-yas: 
	$(RMFILE) $(TARGET_2)
	$(RMDIR) .OBJ.vfu-yas

rebuild-vfu-yas: clean-vfu-yas vfu-yas

link-vfu-yas: .OBJ.vfu-yas $(OBJ_2)
	$(RMFILE) vfu-yas
	$(LD_2) $(OBJ_2) $(LDFLAGS_2) -o $(TARGET_2)


### TARGET OBJECTS FOR TARGET 2: vfu-yas #######################################

.OBJ.vfu-yas/see.o: see.cpp  see.cpp see.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c see.cpp              -o .OBJ.vfu-yas/see.o
.OBJ.vfu-yas/vfu.o: vfu.cpp  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfu.cpp              -o .OBJ.vfu-yas/vfu.o
.OBJ.vfu-yas/vfuarc.o: vfuarc.cpp  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h vfuopt.h \
 see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuarc.cpp           -o .OBJ.vfu-yas/vfuarc.o
.OBJ.vfu-yas/vfucopy.o: vfucopy.cpp  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
 vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfucopy.cpp          -o .OBJ.vfu-yas/vfucopy.o
.OBJ.vfu-yas/vfudir.o: vfudir.cpp  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfudir.cpp           -o .OBJ.vfu-yas/vfudir.o
.OBJ.vfu-yas/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuopt.h \
 see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfufiles.cpp         -o .OBJ.vfu-yas/vfufiles.o
.OBJ.vfu-yas/vfumenu.o: vfumenu.cpp  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfumenu.h vfuview.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfumenu.cpp          -o .OBJ.vfu-yas/vfumenu.o
.OBJ.vfu-yas/vfuopt.o: vfuopt.cpp  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfudir.h vfuview.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuopt.cpp           -o .OBJ.vfu-yas/vfuopt.o
.OBJ.vfu-yas/vfusys.o: vfusys.cpp  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfusys.cpp           -o .OBJ.vfu-yas/vfusys.o
.OBJ.vfu-yas/vfutools.o: vfutools.cpp  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
 vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfutools.cpp         -o .OBJ.vfu-yas/vfutools.o
.OBJ.vfu-yas/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h \
 vfudir.h vfuopt.h see.h vfuview.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuuti.cpp           -o .OBJ.vfu-yas/vfuuti.o
.OBJ.vfu-yas/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuview.cpp          -o .OBJ.vfu-yas/vfuview.o


mm_update:
	


### MAKEMAKE ENDS HERE #########################################################

