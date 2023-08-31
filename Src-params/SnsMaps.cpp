
#include "SnsMaps.h"
#include "Subset.h"




/* ---------------------------------------------------------------- */
/* SnsChansBase --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Derive from persistent settings:
//
// saveBits
//
// Return true if uiSaveChanStr format OK.
//
bool SnsChansBase::deriveSaveBits(
    QString         &err,
    const QString   &stream,
    int             n16BitChans )
{
    err.clear();

    if( Subset::isAllChansStr( uiSaveChanStr ) ) {

        uiSaveChanStr = "all";
        Subset::defaultBits( saveBits, n16BitChans );
    }
    else if( Subset::rngStr2Bits( saveBits, uiSaveChanStr ) ) {

        uiSaveChanStr = Subset::bits2RngStr( saveBits );

        if( !saveBits.count( true ) ) {
            err = QString(
                    "You must specify at least one [%1] channel to save.")
                    .arg( stream );
            return false;
        }

        if( saveBits.size() > n16BitChans ) {
            err = QString(
                    "Save subset [%1] includes channels"
                    " higher than maximum [%2].")
                    .arg( stream )
                    .arg( n16BitChans - 1 );
            return false;
        }

        // in case smaller
        saveBits.resize( n16BitChans );
    }
    else {
        err = QString("Channel save subset [%1] has incorrect format.")
                .arg( stream );
        return false;
    }

    return true;
}


// Call SnsChansBase::deriveSaveBits, and if ok:
// - Add SY channel.
// - Honor lfPairChk iff (nAP != 0).
// - Tidy up uiSaveChanStr.
//
bool SnsChansImec::deriveSaveData(
    QString         &err,
    const QString   &stream,
    int             nC,
    int             nAP )
{
    if( !deriveSaveBits( err, stream, nC ) )
        return false;

    QBitArray   &B = saveBits;

// Always add sync

    B.setBit( nC - 1 );

// Pair LF to AP

    if( nAP ) {

        bool    isAP = false;

        for( int b = 0; b < nAP; ++b ) {
            if( (isAP = B.testBit( b )) )
                break;
        }

        if( isAP ) {

            B.fill( 0, nAP, 2 * nAP );

            for( int b = 0; b < nAP; ++b ) {

                if( B.testBit( b ) )
                    B.setBit( nAP + b );
            }
        }
    }

// Neaten text

    if( B.count( true ) == nC )
        uiSaveChanStr = "all";
    else
        uiSaveChanStr = Subset::bits2RngStr( B );

    return true;
}


// Call SnsChansBase::deriveSaveBits, and if ok:
// - Add SY channel.
// - Tidy up uiSaveChanStr.
//
bool SnsChansObx::deriveSaveData(
    QString         &err,
    const QString   &stream,
    int             nC )
{
    if( !deriveSaveBits( err, stream, nC ) )
        return false;

    QBitArray   &B = saveBits;

// Always add sync

    B.setBit( nC - 1 );

// Neaten text

    if( B.count( true ) == nC )
        uiSaveChanStr = "all";
    else
        uiSaveChanStr = Subset::bits2RngStr( B );

    return true;
}


