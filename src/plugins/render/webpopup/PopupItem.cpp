//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2012   Torsten Rahn      <tackat@kde.org>
// Copyright 2012   Mohammed Nafees   <nafees.technocool@gmail.com>
// Copyright 2012   Dennis Nienhüser  <earthwings@gentoo.org>
// Copyright 2012   Illya Kovalevskyy <illya.kovalevskyy@gmail.com>
//

#include "PopupItem.h"
#include "MarbleWidget.h"

#include <QtWebKit/QWebView>
#include <QtGui/QMouseEvent>
#include <QtGui/QApplication>
#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <qdrawutil.h>

namespace Marble
{

PopupItem::PopupItem( QObject* parent ) :
    QObject( parent ),
    BillboardGraphicsItem(),
    m_widget( new QWidget ),
    m_webView( new QWebView ( m_widget ) ),
    m_textColor( QColor(Qt::black) ),
    m_backColor( QColor(Qt::white) ),
    m_needMouseRelease(false)
{
    setVisible( false );

    QGridLayout *childLayout = new QGridLayout;
    m_titleText = new QLabel( m_widget );
    childLayout->addWidget( m_titleText, 0, 0 );
    QPushButton *hideButton = new QPushButton( m_widget );
    hideButton->setIcon( QIcon( ":/marble/webpopup/icon-remove.png" ) );
    hideButton->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    hideButton->setCursor( QCursor( Qt::PointingHandCursor ) );
    hideButton->setFlat( true );
    childLayout->addWidget( hideButton, 0, 1 );
    QVBoxLayout *layout = new QVBoxLayout( m_widget );
    layout->addLayout( childLayout );
    layout->addWidget( m_webView );
    m_widget->setLayout( layout );
    m_widget->setAttribute( Qt::WA_NoSystemBackground, true );
    setSize( QSizeF( 300, 350 ) );
    QPalette palette = m_webView->palette();
    palette.setBrush(QPalette::Base, Qt::transparent);
    m_webView->setPalette(palette);
    m_webView->page()->setPalette(palette);
    m_webView->setAttribute(Qt::WA_OpaquePaintEvent, false);

    connect( m_webView, SIGNAL(titleChanged(QString)), m_titleText, SLOT(setText(QString)) );
    connect( hideButton, SIGNAL(clicked()), this, SIGNAL(hide()) );

    generateGraphics();
}

PopupItem::~PopupItem()
{
    delete m_widget;
}

void PopupItem::setUrl( const QUrl &url )
{
    if ( m_webView ) {
        m_webView->setUrl( url );
        setVisible( true );

	QPalette palette = m_webView->palette();
	palette.setBrush(QPalette::Base, Qt::transparent);
	m_webView->setPalette(palette);
	m_webView->page()->setPalette(palette);
	m_webView->setAttribute(Qt::WA_OpaquePaintEvent, false);

        emit dirty();
    }
}

void PopupItem::setContent( const QString &html )
{
    m_content = html;
    m_webView->setHtml(html);
}

void PopupItem::setTextColor(const QColor &color)
{
    if(color.isValid() && m_titleText != 0) {
        m_textColor = color;
        QPalette palette(m_titleText->palette());
        palette.setColor(QPalette::WindowText, m_textColor);
        m_titleText->setPalette(palette);
    }
}
void PopupItem::setBackgroundColor(const QColor &color)
{
    if(color.isValid()) {
        m_backColor = color;
        generateGraphics();
    }
}

void PopupItem::colorize(QImage &img, const QColor &col)
{
    if (img.depth() <= 8) return;
    int pixels = img.width()*img.height();
    unsigned int *data = (unsigned int *) img.bits();
    for (int i=0; i < pixels; ++i) {
        int val = qGray(data[i]);
        data[i] = qRgba(col.red()*val/255,col.green()*val/255, col.blue()*val/255, qAlpha(data[i]));
    }
}

void PopupItem::generateGraphics()
{
    m_cache.popup = merge( "webpopup2" );
    m_cache.topLeft = merge( "arrow2_topleft" );
    m_cache.bottomLeft = merge( "arrow2_bottomleft" );
    m_cache.topRight = merge( "arrow2_topright" );
    m_cache.bottomRight = merge( "arrow2_bottomright" );
}

QImage PopupItem::merge( const QString &imageId )
{
  QImage bottom = QImage( QString( ":/marble/webpopup/%1_shadow.png" ).arg( imageId) );
  QImage top = QImage( QString( ":/marble/webpopup/%1.png" ).arg( imageId) );
  colorize( top, m_backColor );
  QPainter painter( &bottom );
  painter.drawImage( QPoint(0,0), top );
  return bottom;
}

void PopupItem::paint( QPainter *painter )
{
    QRect popupRect( -10, -10, size().width(), size().height() );
    qDrawBorderPixmap(painter, popupRect, QMargins( 20, 20, 20, 20 ),
                      QPixmap::fromImage(m_cache.popup));

    if ( alignment() & Qt::AlignRight ) {
        if ( alignment() & Qt::AlignTop ) {
            painter->drawImage( - ( m_cache.topLeft.width() - 3 ), 0,
                                m_cache.topLeft );
        } else if ( alignment() & Qt::AlignBottom ) {
            painter->drawImage( - ( m_cache.bottomLeft.width() - 3 ),
                                size().height() - m_cache.bottomLeft.height(),
                                m_cache.bottomLeft );
        } else { // for no horizontal align value and Qt::AlignVCenter
            painter->drawImage( - ( m_cache.topLeft.width() - 3 ),
                                size().height() / 2 - m_cache.topLeft.height() / 2,
                                m_cache.topLeft );
        }
    } else if ( alignment() & Qt::AlignLeft ) {
        if ( alignment() & Qt::AlignTop ) {
            painter->drawImage( size().width() - 23, 0, m_cache.topRight );
        } else if ( alignment() & Qt::AlignBottom ) {
            painter->drawImage( size().width() - 23, size().height() - m_cache.bottomRight.height(), m_cache.bottomRight );
        } else { // for no horizontal align value and Qt::AlignVCenter
            painter->drawImage( size().width() - 23, size().height() / 2 - m_cache.topRight.height() / 2, m_cache.topRight );
        }
    }

    m_widget->setFixedSize( size().toSize() - QSize( 20, 20 ) );
    m_widget->render( painter, QPoint( 0, 0 ), QRegion() );
}

bool PopupItem::eventFilter( QObject *object, QEvent *e )
{
    MarbleWidget *widget = dynamic_cast<MarbleWidget*> ( object );
    if ( !widget ) {
        return BillboardGraphicsItem::eventFilter( object, e );
    }

    if ( e->type() == QEvent::MouseButtonDblClick
            || e->type() == QEvent::MouseMove
            || e->type() == QEvent::MouseButtonPress
            || e->type() == QEvent::MouseButtonRelease )
    {
        // Mouse events are forwarded to the underlying widget
        QMouseEvent *event = static_cast<QMouseEvent*> ( e );
        QPoint shiftedPos = event->pos();
        QWidget* child = transform( shiftedPos );
        bool const forcedMouseRelease = m_needMouseRelease && e->type() == QEvent::MouseButtonRelease;
        if ( child || forcedMouseRelease ) {
            if ( !m_needMouseRelease && e->type() == QEvent::MouseButtonPress ) {
                m_needMouseRelease = true;
            } else if ( forcedMouseRelease ) {
                m_needMouseRelease = false;
            }
            if ( !child ) {
                child = m_webView;
            }
            widget->setCursor( Qt::ArrowCursor );
            QMouseEvent shiftedEvent = QMouseEvent( e->type(), shiftedPos,
                                                    event->globalPos(), event->button(), event->buttons(),
                                                    event->modifiers() );
            if ( QApplication::sendEvent( child, &shiftedEvent ) ) {
                widget->setCursor( child->cursor() );
                emit dirty();
                return true;
            }
        }
    } else if ( e->type() == QEvent::Wheel ) {
        // Wheel events are forwarded to the underlying widget
        QWheelEvent *event = static_cast<QWheelEvent*> ( e );
        QPoint shiftedPos = event->pos();
        QWidget* child = transform( shiftedPos );
        if ( child ) {
            widget->setCursor( Qt::ArrowCursor );
            QWheelEvent shiftedEvent = QWheelEvent( shiftedPos,
                                                    event->globalPos(), event->delta(), event->buttons(),
                                                    event->modifiers() );
            if ( QApplication::sendEvent( child, &shiftedEvent ) ) {
                widget->setCursor( child->cursor() );
                emit dirty();
                return true;
            }
        }
    }

    return BillboardGraphicsItem::eventFilter( object, e );
}

QWidget* PopupItem::transform( QPoint &point ) const
{
    QList<QPointF> widgetPositions = positions();
    QList<QPointF>::const_iterator it = widgetPositions.constBegin();
    for( ; it != widgetPositions.constEnd(); ++it ) {
        if ( QRectF( *it, size() ).contains( point ) ) {
            point -= it->toPoint();
            QWidget* child = m_widget->childAt( point );
            if ( child ) {
                point -= child->pos();
            }
            return child;
        }
    }
    return 0;
}

}

#include "PopupItem.moc"