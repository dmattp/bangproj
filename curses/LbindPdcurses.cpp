

#if _WINDOWS
# include "stdafx.h"
# include <WTypes.h>
#endif

#include "bang.h"
#include "curses.h"

#include <iostream>
#include <string>
#include <functional>

//#include "threadrunner.h"
#include "nylon-sysport-threaded.h"


namespace {

    class Window;

    class Color
    {
        friend class Window;
        int pair_;
    public:
        Color( int p, int c1, int c2 )
        : pair_( p )
        {
            init_pair( p, c1, c2 );
        }
    };
    
    class Window : public Bang::Function
    {
        WINDOW* w_;
    public:
        Window( int h, int w, int y, int x) 
        {
            w_ = newwin( h, w, y, x );
        }

        Window()
        : w_( stdscr )
        {
        }
        
        Window( WINDOW* w )
        : w_( w )
        {}

        /* doing this because I need a way to force a window
         * to go away e.g., for popups?  I hate it, because it leaves the
         * object in an invalid state, but until I know how to hide a window
         * without calling delwin it's all i've got. */
        void forcedelete()
        {
            if( w_  && w_ != stdscr )
            {
                delwin( w_ );
                w_ = 0;
            }
        }

        ~Window()
        {
//            std::cout << "Destroyed window w=" << (void*)w_ << std::endl;
            this->forcedelete();
        }
        

        int gety() { int x; int y; getyx(w_,y,x); return y; }
        int getx() { int x; int y; getyx(w_,y,x); return x; }
       
        void getmaxx_( Bang::Stack& s) { s.push( (double)getmaxx(w_) ); }
        void getmaxy_( Bang::Stack& s) { s.push( (double)getmaxy(w_) ); }
       
        void printw( const std::string& t ) { wprintw( w_, t.c_str() ); }
        void mvprintw( int y, int x, const std::string& t ) { mvwprintw( w_, y, x, t.c_str() ); }

        void addstr_( Bang::Stack& s )
        {
            const Bang::Value& v = s.pop();
            const std::string& t = v.tostr();
            waddnstr( w_, t.c_str(), t.length() );
        }
        
        void mvaddstr_( Bang::Stack& s)
        {
            const Bang::Value v = s.pop();
            const auto vstr = v.tostr();
            int y = s.pop().tonum();
            int x = s.pop().tonum();
            mvwaddnstr( w_, y, x, vstr.c_str(), vstr.length() );
        }
        
        void refresh( Bang::Stack& ) { wrefresh( w_ ); }
        
        void move( Bang::Stack& s )
        {
            int y = s.pop().tonum();
            int x = s.pop().tonum();
            wmove( w_, y, x );
        }
        
        std::string getstr_() {
            char str[8192];
            wgetnstr( w_, str, sizeof(str)-1 );
            str[sizeof(str)-1] = '\0';
            return str;
        }
        std::string mvgetstr__( int y, int x ) {
            char str[8192];
            mvwgetnstr( w_, y, x, str, sizeof(str)-1 );
            str[sizeof(str)-1] = '\0';
            return str;
        }
        void border_( char ls, char rs, char ts, char bs, char tl, char tr, char bl, char br )
        {
            wborder( w_, ls, rs, ts, bs, tl, tr, bl, br );
        }
        void stdbox_()
        {
            box( w_, ACS_VLINE, ACS_HLINE );
        }
        void box_( int v, int h )
        {
            box( w_, v, h );
        }
        void clrtoeol_()
        {
            wclrtoeol( w_ );
        }
        void clear_() { wclear( w_ ); }

        void attron_( Color* c )  {
            // init_pair( 1, COLOR_BLUE, COLOR_YELLOW );
            // wattron(w_, COLOR_PAIR(1) );
            wattron(w_, COLOR_PAIR(c->pair_) );
        }
        void attron_( int attr )  {
            // init_pair( 1, COLOR_BLUE, COLOR_YELLOW );
            // wattron(w_, COLOR_PAIR(1) );
            wattron(w_, attr );
        }
        void attroff_( Color* c ) {
            //wattroff(w_, COLOR_PAIR(1) );
            wattroff(w_, COLOR_PAIR(c->pair_) );
        }
        void attroff_( int attr ) {
            //wattroff(w_, COLOR_PAIR(1) );
            wattroff(w_, attr);
        }
        void attr_set_( Color* c, int attr ) 
        {
            wattr_set(w_, attr, COLOR_PAIR(c->pair_), (void *)0);
        }
        void vline_( chtype c, int len )
        {
            wvline( w_, c, len );
        }
        void hline_( chtype c, int len )
        {
            whline( w_, c, len );
        }
        
