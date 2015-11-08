#include <ctype.h>
#include <limits.h>
#include <stdio.h>

#include <iostream>
#include <sstream>
#include <string>
#include <list>

#include "bang.h"
#include "arraylib.h"
#include "hashlib.h"




    class RegurgeStream
    {
    public:
        RegurgeStream() {}
        virtual char getc() = 0;
        virtual void accept() = 0;
        virtual void regurg( char ) = 0;
        virtual std::string sayWhere() const { return "(unsure where)"; }
        virtual ~RegurgeStream() {}
    };

    struct ErrorEof
    {
        ErrorEof() {}
    };

    class RegurgeIo  : public RegurgeStream
    {
        std::list<char> regurg_;
    protected:
        DLLEXPORT int getcud();
    public:
        virtual void regurg( char c )
        {
            regurg_.push_back(c);
        }

        void accept() {}
    };




bool iseof(int c)
{
    return c == EOF;
}

struct ErrorNoMatch
{
    ErrorNoMatch() {}
};

    int RegurgeIo::getcud()
    {
        char rv;
        
        if (regurg_.empty())
            return EOF;

        rv = regurg_.back();
        regurg_.pop_back();

        return rv;
    }
    

class StreamMark : public RegurgeStream
{
    RegurgeStream& stream_;
    std::string consumned_;
    //StreamMark( const StreamMark&  );
    StreamMark& operator=( const StreamMark& );
public:
    StreamMark( RegurgeStream& stream )
    : stream_( stream )
    {}
    StreamMark( StreamMark& stream )
    : stream_( stream )
    {}

    virtual std::string sayWhere() const {
        return stream_.sayWhere();
    }

    void dump( std::ostream& out, const std::string& context ) const
    {
        out << "  {{SM @ " << context << "}}: consumned=[[" << consumned_ << "\n";
    }

    char getc()
    {
        char rv = stream_.getc();
        consumned_.push_back(rv);
        return rv;
    }

    void accept()
    {
        consumned_.clear();
    }
    void regurg( char c )
    {
        char lastc = consumned_.back();
        if (lastc != c )
        {
            std::cerr << "last char=" << lastc << " regurged=" << c << std::endl;
            throw std::runtime_error("parser regurged char != last");
        }
        consumned_.pop_back();
        stream_.regurg(c);
    }
    ~StreamMark()
    {
//        std::cerr << "     ~SM: returning(" << consumned_.size() << ") regurg_.size(" << regurg_.size() << ")\n";
        // consumned stuff is pushed back in reverse
        std::for_each( consumned_.rbegin(), consumned_.rend(),
            [&]( char c ) { stream_.regurg(c); } );
    }
}; // end, class StreamMark



class RegurgeFile : public RegurgeIo
{
    std::string filename_;
    FILE* f_;
    struct Location {
        int lineno_;
        int linecol_;
        void advance( char c ) {
//            std::cerr << "ADV: " << c << "\n";
            if (c=='\n') {
                ++lineno_;
                linecol_ = 0;
            } else {
                ++linecol_;
            }
        }
        void rewind( char c ) {
//            std::cerr << "REW: " << c << "\n";
            if (c=='\n') {
                --lineno_;
                linecol_ = -1;
            } else {
                --linecol_;
            }

        }
        Location() : lineno_(1), linecol_(0) {}
    } loc_;
//    Location err_;
public:
    RegurgeFile( const std::string& filename )
    : filename_( filename )
    {
        f_ = fopen( filename.c_str(), "r");
    }
    ~RegurgeFile()
    {
        fclose(f_);
    }
    virtual std::string sayWhere() const
    {
        std::ostringstream oss;
        oss << filename_ << ":" << loc_.lineno_ << " C" << loc_.linecol_;
        return oss.str();
    }

    char getc()
    {
        int icud = RegurgeIo::getcud();

        if (icud != EOF)
        {
            loc_.advance( icud );
            return icud;
        }
        
        int istream = fgetc(f_);
        if (istream==EOF)
            throw ErrorEof();
        else
        {
            loc_.advance( istream );
            return istream;
        }
    }

    virtual void regurg( char c )
    {
        loc_.rewind(c);
        RegurgeIo::regurg(c);
    }
};
    



