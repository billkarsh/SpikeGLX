
#include "IMROTbl_T1200.h"
#include "ShankMap.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1200::toShankMap( ShankMap &S ) const
{
    S.ns = nShank();
    S.nc = nCol_smap();
    S.nr = nRow();
    S.e.clear();

    for( int ic = 0, nC = nAP(); ic < nC; ++ic )
        S.e.push_back( ShankMapDesc( 0, 3, ic, 1 ) );
}


void IMROTbl_T1200::toShankMap_saved(
    ShankMap            &S,
    const QVector<uint> &saved,
    int                 offset ) const
{
    S.ns = nShank();
    S.nc = nCol_smap();
    S.nr = nRow();
    S.e.clear();

    int nC  = nAP(),
        nI  = qMin( saved.size(), nC );

    for( int i = 0; i < nI; ++i ) {

        int ic;

        ic = saved[i] - offset;

        if( ic >= nC )
            break;

        S.e.push_back( ShankMapDesc( 0, 3, ic, 1 ) );
    }
}


void IMROTbl_T1200::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
#define NA  128

    nADC = 12;
    nGrp = 12;

    T.resize( 144 );

// Explicit; reading across rows

    uint i = 0;

// R1
    T[i++] =  84; T[i++] =  11; T[i++] =  85; T[i++] =   5;
    T[i++] =  74; T[i++] =  10; T[i++] =  56; T[i++] = 112;
    T[i++] =  46; T[i++] = 121; T[i++] =  39; T[i++] = 127;

// R2
    T[i++] = 100; T[i++] =  26; T[i++] = 110; T[i++] =  33;
    T[i++] =  69; T[i++] =  24; T[i++] =  63; T[i++] = 109;
    T[i++] =  45; T[i++] =  93; T[i++] =  25; T[i++] =  99;

// R3
    T[i++] =  87; T[i++] =   0; T[i++] =  82; T[i++] =   6;
    T[i++] =  71; T[i++] =  15; T[i++] =  53; T[i++] = 117;
    T[i++] =  43; T[i++] = 122; T[i++] =  42; T[i++] = 116;

// R4
    T[i++] = 102; T[i++] =  28; T[i++] =  81; T[i++] =  34;
    T[i++] =  70; T[i++] =  18; T[i++] =  60; T[i++] = 103;
    T[i++] =  17; T[i++] =  94; T[i++] =  27; T[i++] = 101;

// R5
    T[i++] =  73; T[i++] =   1; T[i++] =  86; T[i++] =   7;
    T[i++] =  68; T[i++] =  16; T[i++] =  50; T[i++] = 106;
    T[i++] =  40; T[i++] = 123; T[i++] =  NA; T[i++] =  NA;

// R6
    T[i++] = 105; T[i++] =  29; T[i++] =  75; T[i++] =  35;
    T[i++] =  67; T[i++] =  12; T[i++] =  54; T[i++] =  89;
    T[i++] =  20; T[i++] =  95; T[i++] =  NA; T[i++] =  NA;

// R7
    T[i++] =  76; T[i++] =   2; T[i++] =  83; T[i++] =   8;
    T[i++] =  65; T[i++] =  13; T[i++] =  47; T[i++] = 118;
    T[i++] =  49; T[i++] = 124; T[i++] =  NA; T[i++] =  NA;

// R8
    T[i++] = 108; T[i++] =  30; T[i++] =  78; T[i++] =  36;
    T[i++] =  64; T[i++] =  14; T[i++] =  51; T[i++] =  90;
    T[i++] =  23; T[i++] =  96; T[i++] =  NA; T[i++] =  NA;

// R9
    T[i++] =  79; T[i++] =   3; T[i++] =  80; T[i++] =   9;
    T[i++] =  62; T[i++] = 114; T[i++] =  44; T[i++] = 119;
    T[i++] =  52; T[i++] = 125; T[i++] =  NA; T[i++] =  NA;

// R10
    T[i++] = 104; T[i++] =  31; T[i++] =  72; T[i++] =  37;
    T[i++] =  61; T[i++] = 113; T[i++] =  57; T[i++] =  91;
    T[i++] =  19; T[i++] =  97; T[i++] =  NA; T[i++] =  NA;

// R11
    T[i++] =  88; T[i++] =   4; T[i++] =  77; T[i++] =  21;
    T[i++] =  59; T[i++] = 111; T[i++] =  41; T[i++] = 120;
    T[i++] =  55; T[i++] = 126; T[i++] =  NA; T[i++] =  NA;

// R12
    T[i++] = 107; T[i++] =  32; T[i++] =  66; T[i++] =  38;
    T[i++] =  58; T[i++] = 115; T[i++] =  48; T[i++] =  92;
    T[i++] =  22; T[i++] =  98; T[i++] =  NA; T[i++] =  NA;
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1200::edit_init() const
{
// forward

    for( int c = 0; c < imType1200Chan; ++c )
        k2s[T0Key( c, 0 )] = IMRO_Site( 0, 3, c );

// inverse

    QMap<T0Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T1200::edit_GUI() const
{
    IMRO_GUI    G = IMROTbl_T0base::edit_GUI();
    G.grid = nRow();   // force lowest bank
    return G;
}


// Return ROI count.
//
int IMROTbl_T1200::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, nRow(), imType1200Col_smap - 1, imType1200Col_smap ) );
    return 1;
}


// - Box count: {1}.
// - Box at col 3.
// - Box encloses all AP channels.
// - Canonical attributes all channels.
//
bool IMROTbl_T1200::edit_isCanonical( tconstImroROIs vR ) const
{
    if( vR.size() != 1 )
        return false;

    const IMRO_ROI  &B = vR[0];
    int             nr = B.rLim - B.r0;

    if( B.c0 != 3 || B.cLim != 4 )
        return false;

    return nr == nAP() && edit_Attr_canonical();
}

