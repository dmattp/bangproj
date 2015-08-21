//#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <iterator>
#include <stdlib.h>   // Needed for _wtoi

#include <string>

#include "bang.h"


#pragma comment(lib,"Ws2_32.lib")


#define LOGIT(fmt,...) do { char formatted[32*1024]; sprintf_s( formatted, sizeof(formatted), "[%s:%d %s] - " fmt, shorten(__FILE__), __LINE__, __FUNCTION__, __VA_ARGS__ ); std::cout << formatted << std::endl; } while (0)

const char *gListeningPort="23130";
//static const unsigned gTerminalId = 92007542; // serial
static const unsigned gTerminalId = 92007563; // IP

//const char *gListeningPort="9011";

namespace
{
    const char* shorten( const char* longfile )
    {
        const char* prev = strrchr(longfile,'\\');
        if (!prev)
        {
            return longfile;
        }
        return prev+1;
    }


    std::runtime_error LogLastSocketError( const char* pContext )
    {
        const int iError = WSAGetLastError();
        
        if (pContext)
        {
            LOGIT("ERROR=%d From=%s", iError, pContext);
        }
        else
        {
            LOGIT("ERROR=%d From=???", iError);
        }

        return std::runtime_error(pContext);
    }
}


namespace BNSocket // bang+nylon socket support
{
    using namespace Bang;
    
    class CSocketish : public Bang::Function
    {
        
    public:
        CSocketish( const std::string sIpAddr, const std::string sPort, bool bUdp=false, bool bListener=false )
        : m_sIpAddr( sIpAddr ),
          m_sPort( sPort ),
          m_bUdp( bUdp ),
          m_bListener( bListener ),
          m_pAddrInfo( 0 ),
          m_hSocket( INVALID_SOCKET )
        {
            this->init();
//            operators = &gHashOperators;
        }

        CSocketish( SOCKET sockhandle )
        : m_bUdp( false ),
          m_bListener( false ),
          m_pAddrInfo( nullptr ),
          m_hSocket( sockhandle )
        {
        }

        void close()
        {
            if (m_hSocket != INVALID_SOCKET)
            {
                ::closesocket( m_hSocket );
                m_hSocket = INVALID_SOCKET;
            }
        }
        

        ~CSocketish()
        {
            this->close();
            
            if (m_pAddrInfo)
            {
                ::freeaddrinfo( m_pAddrInfo );
            }
        }

        friend class CConnectedSocket;

        class CConnectedSocket : public Bang::Function
        {
        public:
            void send( const std::vector<BYTE>& oMsg ) const { m_oSocket.send( oMsg ); }
            int recv( BYTE* pBuffer, size_t nBytes ) const { return m_oSocket.recv( pBuffer, nBytes ); }
            void close()
            {
                m_oSocket.close();
                pSocket_.reset();
            }
            std::string getline()
            {
                std::string s;

                BYTE c;
                int rcvd = this->recv( &c, 1 );

                while( rcvd == 1 )
                {
                    if (c == '\n')
                        break;
                    
                    if (c != '\r')
                        s.push_back((char)c);
                        
                    rcvd = this->recv( &c, 1 );
                }

                if (rcvd != 1)
                    throw std::runtime_error("socket closed?");

                return s;
            }
        private:
            std::unique_ptr<CSocketish> pSocket_;
            friend class CSocketish;
            CConnectedSocket( CSocketish& oSocket )
            : m_oSocket( oSocket )
            {}
            CConnectedSocket( SOCKET sockhandle )
            :   pSocket_( std::unique_ptr<CSocketish>( new CSocketish(sockhandle) ) ),
                m_oSocket( *pSocket_ )
            {}
            ~CConnectedSocket()
            {
                std::cerr << "DESTROY CConnectedSocket" << std::endl;
                this->close();
            }

            CSocketish& m_oSocket;

