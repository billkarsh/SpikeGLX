
#include "NetClient.h"

#include <algorithm>
#include <map>

#include <string.h>

#include <mex.h>
#include <matrix.h>

/* ---------------------------------------------------------------- */
/* Shared-Mem-Filename -------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef WIN32

#include <windows.h>

class SMF {
private:
    const char  *mapName;
    HANDLE      hMapFile;
    void        *pBuf;
    const uint  BUF_SIZE;
public:
    SMF()
    :   mapName("Global\\SpikeGLFileNameShm"),
        hMapFile(NULL), pBuf(NULL), BUF_SIZE(1024) {}
    virtual ~SMF()  {detach();}

    string getName();

private:
    void attach();
    void detach();
};


string SMF::getName()
{
    attach();

    if( hMapFile && pBuf )
        return (char*)pBuf;

    return "Not attached to shared memory.";
}


void SMF::attach()
{
    if( hMapFile )
        return;

    hMapFile = OpenFileMappingA(
                FILE_MAP_ALL_ACCESS,    // read/write access
                FALSE,                  // do not inherit the name
                mapName );              // name of mapping object

    if( !hMapFile ) {
        mexWarnMsgTxt(
        "Could not attach to SpikeGLFileNameShm"
        " -- OpenFileMappingA() failed!." );
        return;
    }

    pBuf = MapViewOfFile(
                hMapFile,               // handle to map object
                FILE_MAP_ALL_ACCESS,    // read/write perms
                0,                      // view offset-hi
                0,                      // view offset-lo
                BUF_SIZE );             // view length

    if( !pBuf ) {
        detach();
        mexWarnMsgTxt(
        "Could not attach to SpikeGLFileNameShm"
        " -- MapViewOfFile() failed." );
    }
}


void SMF::detach()
{
    if( !hMapFile )
        return;

    if( pBuf ) {
        UnmapViewOfFile( pBuf );
        pBuf = NULL;
    }

    if( hMapFile ) {
        CloseHandle( hMapFile );
        hMapFile = NULL;
    }
}


#else

class SMF {
public:
    string getName()    {return "Requires Windows OS.";}
};

#endif

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

typedef map<int,NetClient*> NetClientMap;

/* ---------------------------------------------------------------- */
/* Macros --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifndef _MSC_VER
#define _strcmpi    strcasecmp
#endif

#define RETURN( x )                                                 \
    do {                                                            \
        plhs[0] = mxCreateDoubleScalar( static_cast<double>(x) );   \
        return;                                                     \
    } while(0)

#define RETURN_NULL()                                               \
    do {                                                            \
        plhs[0] = mxCreateDoubleMatrix( 0, 0, mxREAL );             \
        return;                                                     \
    } while(0)

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static SMF          smf;
static NetClientMap clientMap;
static int          handleId = 0;   // keeps getting incremented..

/* ---------------------------------------------------------------- */
/* Helpers -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static NetClient *MapFind( int handle )
{
    NetClientMap::iterator    it = clientMap.find( handle );

    if( it == clientMap.end() )
        return NULL;

    return it->second;
}


static void MapPut( int handle, NetClient *client )
{
    NetClient   *old = MapFind( handle );

    if( old )
        delete old;

    clientMap[handle] = client;
}


static void MapDestroy( int handle )
{
    NetClientMap::iterator  it = clientMap.find( handle );

    if( it != clientMap.end() ) {

        delete it->second;
        clientMap.erase( it );
    }
    else {
        mexWarnMsgTxt(
        "Invalid or unknown handle passed"
        " to CalinsNetMex MapDestroy." );
    }
}


static int GetHandle( int nrhs, const mxArray *prhs[] )
{
    if( nrhs < 1 )
        mexErrMsgTxt( "Need numeric handle argument." );

    const mxArray   *handle = prhs[0];

    if( !mxIsDouble( handle )
        || mxGetM( handle ) != 1
        || mxGetN( handle ) != 1 ) {

        mexErrMsgTxt( "Handle must be a scalar double value." );
    }

    return static_cast<int>(*mxGetPr( handle ));
}


static NetClient *GetNetClient( int nrhs, const mxArray *prhs[] )
{
    NetClient   *nc = MapFind( GetHandle( nrhs, prhs ) );

    if( !nc ) {
        mexErrMsgTxt(
        "INTERNAL ERROR -- Cannot find the NetClient"
        " for the specified handle in CalinsNetMex." );
    }

    return nc;
}

/* ---------------------------------------------------------------- */
/* Handlers ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// handle = createNewClient( host, port )
//
void createNewClient(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs != 1 ) {
        mexErrMsgTxt(
        "createNewClient returns handle to LHS." );
    }

    if( nrhs != 2 ) {
        mexErrMsgTxt(
        "createNewClient requires RHS: host, port." );
    }

    const mxArray   *host = prhs[0],
                    *port = prhs[1];

    if( !mxIsChar( host ) || mxGetM( host ) != 1 )
        mexErrMsgTxt( "Hostname must be a string row vector." );

    if( !mxIsDouble( port )
        || mxGetM( port ) != 1
        || mxGetN( port ) != 1 ) {

        mexErrMsgTxt( "Port must be a scalar numeric value." );
    }

    char        *hostStr    = mxArrayToString( host );
    uint16      portNum     = static_cast<uint16>(*mxGetPr( port ));
    NetClient   *nc         = new NetClient( hostStr, portNum );
    int         h           = handleId++;

    mxFree( hostStr );

    MapPut( h, nc );

    RETURN( h );
}


// destroyClient( handle )
//
void destroyClient(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    MapDestroy( GetHandle( nrhs, prhs ) );
}


// ok = tryConnection( handle )
//
void tryConnection(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "tryConnection returns ok to LHS." );

    NetClient   *nc = GetNetClient( nrhs, prhs );

    try {

        if( !nc->tcpConnect() ) {

            mexWarnMsgTxt( nc->errorReason().c_str() );
            RETURN_NULL();
        }
    }
    catch( const SockErr &e ) {

        const string    &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        RETURN_NULL();
    }

    RETURN( 1 );
}


// closeSocket( handle )
//
void closeSocket(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    NetClient   *nc = GetNetClient( nrhs, prhs );

    nc->tcpDisconnect();
}


// ok = sendString( handle, string )
//
void sendString(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "sendString returns ok to LHS." );

    if( nrhs != 2 )
        mexErrMsgTxt( "sendString requires RHS: handle, string." );

    if( mxGetClassID( prhs[1] ) != mxCHAR_CLASS )
        mexErrMsgTxt( "sendString: arg 2 must be a string." );

    char    *tmp        = mxArrayToString( prhs[1] );
    string  theString   = tmp;
    mxFree( tmp );

    NetClient   *nc = GetNetClient( nrhs, prhs );

    try {
        nc->sendString( theString );
    }
    catch( const SockErr &e ) {

        const string    &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        RETURN_NULL();
    }

    RETURN( 1 );
}


// ok = sendMatrix( handle, matrix )
//
void sendMatrix(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "sendMatrix returns ok to LHS." );

    if( nrhs != 2 )
        mexErrMsgTxt( "sendMatrix requires RHS: handle, matrix." );

    ulong   datalen = mxGetM( prhs[1] ) * mxGetN( prhs[1] );

    switch( mxGetClassID( prhs[1] ) ) {

        case mxINT8_CLASS:
        case mxUINT8_CLASS:
        case mxCHAR_CLASS:
            break;
        case mxINT16_CLASS:
        case mxUINT16_CLASS:
            datalen *= sizeof(short);
            break;
        case mxUINT32_CLASS:
        case mxINT32_CLASS:
            datalen *= sizeof(int);
            break;
        case mxSINGLE_CLASS:
            datalen *= sizeof(float);
            break;
        case mxDOUBLE_CLASS:
            datalen *= sizeof(double);
            break;
        default:
            mexErrMsgTxt( "Sent matrix must have numeric type." );
    }

    NetClient   *nc         = GetNetClient( nrhs, prhs );
    void        *theMatrix  = mxGetPr( prhs[1] );

    try {
        nc->sendData( theMatrix, datalen );
    }
    catch( const SockErr &e ) {

        const string    &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        RETURN_NULL();
    }

    RETURN( 1 );
}


// line = readLine( handle )
//
void readLine(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "readLine returns line to LHS." );

    NetClient   *nc = GetNetClient( nrhs, prhs );

    try {
        vector<char>    line;
        nc->rcvLine( line );
        plhs[0] = mxCreateString( &line[0] );
    }
    catch( const SockErr &e ) {

        const string  &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        RETURN_NULL();  // note empty return..
    }
}


// lines[] = readLines( handle )
//
void readLines(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "readLine returns lines[] to LHS." );

    NetClient   *nc = GetNetClient( nrhs, prhs );

    try {
        vector<vector<char> >   vlines;
        nc->rcvLines( vlines );

        int m = vlines.size();

        if( m ) {

            const char  **lines = (const char**)malloc( m*sizeof(char*) );

            for( int i = 0; i < m; ++i )
                lines[i] = &vlines[i][0];

            plhs[0] = mxCreateCharMatrixFromStrings( m, lines );

            free( lines );
        }
        else
            throw SockErr( "No lines received" );
    }
    catch( const SockErr &e ) {

        const string &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        RETURN_NULL();  // empty return set
    }
}


// matrix = readMatrix( handle, datatype, dims-vector )
//
void readMatrix(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
// -------------
// Validate args
// -------------

    int ndims, datalen;

    if( !nlhs )
        mexErrMsgTxt( "readMatrix returns matrix to LHS." );

    if( nrhs < 3
        || !mxIsChar( prhs[1] )
        || !mxIsDouble( prhs[2] )
        || (ndims = mxGetN( prhs[2] )) < 2
        || mxGetM( prhs[2] ) != 1 ) {

        mexErrMsgTxt(
            "readMatrix needs arguments:\n"
            " (1) Handle\n"
            " (2) String datatype {'double', 'single', 'uint8'}\n"
            " (3) Vector of dims {1x2, 1x3, 1x4} holding m,n[,o,p]" );
    }

// ----------------
// Get datatype str
// ----------------

    const char  *str;
    int         buflen = mxGetM( prhs[1] ) * mxGetN( prhs[1] ) + 1;
    string      stdstr( buflen, '0' );

    mxGetString( prhs[1], &stdstr[0], buflen );
    str = stdstr.c_str();

// -------------------------------------
// Calculate data size (product of dims)
// -------------------------------------

    double  *pdims = mxGetPr( prhs[2] );

    if( ndims > 4 )
        ndims = 4;

    int dims[] = {
        static_cast<int>(pdims[0]),
        static_cast<int>(pdims[1]),
        (ndims >= 3 ? static_cast<int>(pdims[2]) : 1),
        (ndims >= 4 ? static_cast<int>(pdims[3]) : 1)
    };

    datalen = dims[0] * dims[1] * dims[2] * dims[3];

// --------------------------
// Adjust sizing for datatype
// --------------------------

    mxClassID   cls;

    if( !_strcmpi( str, "double" ) ) {
        cls      = mxDOUBLE_CLASS;
        datalen *= sizeof(double);
    }
    else if( !_strcmpi( str, "single" ) ) {
        cls      = mxSINGLE_CLASS;
        datalen *= sizeof(float);
    }
    else if( !_strcmpi( str, "int8" ) ) {
        cls      = mxINT8_CLASS;
        datalen *= sizeof(char);
    }
    else if( !_strcmpi( str, "uint8" ) ) {
        cls      = mxUINT8_CLASS;
        datalen *= sizeof(char);
    }
    else if( !_strcmpi( str, "int16" ) ) {
        cls      = mxINT16_CLASS;
        datalen *= sizeof(short);
    }
    else if( !_strcmpi( str, "uint16" ) ) {
        cls      = mxUINT16_CLASS;
        datalen *= sizeof(short);
    }
    else if( !_strcmpi( str, "int32" ) ) {
        cls      = mxINT32_CLASS;
        datalen *= sizeof(int);
    }
    else if( !_strcmpi( str, "uint32" ) ) {
        cls      = mxUINT32_CLASS;
        datalen *= sizeof(int);
    }
    else {
        mexErrMsgTxt(
        "Output matrix type must be one of {"
        "'single', 'double', 'int8', 'uint8',"
        " 'int16', 'int16', 'int32', 'uint32'." );
    }

// --------------------
// Allocate destination
// --------------------

    plhs[0] = mxCreateNumericArray( ndims, dims, cls, mxREAL );

// --------
// Get data
// --------

    NetClient   *nc = GetNetClient( nrhs, prhs );

    try {
        nc->receiveData( mxGetData( plhs[0] ), datalen );
    }
    catch( const SockErr &e ) {

        const string &why = e.why();

        if( why.length() )
            mexWarnMsgTxt( why.c_str() );

        mxDestroyArray( plhs[0] );
        plhs[0] = 0;

        RETURN_NULL();
    }
}


// filename = fastGetFilename()
//
void fastGetFilename(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
    if( nlhs < 1 )
        mexErrMsgTxt( "fastGetFilename returns filename to LHS." );

    plhs[0] = mxCreateString( smf.getName().c_str() );
}

/* ---------------------------------------------------------------- */
/* Dispatch - Entry point ----------------------------------------- */
/* ---------------------------------------------------------------- */

