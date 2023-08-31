#ifndef SNSMAPS_H
#define SNSMAPS_H

#include "ChanMap.h"
#include "ShankMap.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SnsChansBase {
//
// derived:
// saveBits
// shankMap, (ConfigCtl::validShankMap)
// chanMap,  (ConfigCtl::validChanMap)
//
    QString     shankMapFile,
                chanMapFile,
                uiSaveChanStr;
    QBitArray   saveBits;
    ShankMap    shankMap;

    virtual ~SnsChansBase() {}
    bool deriveSaveBits(
        QString         &err,
        const QString   &stream,
        int             n16BitChans );
};

struct SnsChansImec : public SnsChansBase {
    ShankMap    shankMap_orig;
    ChanMapIM   chanMap;
    bool deriveSaveData(
        QString         &err,
        const QString   &stream,
        int             nC,
        int             nAP );
};

struct SnsChansObx : public SnsChansBase {
    ChanMapOB   chanMap;
    bool deriveSaveData(
        QString         &err,
        const QString   &stream,
        int             nC );
};

struct SnsChansNidq : public SnsChansBase {
    ChanMapNI   chanMap;
};

#endif  // SNSMAPS_H


