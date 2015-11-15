DIR_BANG?=c:/m/n2proj/bang
CPPFLAGS += -I./linux-gtk/sys

include $(DIR_BANG)/build/site.mak

all:: bangnylon$(EXT_SO)

CPPFLAGS += --std=c++11 -O2

ifeq (1,$(USE_GC))
 LDFLAGS_GC=-L$(DIR_BOEHM_LIB) -lgc
 CPPFLAGS_GC=-D USE_GC=1 -I $(DIR_BOEHM_HDR) $(LDFLAGS_GC)
endif

nylonsys.o: nylonsys.cpp $(DIR_BANG)/bang.h
	$(CXX) $(CPPFLAGS) -I$(DIR_BANG) -c $<

bangnylon$(EXT_SO): nylonsys.o $(DIR_BANG)/libbang$(EXT_SO)
	$(CXX) $< -shared -L $(DIR_BANG) -lbang -lpthread -o $@ 