            virtual void customOperator( const bangstring& theOperator, Stack& s)
            {
                // std::cerr << "CConnectedSocket::customOperator=" << theOperator << std::endl;

                const static Bang::bangstring opSend("/send");
                const static Bang::bangstring opClose("/close");
                const static Bang::bangstring opRecv("/recv");
                const static Bang::bangstring opGetline("/getline");
                const static Bang::bangstring opHandle("/sockethandle");
        
                if (theOperator == opHandle)
                {
                    SOCKET sock = m_oSocket.handle();
                    s.push( double( *((unsigned *)&sock) ) );
                }
                if (theOperator == opClose)
                {
                    this->close();
                }
                else if (theOperator == opSend)
                {
                    std::string msgstr = s.pop().tostr();
                    std::vector<BYTE> msg;
                    std::for_each( msgstr.begin(), msgstr.end(),
                        [&]( char c ) { msg.push_back( static_cast<BYTE>(c) ); }
                    );
                    try {
                        this->send(msg);
                    } catch (const std::exception&) {
                    }

                }
                else if (theOperator == opGetline)
                {
                    try {
                        const std::string theLine = this->getline();
                        s.push_bs( bangstring(theLine) );
                        s.push( true );
                    } catch (...) {
                        s.push( false );
                    }
                }
                else if (theOperator == opRecv)
                {
                    unsigned nToRecv = static_cast<unsigned>( s.pop().tonum() );
                    BYTE* pBuffer = static_cast<BYTE*>(alloca(nToRecv));
//                    std::cerr << "/recv getting bytes=" << nToRecv << std::endl;
                    const int rcvd = this->recv( pBuffer, nToRecv );
                    // std::cerr << "/recv returned bytes=" << rcvd << "stksize=" << s.size() << std::endl;
                    if (rcvd > 0)
                    {
                        s.push_bs( bangstring( reinterpret_cast<char*>(pBuffer), rcvd ) );
                        s.push( true );
                    }
                    else
                    {
                        std::cerr << "error, recv returned=" << rcvd << std::endl;
                        s.push( false );
//                        throw std::runtime_error("socket error");
                        //~~~ what to do, what to do??
                    }
                }
            }

            virtual void apply( Stack& s ) {}
            
        };
    
        CConnectedSocket connect()
        {
            const int iResult = ::connect( m_hSocket, m_pAddrInfo->ai_addr, (int)m_pAddrInfo->ai_addrlen);
            
            if (SOCKET_ERROR == iResult)
            {
                throw LogLastSocketError("socket connect failed");
            }

            LOGIT("Socket connected [%s:%s]", m_sIpAddr.c_str(), m_sPort.c_str());
            return CConnectedSocket(*this);
        }

        
        friend class CListeningSocket;

        class CListeningSocket : public Function
        {
        public:
            gcptr<CConnectedSocket> accept() const
            {
                const auto sockhandle = m_oSocket.accept();
                return NEW_BANGFUN( CConnectedSocket, sockhandle );
            }
        private:
            friend class CSocketish;
            CListeningSocket( CSocketish& oSocket )
            : m_oSocket( oSocket )
            {}
            CSocketish& m_oSocket;
            virtual void customOperator( const bangstring& theOperator, Stack& s)
            {
                // std::cerr << "CListeningSocket::customOperator=" << theOperator << std::endl;
                const static Bang::bangstring opAccept("/accept");

                // CListeningSocket& self = reinterpret_cast<CListeningSocket&>(*v.tofun());
                
                if (theOperator == opAccept)
                {
                    auto connected = this->accept();
                    // std::cerr << "CListeningSocket::accepted" << std::endl;
                    s.push( STATIC_CAST_TO_BANGFUN(connected) );
                }
            }

            virtual void apply( Stack& s ) // , CLOSURE_CREF running )
            {
            }
        };

        gcptr<CListeningSocket> bindAndListen()
        {
            if (SOCKET_ERROR == ::bind( m_hSocket, m_pAddrInfo->ai_addr, m_pAddrInfo->ai_addrlen ))
            {
                throw LogLastSocketError("socket bind failed");
            }

            // LOGIT("Socket bound to [%s:%s]", m_sIpAddr.c_str(), m_sPort.c_str());

            if (SOCKET_ERROR == ::listen( m_hSocket, SOMAXCONN ))
            {
                throw LogLastSocketError("socket listen failed");
            }

            LOGIT("Socket bound and listening at [%s:%s]", m_sIpAddr.c_str(), m_sPort.c_str());
            
            return NEW_BANGFUN(CListeningSocket,*this);
        }

            SOCKET handle() { return m_hSocket; }

        
    private:

        SOCKET accept()
        {
            const SOCKET hClient = ::accept(m_hSocket, NULL, NULL);
            
            if (INVALID_SOCKET == hClient)
            {
                throw LogLastSocketError("socket accept() failed");
            }

            return hClient;
        }
        
        // send requires a connected socket, so send through CConnectedSocket
        void send( const std::vector<BYTE>& oMsg )
        {
            const int iResult =
                ::send
                (   m_hSocket,
                    reinterpret_cast<const char*>(&oMsg.front()),
                    oMsg.size(),
                    0 /* flags */
                );

            if (iResult != oMsg.size())
            {
                throw LogLastSocketError( "socket send failed" );
            }
        }

