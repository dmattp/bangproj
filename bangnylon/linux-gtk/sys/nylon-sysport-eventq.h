#ifndef WIN_WEAVE_SYSPORT_EVENTQ_H__
#define WIN_WEAVE_SYSPORT_EVENTQ_H__

#include <iostream>
#include <functional>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <net/if.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <poll.h>

//#include <QtGui/QApplication>

namespace NylonSysCore {

    class SysportEventQueueBase
    {
    protected:
        SysportEventQueueBase()
        {
            int rc = pipe2( selfpipes, O_NONBLOCK );
            (void)rc;
        }
        void wakeup()
        {
            write( selfpipes[1], &selfpipes, 1 ); // wake up "mainloop"
        }
        void waitonevent()
        {
            // current linux implementation:
            // wait a maximum of 100ms, but wake up before that if an
            // event is put in the queue.  So, we may return early even
            // if there are no events in the queue, but we should return
            // with minimal latency when an event is actually added to
            // the queue.
            struct pollfd p;
            struct timespec ts = { 0,
                                   100*(1000*1000) //nanoseconds 
            }; // 100ms, i hope
            char clearit[16];

            p.fd = selfpipes[0];
            p.events = POLLIN;

            (void)ts;
            (void)p;

            int rc = ppoll( &p, 1, &ts, NULL );

            do
            {
               rc = read( selfpipes[0], clearit, 16 );
            }
            while( rc == 16 );
        }

        // return true if weave event rcvd, false if system event
        // no clear impl. for linux (compared to eg windows), so
        // just wait on weave events only.  For e.g., Qt implementation
        // one can patch into Qt and have it call the wakeup mechanism.
        bool waiteventnylonorsystem()
        {
           this->waitonevent();
           return true;
        }
    private:
       int selfpipes[2];
    }; // class SysportEventQueueBase
}
       
#endif
