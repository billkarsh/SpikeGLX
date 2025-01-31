
#include "ProbeTable.h"
#include "Util.h"
#include "IMROTbl.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* CProbeTbl ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CProbeTbl::ss2ini()
{
// Original saved as tab-sep txt
    QFile   fi("C:/Users/labadmin/Desktop/ProbeTable/2024_11_22_Probe_feature_table_tab.txt");
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
    QFile   fo("C:/Users/labadmin/Desktop/ProbeTable/Probe_features.ini");
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
            L = L.trimmed() + ";" + N;
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

// Insert ~mux_tables group
    {
        ts<<"[~mux_tables]\n";
        QStringList key =
            {"mux_np1000","mux_np1200","mux_np2000",
             "mux_np2020","mux_np3010","mux_np3020"},
                    prb =
            {"NP1000","NP1200","NP2000",
             "NP2020","NP3010","NP3020"};
        for( int i = 0, n = key.size(); i < n; ++i ) {
            IMROTbl *R = IMROTbl::alloc( prb[i] );
            ts<<QString("%1=\"%2\"\n").arg( key[i] ).arg( R->muxTable_toString() );
            delete R;
        }
        ts<<"\n";
    }

// Insert ~imro_formats group
    {
        ts<<"[~imro_formats]\n";

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1000_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1000_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1000_elm_flds" )
            .arg( "(channel bank ref_id ap_gain lf_gain ap_hipas_flt)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1000_elm_fmt" )
            .arg( "(%d %d %d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1000_val_def" )
            .arg(
            "type:0"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,tip)(2,bnk0)(3,bnk1)(4,bnk2)"
            " ap_gain:ap_gain_list"
            " lf_gain:lf_gain_list"
            " ap_hipas_flt:[0,1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1110_hdr_flds" )
            .arg( "(type,col_mode,ref_id,ap_gain,lf_gain,ap_hipas_flt)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1110_hdr_fmt" )
            .arg( "(%d,%d,%d,%d,%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1110_elm_flds" )
            .arg( "(group bankA bankB)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1110_elm_fmt" )
            .arg( "(%d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np1110_val_def" )
            .arg(
            "type:1110"
            " col_mode:(0,INNER)(1,OUTER)(2,ALL)"
            " ref_id:(0,ext)(1,tip)"
            " ap_gain:ap_gain_list"
            " lf_gain:lf_gain_list"
            " ap_hipas_flt:[0,1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2000_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2000_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2000_elm_flds" )
            .arg( "(channel bank_mask ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2000_elm_fmt" )
            .arg( "(%d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2000_val_def" )
            .arg(
            "type:21"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank_mask:(bit0,bnk0)(bit1,bnk1)(bit2,bnk2)"
            " ref_id:(0,ext)(1,tip)(2,bnk0)(3,bnk1)(4,bnk2)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2003_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2003_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2003_elm_flds" )
            .arg( "(channel bank_mask ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2003_elm_fmt" )
            .arg( "(%d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2003_val_def" )
            .arg(
            "type:21"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank_mask:(bit0,bnk0)(bit1,bnk1)(bit2,bnk2)"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2010_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2010_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2010_elm_flds" )
            .arg( "(channel shank bank ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2010_elm_fmt" )
            .arg( "(%d %d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2010_val_def" )
            .arg(
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

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2013_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2013_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2013_elm_flds" )
            .arg( "(channel shank bank ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2013_elm_fmt" )
            .arg( "(%d %d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2013_val_def" )
            .arg(
            "type:24"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip0)(3,tip1)(4,tip2)(5,tip3)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2020_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2020_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2020_elm_flds" )
            .arg( "(channel shank bank ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2020_elm_fmt" )
            .arg( "(%d %d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np2020_val_def" )
            .arg(
            "type:2020"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3010_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3010_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3010_elm_flds" )
            .arg( "(channel bank ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3010_elm_fmt" )
            .arg( "(%d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3010_val_def" )
            .arg(
            "type:3010"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3020_hdr_flds" )
            .arg( "(type,num_channels)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3020_hdr_fmt" )
            .arg( "(%d,%d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3020_elm_flds" )
            .arg( "(channel shank bank ref_id electrode)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3020_elm_fmt" )
            .arg( "(%d %d %d %d %d)" );
        ts<<QString("%1=\"%2\"\n")
            .arg( "imro_np3020_val_def" )
            .arg(
            "type:3020"
            " num_channels:num_readout_channels"
            " channel:[0,num_readout_channels-1]"
            " shank:[0,num_shanks-1]"
            " bank:[0,banks_per_shank-1]"
            " ref_id:(0,ext)(1,gnd)(2,tip0)(3,tip1)(4,tip2)(5,tip3)"
            " electrode:[0,electrodes_per_shank-1]" );

        ts<<"\n";
    }
}


void CProbeTbl::ini2json()
{
    QSettings   S( "C:/Users/labadmin/Desktop/ProbeTable/Probe_features.ini", QSettings::IniFormat );
    QJsonObject root,
                probes;

    foreach( const QString &grp, S.childGroups() ) {
        if( grp.startsWith( "~" ) ) {
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

    QFile   fo( "C:/Users/labadmin/Desktop/ProbeTable/Probe_features.json" );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    fo.write( QJsonDocument( root ).toJson() );
}


void CProbeTbl::parsejson()
{
    QJsonParseError err;
    QJsonDocument   doc;
    QFile           fi( "C:/Users/labadmin/Desktop/ProbeTable/Probe_features.json" );
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


