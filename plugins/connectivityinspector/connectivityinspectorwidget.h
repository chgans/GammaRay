/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Christian Gagneraud <chgans@gmail.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to you.

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

#ifndef GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORWIDGET_H
#define GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORWIDGET_H

#include <ui/tooluifactory.h>
#include <ui/uistatemanager.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QModelIndex;
QT_END_NAMESPACE

namespace GammaRay {
namespace Ui {
    class ObjectVisualizerWidget;
}
class DeferredTreeView;
class AcquisitionInterface;
class FilterInterface;
class FilterWidget;

class ConnectivityInspectorWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConnectivityInspectorWidget(QWidget *parent = nullptr);
    ~ConnectivityInspectorWidget();

private:
    void setupClient();
    void setupModels();
    void setupUi();
    void setupConnectionView();
    void setupFilterWidget(FilterWidget *widget, FilterInterface *interface,
                           QAbstractItemModel *model);

    QScopedPointer<Ui::ObjectVisualizerWidget> m_ui;
    AcquisitionInterface *m_interface;
    UIStateManager m_stateManager;
    QAbstractItemModel *m_connectionModel;

    QAbstractItemModel *m_connectionFilterModel;
    FilterInterface *m_connectionFilterInterface;
    QAbstractItemModel *m_threadRecordingModel;
    FilterInterface *m_threadFilterInterface;
    QAbstractItemModel *m_classRecordingModel;
    FilterInterface *m_classFilterInterface;
    QAbstractItemModel *m_objectRecordingModel;
    FilterInterface *m_objectFilterInterface;

    // QObject interface
public:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

class ConnectivityInspectorUiFactory
    : public QObject,
      public StandardToolUiFactory<ConnectivityInspectorWidget> {
    Q_OBJECT
    Q_INTERFACES(GammaRay::ToolUiFactory)
    Q_PLUGIN_METADATA(IID "com.kdab.GammaRay.ToolUiFactory" FILE
                          "gammaray_connectivityinspector.json")
};

} // namespace GammaRay
#endif // GAMMARAY_CONNECTIVITYINSPECTOR_CONNECTIVITYINSECTORWIDGET_H
