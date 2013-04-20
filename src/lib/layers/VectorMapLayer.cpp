//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2011      Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#include "GeoPainter.h"
#include "ViewportParams.h"
#include "VectorMapLayer.h"

#include "VectorComposer.h"

namespace Marble
{

VectorMapLayer::VectorMapLayer( VectorComposer *vectorComposer )
    : m_vectorComposer( vectorComposer )
{
    setStationary( false );
}

QStringList VectorMapLayer::renderPosition() const
{
    return QStringList() << "SURFACE";
}

bool VectorMapLayer::render( GeoPainter *painter,
                             ViewportParams *viewport,
                             const QString &renderPos,
                             GeoSceneLayer *layer )
{
    Q_UNUSED( renderPos )
    Q_UNUSED( layer )

    bool clip = painter->isClipping();
    if (viewport->projection() == Spherical && !viewport->pan().isNull() )
        painter->setClipping( false );

    m_vectorComposer->paintVectorMap( painter, viewport );

    if ( viewport->projection() == Spherical && !viewport->pan().isNull() )
        painter->setClipping( clip );

    return true;
}

qreal VectorMapLayer::zValue() const
{
    return 100.0;
}

}
