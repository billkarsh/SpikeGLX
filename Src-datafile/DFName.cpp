
#include "DFName.h"
#include "KVParams.h"

#include <QDir>


/* ---------------------------------------------------------------- */
/* DFRunTag ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Set to flat directory and _g0_t0 for use in calibration run.
//
DFRunTag::DFRunTag( const QString &dataDir, const QString &runName )
    :   runDir(QString("%1/%2_g0/").arg( dataDir ).arg( runName )),
        runName(runName), t("0"), g(0),
        exported(false), fldPerPrb(false), is3A(false)
{
}


// Parse full filePath into parts useable for flat or folder
// organizations. Deduce if folders are used. Also parse for
// an "exported" tag and if 3A file naming pertains.
//
// Subsequent built names using {run_g_t(), brevname(), filename()}
// will include export tag iff parsed file had export tag.
//
// 3A naming:
// - Uses ".imec." tag without probe index
// - Does not have probe folders.
//
DFRunTag::DFRunTag( const QString &filePath )
{
// Search for filename: (notslash-runName)_g(N)_t(M)(cap4-notslash)EOL

    QRegExp re("([^/\\\\]+)_g(\\d+)_t(\\d+|cat)([^/\\\\]*)$");
    re.setCaseSensitivity( Qt::CaseInsensitive );

    int i = filePath.indexOf( re );

    runName = re.cap(1);
    g       = re.cap(2).toInt();
    t       = re.cap(3);

    QString cap4 = re.cap(4);

    exported = cap4.contains( ".exported" );

    if( cap4.contains( ".nidq", Qt::CaseInsensitive ) ||
        cap4.contains( ".obx",  Qt::CaseInsensitive ) ) {

        runDir = filePath.left( i );

        fldPerPrb =
            QDir(
                QString("%1%2_g%3_imec0")
                .arg( runDir )
                .arg( runName ).arg( g )
            ).exists();

        if( fldPerPrb )
            is3A = false;
        else {

            QString base = QString("%1%2.imec.")
                            .arg( runDir ).arg( run_g_t() );

            QFileInfo   fi;

            fi.setFile( base + "ap.bin" );
            is3A = fi.exists();

            if( !is3A ) {
                fi.setFile( base + "lf.bin" );
                is3A = fi.exists();
            }
        }
    }
    else if( cap4.contains( ".imec.", Qt::CaseInsensitive ) ) {

        runDir = filePath.left( i );

        fldPerPrb   = false;
        is3A        = true;
    }
    else {

        int j = filePath.indexOf(
                    QString("%1_g%2_imec")
                    .arg( runName ).arg( g ),
                    Qt::CaseInsensitive );

        if( j >= 0 ) {
            runDir      = filePath.left( j );
            fldPerPrb   = true;
        }
        else {
            runDir      = filePath.left( i );
            fldPerPrb   = false;
        }

        is3A = false;
    }
}


QString DFRunTag::run_g_t() const
{
    return QString("%1_g%2_t%3%4")
            .arg( runName ).arg( g ).arg( t )
            .arg( exported ? ".exported" : "" );
}


// Create file name without path.
// fType: {0=AP, 1=LF, 2=OB, 3=NI}.
// fType = {2,3}: suffix = {bin, meta}.
// fType = {0,1}: suffix = {ap.bin, lf.meta, etc}.
//
QString DFRunTag::brevname( int fType, int ip, const QString &suffix ) const
{
    if( fType == 3 ) {
        return QString("%1.nidq.%2")
                .arg( run_g_t() ).arg( suffix );
    }
    if( fType == 2 ) {
        return QString("%1.obx%2.obx.%3")
                .arg( run_g_t() ).arg( ip ).arg( suffix );
    }
    else if( is3A ) {
        return QString("%1.imec.%2")
                .arg( run_g_t() ).arg( suffix );
    }
    else {
        return QString("%1.imec%2.%3")
                .arg( run_g_t() ).arg( ip ).arg( suffix );
    }
}


// Create full filename.
// fType: {0=AP, 1=LF, 2=OB, 3=NI}.
// fType = {2,3}: suffix = {bin, meta}.
// fType = {0,1}: suffix = {ap.bin, lf.meta, etc}.
//
QString DFRunTag::filename( int fType, int ip, const QString &suffix ) const
{
    if( fType == 3 ) {
        return QString("%1%2.nidq.%3")
                .arg( runDir )
                .arg( run_g_t() )
                .arg( suffix );
    }
    if( fType == 2 ) {
        return QString("%1%2.obx%3.obx.%4")
                .arg( runDir )
                .arg( run_g_t() )
                .arg( ip ).arg( suffix );
    }
    else if( is3A ) {
        return QString("%1%2.imec.%3")
                .arg( runDir )
                .arg( run_g_t() )
                .arg( suffix );
    }
    else if( fldPerPrb ) {
        return QString("%1%2_g%3_imec%5/%4.imec%5.%6")
                .arg( runDir )
                .arg( runName )
                .arg( g )
                .arg( run_g_t() )
                .arg( ip ).arg( suffix );
    }
    else {
        return QString("%1%2.imec%3.%4")
                .arg( runDir )
                .arg( run_g_t() )
                .arg( ip ).arg( suffix );
    }
}

/* ---------------------------------------------------------------- */
/* DFName --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Static --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString DFName::justExtension( const QString &name )
{
    int idot = name.lastIndexOf( "." );

    if( idot > -1 )
        return QString(name).remove( 0, idot + 1 ).toLower();

    return QString();
}


QString DFName::chopExtension( const QString &name )
{
    int idot = name.lastIndexOf( "." );

    if( idot > -1 )
        return QString(name).remove( idot, name.size() );

    return name;
}


QString DFName::chopType( const QString &name )
{
    QRegExp re("\\.imec.*|\\.obx.*|\\.nidq.*");
    re.setCaseSensitivity( Qt::CaseInsensitive );

    return QString(name).remove( re );
}


QString DFName::forceBinSuffix( const QString &name )
{
    QRegExp re("meta$");
    re.setCaseSensitivity( Qt::CaseInsensitive );

    return QString(name).replace( re, "bin" );
}


QString DFName::forceMetaSuffix( const QString &name )
{
    QRegExp re("bin$");
    re.setCaseSensitivity( Qt::CaseInsensitive );

    return QString(name).replace( re, "meta" );
}


// Return type code {-1=error, 0=AP, 1=LF, 2=OB, 3=NI}.
//
// ip = {-1=NI, 0+=substream}.
//
int DFName::typeAndIP( int &ip, const QString &name, QString *error )
{
    int type    = -1;
    ip          = -1;

    QString fname_no_path = QFileInfo( name ).fileName();
    QRegExp re_ap("\\.imec(\\d+)?\\.ap\\."),
            re_lf("\\.imec(\\d+)?\\.lf\\."),
            re_ob("\\.obx(\\d+)?\\.obx\\.");

    re_ap.setCaseSensitivity( Qt::CaseInsensitive );
    re_lf.setCaseSensitivity( Qt::CaseInsensitive );
    re_ob.setCaseSensitivity( Qt::CaseInsensitive );

    if( fname_no_path.contains( re_ap ) ) {
        type    = 0;
        ip      = re_ap.cap(1).toInt();
    }
    else if( fname_no_path.contains( re_lf ) ) {
        type    = 1;
        ip      = re_lf.cap(1).toInt();
    }
    else if( fname_no_path.contains( re_ob ) ) {
        type    = 2;
        ip      = re_ob.cap(1).toInt();
    }
    else if( fname_no_path.contains( ".nidq.", Qt::CaseInsensitive ) )
        type = 3;
    else if( error ) {
        *error =
            QString("Missing type key in file name '%1'.")
            .arg( fname_no_path );
    }

    return type;
}


bool DFName::isFileReadable( const QFileInfo &fi, QString *error )
{
    QString path = fi.canonicalFilePath();

    if( path.isEmpty()
        || !QFile( path ).open( QIODevice::ReadOnly ) ) {

        if( error ) {
            *error =
                QString("File cannot be opened for reading '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

    return true;
}


// Return size or -1 if error.
//
qint64 DFName::fileSize( const QFileInfo &fi, QString *error )
{
    qint64  size = -1;
    QFile   f( fi.canonicalFilePath() );

    if( f.open( QIODevice::ReadOnly ) )
        size = f.size();
    else if( error ) {
        *error =
            QString("File cannot be opened for reading '%1'.")
            .arg( fi.fileName() );
    }

    return size;
}


// 2016, Feb 25-29, I added metadata items into version 20160120:
// - appVersion     : still in all versions
// - typeEnabled    : phase3A only
// - typeThis       : still in all versions
//
bool DFName::isValidInputFile(
    const QString       &name,
    const QStringList   &reqKeys,
    QString             *error )
{
    if( error )
        error->clear();

    {
        int ip;

        if( -1 == typeAndIP( ip, name, error ) )
            return false;
    }

    QString     bFile = name,
                mFile;
    QFileInfo   fi( bFile );

// -------------------
// Binary file exists?
// -------------------

    if( fi.suffix() == "meta" ) {

        bFile = QString("%1/%2.bin")
                    .arg( fi.canonicalPath() )
                    .arg( fi.completeBaseName() );

        fi.setFile( bFile );
    }

    if( !fi.exists() ) {

        if( error ) {
            *error =
                QString("Binary file not found '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

// ------------------
// Binary file empty?
// ------------------

    qint64  binSize = fileSize( fi, error );

    if( binSize <= 0 ) {

        if( error ) {
            *error =
                QString("Binary file is empty '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

// ----------------
// Metafile exists?
// ----------------

    mFile = forceMetaSuffix( bFile );

    fi.setFile( mFile );

    if( !fi.exists() ) {

        if( error ) {
            *error =
                QString("Metafile not found '%1'.").arg( fi.fileName() );
        }

        return false;
    }

// ------------------
// Metafile readable?
// ------------------

    if( !isFileReadable( fi, error ) )
        return false;

// -------------------
// Meta content valid?
// -------------------

    KVParams    kvp;

    if( !kvp.fromMetaFile( mFile ) ) {

        if( error ) {
            *error =
                QString("Metafile is corrupt '%1'.").arg( fi.fileName() );
        }

        return false;
    }

    // Version check

    QString vFile = kvp["appVersion"].toString();

    if( vFile.compare( "20160120" ) < 0 ) {

        if( error ) {
            *error =
                QString("File version >= %1 required. This file is %2.")
                .arg( "20160120" )
                .arg( vFile.isEmpty() ? "unversioned" : vFile );
        }

        return false;
    }

    // Required finalization keys

    QString key;

    if( !kvp.contains( (key = "fileSHA1") )
        || !kvp.contains( (key = "fileTimeSecs") )
        || !kvp.contains( (key = "fileSizeBytes") ) ) {

        if( error ) {
            *error =
                QString("Metafile missing required key <%1> '%2'.")
                .arg( key )
                .arg( fi.fileName() );
        }

        return false;
    }

    if( kvp["fileSizeBytes"].toLongLong() != binSize ) {

        if( error ) {
            *error =
                QString("Recorded/actual file-size mismatch '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

    // Caller required keys

    foreach( const QString &key, reqKeys ) {

        if( !kvp.contains( key ) ) {

            if( error ) {
                *error =
                    QString("Metafile missing required key <%1> '%2'.")
                    .arg( key )
                    .arg( fi.fileName() );
            }

            return false;
        }
    }

// --
// OK
// --

    return true;
}


