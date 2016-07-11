
#include "ui_ExportDialog.h"

#include "ExportCtl.h"
#include "Util.h"
#include "ConfigCtl.h"
#include "FileViewerWindow.h"
#include "DataFileIMAP.h"
#include "DataFileIMLF.h"
#include "DataFileNI.h"
#include "Subset.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* struct ExportParams -------------------------------------------- */
/* ---------------------------------------------------------------- */

ExportCtl::ExportParams::ExportParams()
    :   inScnsMax(0), inScnSelFrom(-1), inScnSelTo(-1),
        inNG(0), scnFrom(-1), scnTo(-1),
        fmtR(bin), grfR(sel), scnR(all)
{
}


void ExportCtl::ExportParams::loadSettings( QSettings &S )
{
    S.beginGroup( "ExportParams" );

    fmtR = (Radio)S.value( "lastExportFormat", bin ).toInt();

    if( fmtR < bin || fmtR > csv )
        fmtR = bin;

    grfR = (Radio)S.value( "lastExportChans", sel ).toInt();

    if( grfR < all || grfR > custom )
        grfR = sel;

    S.endGroup();
}


void ExportCtl::ExportParams::saveSettings( QSettings &S ) const
{
    S.beginGroup( "ExportParams" );

    S.setValue( "lastExportFormat", fmtR );
    S.setValue( "lastExportChans", grfR );

    S.endGroup();
}

