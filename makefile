### MAKEMAKE STARTS HERE #########################################
#
# Created by makemake.pl on Sun Dec 31 13:12:04 2000
#
##################################################################

### GLOBAL TARGETS ###############################################

default: all


re: rebuild

all: vfu 

clean: clean-vfu 

rebuild: rebuild-vfu 

link: link-vfu 

### TARGET: vfu #########################################

CC_0      = g++
LD_0      = g++
AR_0      = ar rvs
CFLAGS_0  = 
CCFLAGS_0 = -I../vslib -I/usr/include/ncurses -O2 $(CCDEF)
LDFLAGS_0 = -L../vslib -lvslib -lvscon -lncurses $(LDDEF)
ARFLAGS_0 = 
TARGET_0  = vfu

# IN.SRC_0 = *.cpp
# IN.HDR_0 = *.h

### SOURCES FOR TARGET 0: vfu #################################

SRC_0= \
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

#### OBJECTS FOR TARGET 0: vfu ################################

OBJ_0= \
     .OBJ.0.vfu/see.o \
     .OBJ.0.vfu/vfu.o \
     .OBJ.0.vfu/vfuarc.o \
     .OBJ.0.vfu/vfucopy.o \
     .OBJ.0.vfu/vfudir.o \
     .OBJ.0.vfu/vfufiles.o \
     .OBJ.0.vfu/vfumenu.o \
     .OBJ.0.vfu/vfuopt.o \
     .OBJ.0.vfu/vfusys.o \
     .OBJ.0.vfu/vfutools.o \
     .OBJ.0.vfu/vfuuti.o \
     .OBJ.0.vfu/vfuview.o \

### TARGET DEFINITION FOR TARGET 0: vfu #######################

.OBJ.0.vfu: 
	mkdir -p .OBJ.0.vfu

vfu: .OBJ.0.vfu $(OBJ_0)
	$(LD_0) $(OBJ_0) $(LDFLAGS_0) -o $(TARGET_0)

clean-vfu: 
	rm -f $(TARGET_0)
	rm -rf .OBJ.0.vfu

rebuild-vfu: clean-vfu vfu

link-vfu: .OBJ.0.vfu $(OBJ_0)
	rm -f vfu
	$(LD_0) $(OBJ_0) $(LDFLAGS_0) -o $(TARGET_0)

### TARGET OBJECTS FOR TARGET 0: vfu ##########################

.OBJ.0.vfu/see.o:  see.cpp see.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c see.cpp -o .OBJ.0.vfu/see.o
.OBJ.0.vfu/vfu.o:  vfu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfufiles.h vfucopy.h vfudir.h vfuview.h vfumenu.h vfuarc.h vfutools.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfu.cpp -o .OBJ.0.vfu/vfu.o
.OBJ.0.vfu/vfuarc.o:  vfuarc.cpp vfuarc.h vfu.h vfusetup.h vfusys.h vfuuti.h \
 vfuopt.h see.h vfudir.h vfucopy.h vfufiles.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfuarc.cpp -o .OBJ.0.vfu/vfuarc.o
.OBJ.0.vfu/vfucopy.o:  vfucopy.cpp vfu.h vfusetup.h vfusys.h vfudir.h vfumenu.h \
 vfuuti.h vfufiles.h vfuview.h vfuopt.h see.h vfucopy.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfucopy.cpp -o .OBJ.0.vfu/vfucopy.o
.OBJ.0.vfu/vfudir.o:  vfudir.cpp vfudir.h vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfufiles.h vfuview.h vfumenu.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfudir.cpp -o .OBJ.0.vfu/vfudir.o
.OBJ.0.vfu/vfufiles.o:  vfufiles.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuopt.h \
 see.h vfuuti.h vfuview.h vfumenu.h vfudir.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfufiles.cpp -o .OBJ.0.vfu/vfufiles.o
.OBJ.0.vfu/vfumenu.o:  vfumenu.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h \
 vfuuti.h vfumenu.h vfuview.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfumenu.cpp -o .OBJ.0.vfu/vfumenu.o
.OBJ.0.vfu/vfuopt.o:  vfuopt.cpp vfu.h vfusetup.h vfusys.h vfuopt.h see.h vfuuti.h \
 vfudir.h vfuview.h vfumenu.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfuopt.cpp -o .OBJ.0.vfu/vfuopt.o
.OBJ.0.vfu/vfusys.o:  vfusys.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfusys.cpp -o .OBJ.0.vfu/vfusys.o
.OBJ.0.vfu/vfutools.o:  vfutools.cpp vfumenu.h vfuuti.h vfu.h vfusetup.h vfusys.h \
 vfucopy.h vfuview.h vfuopt.h see.h vfufiles.h vfutools.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfutools.cpp -o .OBJ.0.vfu/vfutools.o
.OBJ.0.vfu/vfuuti.o:  vfuuti.cpp vfu.h vfusetup.h vfusys.h vfuuti.h vfumenu.h \
 vfudir.h vfuopt.h see.h vfuview.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfuuti.cpp -o .OBJ.0.vfu/vfuuti.o
.OBJ.0.vfu/vfuview.o:  vfuview.cpp vfu.h vfusetup.h vfusys.h vfufiles.h vfuview.h \
 vfuopt.h see.h vfuuti.h
	$(CC_0) $(CFLAGS_0) $(CCFLAGS_0) -c vfuview.cpp -o .OBJ.0.vfu/vfuview.o


### END ##########################################################
