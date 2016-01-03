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
    QFile           dataFile;
    QString         metaName;
    KVParams        kvp;
    BadData         badData;
    QVector<uint>   chanIds;    // orig (acq) ids
    double          sRate;
    quint64         scanCt;
    int             nSavedChans;
    IOMode          mode;

    // Input mode
    VRange          niRange;
    double          mnGain,
                    maGain;
    int             niCumTypCnt[CniCfg::niNTypes];
    int             trgMode,
                    trgChan;    // neg if not using

    // Output mode only
    mutable QMutex  statsMtx;
    WrapT<Vec2>     meas;
    CSHA1           sha;
    DFWriter        *dfw;
    bool            wrAsync;

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

    bool isOpen() const {return dataFile.isOpen();}
    bool isOpenForRead() const {return isOpen() && mode == Input;}
    bool isOpenForWrite() const {return isOpen() && mode == Output;}

    QString fileName() const {return dataFile.fileName();}
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
        const QBitArray &keepBits );

    // ---------
    // Meta data
    // ---------

    void setParam( const QString &name, const QVariant &value );

    void setRemoteParams( const KeyValMap &kvm );

    quint64 scanCount() const {return scanCt;}
    quint64 sampleCount() const {return scanCt*nSavedChans;}
    int numChans() const {return nSavedChans;}
    double samplingRateHz() const {return sRate;}
    double fileTimeSecs() const {return scanCt/sRate;}
    const VRange &niRng() const {return niRange;}
    const QVector<uint> &channelIDs() const {return chanIds;}
    const int *niCumCnt() const {return niCumTypCnt;}
    int triggerChan() const {return trgChan;}
    int origID2Type( int ic ) const;
    double origID2Gain( int ic ) const;
    ChanMap chanMap() const;

    const QVariant &getParam( const QString &name ) const;

    // ----
    // SHA1
    // ----

    static bool verifySHA1( const QString &filename );

    // ----------------------
    // Performance monitoring
    // ----------------------

    double percentFull() const;
    double writeSpeedBytesSec() const;

    double minimalWriteSpeedRequired() const
        {return sRate*nSavedChans*sizeof(qint16);}

private:
    void parseChanCounts();
    void setSaveChanCounts( const DAQ::Params &p );
    bool doFileWrite( const vec_i16 &scans );
};

#endif  // DATAFILE_H


