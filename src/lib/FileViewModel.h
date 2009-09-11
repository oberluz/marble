//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2007      Murad Tagirov <tmurad@gmail.com>
// Copyright 2009      Patrick Spendrin <ps_ml@gmx.de>
//


#ifndef FILEVIEWMODEL_H
#define FILEVIEWMODEL_H

#include <QtCore/QAbstractListModel>
#include <QtCore/QVariant>
#include <QtGui/QItemSelectionModel>

#include "marble_export.h"

#include "FileManager.h"

namespace Marble
{

class AbstractFileViewItem;

class MARBLE_EXPORT FileViewModel : public QAbstractListModel
{
    Q_OBJECT

  public:
    explicit FileViewModel( QObject* parent = 0 );
    ~FileViewModel();

    virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );

    void setFileManager( FileManager * fileManager );
    QItemSelectionModel * selectionModel();

  Q_SIGNALS:
    void modelChanged();

  public slots:
    void saveFile();
    void closeFile();
    void append( int item );
    void remove( int item );

  private:
    Q_DISABLE_COPY( FileViewModel )
    QItemSelectionModel *m_selectionModel;
    FileManager *m_manager;
    
};

}

#endif
