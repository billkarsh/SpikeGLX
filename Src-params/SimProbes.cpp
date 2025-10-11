
#include "SimProbes.h"
#include "Util.h"
#include "CimCfg.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* SPAddr --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool SPAddr::operator<( const SPAddr &rhs ) const
{
    if( s < rhs.s )
        return true;
    if( s > rhs.s )
        return false;
    if( p < rhs.p )
        return true;
    if( p > rhs.p )
        return false;
    if( d < rhs.d )
        return true;
    if( d > rhs.d )
        return false;
    return e < rhs.e;
}


bool SPAddr::operator==( const SPAddr &rhs ) const
{
    return s == rhs.s && p == rhs.p && d == rhs.d && e == rhs.e;
}

/* ---------------------------------------------------------------- */
/* SimProbes ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void SimProbes::loadSettings()
{
    maddr.clear();
    shwrslots.clear();

    STDSETTINGS( S, "imsimprobes" );
    S.beginGroup( "SIMPROBES" );

    int np = S.value( "naddr", 0 ).toInt();

    for( int ip = 0; ip < np; ++ip ) {
        const QStringList   sl =
            S.value( QString("addr%1").arg( ip ), "" ).toStringList();
        if( sl.size() == 5 ) {
            maddr[SPAddr(
                sl[0].toInt(), sl[1].toInt(),
                sl[2].toInt(), sl[3].toInt())] = sl[4];
        }
    }
}


void SimProbes::saveSettings( QMap<SPAddr,QString> &probes )
{
    maddr = probes;

    STDSETTINGS( S, "imsimprobes" );
    S.beginGroup( "SIMPROBES" );

    int np = maddr.size(),
        ip = 0;

    S.setValue( "naddr", np );

    QMap<SPAddr,QString>::const_iterator
        it  = maddr.begin(),
        end = maddr.end();

    for( ; it != end; ++it ) {
        QStringList sl;
        sl.append( QString("%1").arg( it.key().s ) );
        sl.append( QString("%1").arg( it.key().p ) );
        sl.append( QString("%1").arg( it.key().d ) );
        sl.append( QString("%1").arg( it.key().e ) );
        sl.append( QString("%1").arg( it.value() ) );
        S.setValue( QString("addr%1").arg( ip++ ), sl );
    }
}


void SimProbes::addHwrSlot( int slot )
{
    shwrslots.insert( slot );
}


bool SimProbes::isSimSlot( int slot ) const
{
    return slot >= CimCfg::imSlotSIMMin ||
            !shwrslots.contains( slot );
}


bool SimProbes::isSimProbe( int slot, int port, int dock ) const
{
    return slot >= CimCfg::imSlotSIMMin ||
            maddr.contains( SPAddr( slot, port, dock, true ) );
}


QString SimProbes::file( int slot, int port, int dock ) const
{
    return maddr[SPAddr( slot, port, dock, true )];
}


