
#include "Sha1Verifier.h"
#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "Run.h"

#include "SHA1.h"
#undef TCHAR

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
            QString("Missing SHA1 tag in metafile '%1'.")
            .arg( dataFileNameShort );
        emit result( Failure );
        return;
    }

// Open file

    const int bufSize = 64*1024;

    std::vector<UINT_8> buf( bufSize );
    QFile               f( dataFileName );
    QFileInfo           fi( dataFileName );
    CSHA1               sha1;

    if( !f.open( QIODevice::ReadOnly ) ) {
        extendedError =
            QString("Can't open for reading '%1'.")
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
            << "Wrong file size in metafile ["
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
                QThread::usleep( 20 );  // allow event processing
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

        std::basic_string<char> hStr;
        sha1.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

        if( !sha1FromMeta.compare( hStr.c_str(), Qt::CaseInsensitive ) )
            r = Success;
        else {
            extendedError =
                "Computed SHA1 does not match that in metafile;"
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
            mainApp()->dataDir(),
            "BIN Files (*.bin)" );

    if( dataFile.isEmpty() )
        return;

    QFileInfo   fi( dataFile );
    KVParams    kvp;

// ----------------------------------------
// Point fi at metafile and verify metadata
// ----------------------------------------

    if( fi.suffix() == "meta" )
        dataFile.clear();
    else {

        fi.setFile(
            QString("%1/%2.meta")
            .arg( fi.path() )
            .arg( fi.completeBaseName() ) );

        if( !fi.exists() ) {

            QMessageBox::critical(
                cons,
                "Missing MetaFile",
                QString("SHA1 needs a matching metafile for\n'%1'.")
                .arg( dataFile ) );

            return;
        }
    }

    if( !kvp.fromMetaFile( fi.filePath() ) ) {

        QMessageBox::critical(
            cons,
            "MetaFile Read Error.",
            QString("SHA1 verifier could not read contents of\n'%1'.")
            .arg( fi.fileName() ) );

        return;
    }

// --------------------------------------
// Disallow operation on current acq file
// --------------------------------------

    if( !dataFile.length() ) {

        dataFile = QString("%1/%2.bin")
                    .arg( fi.path() )
                    .arg( fi.completeBaseName() );
    }

    fi.setFile( dataFile );

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

        QString str = QString("SHA1 verified '%1'.").arg( fn );

        Log() << str;

        QMessageBox::information(
            cons,
            QString("SHA1 Verify '%1'").arg( fn ),
            str );
    }
    else if( res == Sha1Worker::Failure ) {

        QString &err = worker->extendedError;

        Warning() << QString("SHA1 Verify Error [%1] '%2'")
                        .arg( err )
                        .arg( fn );

        QMessageBox::warning(
            cons,
            QString("SHA1 Verify Error '%1'").arg( fn ),
            err );
    }
    else
        Log() << QString("SHA1 verify canceled '%1'.").arg( fn );

    delete prog;
    worker->deleteLater();
    delete this;
}


