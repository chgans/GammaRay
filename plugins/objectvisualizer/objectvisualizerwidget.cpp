/*
  This file is part of GammaRay, the Qt application inspection and
  manipulation tool.

  Copyright (C) 2010-2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
  info@kdab.com
  Authors: Kevin Funk <kevin.funk@kdab.com>, Christian Gagneraud
  <chgans@gmail.com>

  Licensees holding valid commercial KDAB GammaRay licenses may use this file in
  accordance with GammaRay Commercial License Agreement provided with the
  Software.

  Contact info@kdab.com if any conditions of this licensing are not clear to
  you.

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

#include "objectvisualizerwidget.h"
#include "vtkcontainer.h"
#include "vtkpanel.h"
#include "vtkwidget.h"

#include <common/objectbroker.h>
#include <common/objectmodel.h>
#include <ui/clientdecorationidentityproxymodel.h>
#include <ui/deferredtreeview.h>
#include <ui/searchlinecontroller.h>

#include <QCoreApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QTableView>

using namespace GammaRay;

GraphViewerWidget::GraphViewerWidget(QWidget *parent)
    : QWidget(parent), m_stateManager(this),
      m_threeDeeWidget(new ThreeDeeWidget(this)) {

    m_model = ObjectBroker::model("com.kdab.GammaRay.ObjectVisualizerModel");

    DeferredTreeView *objectTreeView = new DeferredTreeView(this);
    objectTreeView->header()->setObjectName("objectTreeViewHeader");
    auto proxyModel = new ClientDecorationIdentityProxyModel(this);
    proxyModel->setSourceModel(m_model);
    auto objectSearchLine = new QLineEdit(this);
    new SearchLineController(objectSearchLine, proxyModel);
    objectTreeView->setModel(proxyModel);
    objectTreeView->setSortingEnabled(true);

    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(objectSearchLine);
    leftLayout->addWidget(objectTreeView);
    leftWidget->setLayout(leftLayout);

    auto tabWidget = new QTabWidget(this);
    tabWidget->addTab(new QWidget(this), "2D");
    tabWidget->addTab(m_threeDeeWidget, "3D");

    QSplitter *splitter = new QSplitter(this);
    splitter->addWidget(leftWidget);
    splitter->addWidget(tabWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(splitter);
}

GraphViewerWidget::~GraphViewerWidget()
{
}
