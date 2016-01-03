
#include "WrapBuffer.h"

#include <string.h>
#include <algorithm>


WrapBuffer &WrapBuffer::operator=( const WrapBuffer &rhs )
{
    if( bufsz != rhs.bufsz ) {
        killbuf();
        bufsz   = rhs.bufsz;
        buf     = new char[bufsz];
    }

    head    = rhs.head;
    len     = rhs.len;

    memcpy( buf, rhs.buf, bufsz );

    return *this;
}


void WrapBuffer::resizeAndErase( uint newSize )
{
    if( bufsz != newSize ) {

        killbuf();
        bufsz = newSize;

        if( newSize )
            buf = new char[newSize];
    }

    len = head = 0;
}


void WrapBuffer::zeroFill()
{
    memset( buf, 0, bufsz );
}


// Before calling putData(), you can call this function
// to determine the ranges of slots that putData() will
// overwrite. Range [r10,r1Lim) is used iff r1Lim > 0,
// and [r20,r2Lim) is used iff r2Lim > 0.
//
void WrapBuffer::rangesPutWillChange(
    uint    &r10,
    uint    &r1Lim,
    uint    &r20,
    uint    &r2Lim,
    uint    nBytes )
{
    r1Lim   = 0;
    r2Lim   = 0;

    if( nBytes >= bufsz ) {
        // Keep only newest bufsz-worth.
        r10     = 0;
        r1Lim   = bufsz;
    }
    else {
        // All new data fit, with some room for old data.
        uint    oldtail = cursor(),
                ncpy1   = std::min( nBytes, bufsz - oldtail );

        r10     = oldtail;
        r1Lim   = oldtail + ncpy1;

        r20     = 0;
        r2Lim   = nBytes - ncpy1;
    }
}


void WrapBuffer::putData( const void *data, uint nBytes )
{
    const char  *src = (const char*)data;

    if( nBytes >= bufsz ) {
        // Keep only newest bufsz-worth.
        head    = 0;
        len     = bufsz;
        memcpy( &buf[0], &src[nBytes-bufsz], bufsz );
    }
    else {
        // All new data fit, with some room for old data.
        uint    newlen  = std::min( len + nBytes, bufsz ),
                newhead = (head + len + nBytes - newlen) % bufsz,
                oldtail = cursor(),
                ncpy1   = std::min( nBytes, bufsz - oldtail );

        memcpy( &buf[oldtail], src, ncpy1 );

        if( nBytes -= ncpy1 )
            memcpy( &buf[0], src + ncpy1, nBytes );

        head    = newhead;
        len     = newlen;
    }
}


uint WrapBuffer::dataPtr1( void* &ptr ) const
{
    ptr = (void*)&buf[head];
    return (isBufferWrapped() ? bufsz-head : len);
}


uint WrapBuffer::dataPtr2( void* &ptr ) const
{
    if( isBufferWrapped() ) {
        ptr = (void*)&buf[0];
        return head + len - bufsz;
    }

    ptr = 0;
    return 0;
}


uint WrapBuffer::all( void* &ptr ) const
{
    ptr = (void*)&buf[0];
    return bufsz;
}


