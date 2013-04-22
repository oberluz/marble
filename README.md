marble
======

Marble Virtual Earth

Spherical Projection Panning
----------------------------

This new feature allows users to pan the globe during spherical projection mode. To pan the globe: Shift Key + Mouse Left Button

No gestures and no Routing Layer, as yet. 

I am more than happy to help anyone porting existing plugins, etc to use panning.

Suggestions and improvements are also welcome.

1. Introduction

The rest if this description explains the design of the spherical panning algorithms proposed for the marble application widget.

Spherical projection panning is the action of translating the globe (or the scene viewpoint) in a plane orthogonal to  
the line of sight of the viewer. For flat map (equirect projection) or mercator map (mercator projection) the rotation 
and panning actions are roughly equivalent and hence they remain unchanged.

2. Implementation Notes

There are several areas affected by the spherical panning implementation:

o Texture Mapping
o Layers
o Plugins
o Input Event Processing

2.1 Texture Mapping

Panning is implemented in the SphericalScanlineTextureMapper class using a modified testrure mapping scanline to take panning into account.

2.2 Layers

None of the layers are aware any panning is in effect as it is already taken into account in the texture mapping algorithm. On the other
hand MapInfoDialog and possibly the Routing layer (TBD) are rendered independent of the globe texture layers and also are required to
translate their paint artefacts.

The LayerInterface class has been modified to include a new boolean data member (m_stationary together with a getter and setter) 
initialised to false so that by default layers remain statically positioned and the LayerManager class has been modified to test whether 
the layer is stationary or alternatively should be panned. If so the LayerManager translates the painter by the current amount of pan. 
The layer itself is unaware any panning is taking effect although the viewport display is modified.
 
The TextureColorizer remains the only exception mainly because it creates a painter of its own to paint on a separate image. It cannot
share the main GeoPainter instance because a display device can only have one painter.

2.3 Plugins

Plugins have also been affected similarly. Some plugins require their paint artifacts to be positioned statically in the screen (eg, the
compass, elevation profile, etc) while others require their paint artifacts to be translated (eg, postal codes, earthquakes, photo, etc).

The approach used is exactly the same as that taken for layers. The RenderPlugin class has been modified to include a new boolean data
memeber (m_stationary together with a getter and setter) initialised to false so that by default plugins remain statically positioned.
The AbstractDataPlugin class has been modified to test whether the plugin is stationary or alternatively should be panned. If so the
AbstractDataPlugin translates the painter by the current amount of pan.  The plugin itself is unaware any panning is taking effect
although the viewport display is modified.

The current list of plugins that are modified to be pannable are:

PostalCodePlugin
EarthquakePlugin
PhotoPlugin
StarsPlugin
WikipediaPlugin
OpenCachingComPlugin
WeatherPlugin.cpp
OpenDesktopPlugin.cpp

2.4 Input Event Processing

The MarbleWidgetInputHandler class is responsible for handling most user events such as mouse moves and key presses. MarbleWidgetInputHandler
has been modified to additionally process mouse move events when the shift key is also pressed and the current widget projection is set
to Spherical. The same action during Flat and Mercator projections are processed as a rotation request and handled using the current methods.

When panning commences the input handler records the current position to be used to determine how much panning is in effect. The pan is then
applied to the MarbleWidget through a new method called pan overloaded to take a point or a pair of screen coordinates. The MarbleWidget then
applies the pan to the viewport so that it is accessible in all renderable classes.

