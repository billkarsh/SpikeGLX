
#include "ProbeFiles.h"
#include "Util.h"
#include "CimCfg.h"

#include <QSettings>


bool PFAddr::operator<( const PFAddr &rhs ) const
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


void ProbeFiles::loadSettings()
{
    mfiles.clear();
    shwrslots.clear();

    STDSETTINGS( S, "improbefiles" );
    S.beginGroup( "PROBEFILES" );

    int np = S.value( "nfiles", 0 ).toInt();

    for( int i = 0; i < np; ++i ) {
        const QStringList   sl =
            S.value( QString("file%1").arg( i ), "" ).toStringList();
        if( sl.size() == 4 )
            mfiles[PFAddr(sl[0].toInt(),sl[1].toInt(),sl[2].toInt())]=sl[3];
    }
}


void ProbeFiles::addHwrSlot( int slot )
{
    shwrslots.insert( slot );
}


bool ProbeFiles::isSimSlot( int slot ) const
{
    return !shwrslots.contains( slot );
}


bool ProbeFiles::isSimProbe( int slot, int port, int dock ) const
{
    return mfiles.contains( PFAddr( slot, port, dock ) );
}


QString ProbeFiles::file( int slot, int port, int dock ) const
{
    return mfiles[PFAddr( slot, port, dock )];
}


