#ifndef IMROEDITOR_T1110_H
#define IMROEDITOR_T1110_H

#include <QObject>

namespace Ui {
class IMROEditor_T1110;
}

struct IMROTbl_T1110;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T1110 : public QObject
{
    Q_OBJECT

private:
    QDialog                 *edDlg;
    Ui::IMROEditor_T1110    *edUI;
    IMROTbl_T1110           *Rini,
                            *Rref,
                            *Rcur;
    QString                 iniFile,
                            refFile,
                            lastDir;
    int                     type;
    bool                    running;

public:
    IMROEditor_T1110( QObject *parent );
    virtual ~IMROEditor_T1110();

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

#endif  // IMROEDITOR_T1110_H


