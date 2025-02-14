
#include "ProbeTable.h"
#include "Util.h"
#include "IMROTbl.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>

#define FTAB   "C:/Users/labadmin/Desktop/ProbeTable/2024_11_22_Probe_feature_table_tab.txt"
#define FINI   "C:/Users/labadmin/Desktop/ProbeTable/Probe_features.ini"
#define FSRT   "C:/Users/labadmin/Desktop/ProbeTable/Probe_features_srt.ini"
#define FJ2I   "C:/Users/labadmin/Desktop/ProbeTable/Probe_features_j2i.ini"
#define FJSN   "C:/Users/labadmin/Desktop/ProbeTable/Probe_features.json"

/* ---------------------------------------------------------------- */
/* CProbeTbl ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CProbeTbl::ss2ini()
{
// Original saved as tab-sep txt
    QFile   fi( FTAB );
    fi.open( QIODevice::ReadOnly | QIODevice::Text );

// SS cols = ini keys
    QString hdr = fi.readLine().toLower();
    hdr.replace( "/ leaflet", "" );
    hdr.replace( " (y/n)", "" );
    hdr.replace( "�", "u" );
    hdr.replace( "(", "" );
    hdr.replace( ")", "" );
    hdr.replace( "-", "_" );
    hdr.replace( "/", "_per_" );
    QStringList key = hdr.split( "\t" );
    int         nk  = key.size();
    for( int i = 0; i < nk; ++i )
        key[i] = key[i].trimmed().replace( " ", "_" );

// Open output ini
    QFile   fo( FINI );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream ts( &fo );

// Parse lines = probes = group|0 + {vals}|1:k

    // Quote values containing ',' for json parser,
    // or containing INI special characters '?{}|&~![()^'
    // Ref: https://www.php.net/parse_ini_file
    // Section: Value Interpolation

    QRegExp needQuote( ".*[,\\?\\{\\}\\|\\&\\~\\!\\[\\(\\)\\^].*" );

    for(;;) {
        // get line
        QString L = fi.readLine();
        if( L.isEmpty() )
            break;

        // remove artificial line breaks
        while( L.count( "\t" ) < nk - 1 ) {
            QString N = fi.readLine().trimmed();
            L = L.trimmed() + "," + N;
        }

        // tokenize
        QStringList val = L.split( "\t" );
        val[0] = val[0].trimmed();
        if( val[0].isEmpty() )
            break;

        // group = part number
        ts<<QString("[%1]\n").arg( val[0] );

        // key=value lines
        for( int i = 1; i < nk; ++i ) {
            QString V = val[i].trimmed().replace( "\"", "" );
            if( V == "/" )
                V = "0";
            if( V.contains( needQuote ) )
                ts<<QString("%1=\"%2\"\n").arg( key[i] ).arg( V );
            else
                ts<<QString("%1=%2\n").arg( key[i] ).arg( V );
        }
        ts<<"\n";
    }
}


void CProbeTbl::extini()
{
    QSettings   S( FINI, QSettings::IniFormat );

// Insert imro type column

    foreach( const QString &grp, S.childGroups() ) {
        S.beginGroup( grp );
        QString val;
        if( grp.contains( "NP111" ) )
            val = "imro_np1110";
        else if( grp.contains( "NP202" ) )
            val = "imro_np2020";
        else if( grp.contains( "NP301" ) )
            val = "imro_np3010";
        else if( grp.contains( "NP302" ) )
            val = "imro_np3020";
        else if( S.value( "has_ap_bandpass" ).toString().toUpper() == "N" ) {
            if( S.value( "num_shanks" ) == 4 )
                val = "imro_np2010";
            else
                val = "imro_np2000";
        }
        else
            val = "imro_np1000";
        S.setValue( "imro_table_format_type", val );
        S.endGroup();
    }

// Insert z_mux_tables group

    S.beginGroup( "z_mux_tables" );
    {
        QStringList key =
            {"mux_np1000","mux_np1200","mux_np2000",
             "mux_np2020","mux_np3010","mux_np3020"},
                    prb =
            {"NP1000","NP1200","NP2000",
             "NP2020","NP3010","NP3020"};
        for( int i = 0, n = key.size(); i < n; ++i ) {
            IMROTbl *R = IMROTbl::alloc( prb[i] );
            S.setValue( key[i], R->muxTable_toString() );
            delete R;
        }
    }
    S.endGroup();

// Insert z_imro_formats group

    S.beginGroup( "z_imro_formats" );
    {
        S.setValue(
            "imro_np1000_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np1000_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np1000_elm_flds",
            "(channel bank ref_id ap_gain lf_gain ap_hipas_flt)" );
        S.setValue(
            "imro_np1000_elm_fmt",
            "(%d %d %d %d %d %d)" );
        S.setValue(
            "imro_np1000_val_def",
            "type:0"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,tip)(2,bnk0)(3,bnk1)(4,bnk2)"
            " ap_gain:ap_gain_list"
            " lf_gain:lf_gain_list"
            " ap_hipas_flt:[0,1]" );

        S.setValue(
            "imro_np1110_hdr_flds",
            "(type,col_mode,ref_id,ap_gain,lf_gain,ap_hipas_flt)" );
        S.setValue(
            "imro_np1110_hdr_fmt",
            "(%d,%d,%d,%d,%d,%d)" );
        S.setValue(
            "imro_np1110_elm_flds",
            "(group bankA bankB)" );
        S.setValue(
            "imro_np1110_elm_fmt",
            "(%d %d %d)" );
        S.setValue(
            "imro_np1110_val_def",
            "type:1110"
            " col_mode:(0,INNER)(1,OUTER)(2,ALL)"
            " ref_id:(0,ext)(1,tip)"
            " ap_gain:ap_gain_list"
            " lf_gain:lf_gain_list"
            " ap_hipas_flt:[0,1]" );

        S.setValue(
            "imro_np2000_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np2000_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np2000_elm_flds",
            "(channel bank_mask ref_id electrode)" );
        S.setValue(
            "imro_np2000_elm_fmt",
            "(%d %d %d %d)" );
        S.setValue(
            "imro_np2000_val_def",
            "type:21"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank_mask:(bit0,bnk0)(bit1,bnk1)(bit2,bnk2)"
            " ref_id:(0,ext)(1,tip)(2,bnk0)(3,bnk1)(4,bnk2)"
            " electrode:[0,electrodes_per_shank-1]" );

        S.setValue(
            "imro_np2003_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np2003_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np2003_elm_flds",
            "(channel bank_mask ref_id electrode)" );
        S.setValue(
            "imro_np2003_elm_fmt",
            "(%d %d %d %d)" );
        S.setValue(
            "imro_np2003_val_def",
            "type:21"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank_mask:(bit0,bnk0)(bit1,bnk1)(bit2,bnk2)"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        S.setValue(
            "imro_np2010_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np2010_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np2010_elm_flds",
            "(channel shank bank ref_id electrode)" );
        S.setValue(
            "imro_np2010_elm_fmt",
            "(%d %d %d %d %d)" );
        S.setValue(
            "imro_np2010_val_def",
            "type:24"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)"
            "(1,tip0)(2,tip1)(3,tip2)(4,tip3)"
            "(5,shk0,bnk0)(6,shk0,bnk1)(7,shk0,bnk2)(8,shk0,bnk3)"
            "(9,shk1,bnk0)(10,shk1,bnk1)(11,shk1,bnk2)(12,shk1,bnk3)"
            "(13,shk2,bnk0)(14,shk2,bnk1)(15,shk2,bnk2)(16,shk2,bnk3)"
            "(17,shk3,bnk0)(18,shk3,bnk1)(19,shk3,bnk2)(20,shk3,bnk3)"
            " ap_gain:ap_gain_list"
            " lf_gain:lf_gain_list"
            " ap_hipas_flt:[0,1]" );

        S.setValue(
            "imro_np2013_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np2013_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np2013_elm_flds",
            "(channel shank bank ref_id electrode)" );
        S.setValue(
            "imro_np2013_elm_fmt",
            "(%d %d %d %d %d)" );
        S.setValue(
            "imro_np2013_val_def",
            "type:24"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip0)(3,tip1)(4,tip2)(5,tip3)"
            " electrode:[0,electrodes_per_shank-1]" );

        S.setValue(
            "imro_np2020_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np2020_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np2020_elm_flds",
            "(channel shank bank ref_id electrode)" );
        S.setValue(
            "imro_np2020_elm_fmt",
            "(%d %d %d %d %d)" );
        S.setValue(
            "imro_np2020_val_def",
            "type:2020"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        S.setValue(
            "imro_np3010_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np3010_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np3010_elm_flds",
            "(channel bank ref_id electrode)" );
        S.setValue(
            "imro_np3010_elm_fmt",
            "(%d %d %d %d)" );
        S.setValue(
            "imro_np3010_val_def",
            "type:3010"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        S.setValue(
            "imro_np3020_hdr_flds",
            "(type,num_channels)" );
        S.setValue(
            "imro_np3020_hdr_fmt",
            "(%d,%d)" );
        S.setValue(
            "imro_np3020_elm_flds",
            "(channel shank bank ref_id electrode)" );
        S.setValue(
            "imro_np3020_elm_fmt",
            "(%d %d %d %d %d)" );
        S.setValue(
            "imro_np3020_val_def",
            "type:3020"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip0)(3,tip1)(4,tip2)(5,tip3)"
            " electrode:[0,electrodes_per_shank-1]" );
    }
    S.endGroup();
}


void CProbeTbl::sortini()
{
    QMap<QString,QMap<QString,int>> D;

    QFile   fi( FINI );
    fi.open( QIODevice::ReadOnly | QIODevice::Text );

    QMap<QString,int>   M;
    QString             L, G;
    for(;;) {
        L = fi.readLine();
        if( L.isEmpty() ) {
            if( !G.isEmpty() )
                D[G] = M;
            break;
        }
        else if( L.contains( "=" ) )
            M[L] = 1;
        else if( L.startsWith( "[" ) ) {
            G = L;
            M.clear();
        }
        else {
            D[G] = M;
            G = QString();
        }
    }

    QFile   fo( FSRT );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream ts( &fo );

    QMap<QString,QMap<QString,int>>::iterator
        itd = D.begin(), endd = D.end();

    for( ; itd != endd; ++itd ) {
        ts << itd.key();
        QMap<QString,int>::iterator
            itm = itd.value().begin(), endm = itd.value().end();
        for( ; itm != endm; ++itm )
            ts << itm.key();
        if( itd + 1 != endd )
            ts << "\n";
    }
}


void CProbeTbl::ini2json()
{
    QSettings   S( FINI, QSettings::IniFormat );
    QJsonObject root,
                probes;

    foreach( const QString &grp, S.childGroups() ) {
        if( grp.startsWith( "z" ) ) {
            QJsonObject M;
            S.beginGroup( grp );
            foreach( const QString &key, S.childKeys() )
                M[key] = S.value( key ).toString();
            S.endGroup();
            root[grp] = M;
        }
        else {
            QJsonObject P;
            P["part_number"] = grp;
            S.beginGroup( grp );
            foreach( const QString &key, S.childKeys() )
                P[key] = S.value( key ).toString();
            S.endGroup();
            probes[grp] = P;
        }
    }
    root["neuropixels_probes"] = probes;

    QFile   fo( FJSN );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    fo.write( QJsonDocument( root ).toJson() );
}


void CProbeTbl::json2ini()
{
    QJsonParseError err;
    QJsonDocument   doc;
    QJsonValue      vtbl;
    QJsonObject     tbl;
    QFile           fi( FJSN );
    fi.open( QIODevice::ReadOnly | QIODevice::Text );
    doc = QJsonDocument::fromJson( fi.readAll(), &err );
    if( err.error != QJsonParseError::NoError ) {
        Error() << QString("Json parse error at pos %1 : '%2'.")
                    .arg( err.offset ).arg( err.errorString() );
        return;
    }

    QSettings   S( FJ2I, QSettings::IniFormat );

// probes

    vtbl = doc.object().value( "neuropixels_probes" );
    if( !vtbl.isObject() ) {
        Error() << "Expected 'neuropixels_probes' at root level.";
        return;
    }

    tbl = vtbl.toObject();
    foreach( const QString &pn, tbl.keys() ) {
        S.beginGroup( pn );
        QJsonObject probe = tbl.value( pn ).toObject();
        foreach( const QString &key, probe.keys() ) {
            if( key != "part_number" )
                S.setValue( key, probe.value( key ).toString() );
        }
        S.endGroup();
    }

// imro table

    vtbl = doc.object().value( "z_imro_formats" );
    if( !vtbl.isObject() ) {
        Error() << "Expected 'z_imro_formats' at root level.";
        return;
    }

    tbl = vtbl.toObject();
    S.beginGroup( "z_imro_formats" );
    foreach( const QString &key, tbl.keys() )
        S.setValue( key, tbl.value( key ).toString() );
    S.endGroup();

// mux table

    vtbl = doc.object().value( "z_mux_tables" );
    if( !vtbl.isObject() ) {
        Error() << "Expected 'z_mux_tables' at root level.";
        return;
    }

    tbl = vtbl.toObject();
    S.beginGroup( "z_mux_tables" );
    foreach( const QString &key, tbl.keys() )
        S.setValue( key, tbl.value( key ).toString() );
    S.endGroup();
}


void CProbeTbl::parsejson()
{
    QJsonParseError err;
    QJsonDocument   doc;
    QFile           fi( FJSN );
    fi.open( QIODevice::ReadOnly | QIODevice::Text );
    doc = QJsonDocument::fromJson( fi.readAll(), &err );
    if( err.error != QJsonParseError::NoError ) {
        Error() << QString("Json parse error at pos %1 : '%2'.")
                    .arg( err.offset ).arg( err.errorString() );
        return;
    }

    // There should be root object : neuropixels_probes
    QJsonValue vtbl = doc.object().value( "neuropixels_probes" );
    if( !vtbl.isObject() ) {
        Error() << "Expected 'neuropixels_probes' at root level.";
        return;
    }

    // List the probe part numbers
    QJsonObject tbl = vtbl.toObject();
    foreach( const QString &type, tbl.keys() )
        Log()<<type;
}

/* ---------------------------------------------------------------- */
/* Debug ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


