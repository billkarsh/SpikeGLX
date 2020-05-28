#ifndef IMROEDITOR_T0_H
#define IMROEDITOR_T0_H

#include <QObject>

namespace Ui {
class IMROEditor_T0;
}

struct IMROTbl_T0base;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T0 : public QObject
{
    Q_OBJECT

private:
    QDialog             *edDlg;
    Ui::IMROEditor_T0   *edUI;
    IMROTbl_T0base      *R0,
                        *R;
    QString             inFile,
                        R0File,
                        lastDir;
    quint32             type;
    bool                running;

public:
    IMROEditor_T0( QObject *parent, int type );
    virtual ~IMROEditor_T0();

    bool Edit( QString &outFile, const QString &file, int selectRow );

private slots:
    void defaultBut();
    void blockBut();
    void bankBut();
    void refidBut();
    void apBut();
    void lfBut();
    void hipassBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void fillRefidCB();
    void createR();
    void copyR2R0();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void R2Table();
    bool table2R();
    int  bankMax( int ic );
    int  refidMax();
    bool gainOK( int val );
    void setAllBlock( int val );
    void setAllBank( int val );
    void setAllRefid( int val );
    void setAllAPgain( int val );
    void setAllLFgain( int val );
    void setAllAPfilt( int val );
    void loadFile( const QString &file );
};

#endif  // IMROEDITOR_T0_H


