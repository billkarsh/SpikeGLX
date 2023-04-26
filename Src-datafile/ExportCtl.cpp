
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
        inNSavedChans(0), smpFrom(-1), smpTo(-1),
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
    dfSrc = df;

    QFileInfo   fi( df->binFileName() );

    E.filename  = QString("%1/%2.exported.%3.%4")
                    .arg( fi.absoluteDir().canonicalPath() )
                    .arg( sglFilename( fi ) )
                    .arg( df->fileLblFromObj() )
                    .arg( E.fmtR == ExportParams::bin ? "bin" : "csv" );

    E.inNSavedChans = df->numChans();
    E.inSmpsMax     = df->sampCount();
}


void ExportCtl::initGrfRange( const QBitArray &visBits, int curSel )
{
    E.inVisBits = visBits;

// Custom bits initial setting...
// If there already are channels from last session, keep them.
// Otherwise, set currently selected graph.

    if( !E.exportBits.count( true ) ) {

        E.exportBits.fill( false, E.inNSavedChans );

        if( curSel >= 0 )
            E.exportBits.setBit( curSel );
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
            .arg( dfSrc->fileLblFromObj() )
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

    // ----------------------------------------------------
    // Convert indices (exportBits) to real channel numbers
    // ----------------------------------------------------

    const QVector<uint> &srcChans = dfSrc->fileChans();

    // All
    expUI->grfAllLbl->setText( Subset::vec2RngStr( srcChans ) );

    // Shown
    QVector<uint>   newChans;
    for( int i = 0; i < E.inNSavedChans; ++i ) {

        if( E.inVisBits.testBit( i ) )
            newChans.push_back( srcChans[i] );
    }
    expUI->grfShownLbl->setText( Subset::vec2RngStr( newChans ) );

    // Custom
    newChans.clear();
    for( int i = 0; i < E.inNSavedChans; ++i ) {

        if( E.exportBits.testBit( i ) )
            newChans.push_back( srcChans[i] );
    }
    expUI->grfCustomLE->setText( Subset::vec2RngStr( newChans ) );

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

    double  fOffset = fvw->getFOffset(),
            D       = dfSrc->samplingRateHz();

    expUI->smpAllLbl->setText(
                QString("%1 - %2")
                .arg( fOffset )
                .arg( fOffset + E.inSmpsMax / D, 0, 'f', 4 ) );

// smpSel Text

    if( inSelValid ) {
        expUI->smpSelRadio->setEnabled( true );
        expUI->smpSelLbl->setText(
                    QString("%1 - %2")
                    .arg( fOffset + E.inSmpSelFrom / D, 0, 'f', 4 )
                    .arg( fOffset + E.inSmpSelTo / D, 0, 'f', 4 ) );
    }
    else {
        expUI->smpSelRadio->setEnabled( false );
        expUI->smpSelLbl->setText( "No Selection" );
    }

// smpCustom spinners

    expUI->smpFromSB->setMinimum( fOffset );
    expUI->smpToSB->setMinimum( fOffset );

    double smpMax = fOffset + E.inSmpsMax / D;
    expUI->smpFromSB->setMaximum( smpMax );
    expUI->smpToSB->setMaximum( smpMax );

    expUI->smpFromSB->setValue( fOffset + E.smpFrom / D );
    expUI->smpToSB->setValue( fOffset + E.smpTo / D );

    expUI->smpFromSB->setEnabled( E.smpR == ExportParams::custom );
    expUI->smpToSB->setEnabled( E.smpR == ExportParams::custom );

    estimateFileSize();
}


// Create bits array from user real channel string.
// Include only those intersecting DataFile channels.
//
// Return count.
//
int ExportCtl::customLE2Bits( QBitArray &bits, bool warn )
{
    if( Subset::isAllChansStr( expUI->grfCustomLE->text() ) ) {
        Subset::defaultBits( bits, E.inNSavedChans );
        return E.inNSavedChans;
    }

    bits.fill( false, E.inNSavedChans );

    QVector<uint>   newChans;

    if( !Subset::rngStr2Vec( newChans, expUI->grfCustomLE->text() ) ) {

        if( warn ) {
            QMessageBox::critical(
                dlg,
                "Export Channels Error",
                "Custom graphs list has bad format." );
        }
        return 0;
    }

    const QVector<uint> &srcChans = dfSrc->fileChans();

    for( int ic = 0, nC = newChans.size(); ic < nC; ++ic ) {

        int idx = srcChans.indexOf( newChans[ic] );

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
        nChans = E.inVisBits.count( true );
    else
        nChans = E.inNSavedChans;

// -----
// samps
// -----

    if( E.smpR == ExportParams::sel )
        nSamps = E.inSmpSelTo - E.inSmpSelFrom;
    else if( E.smpR == ExportParams::custom ) {
        nSamps = (expUI->smpToSB->value() - expUI->smpFromSB->value())
                * dfSrc->samplingRateHz();
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
        Subset::defaultBits( E.exportBits, E.inNSavedChans );
    else if( E.grfR == ExportParams::sel )
        E.exportBits = E.inVisBits;
    else if( !customLE2Bits( E.exportBits, true ) )
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
        double  fOffset = fvw->getFOffset();
        E.smpFrom  = (expUI->smpFromSB->value() - fOffset) * dfSrc->samplingRateHz();
        E.smpTo    = (expUI->smpToSB->value() - fOffset) * dfSrc->samplingRateHz();
        E.smpFrom  = qBound( 0LL, E.smpFrom, E.inSmpsMax );
        E.smpTo    = qBound( 0LL, E.smpTo, E.inSmpsMax );
    }

// ---------
// data size
// ---------

    if( !E.exportBits.count( true )
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
            "This format is portable, but lacks metadata such as:\n"
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
    QVector<uint>   indicesOfSrcChans;
    int             prevPerCent = -1;
    bool            ok = false;

    if( dfSrc->subtypeFromObj() == "imec.ap" )
        out = new DataFileIMAP( dfSrc->streamip() );
    else if( dfSrc->subtypeFromObj() == "imec.lf" )
        out = new DataFileIMLF( dfSrc->streamip() );
    else if( dfSrc->subtypeFromObj() == "obx" )
        out = new DataFileOB( dfSrc->streamip() );
    else
        out = new DataFileNI;

    Subset::bits2Vec( indicesOfSrcChans, E.exportBits );

    if( !out->openForExport( *dfSrc, E.filename, indicesOfSrcChans ) ) {

        Error() << "Could not open export file for write.";
        goto exit;
    }

    out->setAsyncWriting( false );
    out->setFirstSample( dfSrc->firstCt() + E.smpFrom );

    for( qint64 i = 0; ; ) {

        qint64  nread;
        nread = dfSrc->readSamps( data, E.smpFrom + i, step, E.exportBits );

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

    double  minV = dfSrc->vRange().rmin,
            spnV = dfSrc->vRange().span(),
            minS,
            spnU,
            sclV;
    int     nOn  = E.exportBits.count( true ),
            prevPerCent = -1;
    bool    hasSYChan   = false;

    switch( DAQ::Params::stream2js( dfSrc->streamFromObj() ) ) {
        case 2:     // Handle 2.0+ app opens 1.0 file
            minS = -qMax( dfSrc->imro()->maxInt(), 512 );
            hasSYChan = true;
            break;
        default:    // obx and nidq
            minS = SHRT_MIN;
    }

    spnU = double(-2 * minS);
    sclV = spnV / spnU;

    fvw->getInverseGains( gain, E.exportBits );

    if( hasSYChan ) // test if last gain is one
        hasSYChan = gain[gain.size()-1] > 0.5;

    for( qint64 i = 0; ; ) {

        qint64  nread;
        nread = dfSrc->readSamps( data, E.smpFrom + i, step, E.exportBits );

        if( nread <= 0 )
            break;

        i += nread;

        qint16  *S = &data[0];

        if( hasSYChan ) {
            int iSY = nOn - 1;
            for( int is = 0; is < nread; ++is ) {
                if( nOn > 1 ) {
                    ts << gain[0] * (minV + sclV * (*S++ - minS));
                    for( int ic = 1; ic < iSY; ++ic )
                        ts << "," << gain[ic] * (minV + sclV * (*S++ - minS));
                    ts << "," << double((*S++ >> 6) & 1);
                }
                else
                    ts << double((*S++ >> 6) & 1);
                ts << "\n";
            }
        }
        else {
            for( int is = 0; is < nread; ++is ) {
                ts << gain[0] * (minV + sclV * (*S++ - minS));
                for( int ic = 1; ic < nOn; ++ic )
                    ts << "," << gain[ic] * (minV + sclV * (*S++ - minS));
                ts << "\n";
            }
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


