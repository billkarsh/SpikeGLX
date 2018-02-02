
#include "DFName.h"
#include "KVParams.h"


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

    // version check

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

    // finalization keys

    if( !kvp.contains( "fileSHA1" )
        || !kvp.contains( "fileTimeSecs" )
        || !kvp.contains( "fileSizeBytes" ) ) {

        if( error ) {
            *error =
                QString("Meta file missing required key '%1'.")
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


