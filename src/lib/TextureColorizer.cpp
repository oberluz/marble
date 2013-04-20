//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2006-2007 Torsten Rahn <tackat@kde.org>
// Copyright 2007      Inge Wallin  <ingwa@kde.org>
// Copyright 2008      Carlos Licea <carlos.licea@kdemail.net>
// Copyright 2012      Cezar Mocan  <mocancezar@gmail.com>
//

#include "TextureColorizer.h"

#include <QtCore/qmath.h>
#include <QtCore/QFile>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QTime>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include "MarbleGlobal.h"
#include "GeoPainter.h"
#include "MarbleDebug.h"
#include "VectorComposer.h"
#include "ViewParams.h"
#include "ViewportParams.h"
#include "MathHelper.h"
#include "GeoDataFeature.h"
#include "GeoDataTypes.h"
#include "GeoDataPlacemark.h"
#include "GeoDataDocument.h"

namespace Marble
{

class EmbossFifo
{
public:
    EmbossFifo()
        : x1( 0 )
        , x2( 0 )
        , x3( 0 )
        , x4( 0 )
    {}

    inline uchar head() const { return x1; }

    inline EmbossFifo &operator<<( uchar value )
    {
        x1 = x2;
        x2 = x3;
        x3 = x4;
        x4 = value;

        return *this;
    }

private:
    uchar  x1;
    uchar  x2;
    uchar  x3;
    uchar  x4;
};


TextureColorizer::TextureColorizer( const QString &seafile,
                                    const QString &landfile,
                                    VectorComposer *veccomposer )
    : m_veccomposer( veccomposer ),
      m_landColor(qRgb( 255, 0, 0 ) ),
      m_seaColor( qRgb( 0, 255, 0 ) )
{
    QTime t;
    t.start();

    QImage   gradientImage ( 256, 1, QImage::Format_RGB32 );
    QPainter  gradientPainter;
    gradientPainter.begin( &gradientImage );
    gradientPainter.setPen( Qt::NoPen );


    int shadingStart = 120;
    QImage    shadingImage ( 16, 1, QImage::Format_RGB32 );
    QPainter  shadingPainter;
    shadingPainter.begin( &shadingImage );
    shadingPainter.setPen( Qt::NoPen );

    int offset = 0;

    QStringList  filelist;
    filelist << seafile << landfile;

    foreach ( const QString &filename, filelist ) {

        QLinearGradient  gradient( 0, 0, 256, 0 );

        QFile  file( filename );
        file.open( QIODevice::ReadOnly );
        QTextStream  stream( &file );  // read the data from the file

        QString      evalstrg;

        while ( !stream.atEnd() ) {
            stream >> evalstrg;
            if ( !evalstrg.isEmpty() && evalstrg.contains( '=' ) ) {
                QString  colorValue = evalstrg.left( evalstrg.indexOf( '=' ) );
                QString  colorPosition = evalstrg.mid( evalstrg.indexOf( '=' ) + 1 );
                gradient.setColorAt( colorPosition.toDouble(),
                                     QColor( colorValue ) );
            }
        }
        gradientPainter.setBrush( gradient );
        gradientPainter.drawRect( 0, 0, 256, 1 );

        QLinearGradient  shadeGradient( - shadingStart, 0, 256 - shadingStart, 0 );

        shadeGradient.setColorAt(0.00, QColor(Qt::white));
        shadeGradient.setColorAt(0.15, QColor(Qt::white));
        shadeGradient.setColorAt(0.75, QColor(Qt::black));
        shadeGradient.setColorAt(1.00, QColor(Qt::black));

        const QRgb * gradientScanLine  = (QRgb*)( gradientImage.scanLine( 0 ) );
        const QRgb * shadingScanLine   = (QRgb*)( shadingImage.scanLine( 0 ) );

        for ( int i = 0; i < 256; ++i ) {

            QRgb shadeColor = *(gradientScanLine + i );
            shadeGradient.setColorAt(0.496, shadeColor);
            shadeGradient.setColorAt(0.504, shadeColor);
            shadingPainter.setBrush( shadeGradient );
            shadingPainter.drawRect( 0, 0, 16, 1 );

            // populate texturepalette[][]
            for ( int j = 0; j < 16; ++j ) {
                texturepalette[j][offset + i] = *(shadingScanLine + j );
            }
        }

        offset += 256;
    }
    shadingPainter.end();  // Need to explicitly tell painter lifetime to avoid crash
    gradientPainter.end(); // on some systems. 

    m_seafile = seafile;
    m_landfile = landfile;

    mDebug() << Q_FUNC_INFO << "Time elapsed:" << t.elapsed() << "ms";
}

void TextureColorizer::addSeaDocument( const GeoDataDocument *seaDocument )
{
    m_seaDocuments.append( seaDocument );
}

void TextureColorizer::addLandDocument( const GeoDataDocument *landDocument )
{
    m_landDocuments.append( landDocument );
}

void TextureColorizer::setShowRelief( bool show )
{
    m_showRelief = show;
}

// This function takes two images, both in viewParams:
//  - The coast image, which has a number of colors where each color
//    represents a sort of terrain (ex: land/sea)
//  - The canvas image, which has a gray scale image, often
//    representing a height field.
//
// It then uses the values of the pixels in the coast image to select
// a color map.  The value of the pixel in the canvas image is used as
// an index into the selected color map and the resulting color is
// written back to the canvas image.  This way we can have different
// color schemes for land and water.
//
// In addition to this, a simple form of bump mapping is performed to
// increase the illusion of height differences (see the variable
// showRelief).
// 

void TextureColorizer::drawIndividualDocument( GeoPainter *painter, const GeoDataDocument *document )
{
    QVector<GeoDataFeature*>::ConstIterator i = document->constBegin();
    QVector<GeoDataFeature*>::ConstIterator end = document->constEnd();

    for ( ; i != end; ++i ) {
        if ( (*i)->nodeType() == GeoDataTypes::GeoDataPlacemarkType ) {

            const GeoDataPlacemark *placemark = static_cast<const GeoDataPlacemark*>( *i );

            if ( placemark->geometry()->nodeType() == GeoDataTypes::GeoDataLineStringType ) {
                const GeoDataLineString *child = static_cast<const GeoDataLineString*>( placemark->geometry() );
                const GeoDataLinearRing ring( *child );
                painter->drawPolygon( ring );
            }

            if ( placemark->geometry()->nodeType() == GeoDataTypes::GeoDataPolygonType ) {
                const GeoDataPolygon *child = static_cast<const GeoDataPolygon*>( placemark->geometry() );
                painter->drawPolygon( *child );
            }

            if ( placemark->geometry()->nodeType() == GeoDataTypes::GeoDataLinearRingType ) {
                const GeoDataLinearRing *child = static_cast<const GeoDataLinearRing*>( placemark->geometry() );
                painter->drawPolygon( *child );
            }
        }
    }
}

void TextureColorizer::drawTextureMap( GeoPainter *painter )
{
    foreach( const GeoDataDocument *doc, m_landDocuments ) {
        painter->setPen( QPen( Qt::NoPen ) );
        painter->setBrush( QBrush( m_landColor ) );
        drawIndividualDocument( painter, doc );
    }

    foreach( const GeoDataDocument *doc, m_seaDocuments ) {
        if ( doc->isVisible() ) {
            painter->setPen( Qt::NoPen );
            painter->setBrush( QBrush( m_seaColor ) );
            drawIndividualDocument( painter, doc );
        }
    }
}

void TextureColorizer::colorize( QImage *origimg, const ViewportParams *viewport, MapQuality mapQuality )
{
    if ( m_coastImage.size() != viewport->size() )
        m_coastImage = QImage( viewport->size(), QImage::Format_RGB32 );

    // update coast image
    m_coastImage.fill( QColor( 0, 0, 255, 0).rgb() );

    const bool antialiased =    mapQuality == HighQuality
                             || mapQuality == PrintQuality;

    GeoPainter painter( &m_coastImage, viewport, mapQuality );
    painter.setRenderHint( QPainter::Antialiasing, antialiased );
    painter.translate(viewport->pan());

    if ( m_landDocuments.isEmpty() ) {
        m_veccomposer->drawTextureMap( &painter, viewport );
    } else {
        drawTextureMap( &painter );
    }

    const qint64   radius   = viewport->radius();

    const int  imgheight = origimg->height();
    const int  imgwidth  = origimg->width();
    const int  imgrx     = imgwidth / 2;
    const int  imgry     = imgheight / 2;
    // This variable is not used anywhere..
    const int  imgradius = imgrx * imgrx + imgry * imgry;

    int     bump = 8;

    if (( radius * radius > imgradius && viewport->pan().isNull() )
         || viewport->projection() == Equirectangular
         || viewport->projection() == Mercator )
    {
        int yTop = 0;
        int yBottom = imgheight;

        if( viewport->projection() == Equirectangular
            || viewport->projection() == Mercator )
        {
            // Calculate translation of center point
            const qreal centerLat = viewport->centerLatitude();

            const float rad2Pixel = (qreal)( 2 * radius ) / M_PI;
            if ( viewport->projection() == Equirectangular ) {
                int yCenterOffset = (int)( centerLat * rad2Pixel );
                yTop = ( imgry - radius + yCenterOffset < 0)? 0 : imgry - radius + yCenterOffset;
                yBottom = ( imgry + yCenterOffset + radius > imgheight )? imgheight : imgry + yCenterOffset + radius;
            }
            else if ( viewport->projection() == Mercator ) {
                int yCenterOffset = (int)( asinh( tan( centerLat ) ) * rad2Pixel  );
                yTop = ( imgry - 2 * radius + yCenterOffset < 0 ) ? 0 : imgry - 2 * radius + yCenterOffset;
                yBottom = ( imgry + 2 * radius + yCenterOffset > imgheight )? imgheight : imgry + 2 * radius + yCenterOffset;
            }
        }

        const int itEnd = yBottom;

        for (int y = yTop; y < itEnd; ++y) {

            QRgb  *writeData         = (QRgb*)( origimg->scanLine( y ) );
            const QRgb  *coastData   = (QRgb*)( m_coastImage.scanLine( y ) );

            uchar *readDataStart     = origimg->scanLine( y );
            const uchar *readDataEnd = readDataStart + imgwidth*4;

            EmbossFifo  emboss;

            for ( uchar* readData = readDataStart; 
                  readData < readDataEnd;
                  readData += 4, ++writeData, ++coastData )
            {

                // Cheap Emboss / Bumpmapping
                uchar&  grey = *readData; // qBlue(*data);

                if ( m_showRelief ) {
                    emboss << grey;
                    bump = ( emboss.head() + 8 - grey );
                    if ( bump  < 0 )  bump = 0;
                    if ( bump  > 15 ) bump = 15;
                }
                setPixel( coastData, writeData, bump, grey );
            }
        }
    }
    else {
        int viewportHeight = viewport->height();
        int pany = viewport->pan().y();
        int skip = 0;
        int yTop = 0;
        if ( viewportHeight / 2 - radius + pany > 0 )
        {
            yTop = viewportHeight / 2 - radius + pany;
            if ( yTop > viewportHeight )
                yTop = viewportHeight;
        }
        int yBottom = 0;
        if ( viewportHeight / 2 + radius + pany > 0 )
        {
            yBottom = viewportHeight / 2 + radius + pany - skip;
            if ( yBottom > viewportHeight )
                yBottom = viewportHeight - skip;
        }
        if ( yTop == yBottom )
            return;

        const int viewportWidth = viewport->width();
        const int panx = viewport->pan().x();

        EmbossFifo  emboss;

        for ( int y = yTop; y < yBottom; ++y ) {
            const int  dy = imgry - y + pany;
            int  rx = (int)sqrt( (qreal)( radius * radius - dy * dy ) );
            int  xLeft  = 0; 
            if ( viewportWidth / 2 - rx + panx > 0 )
            {
                xLeft = viewportWidth / 2 - rx + panx;
                if ( xLeft > viewportWidth )
                    xLeft = viewportWidth;
            }
            int  xRight = 0;
            if ( viewportWidth / 2 + rx + panx > 0 )
            {
                xRight = viewportWidth / 2 + rx + panx;
                if ( xRight > viewportWidth )
                    xRight = viewportWidth;
            }
            if ( xLeft == xRight )
                continue;

            QRgb  *writeData         = (QRgb*)( origimg->scanLine( y ) )  + xLeft;
            const QRgb *coastData    = (QRgb*)( m_coastImage.scanLine( y ) ) + xLeft;

            uchar *readDataStart     = origimg->scanLine( y ) + xLeft * 4;
            const uchar *readDataEnd = origimg->scanLine( y ) + xRight * 4;

 
            for ( uchar* readData = readDataStart;
                  readData < readDataEnd;
                  readData += 4, ++writeData, ++coastData )
            {
                // Cheap Emboss / Bumpmapping

                uchar& grey = *readData; // qBlue(*data);

                if ( m_showRelief ) {
                    emboss << grey;
                    bump = ( emboss.head() + 16 - grey ) >> 1;
                    if ( bump > 15 ) bump = 15;
                    if ( bump < 0 )  bump = 0;
                }
                setPixel( coastData, writeData, bump, grey );
            }
        }
    }
}

void TextureColorizer::setPixel( const QRgb *coastData, QRgb *writeData, int bump, uchar grey )
{
    int alpha = qRed( *coastData );
    if ( alpha == 255 )
        *writeData = texturepalette[bump][grey + 0x100];
    else if( alpha == 0 ){
        *writeData = texturepalette[bump][grey];
    }
    else {
        qreal c = 1.0 / 255.0;

        QRgb landcolor  = (QRgb)(texturepalette[bump][grey + 0x100]);
        QRgb watercolor = (QRgb)(texturepalette[bump][grey]);

        *writeData = qRgb(
                    (int) ( c * ( alpha * qRed( landcolor )
                                  + ( 255 - alpha ) * qRed( watercolor ) ) ),
                    (int) ( c * ( alpha * qGreen( landcolor )
                                  + ( 255 - alpha ) * qGreen( watercolor ) ) ),
                    (int) ( c * ( alpha * qBlue( landcolor )
                                  + ( 255 - alpha ) * qBlue( watercolor ) ) )
                    );
    }
}
}
