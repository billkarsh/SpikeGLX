#ifndef DATAFILE_H
#define DATAFILE_H

#include "DAQ.h"
#include "GeomMap.h"
#include "KVParams.h"

#include "SHA1.h"
#undef TCHAR

#include <QFileInfo>
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

    struct ORec {
        DFWriter                *dfw;
        QFile                   binFile;
        QVector<uint>           iKeep;
        CSHA1                   sha;
        mutable QMutex          statsMtx;
        mutable QVector<uint>   statsBytes;
        KVParams                kvp;
        QString                 metaName;
        ORec() : dfw(0) {}
        virtual ~ORec();
    };

    // Input and Output mode
    QFile                   i_binFile;
    QString                 o_baseName,
                            metaName;
    quint64                 sampCt;
    IOMode                  mode;

    // Input mode
    QString                 i_trgStream;
    int                     i_trgChan;      // neg if not using

    // Output mode only
    std::vector<std::unique_ptr<ORec>> o_rec;
    int                     o_nAcqChans;
    bool                    o_wrAsync;

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

    bool openForRead( QString &error, const QString &filename );
    bool openForWrite(
        const DAQ::Params   &p,
        int                 ig,
        int                 it,
        const QString       &forceName );
    bool openForExport(
        const DataFile      &dfSrc,
        const QString       &filename,
        const QVector<uint> &indicesOfSrcChans );

    bool isOpenForRead() const  {return mode == Input;}
    bool isOpenForWrite() const {return mode == Output;}

    virtual QString subtypeFromObj() const = 0;
    virtual QString streamFromObj() const = 0;
    virtual QString fileLblFromObj() const = 0;
    int streamip() const                {return ip;}

    QString inBinFileName() const               {return i_binFile.fileName();}
    QString outBinFileName( int j = 0 ) const
    {
        if( j >= 0 && o_rec.size() )
            return o_rec[j]->binFile.fileName();
        return QString();
    }
    const QString &metaFileName() const         {return metaName;}

    bool matchedBinFileName( const QFileInfo &fi ) const
    {
        return mode == Output && fi.filePath().contains( o_baseName );
    }

    bool closeAndFinalize();

    DataFile *closeAsync( const KeyValMap &kvm );

    // ------
    // Output
    // ------

    void setAsyncWriting( bool async )  {o_wrAsync = async;}
    bool writeAndInvalSamps( vec_i16 &samps );

    // -----
    // Input
    // -----

    qint64 readSamps(
        vec_i16         &dst,
        quint64         samp0,
        quint64         num2read,
        const QBitArray &keepBits ) const;

    // --------
    // Metadata
    // --------

    void setFirstSample( quint64 firstCt, bool write = false );
    void setParam( const QString &name, const QVariant &value );
    void setRemoteParams( const KeyValMap &kvm );
    void delParam( const QString &name );

    QString notes() const;
    void streamCounts( int &nIm, int &nOb, int &nNi ) const;
    quint64 firstCt() const;
    quint64 sampCount() const               {return sampCt;}
    double samplingRateHz() const           {return sRate;}
    double fileTimeSecs() const             {return sampCt/sRate;}
    const VRange &vRange() const            {return _vRange;}
    int numChans() const                    {return nSavedChans;}
    const QVector<uint> &fileChans() const  {return snsFileChans;}
    double ig2Gain( int ig ) const          {return origID2Gain( snsFileChans[ig] );}
    bool trig_isChan( int acqChan ) const   {return acqChan == i_trgChan;}

    virtual int numNeuralChans() const = 0;
    virtual const IMROTbl* imro() const = 0;
    virtual int origID2Type( int ic ) const = 0;
    virtual const int *cumTypCnt() const = 0;
    virtual double origID2Gain( int ic ) const = 0;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const = 0;
    virtual uint8_t sr_mask() const = 0;
    virtual int svySettleSecs() const = 0;
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
    virtual int subclassGetAcqChanCount( const DAQ::Params &p ) = 0;
    virtual void subclassGetSavChanCount( const DAQ::Params &p ) = 0;
    virtual bool subclassParseMetaData( QString *error ) = 0;
    virtual void subclassStoreMetaData( const DAQ::Params &p ) = 0;

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
    bool doFileWrite( const vec_i16 &samps, int j = 0 );
};

#endif  // DATAFILE_H


