
#include "NetClient.h"

#include <string.h>




NetClient::NetClient(
    const string    &host,
    uint16          port,
    uint            read_timeout_secs )
    :   Socket(TCP), read_timeout_secs(read_timeout_secs)
{
    setHost( host );
    setPort( port );
}


uint NetClient::sendData(
    const void  *src,
    uint        srcBytes ) throw( const SockErr& )
{
    static const uint maxsend = 2*1024*1024;

    vbuf.clear();   // clear response

    if( !srcBytes )
        return 0;

    uint        sent = 0;
    const char  *buf = static_cast<const char*>(src);

    while( sent < srcBytes ) {

        uint    tosend = srcBytes - sent;

        if( tosend > maxsend )
            tosend = maxsend;

        sent += Socket::sendData( buf + sent, tosend );
    }

    return sent;
}


uint NetClient::receiveData(
    void    *dst,
    uint    dstBytes ) throw( const SockErr& )
{
    static const uint maxrecv = 2*1024*1024;

    if( !dstBytes )
        return 0;

    ulong   recvd   = 0,
            nB      = vbuf.size();
    char    *buf    = static_cast<char*>(dst);

    if( nB ) {

        recvd = nB;

        if( recvd > dstBytes )
            recvd = dstBytes;

        memcpy( buf, &vbuf[0], recvd );

        if( (nB -= recvd) > 0 )
            memcpy( &vbuf[0], &vbuf[recvd], nB );

        vbuf.resize( nB );
    }

    while( recvd < dstBytes ) {

        uint    retval,
                nR = nReadyForRead();

        if( nR ) {

            if( nR > dstBytes - recvd )
                nR = dstBytes - recvd;

            if( nR > maxrecv )
                nR = maxrecv;

            retval = Socket::receiveData( buf + recvd, nR );
            recvd += retval;

            if( retval == 0 )
                break;
        }
        else if( !waitData( 1000 * read_timeout_secs ) )
            break;
    }

    return recvd;
}


uint NetClient::sendString( const string &s ) throw( const SockErr& )
{
    vbuf.clear();   // clear response

    return Socket::sendData( s.data(), s.length() );
}


// Return one line; newline stripped, null-terminated.
//
void NetClient::rcvLine( vector<char> &line ) throw( const SockErr& )
{
    line.clear();

    for(;;) {

        char    *v0, *vN;
        int     nB = vbuf.size();

        // ------------------
        // Is '\n' in buffer?
        // ------------------

        if( nB && (vN = (char*)memchr( v0 = &vbuf[0], '\n', nB )) ) {

            // -------------
            // Copy data out
            // -------------

            *vN++ = 0;  // convert '\n' to null

            int nL = vN - v0;

            line.resize( nL );
            memcpy( &line[0], v0, nL );

            // -----------------------
            // Remove line from buffer
            // -----------------------

            if( (nB -= nL) > 0 )
                memcpy( v0, vN, nB );

            vbuf.resize( nB );
            break;
        }

        // -----------------------
        // Else, buffer more chars
        // -----------------------

        int nR = nReadyForRead();

        if( nR ) {

            vbuf.resize( nB + nR );
            Socket::receiveData( &vbuf[nB], nR );
        }
        else if( !waitData( 1000 * read_timeout_secs ) ) {

            line.push_back( 0 );
            break;
        }
    }

// One at a time method -----------------
//
//    line.clear();
//
//    for(;;) {
//
//        if( !nReadyForRead() && !waitData( 1000 * read_timeout_secs ) )
//            break;
//
//        // Fetch one char at a time
//
//        int     num;
//        char    c;
//
//        num = Socket::receiveData( &c, 1 );
//
//        if( !num || c == '\n' )
//            break;
//
//        line.push_back( c );
//    }
//
//    line.push_back( '\0' );
}


void NetClient::rcvLines( vector<vector<char> > &vlines ) throw( const SockErr& )
{
    vlines.clear();

    do {
        vector<char>    line;
        rcvLine( line );

        if( line.size() > 1 )
            vlines.push_back( line );
        else
            break;

        // Early exit if OK
        if( !strcmp( &line[0], "OK" ) )
            break;

    } while( waitData( 1 ) );
}


#ifdef TESTNETCLIENT

#ifdef WIN32
#include <io.h>
#endif

#include <stdio.h>


int main(void)
{
  char buf[2048];
  int nread;
  uint nsent;

  try {

    NetClient c("10.10.10.87", 3333);
    c.setSocketOption(Socket::TCPNoDelay, true);

    if (!c.connect()) {
      string err = c.errorReason();
      fprintf(stderr, "Error connecting: %s\n", err.c_str());
      return 1;
    }

    while ( ( nread = ::read(::fileno(stdin), buf, sizeof(buf)) ) >= 0 ) {
      if (nread >= (int)sizeof(buf)) nread = sizeof(buf)-1;
      buf[nread] = 0;
      nsent = c.sendString(buf);
      fprintf(stderr, "Sent %u\n", nsent);

      vector<vector<char> > vlines;
      c.rcvLines( vlines );
      int n = vlines.size();
      for( int i = 0; i < n; ++i )
        fprintf( stderr, "got: %s\n", &vlines[i][0] );
    }
  }
  catch( const ConnectionClosed &e ) {
    fprintf(stderr, "Connection closed. (%s)\n", e.why().c_str());
    return 2;
  }
  catch( const SockErr &e ) {
    fprintf(stderr, "Caught exception.. (%s)\n", e.why().c_str());
    return 1;
  }

  return 0;
}

#endif  // TESTNETCLIENT


