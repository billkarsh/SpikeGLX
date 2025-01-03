
#include "Stim.h"
#include "SGLTypes.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DAQ.h"
#include "Subset.h"
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;

#include <QThread>
#include <QVector>
#include <string.h>


#define NDATA   10000


// Load buffer with NDATA/2 values 1.0V :: NDATA/2 values -1.0V.
//
static void square10K( qint16 *B )
{
    const double    vmax = 0x7FFF * 1.0/5.0;

    for( int i = 0; i < NDATA/2; ++i )
        B[i] = vmax;
    for( int i = NDATA/2; i < NDATA; ++i )
        B[i] = -vmax;
}


void CStim::stimtest()
{
    vec_i16 buf;
    buf.resize( NDATA );
    square10K( &buf[0] );

    int slot = 21;

//    Log()<<"scan "<<np_scanBS();
//    Log()<<"omap "<<np_mapBS( 21470026, slot );
//    Log()<<"enab "<<np_DAC_enableOutput( slot, 0, true );

// Notes:
// Waveplayer fixed output voltage range [-5,5] V.

    Log()<<"freq "<<np_waveplayer_setSampleFrequency( slot, NDATA );
    Log()<<"writ "<<np_waveplayer_writeBuffer( slot, &buf[0], NDATA );
    Log()<<"warm "<<np_waveplayer_arm( slot, false );
    Log()<<"trig "<<np_setSWTriggerEx( slot, swtrigger2 );
    guiBreathe();

    QThread::msleep( 5000 );
    Log()<<"kill "<<np_waveplayer_arm( slot, true );
}


// In:  list: (AO-index,voltage)()...
// Out: error msg or empty.
//
QString CStim::obx_set_AO( int istr, const QString &chn_vlt )
{
// Decode slot, available AO

    ConfigCtl           *C      = mainApp()->cfgCtl();
    const DAQ::Params   &p      = C->acceptedParams;
    QVector<uint>       vAO;
    int                 isel    = p.im.obx_istr2isel( istr ),
                        slot    = C->prbTab.get_iOneBox( isel ).slot;

    Subset::rngStr2Vec( vAO, p.im.get_iStrOneBox( istr ).uiAOStr );

// Parse into ()

    QStringList prn  = chn_vlt.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         nprn = prn.size();

    if( !nprn ) {
        return
        QString("No (chan,voltage) pairs given for Obx %1 slot %2.")
        .arg( istr ).arg( slot );
    }

// Parse each ()

    QVector<double> vlt;
    QVector<int>    chn;

    foreach( const QString &s, prn ) {

        QStringList val = s.split(
                            QRegExp("^\\s+|\\s*,\\s*"),
                            QString::SkipEmptyParts );

        if( val.size() != 2 ) {
            return
            QString("Malformed chan-voltage pair on Obx %1 slot %2.")
            .arg( istr ).arg( slot );
        }

        bool    okv, okc;
        double  v = val[1].toDouble( &okv );
        int     c = val[0].toInt( &okc );

        if( !okv || v < -5.0 || v > 5.0 ) {
            return
            QString("Voltage '%1' not in range [-5,5] on Obx %2 slot %3.")
            .arg( val[1] ).arg( istr ).arg( slot );
        }

        if( !okc || !vAO.contains( c ) ) {
            return
            QString("Channel '%1' not enabled on Obx %2 slot %3.")
            .arg( val[0] ).arg( istr ).arg( slot );
        }

        vlt.push_back( v );
        chn.push_back( c );
    }

// Set values

    for( int ip = 0; ip < nprn; ++ip ) {

        NP_ErrorCode err = np_DAC_setVoltage( slot, chn[ip], vlt[ip] );

        if( err != SUCCESS ) {
            return
            QString("DAC_setVoltage(slot %1, chn %2, %3)%4")
            .arg( slot ).arg( chn[ip] ).arg( vlt[ip] )
            .arg( makeErrorString( err ) );
        }
    }

    return QString();
}


QString CStim::makeErrorString( int err )
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
}


