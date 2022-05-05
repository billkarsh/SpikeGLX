
// Par2.exe is a tool for creating datafile backup sets, and for
// using those to verify datafile integrity and effect repairs.
// Most things one needs to know about Par2 are found here:
//
// http://parchive.sourceforge.net
//
// Application resources encode v0.4 of par2cmdline.exe by
// Peter Brian Clements (2003). The app deploys that binary
// as "./_Tools/par2.exe" and runs it as a sub-process.

#include "ui_Par2Window.h"

#include "Par2Window.h"
#include "Util.h"
#include "MainApp.h"
#include "DFName.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextEdit>
#include <QScrollBar>
#include <QSettings>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif


/* ---------------------------------------------------------------- */
/* Par2Worker ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Par2Worker::procStarted()
{
    processID();

    Debug()
        << "Par2 process " << pid
        << " started.";
}


void Par2Worker::procFinished( int exitCode, QProcess::ExitStatus status )
{
    Debug()
        << "Par2 process " << pid
        << " done, code: " << exitCode
        << " status: "
        << (status == QProcess::CrashExit ? "Stopped" : "Normal");

    killProc();
}


// Emitted report text does not have terminal '\n'.
//
void Par2Worker::readyOutput()
{
    QString out;

/* -------- */
/* Get text */
/* -------- */

    procMtx.lock();

    if( process )
        out = process->readAllStandardOutput();

    procMtx.unlock();

    if( !out.size() )
        return;

/* ----------- */
/* Clean it up */
/* ----------- */

// Remove all leading whites
    out.replace( QRegExp("^\\s+"), QString() );

// Convert line end runs (and any enclosing space) to single '\n'
    out.replace( QRegExp("[ ]*[\r\n]+[ ]*"), "\n" );

// Subst('\n' -> space)...
// Don't do this in front of low-case app name 'par2...'.
// Do do this in front of 'Free Software Foundation'.
// Do do this if follower not capitalized.
    out.replace( QRegExp("\n(?!par2)(?=Free|[^A-Z])"), " " );

// No terminal '\n'.
// Rather, QTextEdit::append() will insert final '\n',
// or, CmdServer will explicity add final '\n'.
    out.replace( QRegExp("\n*$"), QString() );

    if( out.size() )
        emit report( out );
}


void Par2Worker::procError( QProcess::ProcessError err )
{
    QString msg, s;

    switch( err ) {
        case QProcess::FailedToStart:   s = "Process failed to start."; break;
        case QProcess::Crashed:         s = "Process stopped."; break;
        case QProcess::Timedout:        s = "Process timed out."; break;
        case QProcess::WriteError:      s = "Process write error."; break;
        case QProcess::ReadError:       s = "Process read error."; break;
        default:                        s = "Unknown process error."; break;
    }

    msg = QString("Par2 process %1 error: %2").arg( pid ).arg( s );

    Warning() << msg;

    emit error( msg );
    killProc();
}


static void deployApp( QString &par2Path )
{
// path to executable
#ifdef Q_OS_MAC
    toolPath( par2Path, "par2", true );
#else
    toolPath( par2Path, "par2.exe", true );
#endif

// on Windows, copy it there from resources
#ifdef Q_OS_WIN
    QFile   f_out( par2Path ),
            f_in( ":/par2.exe" );

    if( f_out.open( QIODevice::WriteOnly )
        && f_in.open( QIODevice::ReadOnly ) ) {

        const int           chunk = 16384;
        std::vector<char>   buf( chunk );
        qint64              nread,
                            ntotl = 0;

        while( (nread = f_in.read( &buf[0], chunk )) > 0 ) {
            f_out.write( &buf[0], nread );
            ntotl += nread;
        }

        Debug()
            << "Copied " << ntotl
            << " bytes from resource par2.exe to "
            << f_out.fileName();
    }
#endif
}


