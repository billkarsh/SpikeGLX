
#include "SimProbes.h"
#include "Util.h"
#include "CimCfg.h"

#include <QSettings>


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

    return d < rhs.d;
}


void SimProbes::loadSettings()
{
    maddr.clear();
    shwrslots.clear();

    STDSETTINGS( S, "imsimprobes" );
    S.beginGroup( "SIMPROBES" );

    int np = S.value( "naddr", 0 ).toInt();

    for( int i = 0; i < np; ++i ) {
        const QStringList   sl =
            S.value( QString("addr%1").arg( i ), "" ).toStringList();
        if( sl.size() == 4 )
            maddr[SPAddr(sl[0].toInt(),sl[1].toInt(),sl[2].toInt())]=sl[3];
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
            maddr.contains( SPAddr( slot, port, dock ) );
}


QString SimProbes::file( int slot, int port, int dock ) const
{
    return maddr[SPAddr( slot, port, dock )];
}


