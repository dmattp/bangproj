
#include "bang.h"
#if WIN32
# include <windows.h>
#endif
# include <iostream>
#include <map> // for register_thread, unregister_thread


//#include "sys/nylon-sysport-eventq.h"
#include "mwsr-cbq.h"

#include "nylon-sysport-threaded.h"

#if WIN32
namespace stdfake
{
    class recursive_mutex
    {
    public:
        recursive_mutex()
        {
            InitializeCriticalSection(&m_cs);
        }
        ~recursive_mutex()
        {
            DeleteCriticalSection(&m_cs);
        }
        void lock()
        {
            EnterCriticalSection(&m_cs);
        }
        void unlock()
        {
            LeaveCriticalSection(&m_cs);
        }
        bool try_lock()
        {
            return !!TryEnterCriticalSection(&m_cs);
        }
    private:
        CRITICAL_SECTION m_cs;
        recursive_mutex( const recursive_mutex& ); // not implemented; private to prevent copying.
    };
}
#else
# include <mutex>
# include "nylon-sysport-eventq.h"
#endif

namespace NylonSysCore
{
          enum EEventAvail {
              GOT_NYLON_EVENT,
              GOT_SYSTEM_EVENT
          };

#if WIN32
    using stdfake::recursive_mutex;
#else
    using std::recursive_mutex;
#endif    
    
    
#if WIN32
    class SysportEventQueueBase
    {
    protected:
        SysportEventQueueBase()
        {
            hEventsAvailable =
                CreateEvent
                (  nullptr, FALSE, FALSE,
                    // TEXT("nylon::ApplicationEventQueueApvailable")
                    nullptr
                );
        }
        void wakeup()
        {
//            std::cerr << "set hEventsAvailable" << std::endl;
            SetEvent( hEventsAvailable );
        }
        void waitonevent()
        {
            WaitForSingleObject( hEventsAvailable, INFINITE );
        }

        // return true if nylon event rcvd, false otherwise
        bool waiteventnylonorsystem()
        {
             // see e.g.,
             // http://blogs.msdn.com/b/oldnewthing/archive/2006/01/26/517849.aspx;
             // be sure all messages are processed before waiting again, see http://blogs.msdn.com/b/oldnewthing/archive/2005/02/17/375307.aspx
//            std::cerr << "waiteventnylonorsystem" << std::endl;
            auto rc =
               MsgWaitForMultipleObjects
               ( 1, &hEventsAvailable,
                  FALSE,
//                   10, //INFINITE,
                   INFINITE,
                   QS_ALLINPUT
               );

            // std::cout << "MsgWaitForMultipleObjects rc=" << rc << std::endl;

            switch( rc )
            {
               case WAIT_OBJECT_0:
                   return true;  // nylon event

               case (WAIT_OBJECT_0 + 1):
                   return false;

               default:
                   // NOTE: I still get timeouts here, sort of routinely e.g. on test program for asthread
                   // which is _not_ optimal, but it keeps stuff trucking though it adds useless latency that
                   // could kill throughput in some situations.
                   // std::cout << "UnK return from MsgWaitForMultipleObjects=" << rc << std::endl;
                  return false;
            }
        }
    private:
        HANDLE hEventsAvailable;
    }; // class SysportEventQueueBase
#endif 
    
   class Application
   {
      class ApplicationEventQueue : public SysportEventQueueBase
      {
          SwisserQ< std::function<void(void)> > mwsr_cbq_;
      public:
          
          ApplicationEventQueue()
          {
          }

          void InsertEvent( const std::function< void(void) >& cb )
          {
              mwsr_cbq_.put( cb );
              this->wakeup();
          }

          void* PreAllocateEvent( const std::function< void(void) >& cb )
          {
              return mwsr_cbq_.newthing( cb );
          }

          void InsertPreAllocatedEvent( void* thing )
          {
              mwsr_cbq_.putVoid( thing );
              this->wakeup();
          }
         
          void Wait()
          {
              this->waitonevent();
          }

          EEventAvail WaitNylonSysCoreOrSystem()
          {
              return this->waiteventnylonorsystem() ? GOT_NYLON_EVENT : GOT_SYSTEM_EVENT;
          }
         
          bool Process1()
          {
              if( mwsr_cbq_.isEmpty() )
                  return false;

              try {
                  std::function<void(void)> cbfn = mwsr_cbq_.get();
                  cbfn();
              } catch( ... ) {
                  std::cout << "error, empty q?" << std::endl;
              }
              return true;
          }
   
