
#include "ui_IMSaveChansDlg.h"

#include "SaveChansCtl.h"
#include "DAQ.h"
#include "Subset.h"
#include "Util.h"

#include <QClipboard>
#include <QDialog>
#include <QMessageBox>

#include <math.h>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SaveChansCtl::SaveChansCtl( QWidget *parent, const CimCfg::PrbEach &E, int ip )
    :   QObject(parent), parent(parent), E(E), ip(ip)
{
    svDlg = new QDialog;

    svDlg->setWindowFlags( svDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    svUI = new Ui::IMSaveChansDlg;
    svUI->setupUi( svDlg );

    nAP = E.roTbl->nAP();
    nLF = E.roTbl->nLF();
    nSY = E.roTbl->nSY();

    QString s;

    s = QString("Ranges: AP[0:%1]").arg( nAP - 1 );

    if( nLF )
        s += QString(", LF[%1:%2]").arg( nAP ).arg( nAP + nLF - 1 );

    if( nSY )
        s += QString(", SY[%1:%2]").arg( nAP + nLF ).arg( nAP + nLF + nSY - 1 );

    svUI->rngLbl->setText( s );
    svUI->saveChansLE->setText( E.sns.uiSaveChanStr );
    svUI->pairChk->setEnabled( nLF > 0 );

    ConnectUI( svUI->applyBut, SIGNAL(clicked()), this, SLOT(applyBut()) );
}


SaveChansCtl::~SaveChansCtl()
{
    if( svUI ) {
        delete svUI;
        svUI = 0;
    }

    if( svDlg ) {
        delete svDlg;
        svDlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if changed.
//
bool SaveChansCtl::edit( QString &uistr, bool &lfPairChk )
{
    if( !uistr.isEmpty() )
        svUI->saveChansLE->setText( uistr );

    svUI->pairChk->setChecked( lfPairChk );

// Run dialog until ok or cancel

    bool    changed = false;

    for(;;) {

        if( QDialog::Accepted == svDlg->exec() ) {

            SnsChansImec    sns;
            QString         err;
            int             _nAP = nAP;

            sns.uiSaveChanStr = svUI->saveChansLE->text().trimmed();

            if( !nLF || !svUI->pairChk->isChecked() )
                _nAP = 0;

            if( sns.deriveSaveData(
                    err, DAQ::Params::jsip2stream( jsIM, ip ),
                    nAP+nLF+nSY, _nAP, nSY ) ) {

                lfPairChk = svUI->pairChk->isChecked();

                changed = E.sns.saveBits != sns.saveBits;
                uistr   = sns.uiSaveChanStr;
                QGuiApplication::clipboard()->setText( uistr );
                break;
            }
            else
                QMessageBox::critical( parent, "Save Channels Error", err );
        }
        else
            break;
    }

    return changed;
}


void SaveChansCtl::applyBut()
{
// Set maxRow

    const IMROTbl   *R = E.roTbl;

    int maxRow; // inclusive
    switch( svUI->unitCB->currentIndex() ) {
        case 0:
            maxRow = int(svUI->lenSB->value());
            break;
        default:
            maxRow =
            ceil( (svUI->lenSB->value() - R->tipLength() - R->zPitch())
                / R->zPitch() );
    }
    maxRow = qBound( 0, maxRow, R->nRow() - 1 );

// Set bits <= maxRow

    QBitArray   B( nAP );

    for( int ic = 0; ic < nAP; ++ic ) {
        int col, row;
        R->elShankColRow( col, row, ic );
        if( row <= maxRow )
            B.setBit( ic );
    }

// Get existing bits

    QBitArray   b;

    if( !getCurBits( b ) )
        return;

// Intersect, or replace

    if( b.size() ) {

        b &= B;

        if( b.count( true ) )
            svUI->saveChansLE->setText( Subset::bits2RngStr( b ) );
        else {
            QMessageBox::critical( parent, "Empty Intersection",
            "Existing channels unchanged." );
        }
    }
    else
        svUI->saveChansLE->setText( Subset::bits2RngStr( B ) );
}


bool SaveChansCtl::getCurBits( QBitArray &b )
{
    SnsChansImec    sns;
    QString         err;

    sns.uiSaveChanStr = svUI->saveChansLE->text().trimmed();

    if( sns.deriveSaveData(
            err, DAQ::Params::jsip2stream( jsIM, ip ),
            nAP+nLF+nSY, 0, nSY ) ) {

        b = sns.saveBits;
        return true;
    }

    QMessageBox::critical( parent, "Save Channels Error", err );
    return false;
}


