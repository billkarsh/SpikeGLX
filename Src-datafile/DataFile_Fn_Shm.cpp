
#include <QtGlobal> // define OS

#ifdef Q_OS_WIN

#include "DataFile_Fn_Shm.h"
#include "Util.h"




bool DataFile_Fn_Shm::openForWrite(
    const DAQ::Params   &dp,
    const QString       &name )
{
    bool    ok = DataFile::openForWrite( dp, name );

    if( hMapFile && pBuf ) {

        QString     fn      = fileName();
        QByteArray  bytes   = fn.toUtf8();

        CopyMemory( pBuf, bytes.constData(), bytes.length() + 1 );
    }

    return ok;
}


bool DataFile_Fn_Shm::closeAndFinalize()
{
    bool    ok = DataFile::closeAndFinalize();

    if( hMapFile && pBuf )
        ZeroMemory( pBuf, 1 );

    return ok;
}


void DataFile_Fn_Shm::attach()
{
    hMapFile = CreateFileMappingA(
                INVALID_HANDLE_VALUE,   // use paging file
                NULL,                   // default security
                PAGE_READWRITE,         // read/write access
                0,                      // max object size (hi-order DWORD)
                BUF_SIZE,               // max object size (lo-order DWORD)
                mapName );              // name of mapping object

    if( !hMapFile ) {
        Error()
            << "Could not create file mapping object, error ("
            << GetLastError()
            << ").";
        return;
    }

    pBuf = MapViewOfFile(
                hMapFile,               // handle to map object
                FILE_MAP_ALL_ACCESS,    // read/write perms
                0,                      // view offset-hi
                0,                      // view offset-lo
                BUF_SIZE );             // view length

    if( !pBuf ) {
        Error()
            << "Could not map view of file, error ("
            << GetLastError()
            << ").";
        detach();
        return;
    }

    ZeroMemory( pBuf, BUF_SIZE );
}


void DataFile_Fn_Shm::detach()
{
    if( pBuf ) {
        UnmapViewOfFile( pBuf );
        pBuf = NULL;
    }

    if( hMapFile ) {
        CloseHandle( hMapFile );
        hMapFile = NULL;
    }
}

#endif  // Q_OS_WIN