          void Process()
          {
              while( Process1() )
                  ;
          }
      };
      ApplicationEventQueue m_eventQueue;
       
      static Application& instance()
      {
         static Application* instance;
         if( !instance )
            instance = new Application();
         return *instance;
      }
       static recursive_mutex luaLock_;
   public:
       static void LockBangThreads( bool yes )
       {
           if (yes) { luaLock_.lock(); } else { luaLock_.unlock(); }
       }
      static void InsertEvent( const std::function< void(void) >& cb )
      {
         instance().m_eventQueue.InsertEvent( cb );
      }
      static void* PreAllocateEvent( const std::function<void(void)>& cb )
      {
         return instance().m_eventQueue.PreAllocateEvent( cb );
      }

      static void InsertPreAllocatedEvent( void* thing )
      {
         instance().m_eventQueue.InsertPreAllocatedEvent( thing );
      }

       
       static EEventAvail WaitNylonSysCoreOrSystem()
       {
           return instance().m_eventQueue.WaitNylonSysCoreOrSystem();
       }
      static void ProcessEvents()
      {
         instance().m_eventQueue.Process();
      }
  };

   recursive_mutex Application::luaLock_;
    
} // end, namespace NylonSysCore

DLLEXPORT void NylonInsertEvent( const std::function< void(void) >& cb )
{
    NylonSysCore::Application::InsertEvent( cb );
}

DLLEXPORT void NylonLockBangThreads( bool yes )
{
    NylonSysCore::Application::LockBangThreads( yes );
}


#ifdef _WIN32
namespace {
    double uptime()
    {
#if 0
        ULONG ticks = GetTickCount();
        return (double(ticks/1000) + double(ticks%1000)/1000.0);
#else
        static bool gotInitTime;
        static LARGE_INTEGER Frequency;
        static LARGE_INTEGER StartingTime;
        if( !gotInitTime )
        {
            QueryPerformanceFrequency(&Frequency); 
            QueryPerformanceCounter(&StartingTime);
            gotInitTime = true;
        }
        LARGE_INTEGER EndingTime, ElapsedMicroseconds;
        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;

//
// We now have the elapsed number of ticks, along with the
// number of ticks-per-second. We use these values
// to convert to the number of elapsed microseconds.j
// To guard against loss-of-precision, we convert
// to microseconds *before* dividing by ticks-per-second.
//

        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
        return (double)ElapsedMicroseconds.QuadPart / 1000000.0;
#endif 
    }
} // end, anonymous namespace

#else // not windows
#include <time.h>
namespace {
    double uptime()
    {
       struct timespec ts;
       int rc = clock_gettime( CLOCK_MONOTONIC, &ts );
       return double(ts.tv_sec) + double(ts.tv_nsec)/double(1000*1000*1000);
    }
}
#endif

#include "nylon-sysport-timer.h"

namespace NylonSys
{
    void uptimePrimitive( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        s.push( uptime() );
    }
    
    std::vector<std::shared_ptr<Bang::Thread>> runnableThreads_;
    std::vector<Bang::BANGFUNPTR> oneshots_;

    void oneShotExpiry( Bang::Thread* bthread, Bang::BANGFUNPTR callback )
    {
//        std::cerr << "Got oneShotExpiry\n";
            
        auto bprog = dynamic_cast<Bang::BoundProgram*>( callback.get() );
        if (bprog)
        {
            NylonLockBangThreads(true);
            Bang::CallIntoSuspendedCoroutine( bthread, bprog ); // ->program_, bprog->upvalues_ );
            NylonLockBangThreads(false);
        }
    }

        
    void addOneShot( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const Bang::Value& vtime = s.pop();
        const Bang::Value& vfun = s.pop();

//        std::cerr << "Setting one shot, " << vtime.tonum() << "s\n";
        
        NylonSysCore::Timer::oneShot( std::bind( &oneShotExpiry, ctx.thread, vfun.tofun() ),
            static_cast<unsigned>(vtime.tonum()*1000) );
    }

    void addRunnableThread( std::shared_ptr<Bang::Thread> thread )
    {
        runnableThreads_.emplace_back( thread );
    }
    
    void schedule( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const Bang::Value& v = s.pop();
        if (!v.isthread())
            throw std::runtime_error("NylonSys::schedule - expected therad");
//        std::cerr << "Scheduling thread=" << v.tothread().get() << "\n";
        NylonSysCore::Application::InsertEvent( std::bind( &addRunnableThread, v.tothread() ) );
    }

