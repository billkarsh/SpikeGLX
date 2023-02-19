#ifndef DATAFILENI_H
#define DATAFILENI_H

#include "DataFile.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataFileNI : public DataFile
{
private:
    // Input mode
    double  mnGain,
            maGain;
    int     niCumTypCnt[CniCfg::niNTypes];

public:
    virtual QString subtypeFromObj() const  {return "nidq";}
    virtual QString streamFromObj() const   {return DAQ::Params::jsip2stream( jsNI, 0 );}
    virtual QString fileLblFromObj() const  {return "nidq";}

    // ---------
    // Meta data
    // ---------

    virtual const IMROTbl* imro() const     {return 0;}
    virtual int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const    {return niCumTypCnt;}
    virtual double origID2Gain( int ic ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual ShankMap* shankMap( int, int )  {return 0;}
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
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans );

    virtual void subclassUpdateChanMap(
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans );

private:
    void parseChanCounts();
};

#endif  // DATAFILENI_H


