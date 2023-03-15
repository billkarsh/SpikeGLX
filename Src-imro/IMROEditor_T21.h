#ifndef IMROEDITOR_T21_H
#define IMROEDITOR_T21_H

#include <QObject>

namespace Ui {
class IMROEditor_T21;
}

struct IMROTbl_T21;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T21 : public QObject
{
    Q_OBJECT

private:
    QDialog             *edDlg;
    Ui::IMROEditor_T21  *edUI;
    IMROTbl_T21         *Rini,
                        *Rref,
                        *Rcur;
    QString             pn,
                        iniFile,
                        refFile,
                        lastDir;
    bool                running;

public:
    IMROEditor_T21( QObject *parent, const QString &pn );
    virtual ~IMROEditor_T21();

    bool edit( QString &outFile, const QString &file, int selectRow );

private slots:
    void defaultBut();
    void loadBut();
    void okBut();
    void cancelBut();

private:
    void createRcur();
    void copyRcur2ref();
    void loadSettings();
    void saveSettings() const;
    void loadFile( const QString &file );
};

#endif  // IMROEDITOR_T21_H


