#ifndef DATAFILE_FN_SHM_H
#define DATAFILE_FN_SHM_H

#include "DataFile.h"

#ifdef Q_OS_WIN

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Subclass of DataFile that stores current file name
// in shared memory, which is accessible twice as fast
// if in Windows and on the same machine.
//
class DataFile_Fn_Shm : public DataFile
{
private:
    const char  *mapName;
    HANDLE      hMapFile;
    void        *pBuf;
    const uint  BUF_SIZE;

public:
    DataFile_Fn_Shm()
    :   DataFile(),
        mapName("Global\\SpikeGLFileNameShm"),
        hMapFile(NULL), pBuf(0), BUF_SIZE(1024)
        {attach();}
    virtual ~DataFile_Fn_Shm()
        {detach();}

    bool openForWrite(
        const DAQ::Params   &dp,
        const QString       &name );

    bool closeAndFinalize();

private:
    void attach();
    void detach();
};

#else   // !Windows
#define DataFile_Fn_Shm DataFile
#endif

#endif  // DATAFILE_FN_SHM_H


