#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"

#include <QMap>
#include <QString>

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for IMEC configuration
//
class CimCfg
{
    // ---------
    // Constants
    // ---------

public:
    enum TermConfig {
        Default     = -1,
        RSE         = 10083,
        NRSE        = 10078,
        Diff        = 10106,
        PseudoDiff  = 12529
    };

    enum imTypeId {
        imTypeAP    = 0,
        imTypeLF    = 1,
        imTypeSY    = 2,
        imSumAP     = 0,
        imSumNeural = 1,
        imSumAll    = 2,
        imNTypes    = 3
    };

    // -----
    // Types
    // -----

    struct IMVers {
        QString hwr,
                bas,
                api,
                pSN;
        int     opt;
    };

    // ------
    // Params
    // ------

    // derived:
    // imCumTypCnt[]

public:
    VRange      range;
    double      srate,
                apGain,
                lfGain;
    QString     dev1,
                dev2,
                uiMNStr1,
                uiMAStr1,
                uiXAStr1,
                uiXDStr1;
    int         imCumTypCnt[imNTypes];
    TermConfig  termCfg;
    bool        enabled,
                softStart;

    // -------------
    // Param methods
    // -------------

public:
    double chanGain( int ic ) const;

    void deriveChanCounts (int opt );

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

// -------
// Statics
// -------

public:

    // -----
    // Types
    // -----

    typedef QMap<QString,QStringList>   DeviceChanMap;
    typedef QMap<QString,int>           DeviceChanCount;

    // ----
    // Data
    // ----

    static DeviceChanCount  aiDevChanCount,
                            aoDevChanCount,
                            diDevLineCount;

    // -------
    // Methods
    // -------

    static TermConfig stringToTermConfig( const QString &txt );
    static QString termConfigToString( TermConfig t );

    static bool getVersions( QStringList &sl, IMVers &imVers );
};

#endif  // CIMCFG_H


