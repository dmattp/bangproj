#ifndef WIN_NYLON_SYSPORT_TIMER_H__
#define WIN_NYLON_SYSPORT_TIMER_H__

namespace NylonSysCore {

   class Timer
   {
   public:
      static void
      oneShot
      (  const std::function< void(void)>& callback,
          unsigned expirationTime
      )
      {
         new TQTimer( callback, expirationTime );
      }

   private:
       struct Statics {
           HANDLE queue;
           Statics()
           : queue( 0 )
           {}
       };

       static Statics stTimer;
       
      class TQTimer
      {
         std::function<void(void)> m_callback;
         HANDLE m_timer;
      public:
         TQTimer( const std::function< void(void) >& cb, unsigned expirationTimeMs )
         : m_callback( cb ),
           m_timer( nullptr )
         {
            Timer::InitTimerQueue();
            m_callback = cb;
            auto rc = CreateTimerQueueTimer( &m_timer, stTimer.queue, &TimerCallbackOneShot,
               this, expirationTimeMs, 0, WT_EXECUTEINTIMERTHREAD );
         }
      
      private:
         static VOID CALLBACK TimerCallbackOneShot(PVOID lpParam, BOOLEAN TimerOrWaitFired)
         {
//             std::cout << "TimerCallback; param=" << lpParam << " Fired=" << (DWORD)TimerOrWaitFired 
//                       << " Thread=" << GetCurrentThreadId() << std::endl;
            TQTimer* this1 = static_cast< TQTimer* >( lpParam );

            //~~~ 150626
            Application::InsertEvent( this1->m_callback );
            //this1->m_callback();
            
            // this1->m_callback();
            delete this1;
         }

         ~TQTimer()
         {
            if( m_timer )
            {
               DeleteTimerQueueTimer( stTimer.queue, m_timer, nullptr );
            }
         }
      };
    
      static void InitTimerQueue()
      {
         if( !stTimer.queue )
            stTimer.queue = CreateTimerQueue();
      }
   };

    Timer::Statics Timer::stTimer;

}
       
#endif
