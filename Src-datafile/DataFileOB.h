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
    virtual QString streamFromObj() const   {return DAQ::Params::jsip2stream( jsOB, ip );}
    virtual QString fileLblFromObj() const  {return QString("obx%1.obx").arg(ip);}

    // --------
    // Metadata
    // --------

    virtual int numNeuralChans() const              {return 0;}
    virtual const IMROTbl* imro() const             {return 0;}
    virtual int origID2Type( int ic ) const;
    virtual const int *cumTypCnt() const            {return obCumTypCnt;}
    virtual double origID2Gain( int /*ic*/ ) const  {return 1.0;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;
    virtual ShankMap* shankMap_svy( int, int )      {return 0;}
    virtual ShankMap* shankMap( bool ) const        {return 0;}
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
        const DataFile      &,
        const QVector<uint> & ) {}

    virtual void subclassUpdateChanMap(
        const DataFile      &dfSrc,
        const QVector<uint> &indicesOfSrcChans );

private:
    void parseChanCounts();
};

#endif  // DATAFILEOB_H


