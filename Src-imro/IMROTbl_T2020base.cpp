
#include "IMROTbl_T2020base.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T2020base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T2020base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 24 * imType2020baseShanks;
    nGrp = 16;

    T.resize( imType2020baseChan );

// Generate by pairs of columns

    int ch = 0;

    for( int icol = 0; icol < nADC; icol += 2 ) {

        for( int irow = 0; irow < nGrp; ++irow ) {
            T[nADC*irow + icol]     = ch++;
            T[nADC*irow + icol + 1] = ch++;
        }
    }
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMRO_GUI IMROTbl_T2020base::edit_GUI() const
{
    IMRO_GUI    G;
    G.refs.push_back( "Ground" );
    G.refs.push_back( "Tip" );
    G.gains.push_back( apGain( 0 ) );
    G.nBase = 4;
    return G;
}


IMRO_Attr IMROTbl_T2020base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T2020base::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), 0, 0, 0 );
}


void IMROTbl_T2020base::edit_defaultROI( tImroROIs vR ) const
{
    int rowsPerBank = nChanPerBank() / _ncolhwr;

    vR.clear();

    for( int is = 0; is < imType2020baseShanks; ++is )
        vR.push_back( IMRO_ROI( is, 0, rowsPerBank ) );
}


// - Boxes are whole- or half-shank width.
// - Boxes enclose all channels per shank.
// - Canonical attributes all channels.
//
bool IMROTbl_T2020base::edit_isCanonical( tImroROIs vR ) const
{
// Assess boxes per shank

    int ne[4] = {0,0,0,0};

    for( int ib = 0; ib < (int)vR.size(); ++ib ) {

        const IMRO_ROI  &B = vR[ib];
        int             w  = B.width( _ncolhwr );

        if( !(w == _ncolhwr || w == _ncolhwr/2) )
            return false;

        ne[B.s] += B.area( _ncolhwr );
    }

    for( int is = 0; is < imType2020baseShanks; ++is ) {
        if( ne[is] != imType2020baseChPerShk )
            return false;
    }

    return edit_Attr_canonical();
}


