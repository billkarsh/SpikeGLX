#ifndef IMROEDITOR_H
#define IMROEDITOR_H

#include <QObject>

namespace Ui {
class IMROEditor;
}

struct IMROTbl;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor : public QObject
{
    Q_OBJECT

private:
    QDialog         *edDlg;
    Ui::IMROEditor  *edUI;
    IMROTbl         *R0,
                    *R;
    QString         inFile,
                    R0File,
                    lastDir;
    quint32         type;
    bool            running;

public:
    IMROEditor( QObject *parent, int type );
    virtual ~IMROEditor();

    bool Edit( QString &outFile, const QString &file, int selectRow );

private slots:
    void defaultBut();
    void bankBut();
    void refidBut();
    void apBut();
    void lfBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
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
    void setAllBank( int val );
    void setAllRefid( int val );
    void setAllAPgain( int val );
    void setAllLFgain( int val );
    void adjustType();
    void loadFile( const QString &file );
};

#endif  // IMROEDITOR_H