void Par2Worker::go( const QString &file, Op op, int rPct )
{
/* ----------------- */
/* Validate settings */
/* ----------------- */

    if( isProc() ) {
        Warning() << "Par2 app already running.";
        return;
    }

    killed = false;

    if( file.isEmpty() ) {

        emit error( "Please select an input file." );
        killProc();
        return;
    }

// Apply some smart fix-ups to user selection

    QString     opStr, fname = file;
    QFileInfo   fi( file );

    if( op == Create ) {

        // If selected a par2 file, strip back to the bin file

        if( fi.suffix().compare( "par2", Qt::CaseInsensitive ) == 0 ) {

            fname = QString("%1/%2")
                    .arg( fi.path() ).arg( fi.completeBaseName() );

            fi.setFile( fname );

            if( fi.suffix().compare( "bin", Qt::CaseInsensitive ) != 0 ) {

                emit error(
                    "To CREATE a backup, select a .bin or .meta file." );
                killProc();
                return;
            }

            emit updateFilename( fname );
        }
    }
    else if( fi.suffix().compare( "par2", Qt::CaseInsensitive ) != 0 ) {

        // Should have selected the par2 file

        fname = QString("%1/%2.par2").arg( fi.path() ).arg( fi.fileName() );

        fi.setFile( fname );
        emit updateFilename( fname );
    }

    if( !fi.exists() ) {

        emit error( QString("File not found '%1'.").arg( fi.filePath() ) );
        killProc();
        return;
    }

    if( op == Verify )
        opStr = "v";
    else if( op == Create )
        opStr = QString("c -r%1").arg( rPct );
    else
        opStr = "r";

    fname.prepend( "\"" );
    fname.append( "\"" );

/* ----------------------- */
/* Deploy par2 application */
/* ----------------------- */

    QString par2Path;
    deployApp( par2Path );

/* ----------------------- */
/* Format commandline args */
/* ----------------------- */

    QString cmd = QString("\"%1\" %2 %3")
                    .arg( par2Path ).arg( opStr ).arg( fname );

    Debug() << "Executing: " << cmd;

/* ----------------- */
/* Create subprocess */
/* ----------------- */

    pid     = 0;
    process = new QProcess( this );
    process->setProcessChannelMode( QProcess::MergedChannels );
    Connect( process, SIGNAL(started()),
        this, SLOT(procStarted()) );
    Connect( process, SIGNAL(finished(int,QProcess::ExitStatus)),
        this, SLOT(procFinished(int,QProcess::ExitStatus)) );
    Connect( process, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readyOutput()) );
    Connect( process, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(procError(QProcess::ProcessError)) );
    process->start( cmd );
}


void Par2Worker::processID()
{
    pid = 0;

    procMtx.lock();

    if( process ) {
#if QT_VERSION >= 0x050300
        pid = process->processId();
#elif defined Q_OS_WIN
        // pid() gets pointer to PROCESS_INFORMATION
        if( process->pid() )
            pid = process->pid()->dwProcessId;
#else
        // pid() an integer
        pid = process->pid();
#endif
    }

    procMtx.unlock();
}


void Par2Worker::killProc()
{
    if( !firstKill() )
        return;

    if( isProc() ) {

        if( process->state() != QProcess::NotRunning ) {

            QMetaObject::invokeMethod(
                process, "terminate",
                Qt::AutoConnection );

            QMetaObject::invokeMethod(
                process, "kill",
                Qt::AutoConnection );

            process->waitForFinished( 2000 );
            process->close();
        }

        delete process;
        process = 0;
    }

    emit finished();
}

