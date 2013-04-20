//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2010 Dennis Nienhüser <earthwings@gentoo.org>
//

#include "LayerInterface.h"

namespace Marble
{

class LayerInterface::Private
{
  public:
    Private( )
        : m_stationary( true )
    {
    }

    ~Private()
    {
    }

    bool                m_stationary;
};

LayerInterface::LayerInterface()
    : d( new Private( ) )
{

}

LayerInterface::~LayerInterface()
{
    // nothing to do
}


qreal LayerInterface::zValue() const
{
    return 0.0;
}

QString LayerInterface::runtimeTrace() const
{
    return QString();
}

void LayerInterface::setStationary( bool value )
{
    d->m_stationary = value;
}

bool LayerInterface::stationary() const
{
    return d->m_stationary;
}

} // namespace Marble
