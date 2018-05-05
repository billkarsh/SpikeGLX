#ifndef COLORTTLCTL_H
#define COLORTTLCTL_H

#include "SGLTypes.h"
#include "Sync.h"

#include <QObject>
#include <QMutex>
#include <QString>
#include <QVector>

namespace Ui {
class ColorTTLDialog;
}

class HelpButDialog;
class MGraphX;

class QLabel;
class QGroupBox;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ColorTTLCtl : public QObject
{
    Q_OBJECT

private:
    struct TTLClrEach {
        double      T;
        QString     stream;
        int         chan,
                    bit;
        bool        isAnalog,
                    isOn;

        bool validIm(
            QString             &err,
            const QString       &clr,
            const DAQ::Params   &p );
        bool validNi(
            QString             &err,
            const QString       &clr,
            const DAQ::Params   &p );
        bool valid(
            QString             &err,
            const QString       &clr,
            const DAQ::Params   &p );
    };

    struct TTLClrSet {
        TTLClrEach  clr[4];
        double      minSecs;
        int         inarow;
    };

    struct ItemGrp {
        QGroupBox       *GB;
        QComboBox       *stream;
        QComboBox       *analog;
        QSpinBox        *chan;
        QLabel          *TLbl;
        QDoubleSpinBox  *T;

        void analogChanged( TTLClrEach &C, bool algCBChanged );
    };

    struct Stream : public SyncStream {
        MGraphX *X;

        Stream() : SyncStream(), X(0)   {}
        void init( MGraphX *X, const AIQ *Q, int ip, const DAQ::Params &p )
            {this->X = X; SyncStream::init( Q, ip, p );}
    };

private:
    const DAQ::Params   &p;
    HelpButDialog       *dlg;
    Ui::ColorTTLDialog  *cttlUI;
    Stream              A,
                        B;
    ItemGrp             grp[4];
    TTLClrSet           set,
                        uiSet;
    mutable QMutex      setMtx;
    int                 state[4];   // {0,1}=seek{high,low}

public:
    ColorTTLCtl( QObject *parent, const DAQ::Params &p );
    virtual ~ColorTTLCtl();

    void setClients(
        int         ipa,
        const AIQ   *Qa,
        MGraphX     *Xa,
        int         ipb,
        const AIQ   *Qb,
        MGraphX     *Xb );

    bool valid( QString &err, bool checkStored = true );

    void showDialog();

    void scanBlock(
        const MGraphX   *X,
        const vec_i16   &data,
        quint64         headCt,
        int             nC,
        int             ip );

private slots:
    void C0GBClicked();
    void C1GBClicked();
    void C2GBClicked();
    void C3GBClicked();
    void ana0CBChanged();
    void ana1CBChanged();
    void ana2CBChanged();
    void ana3CBChanged();
    void okBut();

private:
    void loadSettings();
    void saveSettings() const;

    void resetState();

    int eventsScanningThisStream(
        const MGraphX   *X,
        QVector<int>    &clr,
        int             ip ) const;

    bool getChan(
        int     &chan,
        int     &bit,
        int     &thresh,
        int     clr,
        int     ip ) const;

    bool findRisingEdge(
        int         &outCt,
        const short *data,
        int         ntpts,
        int         offset,
        int         nchans,
        int         chan,
        qint16      T ) const;

    bool findBitRisingEdge(
        int         &outCt,
        const short *data,
        int         ntpts,
        int         offset,
        int         nchans,
        int         chan,
        int         bit ) const;

    bool findFallingEdge(
        int         &outCt,
        const short *data,
        int         ntpts,
        int         offset,
        int         nchans,
        int         chan,
        qint16      T ) const;

    bool findBitFallingEdge(
        int         &outCt,
        const short *data,
        int         ntpts,
        int         offset,
        int         nchans,
        int         chan,
        int         bit ) const;

    void processEvents(
        const vec_i16   &data,
        quint64         headCt,
        int             nC,
        QVector<int>    &vClr,
        int             ip );
};

#endif  // COLORTTLCTL_H


