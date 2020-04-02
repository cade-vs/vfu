
### MAKEMAKE STARTS HERE #######################################################


### Created by makemake.pl on Fri Apr  3 02:00:56 2020 #########################


### GLOBAL TARGETS #############################################################

default: mm_update all

re: mm_update rebuild

li: mm_update link

all: mm_update vfu vfu.yas vfu-debug vfu.yas-debug 

clean: mm_update clean-vfu clean-vfu.yas clean-vfu-debug clean-vfu.yas-debug 

rebuild: mm_update rebuild-vfu rebuild-vfu.yas rebuild-vfu-debug rebuild-vfu.yas-debug 

link: mm_update link-vfu link-vfu.yas link-vfu-debug link-vfu.yas-debug 

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

CC_1       = $(CXX)
LD_1       = $(CXX)
AR_1       = ar rv
RANLIB_1   = ranlib
CCFLAGS_1  = -I../vstring -I../vslib -I/usr/include/ncurses -O2 $(CCDEF)  
LDFLAGS_1  = -L../vstring -L../vslib -lvstring -lvslib -lvscon -lpcre -lncurses $(LDDEF) 
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
.OBJ.vfu/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfufiles.h vfu.h vfusetup.h vfusys.h vfuopt.h \
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
.OBJ.vfu/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfufiles.h \
 vfumenu.h vfudir.h vfuopt.h see.h vfuview.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuuti.cpp           -o .OBJ.vfu/vfuuti.o
.OBJ.vfu/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_1) $(CFLAGS_1) $(CCFLAGS_1) -c vfuview.cpp          -o .OBJ.vfu/vfuview.o


### TARGET 2: vfu.yas ##########################################################

CC_2       = $(CXX)
LD_2       = $(CXX)
AR_2       = ar rv
RANLIB_2   = ranlib
CCFLAGS_2  = -I../vstring -I../vslib -I../yascreen -DUSE_YASCREEN -O2        $(CCDEF)  
LDFLAGS_2  = -L../vstring -L../vslib -L../yascreen -lvstring -lvslib -lvscony -lpcre ../yascreen/libyascreen.a -lrt $(LDDEF) 
DEPFLAGS_2 = 
ARFLAGS_2  = 
TARGET_2   = vfu.yas

### SOURCES FOR TARGET 2: vfu.yas ##############################################

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

#### OBJECTS FOR TARGET 2: vfu.yas #############################################

OBJ_2= \
     .OBJ.vfu.yas/see.o \
     .OBJ.vfu.yas/vfu.o \
     .OBJ.vfu.yas/vfuarc.o \
     .OBJ.vfu.yas/vfucopy.o \
     .OBJ.vfu.yas/vfudir.o \
     .OBJ.vfu.yas/vfufiles.o \
     .OBJ.vfu.yas/vfumenu.o \
     .OBJ.vfu.yas/vfuopt.o \
     .OBJ.vfu.yas/vfusys.o \
     .OBJ.vfu.yas/vfutools.o \
     .OBJ.vfu.yas/vfuuti.o \
     .OBJ.vfu.yas/vfuview.o \

### TARGET DEFINITION FOR TARGET 2: vfu.yas ####################################

.OBJ.vfu.yas: 
	$(MKDIR) .OBJ.vfu.yas

vfu.yas:   .OBJ.vfu.yas $(OBJ_2)
	$(LD_2) $(OBJ_2) $(LDFLAGS_2) -o $(TARGET_2)

clean-vfu.yas: 
	$(RMFILE) $(TARGET_2)
	$(RMDIR) .OBJ.vfu.yas

rebuild-vfu.yas: clean-vfu.yas vfu.yas

link-vfu.yas: .OBJ.vfu.yas $(OBJ_2)
	$(RMFILE) vfu.yas
	$(LD_2) $(OBJ_2) $(LDFLAGS_2) -o $(TARGET_2)