        void resize( Bang::Stack& s )
        {
            int lines = s.pop().tonum();
            int col   = s.pop().tonum();
            wresize(w_, lines, col);
        }

        void redraw( Bang::Stack& ) { redrawwin(w_); }
        void mvwin( Bang::Stack& s )
        {
            int row = s.pop().tonum();
            int col = s.pop().tonum();
            ::mvwin(w_, row, col);
        }

        class BangCppFun : public Bang::Function
        {
            std::function<void(Bang::Stack&)> cppfun_;
        public:
            BangCppFun( const std::function<void(Bang::Stack&)>& cbfun )
            : cppfun_( cbfun )
            {
            }
            void apply( Bang::Stack& s ) // , CLOSURE_CREF rc )
            {
                cppfun_( s );
            }
        };
        
        virtual void apply( Bang::Stack& s )
        {
            const Bang::Value& v = s.pop();
            if (!v.isstr())
                throw std::runtime_error("BangRadioFile . operator expects string");

            const auto& str = v.tostr();
                
            auto memfun =
                (  str == "mvwin"    ? &Window::mvwin
                :  str == "redraw"   ? &Window::redraw
                :  str == "resize"   ? &Window::resize
                :  str == "addstr"   ? &Window::addstr_
                :  str == "mvaddstr" ? &Window::mvaddstr_
                :  str == "addstr"   ? &Window::addstr_
                :  str == "refresh"  ? &Window::refresh
                :  str == "move"     ? &Window::move
                :  str == "getmaxx"  ? &Window::getmaxx_
                :  str == "getmaxy"  ? &Window::getmaxy_
                :   nullptr
                );

            if (memfun)
            {
                auto pushit = NEW_BANGFUN(BangCppFun, std::bind( memfun, this, std::placeholders::_1 ) );
                s.push( STATIC_CAST_TO_BANGFUN(pushit) );
            }
        }
    };

    void make_window( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        auto pushit = NEW_BANGFUN(Window); //  std::bind( memfun, this, std::placeholders::_1 ) );
        s.push( STATIC_CAST_TO_BANGFUN(pushit) );
    }

    void initscr_( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        WINDOW* w = ::initscr();
        refresh();
    }

    int getch_()
    {
        return wgetch(stdscr);
    }

    class Key{};

    class Lines{};
    

#if 0    
    void do_getch_loop( ThreadReporter reporter )
    {
//        std::cout << "infinitethreadness" << std::endl;
        while (true)
        {
            auto c = wgetch(stdscr);
            reporter.report(c);
        }
//        std::cout << "exit infinitethreadness" << std::endl;
    }
#endif

    int color_pairs()
    {
        return COLOR_PAIRS;
    }

   // dmp/141005: On linux, lots of these are implemented
   // as macros, so I have to write a distinct function to
   // create a binding.
   void beep_()    { beep();    }
   void flash_()   { flash();   }
   void endwin_()  { endwin();  }
   void clear_()   { clear();   }
   void start_color_()   { start_color();   }

    void refresh_(Bang::Stack& , const Bang::RunContext&) { refresh(); }
    void noecho_(Bang::Stack& , const Bang::RunContext&) { ::noecho(); }
    void raw_(Bang::Stack& , const Bang::RunContext&) { ::raw(); }

    void wait_getch_thread( Bang::Thread* bthread, Bang::Value* v)
    {
        auto c = wgetch(stdscr);
	//        NylonLockBangThreads(true);
//        std::cerr << "C++ running callback\n";
        bthread->stack.push( (double)c );
        Bang::CallIntoSuspendedCoroutine( bthread, v->toboundfun() );
//        std::cerr << "C++ returned from callback\n";
//        NylonLockBangThreads(false);
        delete v;
    };

