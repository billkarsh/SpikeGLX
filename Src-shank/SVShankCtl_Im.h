#ifndef SVSHANKCTL_IM_H
#define SVSHANKCTL_IM_H

#include "SVShankCtl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SVShankCtl_Im : public SVShankCtl
{
    Q_OBJECT

private:
    QAction *audioLAction,
            *audioBAction,
            *audioRAction,
            *spike1Action,
            *spike2Action,
            *spike3Action,
            *spike4Action;
    int     ip;

public:
    SVShankCtl_Im(
        const DAQ::Params   &p,
        int                 ip,
        int                 jpanel,
        QWidget             *parent = 0 );
    virtual void init();
    virtual void mapChanged();
    virtual void qf_enable();
    virtual void setAnatomyPP( const QString &elems, int sk );
    virtual void colorTraces( MGraphX *theX, std::vector<MGraphY> &vY );
    virtual QString getLbl( int s, int r );

public slots:
    virtual void cursorOver( int ic, bool shift );
    virtual void lbutClicked( int ic, bool shift );

    void setAudioL();
    void setAudioB();
    void setAudioR();
    void setSpike1();
    void setSpike2();
    void setSpike3();
    void setSpike4();

private slots:
    void imroChanged( QString newName );

protected:
    QString settingsName() const;
    virtual void loadSettings();
    virtual void saveSettings() const;

    virtual QString screenStateName() const;

private:
    void initMenu();
    void setAudio( int LBR );
    void setSpike( int gp );
};

#endif  // SVSHANKCTL_IM_H


