
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

   void runInNewThread()
   {
       fun_(); //  reporter
   }

    static unsigned __stdcall // static
    threadEntry( void* arg )
    {
        ThreadRunner* this1 = static_cast<ThreadRunner*>( arg );
        this1->runInNewThread();
        return 0;
    }
      
   void run()
   {
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
   std::function<void(void)> fun_;
}; // end, ThreadRunner