bool eatwhitespace( StreamMark& stream )
{
    bool bGotAny = false;
    StreamMark mark( stream );
    try
    {
        while (isspace(mark.getc()))
        {
            bGotAny = true;
            mark.accept();
        }
    }
    catch (const ErrorEof& )
    {
        bGotAny = true;
    }

    return bGotAny;
}

    class RegurgeString  : public RegurgeIo
    {
        const std::string str_;
        bool atEof_;
        size_t currndx_;
        size_t maxndx_;
    public:
        RegurgeString( const std::string& str )
        : str_( str ),
          currndx_( 0 ),
          maxndx_( str.size() ),
          atEof_( maxndx_ == 0 )
        {
        }
        char getc()
        {
            int icud = RegurgeIo::getcud();

            if (icud != EOF)
                return icud;

            if (atEof_)
                throw Bang::ErrorEof();
        
            if (currndx_ == maxndx_)
            {
                atEof_ = true;
                return 0x0a;
            }
            else
            {
                char rc = str_[currndx_]; // .front();
                ++currndx_;
                // str_.erase( str_.begin() );
                return rc;
            }
        }
    };


    namespace JsonNs
    {
        class JsonNull : public Bang::Function
        {
        public:
            JsonNull() {}
            virtual void apply( Bang::Stack& s ) {}
        };
        
        Bang::gcptr<Bang::Function> null()
        {
            static Bang::gcptr<Bang::Function> gcp( STATIC_CAST_TO_BANGFUN( NEW_BANGFUN( JsonNull ) ) );
            return gcp;
        }
    }

class Parser
{
public:
    class Number
    {
        double value_;
    public:
        Number( StreamMark& stream )
        {
            StreamMark mark(stream);

            bool gotDecimal = false;
            bool gotOne = false;
            bool isNegative = false;
            double val = 0;
            double fractional_adj = 0.1;
            char c = mark.getc();

            if (c=='-')
            {
                c = mark.getc();
                if (!isdigit(c))
                    throw ErrorNoMatch();
                mark.accept();
                isNegative = true;
            }

            while (isdigit(c))
            {
                gotOne = true;
                mark.accept();
                if (gotDecimal)
                {
                    val = val + (fractional_adj * (c - '0'));
                    fractional_adj = fractional_adj / 10;
                }
                else    
                {
                    val = val * 10 + (c - '0');
                }
                c = mark.getc();
                if (!gotDecimal)
                {
                    if (c=='.')
                    {
                        gotDecimal = true;
                        c = mark.getc(); 
                        if (!isdigit(c)) // decimal point must be followed by a number
                            break;
                    }
                }
            }

            if (c == 'e' || c == 'E') // scientific notation
            {
                bool negativeExp = false;
                c = mark.getc();

                if (c == '+')
                    c = mark.getc();

                if (c == '-')
                {
                    c = mark.getc();
                    if (isdigit(c))
                    {
                        negativeExp = true;
                        mark.accept();
                    }
                }
                double exp = 0;
                while (isdigit(c))
                {
                    exp = exp * 10 + (c - '0');
                    mark.accept();
                    c = mark.getc();
                }
                if (negativeExp)
                    exp = exp * -1;
                
                val = val * ::pow(10,exp);
            }
            
            value_ = val;
            if (isNegative)
                value_ = value_ * -1.0;
            if (!gotOne)
                throw ErrorNoMatch();
        }
        double value() { return value_; }
    }; // end, class Number


    static bool eatReservedWord( const std::string& rw, StreamMark& stream )
    {
        StreamMark mark(stream);

        for (auto it = rw.begin(); it != rw.end(); ++it)
        {
            if (mark.getc() != *it)
                return false;
        }
        mark.accept();
        return true;
    }

    class Boolean
    {
        bool value_;
    public:
        Boolean( StreamMark& stream )
        {
            StreamMark mark(stream);

            if (eatReservedWord("true", mark))
                value_ = true;
            else if (eatReservedWord("false", mark))
                value_ = false;
            else
                throw ErrorNoMatch();

            mark.accept();
        }
        bool value() { return value_; }
    };

    class ParseString
    {
        std::string content_;
    public:
        ParseString( StreamMark& stream )
        {
            StreamMark mark(stream);
            char delim = mark.getc();
            if (delim == '\'' || delim == '"') // simple strings, no escaping etc
            {
                auto bi = std::back_inserter(content_);
                for (char cstr = mark.getc(); cstr != delim; cstr = mark.getc())
                {
                    if (cstr == '\\')
                    {
                        char c = mark.getc();
                        switch (c) {
                            case 'n':  cstr = '\n'; break;
                            case '"':  cstr = '"';  break;
                            case '\'': cstr = '\''; break;
                            case '\\': cstr = '\\'; break;
                            case 'r':  cstr = '\r'; break;
                            default:
                                //*bi++ = '\\';
                                cstr = c;
                                break;
                        }
                    }
                    *bi++ = cstr;
                }
                mark.accept();
            }
            else
                throw ErrorNoMatch();
        }
        const std::string& content() { return content_; }
    };

    class JsonValue
    {
        Bang::Value v_;
    public:
        JsonValue( StreamMark& stream );
        const Bang::Value& bangValue() { return v_; }
    };

