//#include "stdafx.h"

#define _WINDOWS
// #define DLLEXPORT __declspec(dllexport)
#include <string>

//#include "nylon-runner.h"
#include "bang.h"
#include "psbind-unmanwrap.h"

namespace {
//     class ShellWrapper
//     {
//     public:
//         luabind::object invoke( lua_State* l, const std::string& cmd )
//         {
//             return pshell_->invoke( l, cmd );
//         }
//         ShellWrapper()
//         : pshell_( CreatePshellUnmanWrap() )
//         {
//         };

//         void do_invoke_loop( lua_State* l, const std::string& cmd, ThreadReporter reporter )
//         {
//             pshell_->invoke_threadreporter( l, cmd, reporter );
//         }
//     };
    
    void run( Bang::Stack& s, const Bang::RunContext& rc)
    {
        const Bang::Value& v = s.pop();
        if (!v.isstr())
            throw std::runtime_error("Math library . operator expects string");
        const auto& str = v.tostr();
        
        std::unique_ptr<PshellUnmanWrap> pshell = CreatePshellUnmanWrap();
        pshell->invoke( s, rc, str );
    }

    void lookup( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const Bang::Value& v = s.pop();
        if (!v.isstr())
            throw std::runtime_error("Math library . operator expects string");
        const auto& str = v.tostr();
        const Bang::tfn_primitive p =
            (  str == "run" ? &run : nullptr );
        if (p)
            s.push( p );
        else
            throw std::runtime_error("Psbind library does not implement" + str);
    }
    
} // end, anonymous namespace



extern "C" DLLEXPORT  int bang_open( Bang::Stack* stack, const Bang::RunContext* )
{
//   using namespace luabind;

//   std::cout << "Lua open psbind " << std::endl;

   // open( L ); // wow, don't do this from a coroutine.  make sure the main prog inits luabind.
   // also, dont do this after somebody else has done it, at least with newer versions of luabind; it replaces the
   // table of all registered classes.

//     module( L, "Psbind" ) [
//         class_<ShellWrapper>("Shell")
//         .def(constructor<>() )
//         .def("invoke",&ShellWrapper::invoke)
//         .def("cthread_invoke",&ShellWrapper::cthread_invoke)
//     ];
    stack->push( &lookup );

   return 0;
}

