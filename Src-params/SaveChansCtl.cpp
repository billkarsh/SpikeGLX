
#include "ui_IMSaveChansDlg.h"

#include "SaveChansCtl.h"
#include "DAQ.h"
#include "Subset.h"
#include "Util.h"

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

    ConnectUI( svUI->setBut, SIGNAL(clicked()), this, SLOT(setBut()) );
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
bool SaveChansCtl::edit( QString &out_uistr, bool &lfPairChk )
{
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

                if( changed )
                    out_uistr = sns.uiSaveChanStr;

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


void SaveChansCtl::setBut()
{
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

    QBitArray   B( nAP );

    for( int ic = 0; ic < nAP; ++ic ) {
        int col, row;
        R->elShankColRow( col, row, ic );
        if( row <= maxRow )
            B.setBit( ic );
    }

    svUI->saveChansLE->setText( Subset::bits2RngStr( B ) );
}


