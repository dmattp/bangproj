DIR_BANG?=c:/m/n2proj/bang
DIR_BANGNYLON?=../../bangproj/bangnylon

include $(DIR_BANG)/build/site.mak
include $(DIR_BANGNYLON)/build/use-nylon.mak

all:: bangcurses$(EXT_SO)

LbindPdcurses.o: LbindPdcurses.cpp  $(DIR_BANG)/bang.h
	$(CXX) $(CPPFLAGS) -I$(DIR_BANG) -c $< -o $@

bangcurses$(EXT_SO): LbindPdcurses.o $(DIR_BANG)/bang.h $(DIR_BANG)/libbang$(EXT_SO)
	$(CXX)  $< -shared -L $(DIR_BANG) -lbang -lncurses -o $@
