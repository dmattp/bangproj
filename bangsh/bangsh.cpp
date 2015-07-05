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
    Bang::SHAREDUPVALUE lastUvChain;
//     const Bang::Ast::CloseValue* lastUvChain = nullptr;

    class ShParsingContext;

    class ShEofMarker : public Bang::Ast::EofMarker
    {
    public:
        ShParsingContext& parsectx_;
        const Bang::Ast::CloseValue* uvchain_;
        ShEofMarker( ShParsingContext& ctx, const Bang::Ast::CloseValue* uvchain );

        void repl_prompt( Bang::Stack& ) const
        {
            // std::cerr << "ShEofMarker::repl_prompt\n";
        }

        virtual void report_error( const std::exception& e ) const
        {
            std::cerr << "CRAP.01a Error: " << e.what() << std::endl;
        }
        
        virtual Bang::Ast::Program* getNextProgram( Bang::SHAREDUPVALUE closeValueChain ) const
        {
//            std::cerr << "ShEofMarker::getNextProgram\n";
            lastUvChain = closeValueChain;
//             while (true)
//             {
//                 try {
//                     auto rc = parseStdinToProgram( parsectx_, closeValueChain );
//                     return rc;
//                 } catch (std::exception&e) {
//                     std::cout << "a08 Error: " << e.what() << "\n";
//                     parsectx_.interact.repl_prompt();
//                 }
//             }
            return nullptr; // allow break prog, we'll pick the upval chain back up on re-entry
        }
        
        virtual void dump( int level, std::ostream& o ) const
        {
//            indentlevel(level, o);
            o << "<<< EOF >>>\n";
        }
    };
    
    class ShParsingContext : public Bang::ParsingContext
    {
    public:
        ShParsingContext( Bang::InteractiveEnvironment& i )
        : Bang::ParsingContext(i)
        {
        }
        Bang::Ast::Base* hitEof( const Bang::Ast::CloseValue* uvchain )
        {
//            std::cerr << "ShParsingContext::hitEof\n";
            return new ShEofMarker( *this, uvchain );
        }
    };

    ShEofMarker::ShEofMarker( ShParsingContext& ctx, const Bang::Ast::CloseValue* uvchain )
    : Bang::Ast::EofMarker( ctx ),
      parsectx_(ctx)
    {}
    
    void eval_more_bang_code( Bang::Stack& stack, const Bang::RunContext& ctx )
    {
        Bang::InteractiveEnvironment interact;

        const char* fname = "/tmp/dostuff.bang";

        const auto& errHandler = stack.pop();
        
        ShParsingContext parsectx( interact );

        const auto& v = stack.pop();
        const auto& vstr = v.tostr();
        
//        do
        {
            try
            {
                Bang::RegurgeString strmFile( vstr );
                const Bang::Ast::CloseValue* closeValueChain = lastUvChain ? lastUvChain->upvalParseChain() : static_cast<const Bang::Ast::CloseValue*>(nullptr);
                auto prog = Bang::ParseToProgram( parsectx, strmFile, false, closeValueChain );
                stack.giveTo( thread.stack );
                RunProgram( &thread, prog, lastUvChain );
                thread.stack.giveTo( stack );
                // thread.stack.dump( std::cout );
            }
            catch( const std::exception& e )
            {
                auto bprog = errHandler.toboundfun();
                stack.push( std::string(e.what()) );
                Bang::CallIntoSuspendedCoroutine( ctx.thread, bprog ); // ->program_, bprog->upvalues_ );
//                std::cerr << "Error: " << e.what() << std::endl;
            }
        }
//        while (interact.bEof);
    }
    
    void bangsh_lookup( Bang::Stack& stack_, const Bang::RunContext& rc_ )
    {
        const auto& v = stack_.pop();
        if (v.isstr())
        {
            const std::string& msg = v.tostr();
            if (msg == "eval")
                stack_.push( &eval_more_bang_code );
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
