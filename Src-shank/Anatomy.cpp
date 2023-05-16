
#include "Anatomy.h"
#include "ShankView.h"
#include "DAQ.h"
#include "MGraph.h"

#include <QTextEdit>
#include <math.h>


/* ---------------------------------------------------------------- */
/* Anatomy -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Anatomy::parse( const QString &elems, const IMROTbl *roTbl, int sk )
{
    rgnMtx.lock();

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
            rgnMtx.unlock();
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

    rgnMtx.unlock();
}


void Anatomy::fillLegend( QTextEdit *leg ) const
{
    leg->clear();

// Unique alphabetic names

    QMap<QString,QColor>    mlbl;

    rgnMtx.lock();
        for( int i = 0, n = rgn.size(); i < n; ++i ) {
            const AnatomyRgn   &R = rgn[i];
            mlbl[R.lbl] = QColor( R.r, R.g, R.b );
        }
    rgnMtx.unlock();

// Set text

    QMap<QString,QColor>::const_iterator it = mlbl.begin(), end = mlbl.end();
    for( ; it != end; ++it ) {
        leg->setTextColor( it.value() );
        leg->append( it.key() );
    }
}


void Anatomy::colorShanks( ShankView *view, bool on ) const
{
    std::vector<SVAnaRgn>   vA;

    if( on ) {
        rgnMtx.lock();
            foreach( const AnatomyRgn &R, rgn )
                vA.push_back( SVAnaRgn( R.row0, R.rowN, R.shank, R.r, R.g, R.b ) );
        rgnMtx.unlock();
    }

    view->setAnatomy( vA );
    view->updateNow();
}


void Anatomy::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY, bool on )
{
    if( on ) {
        rgnMtx.lock();

        // Push unique rgn colors onto theX->yColor.
        // Assign anaclr to each rgn.

        for( int ir = 0, nr = rgn.size(); ir < nr; ++ir ) {

            AnatomyRgn  &R = rgn[ir];
            int         nc = theX->yColor.size();
            QColor      C( R.r, R.g, R.b );

            for( int ic = 0; ic < nc; ++ic ) {
                if( theX->yColor[ic] == C ) {
                    R.anaclr = ic;
                    goto next_rgn;
                }
            }

            R.anaclr = nc;
            theX->yColor.push_back( C );
next_rgn:;
        }

        // Color each graph by its {shank,row}

        for( int iy = 0, ny = vY.size(); iy < ny; ++iy ) {

            MGraphY &Y = vY[iy];

            foreach( const AnatomyRgn &R, rgn ) {

                if( Y.anashank == R.shank &&
                    Y.anarow >= R.row0 &&
                    Y.anarow <= R.rowN ) {

                    Y.anaclr = R.anaclr;
                    break;
                }
            }
        }

        rgnMtx.unlock();
    }
    else {
        for( int iy = 0, ny = vY.size(); iy < ny; ++iy )
            vY[iy].anaclr = -1;
    }
}


