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
    IMROTbl *roTbl;

public:
    DataFileIMLF( int ip = 0 ) : DataFile(ip), roTbl(0) {}
    virtual ~DataFileIMLF() {if( roTbl ) {delete roTbl, roTbl = 0;}}

    virtual QString subtypeFromObj() const  {return "imec.lf";}
    virtual QString streamFromObj() const   {return DAQ::Params::jsip2stream( 2, ip );}
    virtual QString fileLblFromObj() const  {return QString("imec%1.lf").arg( ip );}

    // ---------
    // Meta data
    // ---------

    virtual const IMROTbl* imro() const     {return roTbl;}
    virtual int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const    {return imCumTypCnt;}
    virtual double origID2Gain( int ic ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual ShankMap* shankMap( int, int )  {}
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
        const DataFile      &,
        const QVector<uint> & );

    virtual void subclassUpdateChanMap(
        const DataFile      &other,
        const QVector<uint> &idxOtherChans );

private:
    void parseChanCounts();
};

#endif  // DATAFILEIMLF_H


