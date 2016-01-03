#ifndef WRAPBUFFER_H
#define WRAPBUFFER_H

#include <qglobal.h>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class WrapBuffer
{
private:
    char    *buf;
    uint    bufsz,
            head,
            len;

public:
    WrapBuffer( uint size = 0 ) : buf(0), bufsz(0) {resizeAndErase(size);}
    WrapBuffer( const WrapBuffer &rhs ) : buf(0), bufsz(0) {*this=rhs;}
    virtual ~WrapBuffer()   {killbuf();}

    WrapBuffer &operator=( const WrapBuffer &rhs );

    void resizeAndErase( uint newSize );
    void erase() {head = len = 0;}
    void zeroFill();

    uint capacity() const           {return bufsz;}
    uint size() const               {return len;}
    uint unusedCapacity() const     {return bufsz - len;}
    uint cursor() const             {return (head+len) % bufsz;}
    bool isBufferWrapped() const    {return head+len > bufsz;}

    void rangesPutWillChange(
        uint    &r10,
        uint    &r1Lim,
        uint    &r20,
        uint    &r2Lim,
        uint    nBytes );

    void putData( const void *data, uint nBytes );

    // Start & length of unwrapped first part
    uint dataPtr1( void* &ptr ) const;

    // Start & length of any wrapped second part
    uint dataPtr2( void* &ptr ) const;

    // Gets capacity and buffer physical start &buf[0]
    uint all( void* &ptr ) const;

private:
    void killbuf() {if(buf){delete [] buf, buf=0, bufsz=0;}}
};


template<typename T>
class WrapT : protected WrapBuffer {
public:
    WrapT( uint n = 0 ) : WrapBuffer( n*sizeof(T) ) {}

    void resizeAndErase( uint newSize )
        {return WrapBuffer::resizeAndErase( newSize*sizeof(T) );}

    void erase()
        {WrapBuffer::erase();}

    void zeroFill()
        {WrapBuffer::zeroFill();}

    uint capacity() const
        {return WrapBuffer::capacity()/sizeof(T);}

    uint size() const
        {return WrapBuffer::size()/sizeof(T);}

    uint unusedCapacity() const
        {return WrapBuffer::unusedCapacity()/sizeof(T);}

    uint cursor() const
        {return WrapBuffer::cursor()/sizeof(T);}

    bool isBufferWrapped() const
        {return WrapBuffer::isBufferWrapped();}

    void rangesPutWillChange(
        uint    &r10,
        uint    &r1Lim,
        uint    &r20,
        uint    &r2Lim,
        uint    nBytes )
    {
        WrapBuffer::rangesPutWillChange(
            r10, r1Lim, r20, r2Lim, nBytes*sizeof(T) );

        r10   /= sizeof(T);
        r1Lim /= sizeof(T);
        r20   /= sizeof(T);
        r2Lim /= sizeof(T);
    }

    void putData( const T *data, uint n )
        {WrapBuffer::putData((const void*)data, n*sizeof(T));}

    // Start & length of unwrapped first part
    uint dataPtr1( T* &ptr ) const
    {
        void    *p;
        uint    l = WrapBuffer::dataPtr1( p );

        ptr = (T*)p;
        return l / sizeof(T);
    }

    // Start & length of any wrapped second part
    uint dataPtr2( T* &ptr ) const
    {
        void    *p;
        uint    l = WrapBuffer::dataPtr2( p );

        ptr = (T*)p;
        return l / sizeof(T);
    }

    // Caller MUST CHECK [size() != 0].
    T &first() const
    {
        T   *p;

        dataPtr1( p );
        return *p;
    }

    // Caller MUST CHECK [size() != 0].
    T &last() const
    {
        T       *p;
        uint    l = dataPtr2( p );

        if( !l )
            l = dataPtr1( p );

        return p[l - 1];
    }

    uint all( T* &ptr ) const
    {
        void    *p;
        uint    l = WrapBuffer::all( p );

        ptr = (T*)p;
        return l / sizeof(T);
    }

    T &at( uint i ) const
    {
        uint sz = size();

        if( i >= sz )
            i = sz - 1;

        T       *p;
        uint    l1 = dataPtr1( p );

        if( i < l1 )
            return p[i];

        dataPtr2( p );

        return p[i - l1];
    }

    T &operator[]( uint i ) {return this->at( i );}
};

#endif  // WRAPBUFFER_H