### TARGET OBJECTS FOR TARGET 2: vfu.yas #######################################

.OBJ.vfu.yas/see.o: see.cpp  see.cpp see.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c see.cpp              -o .OBJ.vfu.yas/see.o
.OBJ.vfu.yas/vfu.o: vfu.cpp  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfu.cpp              -o .OBJ.vfu.yas/vfu.o
.OBJ.vfu.yas/vfuarc.o: vfuarc.cpp  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h vfuopt.h \
 see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuarc.cpp           -o .OBJ.vfu.yas/vfuarc.o
.OBJ.vfu.yas/vfucopy.o: vfucopy.cpp  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
 vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfucopy.cpp          -o .OBJ.vfu.yas/vfucopy.o
.OBJ.vfu.yas/vfudir.o: vfudir.cpp  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfudir.cpp           -o .OBJ.vfu.yas/vfudir.o
.OBJ.vfu.yas/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfufiles.h vfu.h vfusetup.h vfusys.h vfuopt.h \
 see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfufiles.cpp         -o .OBJ.vfu.yas/vfufiles.o
.OBJ.vfu.yas/vfumenu.o: vfumenu.cpp  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfumenu.h vfuview.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfumenu.cpp          -o .OBJ.vfu.yas/vfumenu.o
.OBJ.vfu.yas/vfuopt.o: vfuopt.cpp  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfudir.h vfuview.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuopt.cpp           -o .OBJ.vfu.yas/vfuopt.o
.OBJ.vfu.yas/vfusys.o: vfusys.cpp  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfusys.cpp           -o .OBJ.vfu.yas/vfusys.o
.OBJ.vfu.yas/vfutools.o: vfutools.cpp  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
 vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfutools.cpp         -o .OBJ.vfu.yas/vfutools.o
.OBJ.vfu.yas/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfufiles.h \
 vfumenu.h vfudir.h vfuopt.h see.h vfuview.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuuti.cpp           -o .OBJ.vfu.yas/vfuuti.o
.OBJ.vfu.yas/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_2) $(CFLAGS_2) $(CCFLAGS_2) -c vfuview.cpp          -o .OBJ.vfu.yas/vfuview.o


### TARGET 3: vfu-debug ########################################################

CC_3       = g++
LD_3       = g++
AR_3       = ar rv
RANLIB_3   = ranlib
CCFLAGS_3  = -I../vstring -I../vslib -I/usr/include/ncurses -O0 -g $(CCDEF)  
LDFLAGS_3  = -L../vstring -L../vslib -lvstring -lvslib -lvscon -lpcre -lncurses -g $(LDDEF) 
DEPFLAGS_3 = 
ARFLAGS_3  = 
TARGET_3   = vfu-debug

### SOURCES FOR TARGET 3: vfu-debug ############################################

SRC_3= \
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

#### OBJECTS FOR TARGET 3: vfu-debug ###########################################

OBJ_3= \
     .OBJ.vfu-debug/see.o \
     .OBJ.vfu-debug/vfu.o \
     .OBJ.vfu-debug/vfuarc.o \
     .OBJ.vfu-debug/vfucopy.o \
     .OBJ.vfu-debug/vfudir.o \
     .OBJ.vfu-debug/vfufiles.o \
     .OBJ.vfu-debug/vfumenu.o \
     .OBJ.vfu-debug/vfuopt.o \
     .OBJ.vfu-debug/vfusys.o \
     .OBJ.vfu-debug/vfutools.o \
     .OBJ.vfu-debug/vfuuti.o \
     .OBJ.vfu-debug/vfuview.o \

### TARGET DEFINITION FOR TARGET 3: vfu-debug ##################################

.OBJ.vfu-debug: 
	$(MKDIR) .OBJ.vfu-debug

vfu-debug:   .OBJ.vfu-debug $(OBJ_3)
	$(LD_3) $(OBJ_3) $(LDFLAGS_3) -o $(TARGET_3)

