
#include "Util.h"
#include "ShankViewUtils.h"

#include <QRegularExpression>




// Lut read from resource "shanklut.csv".
// Expected format:
// Header: "Index,Red,Green,Blue"
// 256 entries, eg., "0,0,0,0"
//
void loadLut( std::vector<SColor> &lut )
{
    lut.clear();

    QFile   f( ":/CommonResources/shanklut.csv" );

    if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        lut.assign( 256, SColor() );

        QStringList sl = QString( f.readAll() ).split(
                            QRegularExpression("[\r\n]+"),
                            Qt::SkipEmptyParts );
        int         n  = sl.size();

        if( n != 257 ) {
            Error() << "Invalid shanklut resource format.";
            return;
        }

        for( int i = 1; i <= 256; ++i ) {

            QStringList cl = sl[i].split(
                        QRegularExpression("^\\s+|\\s*,\\s*"),
                        Qt::SkipEmptyParts );

            SColor  &C = lut[cl[0].toUInt()];
            C.r = cl[1].toUInt();
            C.g = cl[2].toUInt();
            C.b = cl[3].toUInt();
        }
    }
}


