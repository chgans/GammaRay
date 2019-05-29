/*
  objectvisualizerwidget.h

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

#ifndef GAMMARAY_OBJECTVISUALIZER_OBJECTVISUALIZERWIDGET_H
#define GAMMARAY_OBJECTVISUALIZER_OBJECTVISUALIZERWIDGET_H

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
class ObjectVisualizerInterface;

class ObjectVisualizerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectVisualizerWidget(QWidget *parent = nullptr);
    ~ObjectVisualizerWidget();

private:
    void setupClient();
    void setupModels();
    void setupUi();
    void setupConnectionTypeView();
    void setupConnectionView();
    void setupThreadView();
    void setupClassView();
    void setupObjectView();
    void setup2dView();
    void setup3dView();

    QScopedPointer<Ui::ObjectVisualizerWidget> m_ui;
    ObjectVisualizerInterface *m_interface;
    UIStateManager m_stateManager;
    QAbstractItemModel *m_connectionModel;
    QAbstractItemModel *m_connectionTypeModel;
    QAbstractItemModel *m_threadModel;
    QAbstractItemModel *m_classModel;
    QAbstractItemModel *m_objectModel;
    DeferredTreeView *m_objectTreeView;
};

class ObjectVisualizerUiFactory : public QObject,
                                  public StandardToolUiFactory<ObjectVisualizerWidget>
{
    Q_OBJECT
    Q_INTERFACES(GammaRay::ToolUiFactory)
    Q_PLUGIN_METADATA(IID "com.kdab.GammaRay.ToolUiFactory" FILE "gammaray_objectvisualizer.json")
};

} // namespace GammaRay
#endif // GAMMARAY_OBJECTVISUALIZER_OBJECTVISUALIZERWIDGET_H