clean-vfu-debug: 
	$(RMFILE) $(TARGET_3)
	$(RMDIR) .OBJ.vfu-debug

rebuild-vfu-debug: clean-vfu-debug vfu-debug

link-vfu-debug: .OBJ.vfu-debug $(OBJ_3)
	$(RMFILE) vfu-debug
	$(LD_3) $(OBJ_3) $(LDFLAGS_3) -o $(TARGET_3)


### TARGET OBJECTS FOR TARGET 3: vfu-debug #####################################

.OBJ.vfu-debug/see.o: see.cpp  see.cpp see.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c see.cpp              -o .OBJ.vfu-debug/see.o
.OBJ.vfu-debug/vfu.o: vfu.cpp  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfu.cpp              -o .OBJ.vfu-debug/vfu.o
.OBJ.vfu-debug/vfuarc.o: vfuarc.cpp  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h vfuopt.h \
 see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfuarc.cpp           -o .OBJ.vfu-debug/vfuarc.o
.OBJ.vfu-debug/vfucopy.o: vfucopy.cpp  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
 vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfucopy.cpp          -o .OBJ.vfu-debug/vfucopy.o
.OBJ.vfu-debug/vfudir.o: vfudir.cpp  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfudir.cpp           -o .OBJ.vfu-debug/vfudir.o
.OBJ.vfu-debug/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfufiles.h vfu.h vfusetup.h vfusys.h vfuopt.h \
 see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfufiles.cpp         -o .OBJ.vfu-debug/vfufiles.o
.OBJ.vfu-debug/vfumenu.o: vfumenu.cpp  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfumenu.h vfuview.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfumenu.cpp          -o .OBJ.vfu-debug/vfumenu.o
.OBJ.vfu-debug/vfuopt.o: vfuopt.cpp  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfudir.h vfuview.h vfumenu.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfuopt.cpp           -o .OBJ.vfu-debug/vfuopt.o
.OBJ.vfu-debug/vfusys.o: vfusys.cpp  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfusys.cpp           -o .OBJ.vfu-debug/vfusys.o
.OBJ.vfu-debug/vfutools.o: vfutools.cpp  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
 vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfutools.cpp         -o .OBJ.vfu-debug/vfutools.o
.OBJ.vfu-debug/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfufiles.h \
 vfumenu.h vfudir.h vfuopt.h see.h vfuview.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfuuti.cpp           -o .OBJ.vfu-debug/vfuuti.o
.OBJ.vfu-debug/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_3) $(CFLAGS_3) $(CCFLAGS_3) -c vfuview.cpp          -o .OBJ.vfu-debug/vfuview.o


### TARGET 4: vfu.yas-debug ####################################################

CC_4       = g++
LD_4       = g++
AR_4       = ar rv
RANLIB_4   = ranlib
CCFLAGS_4  = -I../vstring -I../vslib -I../yascreen -DUSE_YASCREEN -O0 -g $(CCDEF)  
LDFLAGS_4  = -L../vstring -L../vslib -L../yascreen -lvstring -lvslib -lvscony -lpcre ../yascreen/libyascreen.a -lrt -g $(LDDEF) 
DEPFLAGS_4 = 
ARFLAGS_4  = 
TARGET_4   = vfu.yas-debug

### SOURCES FOR TARGET 4: vfu.yas-debug ########################################

SRC_4= \
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

#### OBJECTS FOR TARGET 4: vfu.yas-debug #######################################

OBJ_4= \
     .OBJ.vfu.yas-debug/see.o \
     .OBJ.vfu.yas-debug/vfu.o \
     .OBJ.vfu.yas-debug/vfuarc.o \
     .OBJ.vfu.yas-debug/vfucopy.o \
     .OBJ.vfu.yas-debug/vfudir.o \
     .OBJ.vfu.yas-debug/vfufiles.o \
     .OBJ.vfu.yas-debug/vfumenu.o \
     .OBJ.vfu.yas-debug/vfuopt.o \
     .OBJ.vfu.yas-debug/vfusys.o \
     .OBJ.vfu.yas-debug/vfutools.o \
     .OBJ.vfu.yas-debug/vfuuti.o \
     .OBJ.vfu.yas-debug/vfuview.o \