    void nylon_getch( Bang::Stack& stack, const Bang::RunContext& ctx)
    {
        //    const auto& v = stack.pop();
//        std::cerr << "C++ starting threadrunner\n";
        Bang::Value* v = new Bang::Value(stack.pop());
//        auto bound = std::bind( &nylon_do_getch, ctx.thread, v );
        auto bound = std::bind( &wait_getch_thread, ctx.thread, v ); //, ctx.thread, v );
        auto pthr = new ThreadRunner(bound);
//        pthr->run();
//        return StdFunctionCaller( std::bind( &do_getch_loop, std::placeholders::_1 ) );
    }

    void curgetch( Bang::Stack& stack, const Bang::RunContext& ctx)
    {
        auto c = wgetch(stdscr);
        stack.push( (double)c );
    }


    void keypad_( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        keypad( stdscr, s.pop().tobool() );
    }

    void Curses_lookup( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const Bang::Value& v = s.pop();
        if (!v.isstr())
            throw std::runtime_error("Curses library '.' operator expects string");
        const auto& str = v.tostr();

        const Bang::tfn_primitive p =
            (  str == "refresh"     ? &refresh_
            :  str == "nylon-getch" ? &nylon_getch
            :  str == "getch" ? &curgetch
            :  str == "make-window" ? &make_window
            :  str == "noecho"      ? &noecho_
            :  str == "raw"         ? &raw_
            :  str == "keypad"      ? &keypad_
            :  str == "initscr"     ? &initscr_
            :  nullptr
            );

        if (p)
            s.push( p );
        else
            throw std::runtime_error("String library does not implement" + str);
    }
   
} // end, namespace anonymous





extern "C" DLLEXPORT void bang_open( Bang::Stack* stack, const Bang::RunContext* )
{
    stack->push( &Curses_lookup );
}

