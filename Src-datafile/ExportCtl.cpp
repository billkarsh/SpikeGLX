
#include "ui_ExportDialog.h"

#include "ExportCtl.h"
#include "Util.h"
#include "ConfigCtl.h"
#include "FileViewerWindow.h"
#include "DataFileIMAP.h"
#include "DataFileIMLF.h"
#include "DataFileNI.h"
#include "DataFileOB.h"
#include "DFName.h"
#include "Subset.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSettings>

#include <math.h>


/* ---------------------------------------------------------------- */
/* struct ExportParams -------------------------------------------- */
/* ---------------------------------------------------------------- */

ExportCtl::ExportParams::ExportParams()
    :   inSmpsMax(0), inSmpSelFrom(-1), inSmpSelTo(-1),
        inNG(0), smpFrom(-1), smpTo(-1),
        fmtR(bin), grfR(sel), smpR(all)
{
}


void ExportCtl::ExportParams::loadSettings( QSettings &S )
{
    S.beginGroup( "FileViewer_Export" );

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
    S.beginGroup( "FileViewer_Export" );

    S.setValue( "lastExportFormat", fmtR );
    S.setValue( "lastExportChans", grfR );

    S.endGroup();
}

/* ---------------------------------------------------------------- */
/* ExportCtl ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ExportCtl::ExportCtl( QWidget *parent ) : QObject(parent)
{
    dlg = new QDialog( parent );

    dlg->setWindowFlags( dlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    expUI = new Ui::ExportDialog;
    expUI->setupUi( dlg );
    ConnectUI( expUI->browseBut, SIGNAL(clicked()), this, SLOT(browseBut()) );
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
    bg->addButton( expUI->smpAllRadio );
    bg->addButton( expUI->smpSelRadio );
    bg->addButton( expUI->smpCustomRadio );

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
// samps changed
// -------------

    ConnectUI( expUI->smpAllRadio, SIGNAL(clicked()), this, SLOT(sampsChanged()) );
    ConnectUI( expUI->smpSelRadio, SIGNAL(clicked()), this, SLOT(sampsChanged()) );
    ConnectUI( expUI->smpCustomRadio, SIGNAL(clicked()), this, SLOT(sampsChanged()) );
    ConnectUI( expUI->smpFromSB, SIGNAL(valueChanged(double)), this, SLOT(sampsChanged()) );
    ConnectUI( expUI->smpToSB, SIGNAL(valueChanged(double)), this, SLOT(sampsChanged()) );
}


ExportCtl::~ExportCtl()
{
    if( expUI ) {
        delete expUI;
        expUI = 0;
    }
}


void ExportCtl::initDataFile( const DataFile *df )
{
    this->df = df;

    QFileInfo   fi( df->binFileName() );

    E.filename  = QString("%1/%2.exported.%3.%4")
                    .arg( fi.absoluteDir().canonicalPath() )
                    .arg( sglFilename( fi ) )
                    .arg( df->fileLblFromObj() )
                    .arg( E.fmtR == ExportParams::bin ? "bin" : "csv" );

    E.inNG      = df->numChans();
    E.inSmpsMax = df->sampCount();
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


void ExportCtl::initSmpRange( qint64 selFrom, qint64 selTo )
{
    E.inSmpSelFrom  = selFrom;
    E.inSmpSelTo    = selTo;
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


void ExportCtl::browseBut()
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

    QString suff = DFName::justExtension( f );

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
            .arg( sglFilename( fi ) )
            .arg( df->fileLblFromObj() )
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


void ExportCtl::sampsChanged()
{
    if( expUI->smpCustomRadio->isChecked() )
        E.smpR = ExportParams::custom;
    else if( expUI->smpSelRadio->isChecked() )
        E.smpR = ExportParams::sel;
    else
        E.smpR = ExportParams::all;

    expUI->smpFromSB->setEnabled( E.smpR == ExportParams::custom );
    expUI->smpToSB->setEnabled( E.smpR == ExportParams::custom );

    estimateFileSize();
}


void ExportCtl::okBut()
{
    if( validateSettings() ) {

        STDSETTINGS( settings, "fileviewer" );
        E.saveSettings( settings );

        doExport();
        dlg->accept();
    }
}


// String from last slash, up to and including _gNN_tSS tag.
//
QString ExportCtl::sglFilename( const QFileInfo &fi )
{
    QRegExp re("([^/\\\\]+_g\\d+_t(\\d+|cat))\\.");
    re.setCaseSensitivity( Qt::CaseInsensitive );

    fi.fileName().indexOf( re );

    return re.cap(1);
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

    // -------------------------------------------------
    // Convert indices (grfBits) to real channel numbers
    // -------------------------------------------------

    const QVector<uint> &src = df->channelIDs();

    // All
    expUI->grfAllLbl->setText( Subset::vec2RngStr( src ) );

    // Shown
    QVector<uint>   chans;
    for( int i = 0; i < E.inNG; ++i ) {

        if( E.inGrfVisBits.testBit( i ) )
            chans.push_back( src[i] );
    }
    expUI->grfShownLbl->setText( Subset::vec2RngStr( chans ) );

    // Custom
    chans.clear();
    for( int i = 0; i < E.inNG; ++i ) {

        if( E.grfBits.testBit( i ) )
            chans.push_back( src[i] );
    }
    expUI->grfCustomLE->setText( Subset::vec2RngStr( chans ) );

    expUI->grfCustomLE->setEnabled( E.grfR == ExportParams::custom );

// -----
// Samps
// -----

// Radio precedence:
// If a custom selection from last session was valid, choose that.
// Else if a valid selection from fvw exists use that.
// Else set all.

    bool    inSelValid = E.inSmpSelFrom >= 0
                        && E.inSmpSelTo >  E.inSmpSelFrom
                        && E.inSmpSelTo <= E.inSmpsMax;

    if( E.smpFrom  >= 0
        && E.smpTo >  E.smpFrom
        && E.smpTo <= E.inSmpsMax ) {

        E.smpR = ExportParams::custom;
        expUI->smpCustomRadio->setChecked( true );
    }
    else if( inSelValid ) {
        E.smpR = ExportParams::sel;
        expUI->smpSelRadio->setChecked( true );
    }
    else {
        E.smpR = ExportParams::all;
        expUI->smpAllRadio->setChecked( true );
    }

// smpAll Text

    double  D = df->samplingRateHz();

    expUI->smpAllLbl->setText(
                QString("0 - %1")
                .arg( E.inSmpsMax / D, 0, 'f', 4 ) );

// smpSel Text

    if( inSelValid ) {
        expUI->smpSelRadio->setEnabled( true );
        expUI->smpSelLbl->setText(
                    QString("%1 - %2")
                    .arg( E.inSmpSelFrom / D, 0, 'f', 4 )
                    .arg( E.inSmpSelTo / D, 0, 'f', 4 ) );
    }
    else {
        expUI->smpSelRadio->setEnabled( false );
        expUI->smpSelLbl->setText( "No Selection" );
    }

// smpCustom spinners

    expUI->smpFromSB->setValue( E.smpFrom / D );
    expUI->smpToSB->setValue( E.smpTo / D );

    expUI->smpFromSB->setMinimum( 0 );
    expUI->smpToSB->setMinimum( 0 );

    D = E.inSmpsMax / D;
    expUI->smpFromSB->setMaximum( D );
    expUI->smpToSB->setMaximum( D );

    expUI->smpFromSB->setEnabled( E.smpR == ExportParams::custom );
    expUI->smpToSB->setEnabled( E.smpR == ExportParams::custom );

    estimateFileSize();
}


// Create E.grfBits from user real channel string.
// Include only those intersecting DataFile channels.
//
// Return count.
//
int ExportCtl::customLE2Bits( QBitArray &bits, bool warn )
{
    if( Subset::isAllChansStr( expUI->grfCustomLE->text() ) ) {
        Subset::defaultBits( bits, E.inNG );
        return E.inNG;
    }

    bits.fill( false, E.inNG );

    QVector<uint>   chans;

    if( !Subset::rngStr2Vec( chans, expUI->grfCustomLE->text() ) ) {

        if( warn ) {
            QMessageBox::critical(
                dlg,
                "Export Channels Error",
                "Custom graphs list has bad format." );
        }
        return 0;
    }

    const QVector<uint> &src = df->channelIDs();

    for( int ic = 0, nC = chans.size(); ic < nC; ++ic ) {

        int idx = src.indexOf( chans[ic] );

        if( idx >= 0 )
            bits.setBit( idx );
    }

    int count = bits.count( true );

    if( !count ) {

        if( warn ) {
            QMessageBox::critical(
                dlg,
                "Export Channels Error",
                "Specified custom graphs not in file." );
        }
    }

    return count;
}


void ExportCtl::estimateFileSize()
{
    qint64  sampleBytes, nChans, nSamps;

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
        QBitArray   b;
        nChans = customLE2Bits( b, false );
    }
    else if( E.grfR == ExportParams::sel )
        nChans = E.inGrfVisBits.count( true );
    else
        nChans = E.inNG;

// -----
// samps
// -----

    if( E.smpR == ExportParams::sel )
        nSamps = E.inSmpSelTo - E.inSmpSelFrom;
    else if( E.smpR == ExportParams::custom ) {
        nSamps = (expUI->smpToSB->value() - expUI->smpFromSB->value())
                * df->samplingRateHz();
    }
    else
        nSamps = E.inSmpsMax;

// ------
// report
// ------

    double  sizeMB = (sampleBytes * nChans * nSamps) / (1024.0*1024.0);

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
            "Output file already exists, overwrite it? '%1'")
            .arg( QFileInfo( fname ).fileName() ),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Yes );

        if( yesNo != QMessageBox::Yes )
            return false;
    }

    E.filename = fname;

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
    else if( !customLE2Bits( E.grfBits, true ) )
        return false;

// -----
// samps
// -----

    if( E.smpR == ExportParams::all ) {
        E.smpFrom  = 0;
        E.smpTo    = E.inSmpsMax;
    }
    else if( E.smpR == ExportParams::sel ) {
        E.smpFrom  = E.inSmpSelFrom;
        E.smpTo    = E.inSmpSelTo;
    }
    else {
        E.smpFrom  = expUI->smpFromSB->value() * df->samplingRateHz();
        E.smpTo    = expUI->smpToSB->value() * df->samplingRateHz();
    }

// ---------
// data size
// ---------

    if( !E.grfBits.count( true )
        || E.smpFrom < 0
        || E.smpTo - E.smpFrom <= 0 ) {

        QMessageBox::critical(
            dlg,
            "Export File Is Empty",
            "Specify at least one channel and a non-zero time range." );

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
    qint64  nsamps  = E.smpTo - E.smpFrom,
            step    = qMin( 1000LL, nsamps );

    QProgressDialog progress(
        QString("Exporting %1 samps...").arg( nsamps ),
        "Abort", 0, 100, dlg );

    progress.setWindowFlags( progress.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    progress.setWindowModality( Qt::WindowModal );
    progress.setMinimumDuration( 0 );

    if( E.fmtR == ExportParams::bin ) {

        if( !exportAsBinary( progress, nsamps, step ) )
            return;
    }
    else if( !exportAsText( progress, nsamps, step ) )
        return;

    progress.setValue( 100 );

    QMessageBox::information(
        dlg,
        "Export Complete",
        "Export completed successfully." );
}


bool ExportCtl::exportAsBinary(
    QProgressDialog &progress,
    qint64          nsamps,
    qint64          step )
{
    vec_i16         data;
    DataFile        *out;
    QVector<uint>   idxOtherChans;
    int             prevPerCent = -1;
    bool            ok = false;

    if( df->subtypeFromObj() == "imec.ap" )
        out = new DataFileIMAP( df->streamip() );
    else if( df->subtypeFromObj() == "imec.lf" )
        out = new DataFileIMLF( df->streamip() );
    else if( df->subtypeFromObj() == "obx" )
        out = new DataFileOB( df->streamip() );
    else
        out = new DataFileNI;

    Subset::bits2Vec( idxOtherChans, E.grfBits );

    if( !out->openForExport( *df, E.filename, idxOtherChans ) ) {

        Error() << "Could not open export file for write.";
        goto exit;
    }

    out->setAsyncWriting( false );
    out->setFirstSample( df->firstCt() + E.smpFrom );

    for( qint64 i = 0; ; ) {

        qint64  nread;
        nread = df->readSamps( data, E.smpFrom + i, step, E.grfBits );

        if( nread <= 0 )
            break;

        i += nread;

        out->writeAndInvalSamps( data );

        int progPerCent = int( 100 * i / nsamps );

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

        qint64  rem = nsamps - i;

        if( rem <= 0 )
            break;

        if( rem < step )
            step = rem;
    }

    out->closeAndFinalize();
    ok = true;

exit:
    delete out;
    return ok;
}


bool ExportCtl::exportAsText(
    QProgressDialog &progress,
    qint64          nsamps,
    qint64          step )
{
    QFile   out( E.filename );

    if( !out.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        Error() << "Could not open export file for write.";
        return false;
    }

    QTextStream         ts( &out );
    vec_i16             data;
    std::vector<double> gain;

    double  minV = df->vRange().rmin,
            spnV = df->vRange().span(),
            minS,
            spnU,
            sclV;
    int     nOn  = E.grfBits.count( true ),
            prevPerCent = -1;

    switch( DAQ::Params::stream2js( df->streamFromObj() ) ) {
        // Handle 2.0+ app opens 1.0 file
        case 2:  minS = -qMax( df->getParam("imMaxInt").toInt(), 512 ); break;
        // obx and nidq
        default: minS = SHRT_MIN;
    }

    spnU = double(-2 * minS);
    sclV = spnV / spnU;

    fvw->getInverseGains( gain, E.grfBits );

    for( qint64 i = 0; ; ) {

        qint64  nread;
        nread = df->readSamps( data, E.smpFrom + i, step, E.grfBits );

        if( nread <= 0 )
            break;

        i += nread;

        qint16  *S = &data[0];

        for( int is = 0; is < nread; ++is ) {

            ts << gain[0] * (minV + sclV * (*S++ - minS));

            for( int ic = 1; ic < nOn; ++ic )
                ts << "," << gain[ic] * (minV + sclV * (*S++ - minS));

            ts << "\n";
        }

        int progPerCent = int( 100 * i / nsamps );

        if( progPerCent > prevPerCent )
            progress.setValue( prevPerCent = progPerCent );

        if( progress.wasCanceled() ) {
            out.close();
            out.remove();
            return false;
        }

        qint64  rem = nsamps - i;

        if( rem <= 0 )
            break;

        if( rem < step )
            step = rem;
    }

    return true;
}


