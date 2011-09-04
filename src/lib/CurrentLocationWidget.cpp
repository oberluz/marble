//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2004-2007 Torsten Rahn  <tackat@kde.org>
// Copyright 2007      Inge Wallin   <ingwa@kde.org>
// Copyright 2007      Thomas Zander <zander@kde.org>
// Copyright 2010      Bastian Holst <bastianholst@gmx.de>
//

// Self
#include "CurrentLocationWidget.h"

// Marble
#include "AdjustNavigation.h"
#include "MarbleDebug.h"
#include "MarbleLocale.h"
#include "MarbleModel.h"
#include "MarbleWidget.h"
#include "MarbleWidgetPopupMenu.h"
#include "GeoDataCoordinates.h"
#include "PositionProviderPlugin.h"
#include "PluginManager.h"
#include "PositionTracking.h"
#include "routing/RoutingManager.h"

using namespace Marble;
/* TRANSLATOR Marble::CurrentLocationWidget */

// Ui
#include "ui_CurrentLocationWidget.h"

#include <QtCore/QDateTime>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>

namespace Marble
{

class CurrentLocationWidgetPrivate
{
 public:
    Ui::CurrentLocationWidget      m_currentLocationUi;
    MarbleWidget                  *m_widget;
    AdjustNavigation *m_adjustNavigation;

    QList<PositionProviderPlugin*> m_positionProviderPlugins;
    GeoDataCoordinates             m_currentPosition;

    QString m_lastOpenPath;
    QString m_lastSavePath;