/* ---------------------------------------------------------------- */
/* ExportCtl ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ExportCtl::ExportCtl( QWidget *parent )
    :   QObject(parent)
{
    dlg = new QDialog( parent );

    dlg->setWindowFlags( dlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    expUI = new Ui::ExportDialog;
    expUI->setupUi( dlg );
    ConnectUI( expUI->browseBut, SIGNAL(clicked()), this, SLOT(browseButClicked()) );
    ConnectUI( expUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );

// -------------
// button groups
// -------------

    QButtonGroup    *bg;

    bg = new QButtonGroup( this );
    bg->addButton( expUI->binRadio );
    bg->addButton( expUI->csvRadio );

    bg = new QButtonGroup( this );
    bg->addButton( expUI->grfAllRadio );
    bg->addButton( expUI->grfShownRadio );
    bg->addButton( expUI->grfCustomRadio );

    bg = new QButtonGroup( this );
    bg->addButton( expUI->scnAllRadio );
    bg->addButton( expUI->scnSelRadio );
    bg->addButton( expUI->scnCustomRadio );

// --------------
// format changed
// --------------

    ConnectUI( expUI->binRadio, SIGNAL(clicked()), this, SLOT(formatChanged()) );
    ConnectUI( expUI->csvRadio, SIGNAL(clicked()), this, SLOT(formatChanged()) );

// --------------
// graphs changed
// --------------

    ConnectUI( expUI->grfAllRadio, SIGNAL(clicked()), this, SLOT(graphsChanged()) );
    ConnectUI( expUI->grfShownRadio, SIGNAL(clicked()), this, SLOT(graphsChanged()) );
    ConnectUI( expUI->grfCustomRadio, SIGNAL(clicked()), this, SLOT(graphsChanged()) );
    ConnectUI( expUI->grfCustomLE, SIGNAL(textChanged(QString)), this, SLOT(graphsChanged()) );

// -------------
// scans changed
// -------------

    ConnectUI( expUI->scnAllRadio, SIGNAL(clicked()), this, SLOT(scansChanged()) );
    ConnectUI( expUI->scnSelRadio, SIGNAL(clicked()), this, SLOT(scansChanged()) );
    ConnectUI( expUI->scnCustomRadio, SIGNAL(clicked()), this, SLOT(scansChanged()) );
    ConnectUI( expUI->scnFromSB, SIGNAL(valueChanged(int)), this, SLOT(scansChanged()) );
    ConnectUI( expUI->scnToSB, SIGNAL(valueChanged(int)), this, SLOT(scansChanged()) );
}


ExportCtl::~ExportCtl()
{
    if( expUI ) {
        delete expUI;
        expUI = 0;
    }
}


void ExportCtl::initDataFile( const DataFile &df )
{
    this->df = &df;

    QFileInfo   fi( df.binFileName() );

    E.filename  = QString("%1/%2.exported.%3.%4")
                    .arg( fi.absoluteDir().canonicalPath() )
                    .arg( fi.baseName() )
                    .arg( df.subtypeFromObj() )
                    .arg( E.fmtR == ExportParams::bin ? "bin" : "csv" );

    E.inNG      = df.numChans();
    E.inScnsMax = df.scanCount();
}


void ExportCtl::initGrfRange( const QBitArray &visBits, int curSel )
{
    E.inGrfVisBits = visBits;

// Custom bits initial setting...
// If there already are channels from last session, keep them.
// Otherwise, set currently selected graph.

    if( !E.grfBits.count( true ) ) {

        E.grfBits.fill( false, E.inNG );

        if( curSel >= 0 )
            E.grfBits.setBit( curSel );
    }
}


void ExportCtl::initScnRange( qint64 selFrom, qint64 selTo )
{
    E.inScnSelFrom  = selFrom;
    E.inScnSelTo    = selTo;
}


bool ExportCtl::showExportDlg( FileViewerWindow *fvw )
{
    this->fvw = fvw;

    ExportParams    Esave = E;
    bool            ok;

    dialogFromParams();
    ok = (QDialog::Accepted == dlg->exec());

    if( !ok )
        E = Esave;

    return ok;
}


void ExportCtl::browseButClicked()
{
    QString     f = expUI->filenameLE->text();
    QStringList types;

    types.push_back( "Binary File (*.bin)" );
    types.push_back( "CSV Text (*.csv *.txt)" );

    f = QFileDialog::getSaveFileName(
            dlg,
            "Specify an export file",
            f,
            types.join( ";;" ),
            &types[E.fmtR] );

// sync file-type and format-radio

    QString suff = QFileInfo( f ).suffix().toLower();

    if( suff == "bin" )
        expUI->binRadio->setChecked( true );
    else if( suff == "csv" || suff == "txt" )
        expUI->csvRadio->setChecked( true );
    else if( E.fmtR == ExportParams::bin )
        f += ".bin";
    else
        f += ".csv";

    expUI->filenameLE->setText( f );

    formatChanged();
}


void ExportCtl::formatChanged()
{
    if( expUI->csvRadio->isChecked() )
        E.fmtR = ExportParams::csv;
    else
        E.fmtR = ExportParams::bin;

    QFileInfo   fi( expUI->filenameLE->text() );

    if( !fi.fileName().isEmpty() ) {

        expUI->filenameLE->setText(
            QString("%1/%2.exported.%3.%4")
            .arg( fi.absoluteDir().canonicalPath() )
            .arg( fi.baseName() )
            .arg( df->subtypeFromObj() )
            .arg( E.fmtR == ExportParams::bin ? "bin" : "csv" ) );
    }

    estimateFileSize();
}


void ExportCtl::graphsChanged()
{
    if( expUI->grfCustomRadio->isChecked() )
        E.grfR = ExportParams::custom;
    else if( expUI->grfShownRadio->isChecked() )
        E.grfR = ExportParams::sel;
    else
        E.grfR = ExportParams::all;

    expUI->grfCustomLE->setEnabled( E.grfR == ExportParams::custom );

    estimateFileSize();
}


void ExportCtl::scansChanged()
{
    if( expUI->scnCustomRadio->isChecked() )
        E.scnR = ExportParams::custom;
    else if( expUI->scnSelRadio->isChecked() )
        E.scnR = ExportParams::sel;
    else
        E.scnR = ExportParams::all;

    expUI->scnFromSB->setEnabled( E.scnR == ExportParams::custom );
    expUI->scnToSB->setEnabled( E.scnR == ExportParams::custom );

    estimateFileSize();
}


void ExportCtl::okBut()
{
    if( validateSettings() ) {

        STDSETTINGS( settings, "cc_fileviewer" );
        E.saveSettings( settings );

        doExport();
        dlg->accept();
    }
}


void ExportCtl::dialogFromParams()
{
// ----
// file
// ----

    expUI->filenameLE->setText( E.filename );

// ------
// format
// ------

    if( E.fmtR == ExportParams::csv )
        expUI->csvRadio->setChecked( true );
    else
        expUI->binRadio->setChecked( true );

// ------
// graphs
// ------

    if( E.grfR == ExportParams::custom )
        expUI->grfCustomRadio->setChecked( true );
    else if( E.grfR == ExportParams::sel )
        expUI->grfShownRadio->setChecked( true );
    else
        expUI->grfAllRadio->setChecked( true );

    expUI->grfShownLbl->setText( Subset::bits2RngStr( E.inGrfVisBits ) );
    expUI->grfCustomLE->setText( Subset::bits2RngStr( E.grfBits ) );
    expUI->grfCustomLE->setEnabled( E.grfR == ExportParams::custom );

// -----
// Scans
// -----

// Radio precedence:
// If a custom selection from last session was valid, choose that.
// Else if a valid selection from fvw exists use that.
// Else set all.

    bool    inSelValid = E.inScnSelFrom >= 0
                        && E.inScnSelTo > E.inScnSelFrom
                        && E.inScnSelTo < E.inScnsMax;

    if( E.scnFrom  >= 0
        && E.scnTo > E.scnFrom
        && E.scnTo < E.inScnsMax ) {

        E.scnR = ExportParams::custom;
        expUI->scnCustomRadio->setChecked( true );
    }
    else if( inSelValid ) {
        E.scnR = ExportParams::sel;
        expUI->scnSelRadio->setChecked( true );
    }
    else {
        E.scnR = ExportParams::all;
        expUI->scnAllRadio->setChecked( true );
    }

// scnAll Text

    expUI->scnAllLbl->setText(
                QString("0 - %1")
                .arg( E.inScnsMax - 1 ) );

// scnSel Text

    if( inSelValid ) {
        expUI->scnSelRadio->setEnabled( true );
        expUI->scnSelLbl->setText(
                    QString("%1 - %2")
                    .arg( E.inScnSelFrom )
                    .arg( E.inScnSelTo ) );
    }
    else {
        expUI->scnSelRadio->setEnabled( false );
        expUI->scnSelLbl->setText( "No Selection" );
    }

// scnCustom spinners

    expUI->scnFromSB->setValue( E.scnFrom );
    expUI->scnFromSB->setMinimum( 0 );
    expUI->scnFromSB->setMaximum( E.inScnsMax - 1 );
    expUI->scnFromSB->setEnabled( E.scnR == ExportParams::custom );

    expUI->scnToSB->setValue( E.scnTo );
    expUI->scnToSB->setMinimum( 0 );
    expUI->scnToSB->setMaximum( E.inScnsMax - 1 );
    expUI->scnToSB->setEnabled( E.scnR == ExportParams::custom );

    estimateFileSize();
}


void ExportCtl::estimateFileSize()
{
    qint64  sampleBytes, nChans, nScans;

// ------
// format
// ------

    if( E.fmtR == ExportParams::bin )
        sampleBytes = sizeof(qint16);
    else
        sampleBytes = 8;    // estimated csv size

// ------
// graphs
// ------

    if( E.grfR == ExportParams::custom ) {

        if( Subset::isAllChansStr( expUI->grfCustomLE->text() ) )
            nChans = E.inNG;
        else {
            QBitArray   b;
            Subset::rngStr2Bits( b, expUI->grfCustomLE->text() );
            nChans = b.count( true );
        }
    }
    else if( E.grfR == ExportParams::sel )
        nChans = E.inGrfVisBits.count( true );
    else
        nChans = E.inNG;

// -----
// scans
// -----

    if( E.scnR == ExportParams::sel )
        nScans = E.inScnSelTo - E.inScnSelFrom + 1;
    else if( E.scnR == ExportParams::custom )
        nScans = expUI->scnToSB->value() - expUI->scnFromSB->value() + 1;
    else
        nScans = E.inScnsMax;

// ------
// report
// ------

    double  sizeMB = (sampleBytes * nChans * nScans) / (1024.0*1024.0);

    expUI->statusLbl->setText(
        QString("Est. file size:  %1 MB")
        .arg( sizeMB, 0, 'f', 2 ) );
}


bool ExportCtl::validateSettings()
{
    QString fname = expUI->filenameLE->text();

// --------
// filename
// --------

    if( QFileInfo( fname ).exists() ) {

        int yesNo = QMessageBox::question(
            dlg,
            "Output File Exists",
            QString(
            "Output file '%1' already exists, overwrite it?")
            .arg( QFileInfo( fname ).fileName() ),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes );

        if( yesNo != QMessageBox::Yes )
            return false;
    }

    E.filename  = fname;

// ------
// format
// ------

// ------
// graphs
// ------

    if( E.grfR == ExportParams::all )
        Subset::defaultBits( E.grfBits, E.inNG );
    else if( E.grfR == ExportParams::sel )
        E.grfBits = E.inGrfVisBits;
    else {

        if( !Subset::
            rngStr2Bits( E.grfBits, expUI->grfCustomLE->text() ) ) {

            QMessageBox::critical(
                dlg,
                "Export Channels Error",
                "Custom graphs list has bad format." );
            return false;
        }

        E.grfBits.resize( E.inNG );
    }

// -----
// scans
// -----

    if( E.scnR == ExportParams::all ) {
        E.scnFrom  = 0;
        E.scnTo    = E.inScnsMax - 1;
    }
    else if( E.scnR == ExportParams::sel ) {
        E.scnFrom  = E.inScnSelFrom;
        E.scnTo    = E.inScnSelTo;
    }
    else {
        E.scnFrom  = expUI->scnFromSB->value();
        E.scnTo    = expUI->scnToSB->value();
    }

// ---------
// data size
// ---------

    if( !E.grfBits.count( true )
        || E.scnFrom < 0
        || E.scnTo - E.scnFrom + 1 <= 0 ) {

        QMessageBox::critical(
            dlg,
            "Export File Is Empty",
            "You must specify at least one channel and one scan." );

        return false;
    }

// ---------------
// user intensions
// ---------------

    if( E.fmtR == ExportParams::csv ) {

        int yesNo = QMessageBox::question(
            dlg,
            "Confirm CSV Format",
            "You chose to export data in the CSV format.\n"
            "This format is portable, but lacks meta-data such as:\n"
            "{sampling rate, electrode id, gain, etc.}\n\n"
            "Use CSV anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes );

        if( yesNo != QMessageBox::Yes )
            return false;
    }

    return true;
}


void ExportCtl::doExport()
{
    qint64  nscans  = E.scnTo - E.scnFrom + 1,
            step    = qMin( 1000LL, nscans );

    QProgressDialog progress(
        QString("Exporting %1 scans...").arg( nscans ),
        "Abort", 0, 100, dlg );

    progress.setWindowFlags( progress.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    progress.setWindowModality( Qt::WindowModal );
    progress.setMinimumDuration( 0 );

    if( E.fmtR == ExportParams::bin ) {

        if( !exportAsBinary( progress, nscans, step ) )
            return;
    }
    else if( !exportAsText( progress, nscans, step ) )
        return;

    progress.setValue( 100 );

    QMessageBox::information(
        dlg,
        "Export Complete",
        "Export completed successfully." );
}


bool ExportCtl::exportAsBinary(
    QProgressDialog &progress,
    qint64          nscans,
    qint64          step )
{
    vec_i16         scan;
    DataFile        *out;
    QVector<uint>   idxOtherChans;
    int             prevPerCent = -1;
    bool            ok = false;

    if( df->subtypeFromObj() == "imec.ap" )
        out = new DataFileIMAP();
    else if( df->subtypeFromObj() == "imec.lf" )
        out = new DataFileIMLF();
    else
        out = new DataFileNI();

    Subset::bits2Vec( idxOtherChans, E.grfBits );

    if( !out->openForExport( *df, E.filename, idxOtherChans ) ) {

        Error() << "Could not open export file for write.";
        goto exit;
    }

    out->setAsyncWriting( false );
    out->setFirstSample( df->firstCt() + E.scnFrom );

    for( qint64 i = 0; i < nscans; i += step ) {

        qint64  nread;
        nread = df->readScans( scan, E.scnFrom + i, step, E.grfBits );

        if( nread <= 0 )
            break;

        out->writeAndInvalScans( scan );

        int progPerCent = int( 100*(i + nread)/nscans );

        if( progPerCent > prevPerCent )
            progress.setValue( prevPerCent = progPerCent );

        if( progress.wasCanceled() ) {

            QString f = out->binFileName(),
                    m = out->metaFileName();

            out->closeAndFinalize();
            QFile::remove( f );
            QFile::remove( m );
            goto exit;
        }
    }

    out->closeAndFinalize();
    ok = true;

exit:
    delete out;
    return ok;
}


bool ExportCtl::exportAsText(
    QProgressDialog &progress,
    qint64          nscans,
    qint64          step )
{
    QFile   out( E.filename );

    if( !out.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        Error() << "Could not open export file for write.";
        return false;
    }

    QTextStream         ts( &out );
    vec_i16             scan;
    std::vector<double> gain;

    double  minV = df->vRange().rmin,
            spnV = df->vRange().span(),
            minS = double(df->typeFromObj() == "nidq" ? SHRT_MIN : -512),
            spnU = double(-2 * minS),
            sclV = spnV / spnU;
    int     nOn  = E.grfBits.count( true ),
            prevPerCent = -1;

    fvw->getInverseGains( gain, E.grfBits );

    for( qint64 i = 0; i < nscans; i += step ) {

        qint64  nread;
        nread = df->readScans( scan, E.scnFrom + i, step, E.grfBits );

        if( nread <= 0 )
            break;

        qint16  *S = &scan[0];

        for( int is = 0; is < nread; ++is ) {

            ts << gain[0] * (minV + sclV * (*S++ - minS));

            for( int ic = 1; ic < nOn; ++ic )
                ts << "," << gain[ic] * (minV + sclV * (*S++ - minS));

            ts << "\n";
        }

        int progPerCent = int( 100*(i + nread)/nscans );

        if( progPerCent > prevPerCent )
            progress.setValue( prevPerCent = progPerCent );

        if( progress.wasCanceled() ) {
            out.close();
            out.remove();
            return false;
        }
    }

    return true;
}


