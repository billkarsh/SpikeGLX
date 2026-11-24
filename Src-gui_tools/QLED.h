/***************************************************************************
 *   Copyright (C) 2010 by P. Sereno                                       *
 *   http://www.sereno-online.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License           *
 *   version 2.1 as published by the Free Software Foundation              *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License for more details.                   *
 *   http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.               *
 ***************************************************************************/

// Bill Karsh - Modifications to Qled library, 4/1/2018.
// -----------------------------------------------------
// Downloaded original library here (version 0.6.2):
// <https://www.linux-apps.com/content/show.php/QLed+?content=72482>
//
// (1) Stored original zip here: SpikeGLX/z3rdPartyTools.
// (2) Copied verbatim resource files to SpikeGLX/Resources/QLEDResources.
// (3) Neatened white-space and style of QLed.cpp/.h for readability.
// (4) Added resPath param to QLED ctor for flexible project organization.
// (5) Added setValueNow() method for faster redrawing.
// (6) Move Q_ENUMS (deprecated) to Q_ENUM after resp. enum{}.
// (7) Add signal "NOTIFY xxxChanged" to Q_PROPERTY.
// (8) Add namespace QLED:: to slot params.
// No other library components are used.
//

#ifndef QLED_H
#define QLED_H

#include <QWidget>

class QColor;
class QSvgRenderer;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class QLED : public QWidget
{
    Q_OBJECT
    Q_PROPERTY( ledShape shape READ shape WRITE setShape NOTIFY shapeChanged )
    Q_PROPERTY( ledColor onColor READ onColor WRITE setOnColor NOTIFY onColorChanged )
    Q_PROPERTY( ledColor offColor READ offColor WRITE setOffColor NOTIFY offColorChanged )
    Q_PROPERTY( bool value READ value WRITE setValue NOTIFY valueChanged )

public:
    enum ledShape {
        Circle  = 0,
        Square,
        Triangle,
        Rounded
    };
    Q_ENUM( ledShape )

    enum ledColor {
        Red     = 0,
        Green,
        Yellow,
        Grey,
        Orange,
        Purple,
        Blue
    };
    Q_ENUM( ledColor )

private:
    QSvgRenderer    *renderer;
protected:
    QStringList     shapes;
    QStringList     colors;
    ledShape        m_shape;
    ledColor        m_onColor,
                    m_offColor;
    bool            m_value;

public:
    QLED( QString resPath = "", QWidget *parent = 0 );
    virtual ~QLED();

    ledShape shape() const      {return m_shape;}
    ledColor onColor() const    {return m_onColor;}
    ledColor offColor() const   {return m_offColor;}
    bool value() const          {return m_value;}

signals:
    void shapeChanged( QLED::ledShape );
    void onColorChanged( QLED::ledColor );
    void offColorChanged( QLED::ledColor );
    void valueChanged( bool );

public slots:
    void setShape( QLED::ledShape );
    void setOnColor( QLED::ledColor );
    void setOffColor( QLED::ledColor );
    void setValue( bool );
    void setValueNow( bool );
    void toggleValue();

protected:
    void paintEvent( QPaintEvent *event );
};

#endif  // QLED_H