    void adjustPositionTrackingStatus( PositionProviderStatus status );
    void changePositionProvider( const QString &provider );
    void trackPlacemark();
    void centerOnCurrentLocation();
    void updateRecenterComboBox( AdjustNavigation::CenterMode centerMode );
    void updateAutoZoomCheckBox( bool autoZoom );
    void updateActivePositionProvider( PositionProviderPlugin* );
    void saveTrack();
    void openTrack();
    void clearTrack();
};

CurrentLocationWidget::CurrentLocationWidget( QWidget *parent, Qt::WindowFlags f )
    : QWidget( parent, f ),
      d( new CurrentLocationWidgetPrivate() )
{
    d->m_currentLocationUi.setupUi( this );

    connect( d->m_currentLocationUi.recenterComboBox, SIGNAL ( currentIndexChanged( int ) ),
            this, SLOT( setRecenterMode( int ) ) );

    connect( d->m_currentLocationUi.autoZoomCheckBox, SIGNAL( clicked( bool ) ),
             this, SLOT( setAutoZoom( bool ) ) );

    bool const smallScreen = MarbleGlobal::getInstance()->profiles() & MarbleGlobal::SmallScreen;
    d->m_currentLocationUi.positionTrackingComboBox->setVisible( !smallScreen );
    d->m_currentLocationUi.locationLabel->setVisible( !smallScreen );
    d->m_currentLocationUi.openTrackPushButton->setVisible( smallScreen );
}

CurrentLocationWidget::~CurrentLocationWidget()
{
    delete d;
}

void CurrentLocationWidget::setMarbleWidget( MarbleWidget *widget )
{
    d->m_widget = widget;

    d->m_adjustNavigation = new AdjustNavigation( d->m_widget, this );
    d->m_widget->model()->routingManager()->setAdjustNavigation( d->m_adjustNavigation );

    const PluginManager* pluginManager = d->m_widget->model()->pluginManager();
    d->m_positionProviderPlugins = pluginManager->createPositionProviderPlugins();
    foreach( const PositionProviderPlugin *plugin, d->m_positionProviderPlugins ) {
        d->m_currentLocationUi.positionTrackingComboBox->addItem( plugin->guiString() );
    }
    if ( d->m_positionProviderPlugins.isEmpty() ) {
        d->m_currentLocationUi.positionTrackingComboBox->setEnabled( false );
        QString html = "<p>No Position Tracking Plugin installed.</p>";
        d->m_currentLocationUi.locationLabel->setText( html );
        d->m_currentLocationUi.locationLabel->setEnabled ( true );
        bool const hasTrack = !d->m_widget->model()->positionTracking()->isTrackEmpty();
        d->m_currentLocationUi.showTrackCheckBox->setEnabled( hasTrack );
        d->m_currentLocationUi.saveTrackPushButton->setEnabled( hasTrack );
        d->m_currentLocationUi.clearTrackPushButton->setEnabled( hasTrack );
    }

    //disconnect CurrentLocation Signals
    disconnect( d->m_widget->model()->positionTracking(),
             SIGNAL( gpsLocation( GeoDataCoordinates, qreal ) ),
             this, SLOT( receiveGpsCoordinates( GeoDataCoordinates, qreal ) ) );
    disconnect( d->m_widget->model()->positionTracking(),
             SIGNAL( positionProviderPluginChanged( PositionProviderPlugin* ) ),
             this, SLOT( updateActivePositionProvider( PositionProviderPlugin* ) ) );
    disconnect( d->m_currentLocationUi.positionTrackingComboBox, SIGNAL( currentIndexChanged( QString ) ),
             this, SLOT( changePositionProvider( QString ) ) );
    disconnect( d->m_currentLocationUi.locationLabel, SIGNAL( linkActivated( QString ) ),
             this, SLOT( centerOnCurrentLocation() ) );
    disconnect( d->m_widget->model()->positionTracking(),
             SIGNAL( statusChanged( PositionProviderStatus) ),this,
             SLOT( adjustPositionTrackingStatus( PositionProviderStatus) ) );

    disconnect( d->m_adjustNavigation, SIGNAL( recenterModeChanged( AdjustNavigation::CenterMode ) ),
             this, SLOT( updateRecenterComboBox( AdjustNavigation::CenterMode ) ) );
    disconnect( d->m_adjustNavigation, SIGNAL( autoZoomToggled( bool ) ),
             this, SLOT( updateAutoZoomCheckBox( bool ) ) );
    disconnect( d->m_widget->model(), SIGNAL( trackedPlacemarkChanged( const GeoDataPlacemark* ) ),
             this, SLOT( trackPlacemark() ) );

    //connect CurrentLoctaion signals
    connect( d->m_widget->model()->positionTracking(),
             SIGNAL( gpsLocation( GeoDataCoordinates, qreal ) ),
             this, SLOT( receiveGpsCoordinates( GeoDataCoordinates, qreal ) ) );
    connect( d->m_widget->model()->positionTracking(),
             SIGNAL( positionProviderPluginChanged( PositionProviderPlugin* ) ),
             this, SLOT( updateActivePositionProvider( PositionProviderPlugin* ) ) );
    d->updateActivePositionProvider( d->m_widget->model()->positionTracking()->positionProviderPlugin() );
    connect( d->m_currentLocationUi.positionTrackingComboBox, SIGNAL( currentIndexChanged( QString ) ),
             this, SLOT( changePositionProvider( QString ) ) );
    connect( d->m_currentLocationUi.locationLabel, SIGNAL( linkActivated( QString ) ),
             this, SLOT( centerOnCurrentLocation() ) );
    connect( d->m_widget->model()->positionTracking(),
             SIGNAL( statusChanged( PositionProviderStatus) ), this,
             SLOT( adjustPositionTrackingStatus( PositionProviderStatus) ) );

    connect( d->m_adjustNavigation, SIGNAL( recenterModeChanged( AdjustNavigation::CenterMode ) ),
             this, SLOT( updateRecenterComboBox( AdjustNavigation::CenterMode ) ) );
    connect( d->m_adjustNavigation, SIGNAL( autoZoomToggled( bool ) ),
             this, SLOT( updateAutoZoomCheckBox( bool ) ) );
    connect (d->m_currentLocationUi.showTrackCheckBox, SIGNAL( clicked(bool) ),
             d->m_widget->model()->positionTracking(), SLOT( setTrackVisible(bool) ));
    connect (d->m_currentLocationUi.showTrackCheckBox, SIGNAL( clicked(bool) ),
             d->m_widget, SLOT(update()));
    if ( d->m_widget->model()->positionTracking()->trackVisible() ) {
        d->m_currentLocationUi.showTrackCheckBox->setCheckState(Qt::Checked);
    }
    connect ( d->m_currentLocationUi.saveTrackPushButton, SIGNAL( clicked(bool)),
              this, SLOT(saveTrack()));
    connect ( d->m_currentLocationUi.openTrackPushButton, SIGNAL( clicked(bool)),
              this, SLOT(openTrack()));
    connect (d->m_currentLocationUi.clearTrackPushButton, SIGNAL( clicked(bool)),
             this, SLOT(clearTrack()));
    connect( d->m_widget->model(), SIGNAL( trackedPlacemarkChanged( const GeoDataPlacemark* ) ),
             this, SLOT( trackPlacemark() ) );
}

void CurrentLocationWidgetPrivate::adjustPositionTrackingStatus( PositionProviderStatus status )
{
    if ( status == PositionProviderStatusAvailable ) {
        return;
    }

    QString html = "<html><body><p>";

    switch ( status ) {
        case PositionProviderStatusUnavailable:
        html += QObject::tr( "Waiting for current location information..." );
            break;
        case PositionProviderStatusAcquiring:
            html += QObject::tr( "Initializing current location service..." );
            break;
        case PositionProviderStatusAvailable:
            Q_ASSERT( false );
            break;
        case PositionProviderStatusError:
            html += QObject::tr( "Error when determining current location: " );
            html += m_widget->model()->positionTracking()->error();
            break;
    }

    html += "</p></body></html>";
    m_currentLocationUi.locationLabel->setEnabled( true );
    m_currentLocationUi.locationLabel->setText( html );
}

void CurrentLocationWidgetPrivate::updateActivePositionProvider( PositionProviderPlugin *plugin )
{
    m_currentLocationUi.positionTrackingComboBox->blockSignals( true );
    if ( !plugin ) {
        m_currentLocationUi.positionTrackingComboBox->setCurrentIndex( 0 );
    } else {
        for( int i=0; i<m_currentLocationUi.positionTrackingComboBox->count(); ++i ) {
            if ( m_currentLocationUi.positionTrackingComboBox->itemText( i ) == plugin->guiString() ) {
                m_currentLocationUi.positionTrackingComboBox->setCurrentIndex( i );
                break;
            }
        }
    }
    m_currentLocationUi.positionTrackingComboBox->blockSignals( false );
    m_currentLocationUi.recenterLabel->setEnabled( plugin );
    m_currentLocationUi.recenterComboBox->setEnabled( plugin );
    m_currentLocationUi.autoZoomCheckBox->setEnabled( plugin );

}

void CurrentLocationWidget::receiveGpsCoordinates( const GeoDataCoordinates &position, qreal speed )
{
    d->m_currentPosition = position;
    QString unitString;
    QString speedString;
    QString distanceUnitString;
    QString distanceString;
    qreal unitSpeed = 0.0;
    qreal distance = 0.0;

    QString html = "<html><body>";
    html += "<table cellspacing=\"2\" cellpadding=\"2\">";
    html += "<tr><td>Longitude</td><td><a href=\"http://edu.kde.org/marble\">%1</a></td></tr>";
    html += "<tr><td>Latitude</td><td><a href=\"http://edu.kde.org/marble\">%2</a></td></tr>";
    html += "<tr><td>Altitude</td><td>%3</td></tr>";
    html += "<tr><td>Speed</td><td>%4</td></tr>";
    html += "</table>";
    html += "</body></html>";

    switch ( MarbleGlobal::getInstance()->locale()->measureSystem() ) {
        case Metric:
        //kilometers per hour
        unitString = tr("km/h");
        unitSpeed = speed * HOUR2SEC * METER2KM;
        distanceUnitString = tr("m");
        distance = position.altitude();
        break;

        case Imperial:
        //miles per hour
        unitString = tr("m/h");
        unitSpeed = speed * HOUR2SEC * METER2KM * KM2MI;
        distanceUnitString = tr("ft");
        distance = position.altitude() * M2FT;
        break;
    }
    // TODO read this value from the incoming signal
    speedString = QLocale::system().toString( unitSpeed, 'f', 1);
    distanceString = QString( "%1 %2" ).arg( distance, 0, 'f', 1, QChar(' ') ).arg( distanceUnitString );

    html = html.arg( position.lonToString() ).arg( position.latToString() );
    html = html.arg( distanceString ).arg( speedString + ' ' + unitString );
    d->m_currentLocationUi.locationLabel->setText( html );
    d->m_currentLocationUi.showTrackCheckBox->setEnabled( true );
    d->m_currentLocationUi.saveTrackPushButton->setEnabled( true );
    d->m_currentLocationUi.clearTrackPushButton->setEnabled( true );
}

void CurrentLocationWidgetPrivate::changePositionProvider( const QString &provider )
{
    bool hasProvider = ( provider != QObject::tr("Disabled") );

    if ( hasProvider ) {
        foreach( PositionProviderPlugin* plugin, m_positionProviderPlugins ) {
            if ( plugin->guiString() == provider ) {
                m_currentLocationUi.locationLabel->setEnabled( true );
                PositionProviderPlugin* instance = plugin->newInstance();
                PositionTracking *tracking = m_widget->model()->positionTracking();
                tracking->setPositionProviderPlugin( instance );
                m_widget->update();
                return;
            }
        }
    }
    else {
        m_currentLocationUi.locationLabel->setEnabled( false );
        m_widget->model()->positionTracking()->setPositionProviderPlugin( 0 );
        m_widget->update();
    }
}

void CurrentLocationWidgetPrivate::trackPlacemark()
{
    foreach( PositionProviderPlugin* plugin, m_positionProviderPlugins ) {
        if ( plugin->nameId() == "Placemark" ) {
            PositionProviderPlugin *instance = plugin->newInstance();
            instance->setMarbleModel( m_widget->model() );
            PositionTracking *tracking = m_widget->model()->positionTracking();
            tracking->setPositionProviderPlugin( instance );
            m_adjustNavigation->setRecenter( AdjustNavigation::AlwaysRecenter );
            m_widget->update();
            return;
        }
    }
}

void CurrentLocationWidget::setRecenterMode( int mode )
{
    if ( mode >= 0 && mode <= AdjustNavigation::RecenterOnBorder ) {
        AdjustNavigation::CenterMode centerMode = ( AdjustNavigation::CenterMode ) mode;
        d->m_adjustNavigation->setRecenter( centerMode );
    }
}

void CurrentLocationWidget::setAutoZoom( bool autoZoom )
{
    d->m_adjustNavigation->setAutoZoom( autoZoom );
}

void CurrentLocationWidgetPrivate::updateAutoZoomCheckBox( bool autoZoom )
{
    m_currentLocationUi.autoZoomCheckBox->setChecked( autoZoom );
}

void CurrentLocationWidgetPrivate::updateRecenterComboBox( AdjustNavigation::CenterMode centerMode )
{
    m_currentLocationUi.recenterComboBox->setCurrentIndex( centerMode );
}

void CurrentLocationWidgetPrivate::centerOnCurrentLocation()
{
    m_widget->centerOn(m_currentPosition, true);
}

void CurrentLocationWidgetPrivate::saveTrack()
{
    QString suggested = m_lastSavePath;
    QString fileName = QFileDialog::getSaveFileName(m_widget, QObject::tr("Save Track"), // krazy:exclude=qclasses
                                                    suggested.append('/' + QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss") + ".kml"),
                            QObject::tr("KML File (*.kml)"));
    if ( !fileName.isEmpty() ) {
        QFileInfo file( fileName );
        m_lastSavePath = file.absolutePath();
        m_widget->model()->positionTracking()->saveTrack( fileName );
    }
}

void CurrentLocationWidgetPrivate::openTrack()
{
    QString suggested = m_lastOpenPath;
    QString fileName = QFileDialog::getOpenFileName( m_widget, QObject::tr("Open Track"), // krazy:exclude=qclasses
                                                    suggested, QObject::tr("KML File (*.kml)"));
    if ( !fileName.isEmpty() ) {
        QFileInfo file( fileName );
        m_lastOpenPath = file.absolutePath();
        m_widget->model()->addGeoDataFile( fileName );
    }
}

void CurrentLocationWidgetPrivate::clearTrack()
{
    const int result = QMessageBox::question( m_widget,
                                              QObject::tr( "Clear current track" ),
                                              QObject::tr( "Are you sure you want to clear the current track?" ),
                                              QMessageBox::Yes,
                                              QMessageBox::No );

    if ( result == QMessageBox::Yes ) {
        m_widget->model()->positionTracking()->clearTrack();
        m_widget->update();
        m_currentLocationUi.saveTrackPushButton->setEnabled( false );
        m_currentLocationUi.clearTrackPushButton->setEnabled( false );
    }
}

AdjustNavigation::CenterMode CurrentLocationWidget::recenterMode() const
{
    return d->m_adjustNavigation->recenterMode();
}

bool CurrentLocationWidget::autoZoom() const
{
    return d->m_adjustNavigation->autoZoom();
}

bool CurrentLocationWidget::trackVisible() const
{
    return d->m_widget->model()->positionTracking()->trackVisible();
}

QString CurrentLocationWidget::lastOpenPath() const
{
    return d->m_lastOpenPath;
}

QString CurrentLocationWidget::lastSavePath() const
{
    return d->m_lastSavePath;
}

void CurrentLocationWidget::setTrackVisible( bool visible )
{
    d->m_currentLocationUi.showTrackCheckBox->setChecked( visible );
    d->m_widget->model()->positionTracking()->setTrackVisible( visible );
}

void CurrentLocationWidget::setLastOpenPath( const QString &path )
{
    d->m_lastOpenPath = path;
}

void CurrentLocationWidget::setLastSavePath( const QString &path )
{
    d->m_lastSavePath = path;
}

}

#include "CurrentLocationWidget.moc"
