# use this to disable flto optimizations:
#   make NO_FLTO=1
# and this to enable verbose mode:
#   make V=1

STRIP?=strip
PKG_CONFIG?=pkg-config

PCRE08_CC?=$(shell $(PKG_CONFIG) --cflags libpcre2-8)
PCRE08_LD?=$(shell $(PKG_CONFIG) --libs libpcre2-8)
PCRE32_CC?=$(shell $(PKG_CONFIG) --cflags libpcre2-32)
PCRE32_LD?=$(shell $(PKG_CONFIG) --libs libpcre2-32)
YASCREEN_CC?=$(shell $(PKG_CONFIG) --cflags yascreen)
YASCREEN_LD?=$(shell $(PKG_CONFIG) --libs yascreen)
NCURSES_CC?=$(shell $(PKG_CONFIG) --cflags ncursesw)
NCURSES_LD?=$(shell $(PKG_CONFIG) --libs ncursesw)

BINS:=vfu vfu.yas
ifeq ($(YASCREEN_LD),)
	BINS:=$(filter-out vfu.yas,$(BINS)
endif
ifeq ($(NCURSES_LD),)
	BINS:=$(filter-out vfu,$(BINS)
endif

all: $(BINS)

SRCS:=\
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
	vfuview.cpp
OBJS:=$(SRCS:.cpp=.o)
DEPS:=$(OBJS:.o=.d)

ifndef NO_FLTO
CXXFLAGS?=-O3 -fno-stack-protector -mno-stackrealign
CXXFLAGS+=-flto=auto
else
CXXFLAGS?=-O3 -fno-stack-protector -mno-stackrealign
endif

# some architectures do not have -mno-stackrealign
HAVESREA:=$(shell if $(CXX) -mno-stackrealign -xc -c /dev/null -o /dev/null >/dev/null 2>/dev/null;then echo yes;else echo no;fi)
# old comiplers do not have -Wdate-time
HAVEWDTI:=$(shell if $(CXX) -Wdate-time -xc -c /dev/null -o /dev/null >/dev/null 2>/dev/null;then echo yes;else echo no;fi)

MYCXXFLAGS:=$(CPPFLAGS) $(CXXFLAGS) $(PCRE08_CC) $(PCRE32_CC) $(YASCREEN_CC) $(NCURSES_CC) -Wall -Wextra -Wformat -Werror=format-security -Wdate-time -D_FORTIFY_SOURCE=2 -fPIE -I. -I../vstring -I../vslib
ifeq ("$(HAVESREA)","no")
MYCXXFLAGS:=$(filter-out -mno-stackrealign,$(MYCXXFLAGS))
endif
ifeq ("$(HAVEWDTI)","no")
MYCXXFLAGS:=$(filter-out -Wdate-time,$(MYCXXFLAGS))
endif

MYLDFLAGS:=$(MYCXXFLAGS) $(LDFLAGS) -fPIE -pie
MYLIBS:=$(LIBS) $(PCRE08_LD) $(PCRE32_LD)

ifeq ("$(V)","1")
Q:=
E:=@true
else
Q:=@
E:=@echo
endif

%.o: %.cpp
	$(E) DE $@
	$(Q)$(CXX) $(MYCXXFLAGS) -MM -MT $@ -MF $(patsubst %.o,%.d,$@) $<
	$(E) CXX $@
	$(Q)$(CXX) $(MYCXXFLAGS) -c -o $@ $<

VFUOBJ:=\
	see.o \
	vfu.o \
	vfuarc.o \
	vfucopy.o \
	vfudir.o \
	vfufiles.o \
	vfumenu.o \
	vfuopt.o \
	vfusys.o \
	vfutools.o \
	vfuuti.o \
	vfuview.o

vfu: $(VFUOBJ) ../vstring/libvstring.a ../vslib/libvslib.a ../vslib/libvscon.a
	$(E) LD $@
	$(Q)$(CXX) -o $@ $(MYLDFLAGS) $(VFUOBJ) $(MYLIBS) $(NCURSES_LD) -L../vstring -lvstring -L../vslib -lvslib -L../vslib -lvscon

vfu.yas: $(VFUOBJ) ../vstring/libvstring.a ../vslib/libvslib.a ../vslib/libvscony.a
	$(E) LD $@
	$(Q)$(CXX) -o $@ $(MYLDFLAGS) $(VFUOBJ) $(MYLIBS) $(YASCREEN_LD) -L../vstring -lvstring -L../vslib -lvslib -L../vslib -lvscony

clean:
	$(E) CLEAN
	$(Q) rm -f *.a *.o *.d t/test t/*.o

re:
	$(Q)$(MAKE) --no-print-directory clean
	$(Q)$(MAKE) --no-print-directory -j

-include $(DEPS)

.PHONY: all clean re