#if 0
{
   using namespace luabind;

   // std::cout << "Nylon open Pdcurses" << std::endl;

   // open( L ); // wow, don't do this from a coroutine.  make sure the main prog inits luabind.
   // also, dont do this after somebody else has done it, at least with newer versions of luabind; it replaces the
   // table of all registered classes.

   module( L, "Pdcurses" ) [
       namespace_("Static") [
//           def("initscr",&initscr_)
#if _WINDOWS
           ,def("mouse_on",&::mouse_on)
#endif
           ,def("endwin",     &endwin_)
           ,def("refresh",    &refresh_)
           ,def("curs_set",   &::curs_set)
           ,def("getch",      &getch_)
           ,def("noecho",     &::noecho)
           ,def("noraw",      &::noraw)
           ,def("nocbreak",   &::nocbreak)
           ,def("cbreak",   &::cbreak)
           ,def("raw",        &::raw)
           ,def("clear",      &clear_)
           ,def("keypad",     &keypad_)
           ,def("beep",       &beep_)
           ,def("flash",      &flash_)
           ,def("start_color",&start_color_)
           ,def("color_pairs",&color_pairs)
           ,def("cthread_getch_loop",&::cthread_getch_loop)           
       ],
       class_<Key>("key") 
           .enum_("constants") [

               value("dc",     KEY_DC),    // delete key
               value("up",     KEY_UP),
               value("down",   KEY_DOWN),
               value("npage",  KEY_NPAGE),
               value("ppage",  KEY_PPAGE),
               value("end",    KEY_END),
               value("home",   KEY_HOME),
               value("enter",  KEY_ENTER),
               value("left",   KEY_LEFT),
               value("right",  KEY_RIGHT),
               value("mouse",  KEY_MOUSE),               
               value("backspace", KEY_BACKSPACE),
               value("sleft",  KEY_SLEFT),
               value("sright", KEY_SRIGHT),
               value("send",   KEY_SEND),
               value("shome",  KEY_SHOME),
#if _WINDOWS               
               value("sup",    KEY_SUP),
               value("sdown",  KEY_SDOWN),
               value("alt_del",   ALT_DEL ),       
               value("alt_ins",   ALT_INS),       
               value("alt_tab",   ALT_TAB),       
               value("alt_home",  ALT_HOME),      
               value("alt_pgup",  ALT_PGUP),      
               value("alt_pgdn",  ALT_PGDN),      
               value("alt_end",   ALT_END),       
               value("alt_up",    ALT_UP),        
               value("alt_down",  ALT_DOWN),      
               value("alt_right", ALT_RIGHT),     
               value("alt_left",  ALT_LEFT),      
               value("alt_enter", ALT_ENTER),     
               value("alt_esc",   ALT_ESC),       
               value("alt_bksp",  ALT_BKSP),       
               value("alt_left",  KEY_ALT_L),       
               value("alt_right", KEY_ALT_R),       
               value("ctl_del",   CTL_DEL),       
               value("ctl_ins",   CTL_INS),
               value("ctl_tab",   CTL_TAB),       
               value("ctl_up",    CTL_UP),        
               value("ctl_down",  CTL_DOWN),      
               value("ctl_enter", CTL_ENTER),       
               value("ctl_left",  CTL_LEFT),       
               value("ctl_right", CTL_RIGHT),       
               value("ctl_pgup",  CTL_PGUP),       
               value("ctl_pddn",  CTL_PGDN),       
               value("ctl_home",  CTL_HOME),       
               value("ctl_end",   CTL_END),
#endif                
               value("f0", KEY_F0)
           ],
       class_<Lines>("Lines") 
           .enum_("constants") [
               value("vline", ACS_VLINE),
               value("hline", ACS_HLINE),
               value("ulcorner", ACS_ULCORNER),
               value("urcorner", ACS_URCORNER),
               value("llcorner", ACS_LLCORNER),
               value("lrcorner", ACS_LRCORNER)
           ],
       class_<Color>("Color")
       .def( constructor<int,int,int>() )
       .enum_("constants") [
           value("red", COLOR_RED),
           value("blue", COLOR_BLUE),
           value("green", COLOR_GREEN),
           value("black", COLOR_BLACK),
           value("white", COLOR_WHITE),
           value("cyan", COLOR_CYAN), // and cyan is sort of a blue green, I guess, not what I think of as cyan
           value("magenta", COLOR_MAGENTA), // magenta is like, dark blue?
           value("yellow", COLOR_YELLOW), //  yellow just seems to be bold white.

           value("a_bold", A_BOLD), 
           value("a_dim", A_DIM), 
           value("a_reverse", A_REVERSE),
           value("a_blink", A_BLINK)
       ],
       class_<Window>("Window")
       .def( constructor<int,int,int,int>() )
       .def( constructor<>() ) // get window for stdscr
       .def( "getx",        &Window::getx )
       .def( "gety",        &Window::gety )
       .def( "getmaxx",     &Window::getmaxx_ )
       .def( "getmaxy",     &Window::getmaxy_ )
       .def( "printw",      &Window::printw )
       .def( "mvprintw",    &Window::mvprintw )
       .def( "refresh",     &Window::refresh )
       .def( "move",        &Window::move )
       .def( "addstr",      &Window::addstr_ )
       .def( "mvaddstr",    &Window::mvaddstr_ )
       .def( "getstr",      &Window::getstr_ )
       .def( "mvgetstr",    &Window::mvgetstr__ )
       .def( "border",      &Window::border_ )
       .def( "stdbox_",     &Window::stdbox_ )
       .def( "box",     &Window::box_ )
       .def( "clear",       &Window::clear_)
       .def( "clrtoeol",    &Window::clrtoeol_)
       .def( "attron",      (void (Window::*)(int))&Window::attron_)
       .def( "attron",      (void (Window::*)(Color*))&Window::attron_)
       .def( "attroff",      (void (Window::*)(int))&Window::attroff_)
       .def( "attroff",      (void (Window::*)(Color*))&Window::attroff_)
       .def( "attr_set",    &Window::attr_set_)
       .def( "forcedelete", &Window::forcedelete)
       .def( "hline",       &Window::hline_)
       .def( "vline",       &Window::vline_)
       .def( "redraw",      &Window::redraw)
       .def( "mvwin",       &Window::mvwin)
       .def( "resize",      &Window::resize)
   ];
   
   //std::cout << "Nylon Pdcurses opened" << std::endl;
   return 0;
}
#endif 