    void runboundfun( Bang::bangthreadptr_t bthread, Bang::gcptr<Bang::BoundProgram> bp )
    {
//        NylonLockBangThreads(true);
//        std::cerr << "running bound fun=" << bthread.get() << " stk=" << bthread->stack.size() << std::endl;
//        std::cerr << std::flush;
//        NylonLockBangThreads(false);
        
        Bang::CallIntoSuspendedCoroutine( bthread.get(), bp.get() );

//        NylonLockBangThreads(true);
//        std::cerr << "returned from bound fun=" << bthread.get() << " stk=" << bthread->stack.size() << std::endl;
//        std::cerr << std::flush;
//        NylonLockBangThreads(false);
        
        NylonSysCore::Application::InsertEvent( std::bind( &addRunnableThread, bthread ) );
//        std::cerr << "adding cord to runlist\n";
    }

    void asthread( Bang::Stack& s, const Bang::RunContext& ctx )
    {
        const auto vthread = s.pop();
        const auto theThread = vthread.tothread();
        const auto v = s.pop();

        auto bp = v.toboundfunhold();

//        std::cerr << "queuing bound fun=" << theThread.get() << " stk=" << theThread->stack.size() << std::endl;
//        std::cerr << std::flush;
        
        auto cppbound = std::bind( &runboundfun, theThread, bp );

//        std::cerr << "asthread invoked, thread=" << ctx.thread << ", creating threadrunner\n";
//         NylonLockBangThreads(true);
//         NylonLockBangThreads(false);

#if 1
        auto tr = new ThreadRunner( cppbound );
#else
        cppbound();
#endif 
        
    }


    void waitforthread( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        NylonLockBangThreads(false);
        while (1)
        {
//            std::cerr << "nylonsys waiting for event\n";

//             if (runnableThreads_.size() < 1)
//                 Sleep(100);
//             else
            if (runnableThreads_.size() > 0)
            {
                NylonLockBangThreads(true);
                const auto thread = runnableThreads_.front();
//                std::cerr << "got thread=" << thread.get() << "\n";
                s.push( Bang::Value(thread) );
                runnableThreads_.erase( runnableThreads_.begin() );
                break;
            }
            else
            {
                auto rc = NylonSysCore::Application::WaitNylonSysCoreOrSystem();

                // Sleep(1);
//            std::cerr << "nylonsys got event rc=" << rc << '\n';
            
                if (rc == NylonSysCore::GOT_NYLON_EVENT)
                    NylonSysCore::Application::ProcessEvents();
            }
        }
    }
    
    void testSysSleep( Bang::Stack& s, const Bang::RunContext& ctx)
    {
#if WIN32        
        int nsleep = (int)(s.pop().tonum()*1000.0);
        Sleep(nsleep);
#endif 
    }

    static std::map<Bang::Thread*, bool> gActiveThreads;
    
    void register_thread( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const auto thread = s.pop().tothread();
        gActiveThreads[ thread.get() ] = true;
    }

    void unregister_thread( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        const auto thread = s.pop().tothread();
        auto it = gActiveThreads.find( thread.get() );
        if (it != gActiveThreads.end() )
            gActiveThreads.erase( it );
    }

    void have_threads( Bang::Stack& s, const Bang::RunContext& ctx)
    {
        s.push( gActiveThreads.empty() ? false : true );
    }

    
    void lookup( Bang::Stack & s, const Bang::RunContext& ctx)
    {
        const Bang::Value& v = s.pop();
        if (!v.isstr())
            throw std::runtime_error("NylonSys error: . operator expects string");
        const auto& str = v.tostr();

        const Bang::tfn_primitive p =
            (  str == "waitforthread" ? &waitforthread
            :  str == "schedule"      ? &schedule
            :  str == "addOneShot"    ? &addOneShot
            :  str == "uptime"        ? &uptimePrimitive
            :  str == "asthread"      ? &asthread
            :  str == "register-thread"   ? &register_thread
            :  str == "unregister-thread" ? &unregister_thread
            :  str == "have-threads" ? &have_threads
            :  str == "testSysSleep"  ? &testSysSleep
            :  nullptr
            );

//        std::cerr << "nylon lookup invoked, msg=" << str << " memfun=" << p << "\n";

        if (p)
            s.push( p );
        else
            throw std::runtime_error("Math library does not implement" + std::string(str));
    }
    
} // end namespace Math


extern "C"
#if _WINDOWS
__declspec(dllexport)
#endif 
void bang_open( Bang::Stack* stack, const Bang::RunContext* )
{
    stack->push( &NylonSys::lookup );
}