/* ---------------------------------------------------------------- */
/* Par2Window ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

Par2Window::Par2Window( QWidget *parent )
    :   QWidget(parent), worker(0), op(Par2Worker::Verify)
{
    p2wUI = new Ui::Par2Window;
    p2wUI->setupUi( this );
    p2wUI->verifyRB->setChecked( true );
    p2wUI->cancelBut->setEnabled( false );
    ConnectUI( p2wUI->browseBut, SIGNAL(clicked()), this, SLOT(browseBut()) );
    ConnectUI( p2wUI->createRB, SIGNAL(clicked()), this, SLOT(radioChecked()) );
    ConnectUI( p2wUI->verifyRB, SIGNAL(clicked()), this, SLOT(radioChecked()) );
    ConnectUI( p2wUI->repairRB, SIGNAL(clicked()), this, SLOT(radioChecked()) );
    ConnectUI( p2wUI->goBut, SIGNAL(clicked()), this, SLOT(goBut()) );
}


Par2Window::~Par2Window()
{
    if( worker ) {
        worker->disconnect();
        delete worker;
        worker = 0;
    }

    if( p2wUI ) {
        delete p2wUI;
        p2wUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Par2Window::updateFilename( const QString &name )
{
    p2wUI->fileLE->setText( name );
}


void Par2Window::report( const QString &s )
{
    QTextEdit   *te = p2wUI->outputTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


// Note that disconnect is needed to break infinite loop:
// this::finished->worker::delete->worker::killProc->this::finished...
//
void Par2Window::finished()
{
    if( worker ) {
        worker->disconnect();
        delete worker;
        worker = 0;
    }

    p2wUI->specifyGB->setEnabled( true );
    p2wUI->cancelBut->setEnabled( false );
}


static QString getLastFileName( int op )
{
    STDSETTINGS( settings, "par2window" );
    settings.beginGroup( "Par2Window" );

    return settings.value(
            QString("lastOpenFile_%1").arg( op ),
            mainApp()->dataDir() ).toString();
}


static void saveLastFileName( int op, const QString &name )
{
    if( name.isEmpty() )
        return;

    STDSETTINGS( settings, "par2window" );
    settings.beginGroup( "Par2Window" );

    QString key = QString("lastOpenFile_%1").arg( op );

    settings.setValue( key, name );
}


void Par2Window::browseBut()
{
// Where to look

    QString f = getLastFileName( op );

// Select file

    if( op == Par2Worker::Create ) {
        f = QFileDialog::getOpenFileName(
            this,
            "Select a data file for PAR2 operation",
            f );
    }
    else {
        f = QFileDialog::getOpenFileName(
            this,
            "Select a PAR2 file for verify",
            f,
            "PAR2 Files (*.par2)" );
    }

    if( f.isEmpty() )
        return;

// Map meta -> bin

    if( op == Par2Worker::Create && f.endsWith( ".meta" ) ) {

        f = DFName::forceBinSuffix( f );

        if( !QFile( f ).exists() ) {

            QMessageBox::critical(
                this,
                "Input File Error",
                "Selected .meta file doesn't have a binary counterpart." );
            return;
        }
    }

// Accept, save

    p2wUI->fileLE->setText( f );

    saveLastFileName( op, f );
}


void Par2Window::radioChecked()
{
    QRadioButton    *rb = dynamic_cast<QRadioButton*>(sender());
    bool            rEn = false;

    if( !rb ) {
        Warning()
            << "Par2Window::radioChecked()"
            " sender should be a radio button.";
        return;
    }

    if( rb == p2wUI->createRB ) {
        op  = Par2Worker::Create;
        rEn = true;
    }
    else if( rb == p2wUI->verifyRB )
        op = Par2Worker::Verify;
    else if( rb == p2wUI->repairRB )
        op = Par2Worker::Repair;

    p2wUI->redundancyLbl->setEnabled( rEn );
    p2wUI->redundancySB->setEnabled( rEn );
}


void Par2Window::goBut()
{
    worker = new Par2Worker(
                    p2wUI->fileLE->text().trimmed(),
                    op, p2wUI->redundancySB->value() );

    Connect( worker, SIGNAL(updateFilename(QString)), this, SLOT(updateFilename(QString)) );
    Connect( worker, SIGNAL(report(QString)), this, SLOT(report(QString)) );
    Connect( worker, SIGNAL(error(QString)), this, SLOT(report(QString)) );
    Connect( worker, SIGNAL(finished()), this, SLOT(finished()) );

    Connect( p2wUI->cancelBut, SIGNAL(clicked()), worker, SLOT(cancel()) );

    p2wUI->specifyGB->setEnabled( false );
    p2wUI->cancelBut->setEnabled( true );
    p2wUI->outputTE->clear();
    p2wUI->outputTE->append( "----------------" );

    worker->run();
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void Par2Window::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QWidget::keyPressEvent( e );
}


void Par2Window::closeEvent( QCloseEvent *e )
{
    if( worker ) {

        int yesNo = QMessageBox::question(
            this,
            "Stop current task?",
            "Par2 app is running -- close anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo == QMessageBox::Yes ) {

            QMetaObject::invokeMethod(
                worker, "cancel",
                Qt::AutoConnection );

            e->accept();
        }
        else
            e->ignore();
    }
    else
        e->accept();

    if( e->isAccepted() )
        emit closed( this );
}


