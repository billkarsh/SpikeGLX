#ifndef DATAFILEIMLF_H
#define DATAFILEIMLF_H

#include "DataFile.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataFileIMLF : public DataFile
{
private:
    // Input mode
    int     imCumTypCnt[CimCfg::imNTypes];
    IMROTbl roTbl;

public:
    DataFileIMLF( int iProbe = 0 ) : DataFile(iProbe)   {}

    virtual QString subtypeFromObj() const  {return "imec.lf";}
    virtual QString streamFromObj() const   {return QString("imec%1").arg(iProbe);}
    virtual QString fileLblFromObj() const  {return QString("imec%1.lf").arg(iProbe);}

    // ---------
    // Meta data
    // ---------

    const int *cumTypCnt() const            {return imCumTypCnt;}
    int origID2Type( int ic ) const;
    virtual double origID2Gain( int ic ) const;
    virtual ChanMap* chanMap() const;
    virtual ShankMap* shankMap() const      {return 0;}

protected:
    virtual void subclassParseMetaData();
    virtual void subclassStoreMetaData( const DAQ::Params &p );
    virtual int subclassGetAcqChanCount( const DAQ::Params &p );
    virtual int subclassGetSavChanCount( const DAQ::Params &p );

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc );

    virtual void subclassUpdateShankMap(
        const DataFile      &,
        const QVector<uint> & ) {}

    virtual void subclassUpdateChanMap(
        const DataFile      &other,
        const QVector<uint> &idxOtherChans );

private:
    void parseChanCounts();
};

#endif  // DATAFILEIMLF_H


