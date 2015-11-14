DLLEXPORT void NylonInsertEvent( const std::function< void(void) >& cb );

extern "C" {
#include <pthread.h>
}

class ThreadRunner
{
public:
//    ThreadRunner
//    (  const std::function<void(void)>& fun
//    )
//    : fun_( fun ),
//      exiter_( 0 )
//    {
//       init();
//    }

   ThreadRunner
   (  const std::function<void(void)>& fun
//       const luabind::object& o,
//       const luabind::object& exiter
   )
   : fun_( fun )
//      o_( new luabind::object(o) ),
//      exiter_( new luabind::object( exiter ) )
   {
       // Nylon events are processed when all coroutines are
       // yielded, ensures no contention on thread stack
       NylonInsertEvent( std::bind(&ThreadRunner::run, this) ); 
   }
   
   ~ThreadRunner()
   {
   }

   void whenThreadHasExited()
   {
#if 0       
      void* rc;

      if( exiter_ )
      {
         //preReport();
         (*exiter_)();
         //postReport();
      }
      
      pthread_join( thread_, &rc );

      delete this;
#endif 
   }

   void runInNewThread()
   {
       fun_(); 

#if 0       
      NylonSysCore::Application::InsertEvent
      (  std::bind( &ThreadRunner::whenThreadHasExited, this )
      );
#endif 
   }
   
   static void* // static
   threadEntry( void* arg )
   {
      ThreadRunner* this1 = static_cast<ThreadRunner*>( arg );
      this1->runInNewThread();
      return 0;
   }
      
   void run()
   {
//      std::cout << "ThreadRunner::run()" << std::endl;
      pthread_attr_t attr;
      pthread_attr_init( &attr );
      pthread_create
      (  &thread_, &attr,
         &ThreadRunner::threadEntry,
         this
      );
   }
private:
   pthread_t thread_;
//    static pthread_mutex_t allThreadsReportlock_;
//    pthread_mutex_t waitForReader_;
//    pthread_mutex_t waitForValues_;
   std::function<void(void)> fun_;
//    luabind::object* o_;
//    luabind::object* exiter_;

//    void preReport()
//    {
// //      std::cout << "signalling thread is done, ready to return values" << std::endl;

//       // signal ready
//       NylonSysCore::Application::InsertEvent
//       (  std::bind( &ThreadRunner::whenLuaIsReadyToReadValues, this )
//       );

//       // lock
// //      std::cout << "waiting for main thread to come and read" << std::endl;
//       pthread_mutex_lock( &waitForReader_ );

//       pthread_mutex_lock( &allThreadsReportlock_ );

//       // NylonSysCore blocked; now safe to operate on lua.
//    }

//    void postReport()
//    {
//       // Now unblock NylonSysCore
//       pthread_mutex_unlock( &waitForValues_ );

//       // and allow other threads to report
//       pthread_mutex_unlock( &allThreadsReportlock_ );

// //      std::cout << "done returning values, now unblocking main thread" << std::endl;
//    }
};
