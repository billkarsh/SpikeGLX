#ifndef SVGRAFSM_IM_H
#define SVGRAFSM_IM_H

#include "SVGrafsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVGrafsM_Im : public SVGrafsM
{
    Q_OBJECT

private:
    QVector<float>  dcLvl,
                    dcSum;
    QVector<int>    dcCnt;
    double          dcClock;

public:
    SVGrafsM_Im( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsM_Im();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual int chanCount() const;
    virtual bool isBandpass() const         {return false;}
    virtual QString filterChkTitle() const  {return "AP=AP+LF";}
    virtual QString dcChkTitle() const      {return "- DC";}
    virtual bool isSelAnalog() const;

public slots:
    virtual void bandSelChanged( int )      {}
    virtual void filterChkClicked( bool checked );
    virtual void dcChkClicked( bool checked );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );

protected:
    virtual double mySampRate();
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits();
    virtual int mySetUsrTypes();

    virtual void saveSettings();
    virtual void loadSettings();

private:
    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );
};

#endif  // SVGRAFSM_IM_H


