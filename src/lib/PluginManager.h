//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2006-2008 Torsten Rahn <tackat@kde.org>
// Copyright 2009      Jens-Michael Hoffmann <jensmh@gmx.de>
//


#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QtCore/QList>
#include <QtCore/QObject>
#include "marble_export.h"


namespace Marble
{

class RenderPlugin;
class NetworkPlugin;
class AbstractFloatItem;
class PluginManagerPrivate;

/**
 * @short The class that handles Marble's plugins.
 *
 * Ownership policy for plugins:
 *
 * On every invocation of createRenderPlugins, createNetworkPlugins and
 * createFloatItems the PluginManager creates new objects and transfers
 * ownership to the calling site. In order to create
 * the objects, the PluginManager internally has a list of the plugins
 * which are owned by the PluginManager and destroyed by it.
 *
 */

class MARBLE_EXPORT PluginManager : public QObject
{
    Q_OBJECT

 public:
    explicit PluginManager( QObject *parent = 0 );
    ~PluginManager();

    /**
     * This methods creates a new set of plugins and transfers ownership
     * of them to the client.
     */
    QList<AbstractFloatItem *> createFloatItems() const;

    /**
     * This methods creates a new set of plugins and transfers ownership
     * of them to the client.
     */
    QList<RenderPlugin *> createRenderPlugins() const;

    /**
     * This methods creates a new set of plugins and transfers ownership
     * of them to the client.
     */
    QList<NetworkPlugin *> createNetworkPlugins() const;

 private:
    Q_DISABLE_COPY( PluginManager )

    /**
     * @brief Browses the plugin directories and loads plugins.
     *
     * This method deletes all previously loaded render plugin templates and is only
     * for internal use of PluginManager.
     *
     * loadPlugins browses all plugin directories and loads
     * all plugins found in there.
     *
     * FIXME: this can be public once the network plugins issue is resolved.
     */
    void loadPlugins();

    PluginManagerPrivate  * const d;
};

}

#endif // PLUGINMANAGER_H
