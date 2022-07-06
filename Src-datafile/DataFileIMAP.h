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
    IMROTbl *roTbl;

public:
    DataFileIMAP( int ip = 0 ) : DataFile(ip), roTbl(0) {}
    virtual ~DataFileIMAP() {if( roTbl ) {delete roTbl, roTbl = 0;}}

    virtual QString subtypeFromObj() const  {return "imec.ap";}
    virtual QString streamFromObj() const   {return DAQ::Params::jsip2stream( 2, ip );}
    virtual QString fileLblFromObj() const  {return QString("imec%1.ap").arg( ip );}

    // ---------
    // Meta data
    // ---------

    virtual const IMROTbl* imro() const     {return roTbl;}
    virtual int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const    {return imCumTypCnt;}
    virtual double origID2Gain( int ic ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual ShankMap* shankMap( int shank, int bank );
    virtual ShankMap* shankMap() const;
    virtual ChanMap* chanMap() const;

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


