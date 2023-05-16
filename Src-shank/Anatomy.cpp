
#include "Anatomy.h"
#include "ShankView.h"
#include "DAQ.h"

#include <QTextEdit>
#include <math.h>


/* ---------------------------------------------------------------- */
/* Anatomy -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Anatomy::parse( const QString &elems, const IMROTbl *roTbl, int sk )
{
// Remove entries for this shank

    for( int i = int(rgn.size()) - 1; i >= 0; --i ) {
        if( rgn[i].shank == sk )
            rgn.erase( rgn.begin() + i );
    }

// Parse entries

    QStringList sle = elems.split(
                        QRegExp("^\\s*\\(\\s*|\\s*\\)\\s*\\(\\s*|\\s*\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         ne  = sle.size();

    if( !ne )
        return;

    for( int ie = 0; ie < ne; ++ie ) {

        QStringList slp = sle[ie].split(
                            QRegExp("^\\s*|\\s*,\\s*|\\s*$"),
                            QString::SkipEmptyParts );
        int         np  = slp.size();

        if( np != 6 ) {
            parse( "", roTbl, sk );
            return;
        }

        AnatomyRgn R( sk );
        R.lbl   = slp[5];
        R.row0  =
            qBound(
            0,
            int(floor((slp[0].toDouble() - roTbl->tipLength())/roTbl->zPitch())),
            roTbl->nRow()-1 );
        R.rowN  =
            qBound(
            0,
            int(ceil((slp[1].toDouble() - roTbl->tipLength())/roTbl->zPitch())),
            roTbl->nRow()-1 );
        R.r     = slp[2].toInt();
        R.g     = slp[3].toInt();
        R.b     = slp[4].toInt();
        rgn.push_back( R );
    }
}


void Anatomy::fillLegend( QTextEdit *leg )
{
    leg->clear();

// Unique alphabetic names

    QMap<QString,QColor>    mlbl;
    for( int i = 0, n = rgn.size(); i < n; ++i ) {
        const AnatomyRgn   &R = rgn[i];
        mlbl[R.lbl] = QColor( R.r, R.g, R.b );
    }

// Set text

    QMap<QString,QColor>::const_iterator it = mlbl.begin(), end = mlbl.end();
    for( ; it != end; ++it ) {
        leg->setTextColor( it.value() );
        leg->append( it.key() );
    }
}


void Anatomy::colorShanks( ShankView *view, bool on )
{
    std::vector<SVAnaRgn>   vA;

    if( on ) {
        foreach( const AnatomyRgn &R, rgn )
            vA.push_back( SVAnaRgn( R.row0, R.rowN, R.shank, R.r, R.g, R.b ) );
    }

    view->setAnatomy( vA );
    view->updateNow();
}


