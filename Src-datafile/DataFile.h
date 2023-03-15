#ifndef DATAFILE_H
#define DATAFILE_H

#include "DAQ.h"
#include "GeomMap.h"
#include "KVParams.h"

#include "SHA1.h"
#undef TCHAR

#include <QFile>
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

private:
    enum IOMode {
        Undefined,
        Input,
        Output
    };

    // Input and Output mode
    QFile                   binFile;
    QString                 metaName;
    quint64                 sampCt;
    IOMode                  mode;

    // Input mode
    QString                 trgStream;
    int                     trgChan;    // neg if not using

    // Output mode only
    mutable QMutex          statsMtx;
    mutable QVector<uint>   statsBytes;
    CSHA1                   sha;
    DFWriter                *dfw;
    int                     nMeasMax;
    bool                    wrAsync;

protected:
    // Input and Output mode
    KVParams                kvp;
    QVector<uint>           snsFileChans;   // orig (acq) ids
    VRange                  _vRange;
    double                  sRate;
    int                     ip,
                            nSavedChans;

public:
    DataFile( int ip = 0 );
    virtual ~DataFile();

    // ----------
    // Open/close
    // ----------

    bool openForRead( const QString &filename, QString &error );
    bool openForWrite(
        const DAQ::Params   &p,
        int                 ig,
        int                 it,
        const QString       &forceName );
    bool openForExport(
        const DataFile      &dfSrc,
        const QString       &filename,
        const QVector<uint> &indicesOfSrcChans );

    bool isOpen() const         {return binFile.isOpen();}
    bool isOpenForRead() const  {return isOpen() && mode == Input;}
    bool isOpenForWrite() const {return isOpen() && mode == Output;}

    virtual QString subtypeFromObj() const = 0;
    virtual QString streamFromObj() const = 0;
    virtual QString fileLblFromObj() const = 0;
    int streamip() const                {return ip;}

    QString binFileName() const         {return binFile.fileName();}
    const QString &metaFileName() const {return metaName;}

    bool closeAndFinalize();

    DataFile *closeAsync( const KeyValMap &kvm );

    // ------
    // Output
    // ------

    void setAsyncWriting( bool async )  {wrAsync = async;}

    bool writeAndInvalSamps( vec_i16 &samps );
    bool writeAndInvalSubset( const DAQ::Params &p, vec_i16 &samps );

    // -----
    // Input
    // -----

    qint64 readSamps(
        vec_i16         &dst,
        quint64         samp0,
        quint64         num2read,
        const QBitArray &keepBits ) const;

    // ---------
    // Meta data
    // ---------

    void setFirstSample( quint64 firstCt, bool write = false );
    void setParam( const QString &name, const QVariant &value );
    void setRemoteParams( const KeyValMap &kvm );

    QString notes() const;
    int probeType() const;
    void streamCounts( int &nIm, int &nOb, int &nNi ) const;
    quint64 firstCt() const;
    quint64 sampCount() const               {return sampCt;}
    double samplingRateHz() const           {return sRate;}
    double fileTimeSecs() const             {return sampCt/sRate;}
    const VRange &vRange() const            {return _vRange;}
    int numChans() const                    {return nSavedChans;}
    const QVector<uint> &fileChans() const  {return snsFileChans;}
    double ig2Gain( int ig ) const          {return origID2Gain( snsFileChans[ig] );}
    bool trig_isChan( int acqChan ) const   {return acqChan == trgChan;}

    virtual int numNeuralChans() const = 0;
    virtual const IMROTbl* imro() const = 0;
    virtual int origID2Type( int ic ) const = 0;
    virtual const int *cumTypCnt() const = 0;
    virtual double origID2Gain( int ic ) const = 0;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const = 0;
    virtual ShankMap* shankMap_svy( int shank, int bank ) = 0;
    virtual ShankMap* shankMap( bool forExport ) const = 0;
    virtual ChanMap* chanMap() const = 0;

    const QVariant getParam( const QString &name ) const;

    // ----
    // SHA1
    // ----

    static bool verifySHA1( const QString &filename );

    // ----------------------
    // Performance monitoring
    // ----------------------

    double percentFull() const;
    double writtenBytes() const;
    double requiredBps() const  {return sRate*nSavedChans*sizeof(qint16);}

protected:
    virtual void subclassParseMetaData() = 0;
    virtual void subclassStoreMetaData( const DAQ::Params &p ) = 0;
    virtual int subclassGetAcqChanCount( const DAQ::Params &p ) = 0;
    virtual int subclassGetSavChanCount( const DAQ::Params &p ) = 0;

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc ) = 0;

    virtual GeomMap* geomMap( bool forExport ) const = 0;

    virtual void subclassUpdateGeomMap(
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans ) = 0;

    virtual void subclassUpdateShankMap(
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans ) = 0;

    virtual void subclassUpdateChanMap(
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans ) = 0;

private:
    bool doFileWrite( const vec_i16 &samps );
};

#endif  // DATAFILE_H


