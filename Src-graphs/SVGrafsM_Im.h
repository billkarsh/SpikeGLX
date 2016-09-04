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
    QAction *imroAction,
            *stbyAction;

public:
    SVGrafsM_Im( GraphsWindow *gw, DAQ::Params &p );
    virtual ~SVGrafsM_Im();

    virtual void putScans( vec_i16 &data, quint64 headCt );

    virtual int chanCount() const;
    virtual bool isBandpass() const         {return false;}
    virtual QString filterChkTitle() const  {return "AP=AP+LF";}
    virtual bool isSelAnalog() const;
    virtual void setRecordingEnabled( bool checked );

public slots:
    virtual void bandSelChanged( int )      {}
    virtual void filterChkClicked( bool checked );
    virtual void sAveRadChanged( int radius );

private slots:
    virtual void mySaveGraphClicked( bool checked );

    virtual void myMouseOverGraph( double x, double y, int iy );
    virtual void myClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy );
    void editImro();
    void editStby();

protected:
    virtual void myInit();
    virtual double mySampRate() const;
    virtual void mySort_ig2ic();
    virtual QString myChanName( int ic ) const;
    virtual QBitArray& mySaveBits() const;
    virtual int mySetUsrTypes();

    virtual void loadSettings();
    virtual void saveSettings() const;

private:
    double scalePlotValue( double v, double gain ) const;
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit ) const;
    bool stbyDialog( QString &stbyStr );
};

#endif  // SVGRAFSM_IM_H


