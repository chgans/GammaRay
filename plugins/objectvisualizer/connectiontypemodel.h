/*
  connectiontypemodel.h

  This file is part of QGraphViz, a Qt wrapper around GraphViz libraries.

  Copyright (C) 2019

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONNECTIONTYPEMODEL_H
#define CONNECTIONTYPEMODEL_H

#include <QAbstractTableModel>
#include <qnamespace.h>

namespace GammaRay {

class ConnectionTypeModel : public QAbstractTableModel
{
     Q_OBJECT

public:
    enum { TypeColumn = 0, CountColumn, IsRecordingColumn, IsVisibleColumn, ColumnCount };
    static const int RowCount;

    explicit ConnectionTypeModel(QObject *parent = nullptr);
    ~ConnectionTypeModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

public slots:
    void increaseCount(int type);
    void decreaseCount(int type);
    void resetCounts();

    bool isRecording(Qt::ConnectionType type) const;
    void recordAll();
    void recordNone();

    bool isVisible(Qt::ConnectionType type) const;
    void showAll();
    void showNone();

signals:
    void recordingChanged();

private:
    struct Data
    {
        quint64 count : 54;
        quint64 type : 8; // 0->3 + 0x80 flag
        quint64 isRecording : 1;
        quint64 isVisible : 1;
    };
    QVector<Data> m_data;
    static const QVector<Data> s_initialData;
};

} // namespace GammaRay

#endif // CONNECTIONTYPEMODEL_H
