#ifndef DATAFILEOB_H
#define DATAFILEOB_H

#include "DataFile.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataFileOB : public DataFile
{
private:
    // Input mode
    int obCumTypCnt[CimCfg::obNTypes];

public:
    DataFileOB( int ip = 0 ) : DataFile(ip) {}

    virtual QString subtypeFromObj() const  {return "obx";}
    virtual QString streamFromObj() const   {return DAQ::Params::jsip2stream( 1, ip );}
    virtual QString fileLblFromObj() const  {return QString("obx%1.obx").arg(ip);}

    // ---------
    // Meta data
    // ---------

    int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const        {return obCumTypCnt;}
    virtual double origID2Gain( int ic ) const  {return 1.0;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual void muxTable( int &, int &, std::vector<int> & ) const {}
    virtual ChanMap* chanMap() const;
    virtual ShankMap* shankMap() const          {return 0;}

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

#endif  // DATAFILEOB_H

