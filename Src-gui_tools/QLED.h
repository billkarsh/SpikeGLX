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
    Q_ENUMS( ledShape )
    Q_ENUMS( ledColor )
    Q_PROPERTY( ledShape shape READ shape WRITE setShape )
    Q_PROPERTY( ledColor onColor READ onColor WRITE setOnColor )
    Q_PROPERTY( ledColor offColor READ offColor WRITE setOffColor )
    Q_PROPERTY( bool value READ value WRITE setValue )

public:
    enum ledShape {
        Circle  = 0,
        Square,
        Triangle,
        Rounded
    };
    enum ledColor {
        Red     = 0,
        Green,
        Yellow,
        Grey,
        Orange,
        Purple,
        Blue
    };
private:
    QSvgRenderer    *renderer ;
protected:
    QStringList     shapes;
    QStringList     colors;
    ledShape        m_shape;
    ledColor        m_onColor,
                    m_offColor;
    bool            m_value;

public:
    QLED( QWidget *parent = 0 );
    virtual ~QLED();

    ledShape shape() const      {return m_shape;}
    ledColor onColor() const    {return m_onColor;}
    ledColor offColor() const   {return m_offColor;}
    bool value() const          {return m_value;}

public slots:
    void setShape( ledShape );
    void setOnColor( ledColor );
    void setOffColor( ledColor );
    void setValue( bool );
    void setValueNow( bool );
    void toggleValue();

protected:
    void paintEvent( QPaintEvent *event );
};

#endif  // QLED_H


