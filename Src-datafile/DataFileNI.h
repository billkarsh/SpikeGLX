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
    virtual QString streamFromObj() const   {return "nidq";}
    virtual QString fileLblFromObj() const  {return "nidq";}

    // ---------
    // Meta data
    // ---------

    int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const    {return niCumTypCnt;}
    virtual double origID2Gain( int ic ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual void muxTable( int &, int &, std::vector<int> & ) const {}
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

#endif  // DATAFILENI_H


