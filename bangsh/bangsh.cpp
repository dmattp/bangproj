#define BANGSH_VERSION "0.001"

#include <iostream>

#include "bang.h"

namespace Bang
{
    class RegurgeString  : public Bang::RegurgeIo
    {
        std::string str_;
        bool atEof_;
    public:
        RegurgeString( const std::string& str )
        : str_( str ),
          atEof_( str.size() < 1 )
        {
        }
        char getc()
        {
            int icud = RegurgeIo::getcud();

            if (icud != EOF)
                return icud;

            if (atEof_)
                throw Bang::ErrorEof();
        
            if (str_.size() < 1)
            {
                atEof_ = true;
                return 0x0a;
            }
            else
            {
                char rc = str_.front();
                str_.erase( str_.begin() );
                return rc;
            }
        }
    };
}


namespace Bangsh
{
    Bang::Thread thread;

    void eval_bang_code( Bang::Stack& stack, const Bang::RunContext& ctx )
    {
        Bang::InteractiveEnvironment interact;

        const char* fname = "/tmp/dostuff.bang";

        // interact.repl_prompt();
    
        Bang::ParsingContext parsectx( interact );

        const auto& v = stack.pop();
        const auto& vstr = v.tostr();
        
        do
        {
            try
            {
                Bang::RegurgeString strmFile( vstr );
//                Bang::RequireKeyword requireMain( fname );
                auto prog = ParseToProgram( parsectx, strmFile, false, nullptr );
//                Bang::Stack stack;
                stack.giveTo( thread.stack );
                RunProgram( &thread, prog, std::shared_ptr<Bang::Upvalue>() );
                thread.stack.giveTo( stack );
                // thread.stack.dump( std::cout );
            }
            catch( const std::exception& e )
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
        while (interact.bEof);
    }
    
    void bangsh_lookup( Bang::Stack& stack_, const Bang::RunContext& rc_ )
    {
        const auto& v = stack_.pop();
        if (v.isstr())
        {
            const std::string& msg = v.tostr();
            if (msg == "eval")
                stack_.push( &eval_bang_code );
        }
    }
}




extern "C" DLLEXPORT
void bang_open( Bang::Stack *stack_, const Bang::RunContext* rc_ )
{
//    std::cerr << "got call to bang_open in radradio.cpp stack=0x"<<std::hex << (void*)stack_ << std::dec<<  "\n";
//    stack_->push( 1234 );
//    stack_->push( 3.14159 );
//    std::cerr << "pushed number\n";
    stack_->push( &Bangsh::bangsh_lookup );
}
