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

    // --------
    // Metadata
    // --------

    virtual int numNeuralChans() const;
    virtual const IMROTbl* imro() const         {return 0;}
    virtual int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const        {return niCumTypCnt;}
    virtual double origID2Gain( int ic ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual ShankMap* shankMap_svy( int, int )  {return 0;}
    virtual ShankMap* shankMap( bool forExport ) const;
    virtual ChanMap* chanMap() const;

protected:
    virtual bool subclassParseMetaData( QString *error );
    virtual void subclassStoreMetaData( const DAQ::Params &p );
    virtual int subclassGetAcqChanCount( const DAQ::Params &p );
    virtual int subclassGetSavChanCount( const DAQ::Params &p );

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc );

    virtual GeomMap* geomMap( bool ) const  {return 0;}

    virtual void subclassUpdateGeomMap(
        const DataFile      &,
        const QVector<uint> & ) {}

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


