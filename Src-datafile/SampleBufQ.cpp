
#include "SampleBufQ.h"
#include "Util.h"




void SampleBufQ::enqueue( vec_i16 &src )
{
    QMutexLocker    ml( &dataQMtx );

    if( dataQ.size() >= maxQSize )
        overflowWarning();

    dataQ.push_back( SampleBuf( src ) );

// Have an entry; wake a waiting dequeue caller

    condBufQIsEntry.wakeAll();
}


// Returns true if data ready to be written...
// ...if true, dst is swapped for a data buffer in the deque.
//
bool SampleBufQ::dequeue( vec_i16 &dst, bool wait )
{
    dst.clear();

    if( !dataQMtx.tryLock( 2000 ) )
        return false;

// Caller sleeps here if no data...

    if( wait && !dataQ.size() )
        condBufQIsEntry.wait( &dataQMtx );

// ...And wakes up here when there is

    int     N           = (int)dataQ.size();
    bool    dataReady   = (N != 0);

    if( N ) {

        // First, dequeue one block

        dst.swap( dataQ.front().data );
        dataQ.pop_front();
        --N;

        // In the following, if the queue is lagging we take action--
        // We dequeue and join up to joinMax words, but not more than
        // memory allows, of course. Writing larger blocks clears the
        // queue faster, and is more efficient for sequential I/O.

        const int   actionThresh    = 20;
        const uint  joinMax         = 8*1024*1024/2;

        if( N >= actionThresh ) {

            for( int i = 0; N > 0 && dst.size() < joinMax; ++i ) {

                vec_i16 &src = dataQ.front().data;

                try {
                    dst.insert( dst.end(), src.begin(), src.end() );
                }
                catch( const std::exception& ) {
                    Error() << "Write queue low mem.";
                    break;
                }

                dataQ.pop_front();
                --N;
            }
        }
    }

    if( !dataQ.size() )
        condBufQIsEmpty.wakeAll();

    dataQMtx.unlock();
    return dataReady;
}


// Return true if queue empty within (ms) timeout.
// Waits indefinitely if ms = -1.
//
bool SampleBufQ::waitForEmpty( int ms )
{
    bool    empty = false;

    if( dataQMtx.tryLock( ms ) ) {

        empty = !dataQ.size();

        if( !empty ) {

            // Note that wait() requires a locked mutex on entry.
            // It then unlocks the mutex, performs the wait, and
            // relocks the mutex on behalf of the caller on exit.
            // On exit, the mutex is locked regardless of (ok).

            empty = condBufQIsEmpty.wait(
                        &dataQMtx,
                        (ms < 0 ? ULONG_MAX : ms) );
        }

        dataQMtx.unlock();
    }

    return empty;
}


void SampleBufQ::overflowWarning()
{
    Error()
        << "Write queue overflow (capacity: "
        << maxQSize
        << " buffers).";
}