### TARGET DEFINITION FOR TARGET 4: vfu.yas-debug ##############################

.OBJ.vfu.yas-debug: 
	$(MKDIR) .OBJ.vfu.yas-debug

vfu.yas-debug:   .OBJ.vfu.yas-debug $(OBJ_4)
	$(LD_4) $(OBJ_4) $(LDFLAGS_4) -o $(TARGET_4)

clean-vfu.yas-debug: 
	$(RMFILE) $(TARGET_4)
	$(RMDIR) .OBJ.vfu.yas-debug

rebuild-vfu.yas-debug: clean-vfu.yas-debug vfu.yas-debug

link-vfu.yas-debug: .OBJ.vfu.yas-debug $(OBJ_4)
	$(RMFILE) vfu.yas-debug
	$(LD_4) $(OBJ_4) $(LDFLAGS_4) -o $(TARGET_4)


### TARGET OBJECTS FOR TARGET 4: vfu.yas-debug #################################

.OBJ.vfu.yas-debug/see.o: see.cpp  see.cpp see.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c see.cpp              -o .OBJ.vfu.yas-debug/see.o
.OBJ.vfu.yas-debug/vfu.o: vfu.cpp  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfu.cpp              -o .OBJ.vfu.yas-debug/vfu.o
.OBJ.vfu.yas-debug/vfuarc.o: vfuarc.cpp  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h vfuopt.h \
 see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfuarc.cpp           -o .OBJ.vfu.yas-debug/vfuarc.o
.OBJ.vfu.yas-debug/vfucopy.o: vfucopy.cpp  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
 vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfucopy.cpp          -o .OBJ.vfu.yas-debug/vfucopy.o
.OBJ.vfu.yas-debug/vfudir.o: vfudir.cpp  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfudir.cpp           -o .OBJ.vfu.yas-debug/vfudir.o
.OBJ.vfu.yas-debug/vfufiles.o: vfufiles.cpp  vfufiles.cpp vfufiles.h vfu.h vfusetup.h vfusys.h vfuopt.h \
 see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfufiles.cpp         -o .OBJ.vfu.yas-debug/vfufiles.o
.OBJ.vfu.yas-debug/vfumenu.o: vfumenu.cpp  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfumenu.h vfuview.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfumenu.cpp          -o .OBJ.vfu.yas-debug/vfumenu.o
.OBJ.vfu.yas-debug/vfuopt.o: vfuopt.cpp  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfudir.h vfuview.h vfumenu.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfuopt.cpp           -o .OBJ.vfu.yas-debug/vfuopt.o
.OBJ.vfu.yas-debug/vfusys.o: vfusys.cpp  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfusys.cpp           -o .OBJ.vfu.yas-debug/vfusys.o
.OBJ.vfu.yas-debug/vfutools.o: vfutools.cpp  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
 vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfutools.cpp         -o .OBJ.vfu.yas-debug/vfutools.o
.OBJ.vfu.yas-debug/vfuuti.o: vfuuti.cpp  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfufiles.h \
 vfumenu.h vfudir.h vfuopt.h see.h vfuview.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfuuti.cpp           -o .OBJ.vfu.yas-debug/vfuuti.o
.OBJ.vfu.yas-debug/vfuview.o: vfuview.cpp  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_4) $(CFLAGS_4) $(CCFLAGS_4) -c vfuview.cpp          -o .OBJ.vfu.yas-debug/vfuview.o


mm_update:
	


### MAKEMAKE ENDS HERE #########################################################

