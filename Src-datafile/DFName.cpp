
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
        runName(runName), t("0"), g(0), fldPerPrb(false)
{
}


// Parse full filePath into parts useable for flat or folder
// organizations. Deduce if folders are used.
//
DFRunTag::DFRunTag( const QString &filePath )
{
    QRegExp re("([^/\\\\]+)_g(\\d+)_t(\\d+|cat)(.*)");

    re.setCaseSensitivity( Qt::CaseInsensitive );

    int i = filePath.indexOf( re );

    runName = re.cap(1);
    g       = re.cap(2).toInt();
    t       = re.cap(3);

    if( re.cap(4).contains( ".nidq", Qt::CaseInsensitive ) ) {

        runDir = filePath.left( i );

        fldPerPrb =
            QDir(
                QString("%1%2_g%3_imec0")
                .arg( runDir )
                .arg( runName ).arg( g )
            ).exists();
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
    }
}


QString DFRunTag::run_g_t() const
{
    return QString("%1_g%2_t%3").arg( runName ).arg( g ).arg( t );
}


// Create file name without path.
// ip = -1, suffix = {bin, meta}.
// ip = 0+, suffix = {ap.bin, lf.meta, etc}.
//
QString DFRunTag::brevname( int ip, const QString &suffix ) const
{
    if( ip < 0 ) {

        return QString("%1.nidq.%2")
                .arg( run_g_t() ).arg( suffix );
    }
    else {
        return QString("%1.imec%2.%3")
                .arg( run_g_t() ).arg( ip ).arg( suffix );
    }
}


// Create full filename.
// ip = -1, suffix = {bin, meta}.
// ip = 0+, suffix = {ap.bin, lf.meta, etc}.
//
QString DFRunTag::filename( int ip, const QString &suffix ) const
{
    if( ip < 0 ) {

        return QString("%1%2_g%3_t%4.nidq.%5")
                .arg( runDir )
                .arg( runName ).arg( g ).arg( t )
                .arg( suffix );
    }
    else if( fldPerPrb ) {

        return QString("%1%2_g%3_imec%5/%2_g%3_t%4.imec%5.%6")
                .arg( runDir )
                .arg( runName ).arg( g ).arg( t )
                .arg( ip ).arg( suffix );
    }
    else {
        return QString("%1%2_g%3_t%4.imec%5.%6")
                .arg( runDir )
                .arg( runName ).arg( g ).arg( t )
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

    return QString::null;
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
    QRegExp re("\\.imec.*|\\.nidq.*");
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


// Return type code {-1=error, 0=AP, 1=LF, 2=NI}.
//
// ip = {-1=NI, 0=imec0, 1=imec1...}.
//
int DFName::typeAndIP( int &ip, const QString &name, QString *error )
{
    int type    = -1;
    ip          = -1;

    QString fname_no_path = QFileInfo( name ).fileName();
    QRegExp re_ap("\\.imec(\\d+)?\\.ap\\."),
            re_lf("\\.imec(\\d+)?\\.lf\\.");

    re_ap.setCaseSensitivity( Qt::CaseInsensitive );
    re_lf.setCaseSensitivity( Qt::CaseInsensitive );

    if( fname_no_path.contains( re_ap  ) ) {
        type    = 0;
        ip      = re_ap.cap(1).toInt();
    }
    else if( fname_no_path.contains( re_lf ) ) {
        type    = 1;
        ip      = re_lf.cap(1).toInt();
    }
    else if( fname_no_path.contains( ".nidq.", Qt::CaseInsensitive ) )
        type = 2;
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


bool DFName::isValidInputFile(
    const QString   &name,
    QString         *error,
    const QString   &reqVers )
{
    if( error )
        error->clear();

    int ip;

    if( -1 == typeAndIP( ip, name, error ) )
        return false;

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

// -----------------
// Meta file exists?
// -----------------

    mFile = forceMetaSuffix( bFile );

    fi.setFile( mFile );

    if( !fi.exists() ) {

        if( error ) {
            *error =
                QString("Meta file not found '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

// -------------------
// Meta file readable?
// -------------------

    if( !isFileReadable( fi, error ) )
        return false;

// -------------------
// Meta content valid?
// -------------------

    KVParams    kvp;

    if( !kvp.fromMetaFile( mFile ) ) {

        if( error ) {
            *error =
                QString("Meta file is corrupt '%1'.")
                .arg( fi.fileName() );
        }

        return false;
    }

    // Version check

    // BK: Viewer launch can be blocked here based upon version of
    // creating application, that is, based upon set of available
    // meta data.

    QString vFile = kvp["appVersion"].toString();

    if( vFile.compare( reqVers ) < 0 ) {

        if( error ) {
            *error =
                QString("File version >= %1 required. This file is %2.")
                .arg( reqVers )
                .arg( vFile.isEmpty() ? "unversioned" : vFile );
        }

        return false;
    }

    // Required phase 3B and finalization keys

    QString key;

    if( !kvp.contains( (key = "typeImEnabled") )
        || !kvp.contains( (key = "fileSHA1") )
        || !kvp.contains( (key = "fileTimeSecs") )
        || !kvp.contains( (key = "fileSizeBytes") ) ) {

        if( error ) {
            *error =
                QString("Meta file missing required key <%1> '%2'.")
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

// --
// OK
// --

    return true;
}


