#ifndef COLORTTLCTL_H
#define COLORTTLCTL_H

#include "SGLTypes.h"

#include <QObject>
#include <QString>
#include <QVector>

namespace DAQ {
struct Params;
}

class HelpButDialog;
class MGraphX;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TTLClr {
    double      T;
    QString     stream;
    int         chan,
                bit;
    bool        isAnalog,
                isOn;
};


struct ColorTTLSet {
    TTLClr      clr[4];
    double      minSecs;
    int         inarow;
};


class ColorTTLCtl : public QObject
{
    Q_OBJECT

private:
    const DAQ::Params   &p;
    HelpButDialog       *dlg;
    MGraphX             *Xim,
                        *Xni;
    ColorTTLSet         set,
                        uiSet;
    int                 state[4];   // {0,1}=seek{high,low}

public:
    ColorTTLCtl(
        QObject             *parent,
        MGraphX             *Xim,
        MGraphX             *Xni,
        const DAQ::Params   &p );
    virtual ~ColorTTLCtl();

    void showDialog();

    void scanBlock(
        const vec_i16   &data,
        quint64         headCt,
        int             nC,
        bool            isImec );

private slots:

private:
    void loadSettings();
    void saveSettings() const;

    int anyEvents( QVector<int> &clr, bool isImec ) const;

    bool getChan(
        int     &chan,
        int     &bit,
        int     &thresh,
        int     clr,
        bool    isImec ) const;

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
        bool            isImec );
};

#endif  // COLORTTLCTL_H


