
DLLEXPORT void NylonInsertEvent( const std::function< void(void) >& cb );
DLLEXPORT void NylonLockBangThreads( bool yes );

#include <process.h>

/** long term, ThreadRunner should probably operate on a pool of threads,
 * and rather than respawning a thread for each threadrunner, grab from
 * the threadpool and pass the request to an existing thread which is suspended
 * waiting for work.  But for simplicity of implementation,
 * we can delay that until efficiency becomes a concern.  The interface should not
 * require any changes, so it won't break existing code.
 */
class ThreadRunner
{
public:
   ThreadRunner
   (  const std::function<void(void)>& fun
   )
   : fun_( fun )
   {
       // Nylon events are processed when all coroutines are
       // yielded, ensures no contention on thread stack
        NylonInsertEvent( std::bind(&ThreadRunner::run, this) ); 
   }

   ~ThreadRunner()
   {
   }

#if 0
    void whenLuaIsReadyToReadValues()
   {
       //std::cout << "main thread now ready to read values" << std::endl;

      gl_MainThreadWaitingForValues = true; 
      ReleaseSemaphore( waitForReader_, 1, 0 );  // allow thread to return values

//      std::cout << "main thread now waiting for values" << std::endl;
      
      auto rc = WaitForSingleObject( waitForValues_, INFINITE ); // wait for thread to finish returning values
      gl_MainThreadWaitingForValues = false;

      if (rc != WAIT_OBJECT_0)
      {
          hardfail( "ERROR02b: waitForValues_ failed, rc=", rc );
      }

      //std::cout << "main thread now released, values should have been returned" << std::endl;
   }

   void whenThreadHasExited()
   {
//      void* rc;
       // ULONG tbeg = GetTickCount();
       WaitForSingleObject( thread_, INFINITE ); // MS equivalent of pthread_join

       //ULONG tend  = GetTickCount();

       // std::cout << "whenThreadHasExited tick elapsed=" << (tend-tbeg) << std::endl;
                                       
//        if( exiter_ )
//        {
//            (*exiter_)();
//        }

//      pthread_join( thread_, &rc );

      delete this;
   }
#endif 

   void runInNewThread()
   {
//      std::cout << "ENTER ThreadRunner::runInNewThread()" << std::endl;
//      ThreadReporter reporter( *this, *o_ );

       fun_(); //  reporter
      
//      std::cout << "EXIT ThreadRunner::runInNewThread()" << std::endl;
//       NylonSysCore::Application::InsertEvent
//       (  std::bind( &ThreadRunner::whenThreadHasExited, this )
//       );
   }

    static unsigned __stdcall // static
    threadEntry( void* arg )
    {
//        std::cout << "ThreadRunner::threadEntry()" << std::endl;
        ThreadRunner* this1 = static_cast<ThreadRunner*>( arg );
        // this1->initMutexes();
        this1->runInNewThread();
        return 0;
    }
      
   void run()
   {
//      std::cout << "ThreadRunner::run()" << std::endl;

// uintptr_t _beginthreadex( // NATIVE CODE
//    void *security,
//    unsigned stack_size,
//    unsigned ( __stdcall *start_address )( void * ),
//    void *arglist,
//    unsigned initflag,
//    unsigned *thrdaddr 
// );

      thread_ = (HANDLE)
          _beginthreadex
          (   0, // void *security,
              0, // unsigned stack_size,
              &threadEntry, // unsigned ( __stdcall *start_address )( void * ),
              this, // void *arglist,
              0, // unsigned initflag, 0 == creates running (rather than suspended)
              &threadaddr_ // unsigned *thrdaddr 
          );
   }
    
private:
   HANDLE thread_;
   unsigned threadaddr_; // really don't know what this is that's different
                         // from HANDLE; MS docs suggest its the 'thread id'??
//   static HANDLE allThreadsReportlock_;
   std::function<void(void)> fun_;
}; // end, ThreadRunner