    class JsonArray
    {
    public:
        Bang::Value v_;
        JsonArray( StreamMark& stream )
        {
            eatwhitespace( stream );
            
            StreamMark mark(stream);
            
            char delim = mark.getc();
            
            if (delim == '[')
            {
                mark.accept();
                auto pArray = NEW_BANGFUN( Bang::Array );
                v_ = STATIC_CAST_TO_BANGFUN(pArray);

                while (true)
                {
                    eatwhitespace(mark);
                    
                    try {
                        JsonValue jv( mark );
                        mark.accept();
                        pArray->push_back( jv.bangValue() );
                    } catch ( const ErrorNoMatch& ) {
                    }

                    eatwhitespace(mark);

                    char nextdelim = mark.getc();
                    if (nextdelim == ']')
                        break;
                    
                    if (nextdelim == ',')
                        mark.accept();
                    else
                        throw ErrorNoMatch();
                }
            }
            else
                throw ErrorNoMatch();
        }
    };

    class JsonHash
    {
    public:
        Bang::Value v_;
        JsonHash( StreamMark& stream )
        {
            eatwhitespace( stream );
            
            StreamMark mark(stream);
            
            char delim = mark.getc();
            
            if (delim == '{')
            {
                //              mark.accept();
                auto pHash = NEW_BANGFUN( Hashlib::BangHash );
                v_ = STATIC_CAST_TO_BANGFUN(pHash);

                while (true)
                {
                    try {
                        eatwhitespace(mark);

                        char c = mark.getc();
                        if (c == '}')
                        {
                            mark.accept();
                            break;
                        }
                        else
                            mark.regurg(c);

                        ParseString keyname( mark );
                        
                        eatwhitespace(mark);

                        if (mark.getc() != ':')
                            throw ErrorNoMatch();

                        eatwhitespace(mark);
                        JsonValue jv( mark );
                        
                        pHash->set( keyname.content(), jv.bangValue() );

                        eatwhitespace( mark );

                        char nextdelim = mark.getc();
                        
                        if (nextdelim == ',')
                            ; // continue loop
                        else if (nextdelim == '}')
                        {
                            mark.accept();
                            break;
                        }
                        else
                            throw ErrorNoMatch();
                        
                    }
                    catch ( const ErrorNoMatch& )
                    {
                    }

                }
            }
            else
                throw ErrorNoMatch();
        }
    };
    
};


Parser::JsonValue::JsonValue( StreamMark& stream )
{
    eatwhitespace(stream);

    do
    {
        try {
            Number parsed(stream);
            v_ = Bang::Value( parsed.value() );
            break;
        } catch ( const ErrorNoMatch& ) {
        }

        try {
            ParseString str(stream);
            v_ = Bang::Value( str.content() );
            break;
        } catch ( const ErrorNoMatch& ) {
        }

        try {
            Boolean parsed(stream);
            v_ = Bang::Value( parsed.value() );
            break;
        } catch ( const ErrorNoMatch& ) {
        }

        try {
            JsonArray parsed(stream);
            v_ = Bang::Value( parsed.v_ );
            break;
        } catch ( const ErrorNoMatch& ) {
        }
        
        try {
            JsonHash parsed(stream);
            v_ = Bang::Value( parsed.v_ );
            break;
        } catch ( const ErrorNoMatch& ) {
        }
        
        try {
            StreamMark mark(stream);
            if (eatReservedWord("null", mark))
            {
                mark.accept();
                v_ = Bang::Value( JsonNs::null() );
                break;
            }
        } catch ( const ErrorNoMatch& ) {
        }
    }
    while (0);
}


    namespace JsonNs
    {
        void parseString( Bang::Stack& s, const Bang::RunContext& rc )
        {
            const auto v_in = s.pop();
            const std::string jsonStr = v_in.tostr();
            RegurgeString regurger( jsonStr );
            StreamMark marked( regurger );
            try
            {
                Parser::JsonValue v(marked);
                s.push( v.bangValue() );
            }
            catch( const std::exception& )
            {
            }
        }

        void lookup( Bang::Stack& s, const Bang::RunContext& ctx)
        {
            const Bang::Value& v = s.pop();
            if (!v.isstr())
                throw std::runtime_error("BangJSON library . operator expects string");
            const auto& str = v.tostr();
        
            const Bang::tfn_primitive p =
                (  str == "from-string" ? &parseString
//                :  str == "new"        ? &stackNew // test
                :  nullptr
                );

            if (p)
                s.push( p );
            else
            {
                if (str == "null")
                    s.push( JsonNs::null() );
                else
                    throw std::runtime_error("Array library does not implement: " + std::string(str));
            }
        }
    } // end namespace JsonNs
    

extern "C"
#if _WINDOWS
__declspec(dllexport)
#endif 
void bang_open( Bang::Stack* stack, const Bang::RunContext* )
{
    stack->push( &JsonNs::lookup );
}
