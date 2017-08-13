#ifndef DATAFILEIMAP_H
#define DATAFILEIMAP_H

#include "DataFile.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataFileIMAP : public DataFile
{
private:
    // Input mode
    int     imCumTypCnt[CimCfg::imNTypes];
    IMROTbl roTbl;

public:
    virtual QString subtypeFromObj() const  {return "imec.ap";}
    virtual QString streamFromObj() const   {return "imec";}
    virtual QString fileLblFromObj() const  {return "imec.ap";}

    // ---------
    // Meta data
    // ---------

    int origID2Type( int ic ) const;
    virtual double origID2Gain( int ic ) const;
    virtual ChanMap* chanMap() const;
    virtual ShankMap* shankMap() const;

protected:
    virtual void subclassParseMetaData();
    virtual void subclassStoreMetaData( const DAQ::Params &p );
    virtual int subclassGetAcqChanCount( const DAQ::Params &p );
    virtual int subclassGetSavChanCount( const DAQ::Params &p );

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc );

    virtual void subclassUpdateShankMap(
        const DataFile      &other,
        const QVector<uint> &idxOtherChans );

    virtual void subclassUpdateChanMap(
        const DataFile      &other,
        const QVector<uint> &idxOtherChans );

private:
    void parseChanCounts();
};

#endif  // DATAFILEIMAP_H


