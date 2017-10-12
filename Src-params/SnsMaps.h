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
// chanMap, (ConfigCtl::validChanMap)
// saveBits
//
    QString     shankMapFile,
                chanMapFile,
                uiSaveChanStr;
    QBitArray   saveBits;
    ShankMap    shankMap;

    bool deriveSaveBits(
        QString         &err,
        const QString   &stream,
        int             n16BitChans );
};

struct SnsChansImec : public SnsChansBase {
    ShankMap    shankMap_orig;
    ChanMapIM   chanMap;
};

struct SnsChansNidq : public SnsChansBase {
    ChanMapNI   chanMap;
};

#endif  // SNSMAPS_H


