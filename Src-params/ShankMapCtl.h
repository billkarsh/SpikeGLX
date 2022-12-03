#ifndef SHANKMAPCTL_H
#define SHANKMAPCTL_H

#include "ShankMap.h"

#include <QObject>

namespace Ui {
class ShankMappingDlg;
}

class IMROTbl;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankMapCtl : public QObject
{
    Q_OBJECT

private:
    QDialog             *mapDlg;
    Ui::ShankMappingDlg *mapUI;
    const IMROTbl       *imro;
    ShankMap            *Mref,
                        *Mcur;
    QString             type,
                        iniFile,
                        refFile,
                        lastDir;
    const int           nChan;

public:
    ShankMapCtl(
        QObject         *parent,
        const IMROTbl   *imro,
        const QString   &type,
        const int       nChan );
    virtual ~ShankMapCtl();

    QString edit( const QString &file );

private slots:
    void hdrChanged();
    void defaultBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void emptyMcur();
    void copyMcur2ref();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void Mcur2table();
    bool table2Mcur();
    void Mcur2Header();
    void autoFillMcur( int ns, int nc, int nr );
    void loadFile( const QString &file );
};

#endif  // SHANKMAPCTL_H


