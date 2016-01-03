
#include "SampleBufQ.h"
#include "Util.h"




SampleBufQ::SampleBufQ( int capacitySecs )
    : maxQSize(capacitySecs * daqAINumFetchesPerSec())
{
}


void SampleBufQ::enqueue( vec_i16 &src, quint64 firstCt )
{
    QMutexLocker    ml( &dataQMtx );

    if( dataQ.size() >= maxQSize ) {

        overflowWarning();
        dataQ.pop_front();
    }

    dataQ.push_back( SampleBuf( src, firstCt ) );
    condBufQIsEntry.wakeOne();
}


// Returns count of available blocks.
// -- if > 0, dst is swapped for a data buffer in the deque.
//
int SampleBufQ::dequeue(vec_i16 &dst, quint64 &firstCt, bool wait )
{
    int N = 0;
    dst.clear();

    if( !dataQMtx.tryLock( 2000 ) )
        return 0;

    if( wait && !dataQ.size() )
        condBufQIsEntry.wait( &dataQMtx );

    if( (N = (int)dataQ.size()) ) {

        SampleBuf   &buf = dataQ.front();

        dst.swap( buf.data );
        firstCt = buf.firstCt;

        dataQ.pop_front();
    }

    if( !dataQ.size() )
        condBufQIsEmpty.wakeAll();

    dataQMtx.unlock();
    return N;
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
    Warning()
        << "Write queue overflow (capacity: "
        << maxQSize
        << " buffers). Dropping a buffer.";
}


