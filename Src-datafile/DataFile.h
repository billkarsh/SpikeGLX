#ifndef DATAFILE_H
#define DATAFILE_H

#include "DAQ.h"
#include "KVParams.h"
#include "Vec2.h"
#include "WrapBuffer.h"

#define SHA1_HAS_TCHAR
#include "SHA1.h"

#include <QList>
#include <QFile>
#include <QPair>
#include <QVector>
#include <QMutex>

class DFWriter;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// virtual base class
//
class DataFile
{
    friend class DFWriterWorker;
    friend class DFCloseAsyncWorker;

public:
    // List of <scan_number,number_of_scans>
    // runs of bad/faked data due to overrun.
    typedef QList<QPair<quint64,quint64> >  BadData;

private:
    enum IOMode {
        Undefined,
        Input,
        Output
    };

    // Input and Output mode
    QFile           binFile;
    QString         metaName;
    BadData         badData;
    quint64         scanCt;
    IOMode          mode;

    // Input mode
    int             trgChan;    // neg if not using

    // Output mode only
    mutable QMutex  statsMtx;
    WrapT<Vec2>     meas;
    CSHA1           sha;
    DFWriter        *dfw;
    bool            wrAsync;

protected:
    // Input and Output mode
    KVParams        kvp;
    QVector<uint>   chanIds;    // orig (acq) ids
    double          sRate;
    int             nSavedChans;

public:
    DataFile();
    virtual ~DataFile();

    // ----------
    // Open/close
    // ----------

    static bool isValidInputFile(
        const QString   &filename,
        QString         *error = 0 );

    bool openForRead( const QString &filename );

    bool openForWrite( const DAQ::Params &p, const QString &binName );

    // Special purpose method for FileViewerWindow exporter.
    // Data from preexisting 'other' file are copied to 'filename'.
    // 'idxOtherChans' are chanIds[] indices, not array elements.
    // For example, if other contains channels: {0,1,2,3,6,7,8},
    // export the last three by setting idxOtherChans = {4,5,6}.

    bool openForExport(
        const DataFile      &other,
        const QString       &filename,
        const QVector<uint> &idxOtherChans );

    bool isOpen() const {return binFile.isOpen();}
    bool isOpenForRead() const {return isOpen() && mode == Input;}
    bool isOpenForWrite() const {return isOpen() && mode == Output;}

    QString binFileName() const {return binFile.fileName();}
    const QString &metaFileName() const {return metaName;}

    bool closeAndFinalize();

    DataFile *closeAsync( const KeyValMap &kvm );

    // ------
    // Output
    // ------

    void setAsyncWriting( bool async )  {wrAsync = async;}

    bool writeAndInvalScans( vec_i16 &scans );
    bool writeAndInvalSubset( const DAQ::Params &p, vec_i16 &scans );

    // --------
    // Bad data
    // --------

    void pushBadData( quint64 scan, quint64 length )
        {badData.push_back( QPair<quint64,quint64>( scan, length ) );}

    const BadData &badDataList() const {return badData;}

    // -----
    // Input
    // -----

    // Read scans from file (after openForRead()).
    // Return number of scans actually read or -1 on failure.
    // If num2read > available, available count is used.

    qint64 readScans(
        vec_i16         &dst,
        quint64         scan0,
        quint64         num2read,
        const QBitArray &keepBits ) const;

    // ---------
    // Meta data
    // ---------

    void setParam( const QString &name, const QVariant &value );

    void setRemoteParams( const KeyValMap &kvm );

    quint64 scanCount() const {return scanCt;}
    int numChans() const {return nSavedChans;}
    double samplingRateHz() const {return sRate;}
    double fileTimeSecs() const {return scanCt/sRate;}
    const QVector<uint> &channelIDs() const {return chanIds;}
    int triggerChan() const {return trgChan;}

    const QVariant &getParam( const QString &name ) const;

    // ----
    // SHA1
    // ----

    static bool verifySHA1( const QString &filename );

    // ----------------------
    // Performance monitoring
    // ----------------------

    double percentFull() const;
    double writeSpeedBps() const;

    double requiredBps() const
        {return sRate*nSavedChans*sizeof(qint16);}

protected:
    virtual void subclassParseMetaData() = 0;
    virtual void subclassStoreMetaData( const DAQ::Params &p ) = 0;
    virtual int subclassGetAcqChanCount( const DAQ::Params &p ) = 0;
    virtual int subclassGetSavChanCount( const DAQ::Params &p ) = 0;

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc ) = 0;

    virtual void subclassListSavChans(
        QVector<uint>       &v,
        const DAQ::Params   &p ) = 0;

private:
    bool doFileWrite( const vec_i16 &scans );
};

#endif  // DATAFILE_H


