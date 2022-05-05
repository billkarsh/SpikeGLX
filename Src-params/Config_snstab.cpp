
#include "ui_SNSTab.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Config_snstab.h"

#include <QMessageBox>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_snstab::Config_snstab( QWidget *tab )
    : snsTabUI(0)
{
    app = mainApp();

    snsTabUI = new Ui::SNSTab;
    snsTabUI->setupUi( tab );
    ConnectUI( snsTabUI->dataDirBut, SIGNAL(clicked()), this, SLOT(dataDirBut()) );
    ConnectUI( snsTabUI->diskBut, SIGNAL(clicked()), this, SLOT(diskBut()) );
}


Config_snstab::~Config_snstab()
{
    if( snsTabUI ) {
        delete snsTabUI;
        snsTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_snstab::toGUI( const DAQ::Params &p )
{
    snsTabUI->notesTE->setPlainText( p.sns.notes );

    setDataDirLbl();
    snsTabUI->runNameLE->setText( p.sns.runName );
    snsTabUI->fldChk->setChecked( p.sns.fldPerPrb );
    snsTabUI->fldChk->setEnabled( app->cfgCtl()->usingIM );

    snsTabUI->diskSB->setValue( p.sns.reqMins );

// --------------------
// Observe dependencies
// --------------------
}


void Config_snstab::fromGUI( DAQ::Params &q )
{
    q.sns.notes     = snsTabUI->notesTE->toPlainText().trimmed();
    q.sns.runName   = snsTabUI->runNameLE->text().trimmed();
    q.sns.fldPerPrb = snsTabUI->fldChk->isChecked();
    q.sns.reqMins   = snsTabUI->diskSB->value();
}


void Config_snstab::setDialogNoAccess()
{
    snsTabUI->diskTE->clear();
}


void Config_snstab::setRunName( const QString &name )
{
    snsTabUI->runNameLE->setText( name );
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_snstab::dataDirBut()
{
    app->options_PickDataDir();
    setDataDirLbl();
}


void Config_snstab::diskBut()
{
    ConfigCtl   *cfg = app->cfgCtl();

    snsTabUI->diskTE->clear();

    DAQ::Params q;
    QString     err;

    if( !cfg->validDataDir( err ) ) {
        diskWrite( err );
        return;
    }

    if( !cfg->diskParamsToQ( err, q ) ) {
        diskWrite( "Parameter error" );
        QMessageBox::critical( cfg->dialog(), "ACQ Parameter Error", err );
        return;
    }

    for( int idir = 0, ndir = app->nDataDirs(); idir < ndir; ++idir ) {

        double  BPS = 0;

        diskWrite( QString("Directory %1 ----------------").arg( idir ) );

        if( cfg->usingIM ) {

            for( int ip = 0, np = q.stream_nIM(); ip < np; ++ip ) {

                if( ip % ndir != idir )
                    continue;

                const CimCfg::PrbEach   &E = q.im.prbj[ip];

                int     ch  = E.apSaveChanCount();
                double  bps = ch * E.srate * 2;

                BPS += bps;

                QString s =
                    QString("AP %1: %2 chn @ %3 Hz = %4 MB/s")
                    .arg( ip )
                    .arg( ch )
                    .arg( int(E.srate) )
                    .arg( bps / (1024*1024), 0, 'f', 3 );

                diskWrite( s );

                if( E.lfIsSaving() ) {

                    ch  = E.lfSaveChanCount();
                    bps = ch * E.srate/12 * 2;

                    BPS += bps;

                    s =
                        QString("LF %1: %2 chn @ %3 Hz = %4 MB/s")
                        .arg( ip )
                        .arg( ch )
                        .arg( int(E.srate/12) )
                        .arg( bps / (1024*1024), 0, 'f', 3 );

                    diskWrite( s );
                }
            }
        }

        if( idir == 0 ) {

            if( cfg->usingOB ) {

                for( int ip = 0, np = q.stream_nOB(); ip < np; ++ip ) {

                    const CimCfg::ObxEach   &E = q.im.obxj[ip];

                    int     ch  = E.sns.saveBits.count( true );
                    double  bps = ch * E.srate * 2;

                    BPS += bps;

                    QString s =
                        QString("OB %1: %2 chn @ %3 Hz = %4 MB/s")
                        .arg( ip )
                        .arg( ch )
                        .arg( int(E.srate) )
                        .arg( bps / (1024*1024), 0, 'f', 3 );

                    diskWrite( s );
                }
            }

            if( cfg->usingNI ) {

                int     ch  = q.ni.sns.saveBits.count( true );
                double  bps = ch * q.ni.srate * 2;

                BPS += bps;

                QString s =
                    QString("NI: %1 chn @ %2 Hz = %3 MB/s")
                    .arg( ch )
                    .arg( int(q.ni.srate) )
                    .arg( bps / (1024*1024), 0, 'f', 3 );

                diskWrite( s );
            }
        }

        quint64 avail = availableDiskSpace( idir );
        QString s;

        if( BPS > 0 ) {
            s = QString("Avail: %1 GB / %2 MB/s = %3 min")
                .arg( avail / (1024UL*1024*1024) )
                .arg( BPS / (1024*1024), 0, 'f', 3 )
                .arg( avail / BPS / 60, 0, 'f', 1 );
        }
        else {
            s = QString("Avail: %1 GB")
                .arg( avail / (1024UL*1024*1024) );
        }

        diskWrite( s );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_snstab::setDataDirLbl() const
{
    snsTabUI->dataDirLbl->setText(
        QString("%1 dirs, main: %2")
        .arg( app->nDataDirs() ).arg( app->dataDir() ) );
}


void Config_snstab::diskWrite( const QString &s )
{
    QTextEdit   *te = snsTabUI->diskTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


