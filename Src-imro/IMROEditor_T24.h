#ifndef IMROEDITOR_T24_H
#define IMROEDITOR_T24_H

#include <QObject>

namespace Ui {
class IMROEditor_T24;
}

struct IMROTbl_T24;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T24 : public QObject
{
    Q_OBJECT

private:
    QDialog             *edDlg;
    Ui::IMROEditor_T24  *edUI;
    IMROTbl_T24         *Rini,
                        *Rref,
                        *Rcur;
    QString             pn,
                        iniFile,
                        refFile,
                        lastDir;
    bool                running;

public:
    IMROEditor_T24( QObject *parent, const QString &pn );
    virtual ~IMROEditor_T24();

    bool Edit( QString &outFile, const QString &file, int selectRow );

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

#endif  // IMROEDITOR_T24_H


