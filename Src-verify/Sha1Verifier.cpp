
#include "Sha1Verifier.h"
#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "Run.h"

#define SHA1_HAS_TCHAR
#include "SHA1.h"

#include <QThread>
#include <QProgressDialog>
#include <QMessageBox>
#include <QFileDialog>


/* ---------------------------------------------------------------- */
/* Sha1Worker ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

Sha1Worker::Sha1Worker(
    const QString   &dataFileName,
    const KeyValMap &kvm )
    :   QObject(0),
        dataFileName(dataFileName), kvm(kvm),
        pleaseStop(false)
{
    dataFileNameShort = QFileInfo( dataFileName ).fileName();
}


void Sha1Worker::run()
{
    extendedError.clear();
    emit progress( 0 );

// Get metafile tag

    QString sha1FromMeta = kvm["fileSHA1"].toString().trimmed();

    if( sha1FromMeta.isEmpty() ) {
        extendedError =
            QString("Missing sha1 tag in '%1' meta file.")
                .arg( dataFileNameShort );
        emit result( Failure );
        return;
    }

// Open file

    const int bufSize = 64*1024;

    QVector<UINT_8> buf( bufSize );
    QFile           f( dataFileName );
    QFileInfo       fi( dataFileName );
    CSHA1           sha1;

    if( !f.open( QIODevice::ReadOnly ) ) {
        extendedError =
            QString("'%1' could not be opened for reading.")
                .arg( dataFileNameShort );
        emit result( Failure );
        return;
    }

// Check size

    qint64  size    = fi.size(),
            read    = 0,
            step    = qMax( 1LL, size/100 ),
            lastPct = 0;

    if( size != kvm["fileSizeBytes"].toLongLong() ) {
        Warning()
            << "Wrong file size in meta file ["
            << kvm["fileSizeBytes"].toString()
            << "] vs actual ("
            << size
            << ") for data file '"
            << dataFileNameShort
            << "'.";
    }

// Hash

    while( !isStopped() && !f.atEnd() ) {

//        qint64  bytes = readChunky( f, &buf[0], bufSize );
        qint64  bytes = f.read( (char*)&buf[0], bufSize );

        if( bytes < 0 ) {
            extendedError = f.errorString();
            break;
        }
        else if( bytes > 0 ) {

            qint64 pct = (read += bytes) / step;

            sha1.Update( &buf[0], bytes );

            if( pct >= lastPct + 5 ) {
                emit progress( pct );
                lastPct = pct;
                usleep( 20 );   // periodically allow event processing
            }
        }
        else {

            if( !f.atEnd() )
                extendedError = f.errorString();

            break;
        }
    }

// Report

    Result  r = Failure;

    if( isStopped() )
        r = Canceled;
    else if( f.atEnd() && extendedError.isEmpty() ) {

        sha1.Final();

        std::basic_string<TCHAR>    hStr;
        sha1.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

        if( !sha1FromMeta.compare( hStr.c_str(), Qt::CaseInsensitive ) )
            r = Success;
        else {
            extendedError =
                "Computed SHA1 does not match that in meta file;"
                " data file corrupt.";
            r = Failure;
        }
    }

    if( lastPct < 100 )
        emit progress( 100 );

    emit result( r );
}

/* ---------------------------------------------------------------- */
/* Sha1Verifier --------------------------------------------------- */
/* ---------------------------------------------------------------- */

Sha1Verifier::Sha1Verifier()
    :   QObject(0), cons(0), prog(0), thread(0), worker(0)
{
// -----------
// Pick a file
// -----------

    cons = mainApp()->console();

    QString dataFile =
        QFileDialog::getOpenFileName(
            cons,
            "Select data file for SHA1 verification",
            mainApp()->runDir() );

    if( dataFile.isEmpty() )
        return;

    QFileInfo   fi( dataFile );
    KVParams    kvp;

// ---------------------
// Point fi at meta file
// ---------------------

    if( fi.suffix() != "meta" ) {

        fi.setFile( QString("%1/%2.meta")
                        .arg( fi.path() )
                        .arg( fi.completeBaseName() ) );
    }
    else
        dataFile.clear();

    if( !fi.exists() ) {

        QMessageBox::critical(
            cons,
            "Missing Meta File",
            QString("SHA1 needs a matching meta-file for\n[%1].")
                .arg( dataFile ) );

        return;
    }

// ------------------------------
// Get binary file from meta data
// ------------------------------

    if( !kvp.fromMetaFile( fi.filePath() ) ) {

        QMessageBox::critical(
            cons,
            QString("%1 Read Error.")
                .arg( fi.fileName() ),
            QString("SHA1 verifier could not read contents of\n[%1].")
                .arg( fi.fileName() ) );

        return;
    }

    if( !dataFile.length() ) {

        dataFile = QString("%1/%2.bin")
                    .arg( fi.path() )
                    .arg( fi.completeBaseName() );
    }

    mainApp()->makePathAbsolute( dataFile );

// --------------------------------------
// Disallow operation on current acq file
// --------------------------------------

    fi = QFileInfo( dataFile );

    if( mainApp()->getRun()->dfIsInUse( fi ) ) {

        QMessageBox::critical(
            cons,
            "Selected File In Use",
            "Cannot run SHA1 on the current data acquisition file." );

        return;
    }

// -----
// Begin
// -----

    prog = new QProgressDialog(
                QString("Verifying SHA1 hash of '%1'...")
                    .arg( fi.fileName() ),
                "Cancel",
                0, 100,
                cons );

    prog->setWindowFlags( prog->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    thread  = new QThread;
    worker  = new Sha1Worker( dataFile, kvp );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );

    Connect( worker, SIGNAL(progress(int)), prog, SLOT(setValue(int)) );
    Connect( prog, SIGNAL(canceled()), this, SLOT(cancel()) );
    Connect( worker, SIGNAL(result(int)), this, SLOT(result(int)) );

    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );
    Connect( thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

    prog->show();
    thread->start();
}


void Sha1Verifier::result( int res )
{
    QString &fn = worker->dataFileNameShort;

    if( res == Sha1Worker::Success ) {

        QString str = QString("'%1' SHA1 verified.").arg( fn );

        Log() << str;

        QMessageBox::information(
            cons,
            QString("'%1' SHA1 Verify").arg( fn ),
            str );
    }
    else if( res == Sha1Worker::Failure ) {

        QString &err = worker->extendedError;

        Warning() << QString("'%1' SHA1 Verify Error [%2]")
                        .arg( fn )
                        .arg( err );

        QMessageBox::warning(
            cons,
            QString("'%1' SHA1 Verify Error").arg( fn ),
            err );
    }
    else
        Log() << QString("'%1' SHA1 verify canceled.").arg( fn );

    delete prog;
    worker->deleteLater();
    delete this;
}


