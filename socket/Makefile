DIR_BANG?=c:/m/n2proj/bang

include $(DIR_BANG)/build/site.mak
ifeq (,$(BUILD_FOR)) # if site.mak doesn't include a target (linux.mak,win32.mak) default to linux
 include $(DIR_BANG)/build/linux.mak
endif

all:: bangsocket$(EXT_SO)

CPPFLAGS += --std=c++11 -O2

ifeq (1,$(USE_GC))
 LDFLAGS_GC=-L$(DIR_BOEHM_LIB) -lgc
 CPPFLAGS_GC=-D USE_GC=1 -I $(DIR_BOEHM_HDR) $(LDFLAGS_GC)
endif

bangsocket$(EXT_SO): socket.cpp $(DIR_BANG)/bang.h $(DIR_BANG)/libbang$(EXT_SO)
	$(CXX) $(CPPFLAGS) -I$(DIR_BANG) -shared -L $(DIR_BANG) -lbang -o $@ $<