typedef void (*T_Mexfunc)( int, mxArray**, int, const mxArray** );

static map<string,T_Mexfunc>    cmd2fun;


// var = mexFunction( cmdstring, var )
//
void mexFunction(
    int             nlhs,
    mxArray         *plhs[],
    int             nrhs,
    const mxArray   *prhs[] )
{
// ---------------------------------------
// Build function table (lowercase keys!!)
// ---------------------------------------

    if( !cmd2fun.size() ) {
        cmd2fun["create"]                       = createNewClient;
        cmd2fun["destroy"]                      = destroyClient;
        cmd2fun["connect"]                      = tryConnection;
        cmd2fun["disconnect"]                   = closeSocket;
        cmd2fun["sendstring"]                   = sendString;
        cmd2fun["sendmatrix"]                   = sendMatrix;
        cmd2fun["readline"]                     = readLine;
        cmd2fun["readlines"]                    = readLines;
        cmd2fun["readmatrix"]                   = readMatrix;
        cmd2fun["getspikeglfilenamefromshm"]    = fastGetFilename;
    }

// ------------------
// At least two args?
// ------------------

    const mxArray   *cmd;
    string          scmd,
                    serr;

    if( nrhs < 2 ) {
        serr += "CalinsNetMex requires at least two RHS args.\n";
        goto usage;
    }
    else
        cmd = prhs[0];

// --------------------
// Command is a string?
// --------------------

    if( !mxIsChar( cmd ) ) {
        serr +=  "CalinsNetMex arg 1 must be a string.\n";
        goto usage;
    }

    if( mxGetM( cmd ) != 1 ) {
        serr += "CalinsNetMex arg 1 must be a row vector.\n";
        goto usage;
    }

// -------------------------
// Convert command to string
// -------------------------

    {
        char    *tmp = mxArrayToString( cmd );
        scmd = tmp;
        mxFree( tmp );
    }

// --------------------------------
// Dispatch using lowercase command
// --------------------------------

    {
        transform( scmd.begin(), scmd.end(), scmd.begin(), tolower );

        map<string,T_Mexfunc>::iterator it = cmd2fun.find( scmd );

        if( it != cmd2fun.end() ) {
            it->second( nlhs, plhs, nrhs - 1, prhs + 1 );
            return;
        }
    }

    serr += "CalinsNetMex unknown command <" + scmd + ">.\n";

// -----
// Error
// -----

usage:
    serr += "Legal commands:\n";

    map<string,T_Mexfunc>::iterator it = cmd2fun.begin();

    while( it != cmd2fun.end() )
        serr += " - " + it++->first + "\n";

    mexErrMsgTxt( serr.c_str() );
}


