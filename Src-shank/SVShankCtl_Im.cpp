
#include "ui_ShankWindow.h"

#include "SVShankCtl_Im.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"

#include <QSettings>
#include <QTextEdit>

#include <math.h>


/* ---------------------------------------------------------------- */
/* SVAnatomy ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void SVAnatomy::parse( const QString &elems, const DAQ::Params &p, int ip, int sk )
{
// Remove entries for this shank

    for( int i = int(rgn.size()) - 1; i >= 0; --i ) {
        if( rgn[i].shank == sk )
            rgn.erase( rgn.begin() + i );
    }

// Parse entries

    QStringList sle = elems.split(
                        QRegExp("^\\s*\\(\\s*|\\s*\\)\\s*\\(\\s*|\\s*\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         ne  = sle.size();

    if( !ne )
        return;

    IMROTbl *roTbl = p.im.prbj[ip].roTbl;

    for( int ie = 0; ie < ne; ++ie ) {

        QStringList slp = sle[ie].split(
                            QRegExp("^\\s*|\\s*,\\s*|\\s*$"),
                            QString::SkipEmptyParts );
        int         np  = slp.size();

        if( np != 6 ) {
            parse( "", p, ip, sk );
            return;
        }

        SVCtlAnaRgn R( sk );
        R.lbl   = slp[5];
        R.row0  =
            qBound(
            0,
            int(floor((slp[0].toDouble() - roTbl->tipLength())/roTbl->zPitch())),
            roTbl->nRow()-1 );
        R.rowN  =
            qBound(
            0,
            int(ceil((slp[1].toDouble() - roTbl->tipLength())/roTbl->zPitch())),
            roTbl->nRow()-1 );
        R.r     = slp[2].toInt();
        R.g     = slp[3].toInt();
        R.b     = slp[4].toInt();
        rgn.push_back( R );
    }
}


void SVAnatomy::fillLegend( QTextEdit *leg )
{
    leg->clear();

// Unique alphabetic names

    QMap<QString,QColor>    mlbl;
    for( int i = 0, n = rgn.size(); i < n; ++i ) {
        const SVCtlAnaRgn   &R = rgn[i];
        mlbl[R.lbl] = QColor( R.r, R.g, R.b );
    }

// Set text

    QMap<QString,QColor>::const_iterator it = mlbl.begin(), end = mlbl.end();
    for( ; it != end; ++it ) {
        leg->setTextColor( it.value() );
        leg->append( it.key() );
    }
}


void SVAnatomy::colorShanks( ShankView *view, bool on )
{
    std::vector<SVAnaRgn>   vA;

    if( on ) {
        foreach( const SVCtlAnaRgn &R, rgn )
            vA.push_back( SVAnaRgn( R.row0, R.rowN, R.shank, R.r, R.g, R.b ) );
    }

    view->setAnatomy( vA );
    view->updateNow();
}

/* ---------------------------------------------------------------- */
/* SVShankCtl_Im -------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVShankCtl_Im::SVShankCtl_Im(
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel,
    QWidget             *parent )
    :   SVShankCtl(p, jpanel, parent), ip(ip)
{
}


void SVShankCtl_Im::init()
{
    parInit( jsIM, ip );

    if( seTab )
        ConnectUI( seTab, SIGNAL(imroChanged(QString)), this, SLOT(imroChanged(QString)) );

    ConnectUI( svTab, SIGNAL(colorShanks(bool)), this, SLOT(colorShanks(bool)) );

    scUI->statusLbl->setToolTip(
        "Use shift-key or right-clicks to see/select LF chans" );

    setWindowTitle( QString("Imec%1 Shank Activity").arg( ip ) );
}


void SVShankCtl_Im::mapChanged()
{
    view()->setShankMap( &p.im.prbj[ip].sns.shankMap );
}


void SVShankCtl_Im::setAnatomyPP( const QString &elems, int sk )
{
    A.parse( elems, p, ip, sk );
    A.fillLegend( svTab->getLegend() );
    colorShanks( svTab->isShanksChecked() );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankCtl_Im::colorShanks( bool on )
{
    A.colorShanks( view(), on );
}


void SVShankCtl_Im::cursorOver( int ic, bool shift )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( ic < 0 ) {
        setStatus( QString() );
        return;
    }

    int r = view()->getSmap()->e[ic].r;

    if( shift && E.roTbl->nLF() )
        ic += E.imCumTypCnt[CimCfg::imSumAP];

    setStatus(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( E.sns.chanMap.name( ic, p.trig_isChan( jsIM, ip, ic ) ) ) );
}


void SVShankCtl_Im::lbutClicked( int ic, bool shift )
{
    if( shift ) {

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        if( !E.roTbl->nLF() )
            shift = false;
    }

    cursorOver( ic, shift );
    emit selChanged( ic, shift );
}


void SVShankCtl_Im::imroChanged( QString newName )
{
    Run *run = mainApp()->getRun();

    if( !run->dfIsSaving() ) {

        const CimCfg::ImProbeTable  &T = mainApp()->cfgCtl()->prbTab;
        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );

        if( !T.simprb.isSimProbe( P.slot, P.port, P.dock ) ) {

            run->grfHardPause( true );
            run->grfWaitPaused();
            mainApp()->cfgCtl()->graphSetsImroFile( newName, ip );
            run->grfHardPause( false );
            run->imecUpdate( ip );
        }
        else
            mainApp()->cfgCtl()->graphSetsImroFile( newName, ip );

        run->grfUpdateProbe( ip, true, true );
    }
    else
        seTab->beep( "Recording in progress!" );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString SVShankCtl_Im::settingsName() const
{
    return QString("ShankView_Imec_Panel%1").arg( jpanel );
}


// Called only from init().
//
void SVShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->loadSettings( settings );
    settings.endGroup();
}


void SVShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


