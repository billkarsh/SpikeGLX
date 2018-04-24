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
// https://www.linux-apps.com/content/show.php/QLed+?content=72482
//
// (1) Copied verbatim resource files to SpikeGLX/Resources/QLEDResources.
// (2) Neatened white-space and style of QLed.cpp/.h for readability.
// (3) Add resPath param to QLED ctor for flexible project organization.
// (4) Add setValueNow method for faster redrawing.
// No other library components are used.
//

#include "QLED.h"

#include <QtSvg>


QLED::QLED( QString resPath, QWidget *parent ) : QWidget(parent)
{
    renderer = new QSvgRenderer;

    if( resPath.isEmpty() )
        resPath = ":/resources/";
    else if( !resPath.endsWith( "/" ) )
        resPath += "/";

    shapes
        << resPath + "circle_"
        << resPath + "square_"
        << resPath + "triang_"
        << resPath + "round_";

    colors
        << "red.svg"
        << "green.svg"
        << "yellow.svg"
        << "grey.svg"
        << "orange.svg"
        << "purple.svg"
        << "blue.svg";

    m_shape      = Circle;
    m_onColor    = Red;
    m_offColor   = Grey;
    m_value      = false;
}


QLED::~QLED()
{
    if( renderer )
        delete renderer;
}


void QLED::setShape( ledShape newShape )
{
    m_shape = newShape;
    update();
}


void QLED::setOnColor( ledColor newColor )
{
    m_onColor = newColor;
    update();
}


void QLED::setOffColor( ledColor newColor )
{
    m_offColor = newColor;
    update();
}


void QLED::setValue( bool value )
{
    if( m_value != value ) {
        m_value = value;
        update();
    }
}


void QLED::setValueNow( bool value )
{
    m_value = value;
    repaint();
}


void QLED::toggleValue()
{
    m_value = !m_value;
    update();
}


void QLED::paintEvent( QPaintEvent * )
{
    QString ledShapeAndColor = shapes[m_shape];
    ledShapeAndColor.append( colors[m_value ? m_onColor : m_offColor] );
    renderer->load( ledShapeAndColor );

    QPainter    painter( this );
    painter.setRenderHint( QPainter::Antialiasing, true );
    renderer->render( &painter );
}


