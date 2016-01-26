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
    VRange  niRange;
    double  mnGain,
            maGain;
    int     niCumTypCnt[CniCfg::niNTypes];

public:
    // ---------
    // Meta data
    // ---------

    const VRange &niRng() const {return niRange;}
    int origID2Type( int ic ) const;
    double origID2Gain( int ic ) const;
    ChanMapNI chanMap() const;

protected:
    virtual void subclassParseMetaData();
    virtual void subclassStoreMetaData( const DAQ::Params &p );
    virtual int subclassGetAcqChanCount( const DAQ::Params &p );
    virtual int subclassGetSavChanCount( const DAQ::Params &p );

    virtual void subclassSetSNSChanCounts(
        const DAQ::Params   *p,
        const DataFile      *dfSrc );

    virtual void subclassListSavChans(
        QVector<uint>       &v,
        const DAQ::Params   &p );

private:
    void parseChanCounts();
};

#endif  // DATAFILENI_H


