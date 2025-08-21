
#include "IMROTbl_T1221.h"
#include "Util.h"

#include <QRegularExpression>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMROTbl_T1221::IMROTbl_T1221( const QString &pn )
    :   IMROTbl_T1200(pn)
{
    QString path = QString("%1/_ImroX/NP1221_X.ini").arg( appPath() );

    if( !QFile( path ).exists() ) {
        Log() <<
        QString("NP1221 can't find file '%1', defaulting to NP1200 layout.")
        .arg( path );
        return;
    }

    QSettings S( path, QSettings::IniFormat );
    S.beginGroup( "Layout" );

    QString     s;
    QStringList sl;

    _shankpitch = S.value( "shank_pitch_um_float", 0 ).toFloat();
    _shankwid   = S.value( "shank_width_um_float", 125 ).toFloat();
    _tiplength  = S.value( "tip_length_um_float", 372 ).toFloat();

    _ncolhwr    = S.value( "n_physical_columns", 2 ).toInt();
    _ncolvis    = S.value( "n_GUI_columns", 4 ).toInt();

    _col2vis_ev.clear();
    s  = S.value( "occupied_GUI_columns_on_even_rows", "1 3" ).toString();
    sl = s.split( QRegularExpression("\\s+"), Qt::SkipEmptyParts );
    for( int i = 0, n = sl.size(); i < n; ++i )
        _col2vis_ev.push_back( sl.at( i ).toInt() );

    _col2vis_od.clear();
    s  = S.value( "occupied_GUI_columns_on_odd_rows", "0 2" ).toString();
    sl = s.split( QRegularExpression("\\s+"), Qt::SkipEmptyParts );
    for( int i = 0, n = sl.size(); i < n; ++i )
        _col2vis_od.push_back( sl.at( i ).toInt() );

    _x0_ev   = S.value( "offset_to_first_site_center_on_even_rows_um_float", 56.5f ).toFloat();
    _x0_od   = S.value( "offset_to_first_site_center_on_odd_rows_um_float", 36.5f ).toFloat();
    _xpitch  = S.value( "physical_col_to_col_pitch_um_float", 32 ).toFloat();
    _zpitch  = S.value( "physical_row_to_row_pitch_um_float", 31 ).toFloat();
}