        int recv( BYTE* pBuffer, size_t nBytes )
        {
            const int rc =
                ::recv
                (   m_hSocket,
                    reinterpret_cast<char*>(pBuffer),
                    nBytes,
                    MSG_WAITALL
                );
            return rc;
        }
        
        //////////////////////////////////////////////////////////////////
        // Create the socket for given IP addr and port, UDP/TCP, and listen/not listen
        void init()
        {
            struct addrinfo hints = { 0 };

            hints.ai_family = AF_INET;

            if (m_bUdp)
            {
                hints.ai_socktype = SOCK_DGRAM;
                hints.ai_protocol = IPPROTO_UDP;
            }
            else
            {
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_protocol = IPPROTO_TCP;
            }

            if (m_bListener)
            {
                hints.ai_flags = AI_PASSIVE;
            }

            const int iResult = getaddrinfo(m_sIpAddr.c_str(), m_sPort.c_str(), &hints, &m_pAddrInfo);
            
            if (iResult != 0)
            {
                LOGIT("getaddrinfo[%s:%s] failed: %d\n", m_sIpAddr.c_str(), m_sPort.c_str(), iResult);
                throw std::runtime_error("socket init failed");
            }
        
            m_hSocket = socket( m_pAddrInfo->ai_family, m_pAddrInfo->ai_socktype, m_pAddrInfo->ai_protocol);
        
            if (INVALID_SOCKET == m_hSocket)
            {
                throw LogLastSocketError("create socket failed");
            }

            LOGIT("Socket created");
        }

    template <class T>
    class BangMemFun : public Bang::Function
    {
        gcptr<T> cppfun_;
        void (T::* memfun_)(Bang::Stack&);
        
    public:
        BangMemFun( T* bangfun, void (T::* memfun)(Bang::Stack&) )
        : cppfun_( bangfun ),
          memfun_( memfun )
        {
        }
        void apply( Bang::Stack& s ) // , CLOSURE_CREF rc )
        {
            ((*cppfun_).*memfun_)( s );
        }
    };

    void customOperator( const bangstring& theOperator, Stack& s)
    {
        std::cerr << "CSocketish::customOperator: " << theOperator << std::endl;

        const static Bang::bangstring opBindAndListen("/bindAndListen");
        const static Bang::bangstring opHandle("/sockethandle");
        
        if (theOperator == opHandle)
        {
            SOCKET sock = this->handle();
            s.push( double( *((unsigned *)&sock) ) );
        }
        else if (theOperator == opBindAndListen)
        {
            std::cerr << "binding and listening\n";
            // CSocketish& self = reinterpret_cast<CSocketish&>(*v.tofun());
            const auto listening = this->bindAndListen();
            s.push( STATIC_CAST_TO_BANGFUN(listening) );
        }
    }
        
        virtual void apply( Stack& s ) // , CLOSURE_CREF running )
        {
            std::cerr << "CSocketish::apply?? Y U apply??\n";
        }

        const std::string m_sIpAddr;
        const std::string m_sPort;
        const bool m_bUdp;
        const bool m_bListener;
        struct addrinfo* m_pAddrInfo;
        SOCKET m_hSocket;
    };  // end, class CSocketish

//    CSocketish::HashOps CSocketish::gHashOperators;
    void create( Bang::Stack&s, const Bang::RunContext& )
    {
        const auto port = static_cast<unsigned>( s.pop().tonum() );
        const auto& ipstr_v = s.pop();
        const auto ipstr = ipstr_v.tostr();
        char portstr[999]; sprintf(portstr, "%d", port );
        auto socket = NEW_BANGFUN(CSocketish, ipstr, portstr, false, true );
        s.push( STATIC_CAST_TO_BANGFUN(socket) );
    }
    
    void lookup( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const Bang::Value& v = s.pop();
        if (!v.isstr())
            throw std::runtime_error("Socket error: . operator expects string");
        const auto& str = v.tostr();

        const Bang::tfn_primitive p =
            (  str == "create" ? &create
            :  nullptr
            );
        
        if (p)
            s.push( p );
        else
            throw std::runtime_error("Socket library does not implement" + std::string(str));
    }
}

extern "C"
#if _WINDOWS
__declspec(dllexport)
#endif 
void bang_open( Bang::Stack* stack, const Bang::RunContext* )
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2,2), &wsaData);
    stack->push( &BNSocket::lookup );
}

