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

#include "connectivityinspectorwidget.h"
#include "ui_acquisitionwidget.h"
#include "ui_connectivityinspectorwidget.h"

#include "acquisitioncontroller.h"
#include "connectivityinspectorcommon.h"
#include "countdecoratorproxymodel.h"
#include "filtercontroller.h"

#include <common/objectbroker.h>
#include <ui/searchlinecontroller.h>

namespace {
QObject *createObjectVisualizerClient(const QString & /*name*/, QObject *parent)
{
    return new GammaRay::AcquisitionController(parent);
}
} // namespace

using namespace GammaRay;
using namespace GammaRay::CI;

Filter::Filter(FilterWidget *widget, const QString &name, QObject *parent)
{
    outputModel = ObjectBroker::model(filteredModelId(name));
    Q_ASSERT(outputModel);
    filterModel = ObjectBroker::model(filterModelId(name));
    Q_ASSERT(filterModel);
    filterInterface = new FilterController(name, parent);
    auto proxy = new CountDecoratorProxyModel(parent);
    proxy->setSourceModel(filterModel);
    widget->setup(filterInterface, proxy);
    filterWidget = widget;
}

ConnectivityInspectorWidget::ConnectivityInspectorWidget(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::ObjectVisualizerWidget)
    , m_stateManager(this)
{
    m_ui->setupUi(this);

    ObjectBroker::registerClientObjectFactoryCallback<AcquisitionInterface *>(
        createObjectVisualizerClient);
    m_interface = ObjectBroker::object<AcquisitionInterface *>();

    m_ui->acquisitionWidget->setAcquisitionInterface(m_interface);

    // clang-format off
    m_filters.insert(typeFilterName(),
                     Filter(m_ui->typeFilterWidget, typeFilterName(), this));
    m_filters.insert(classFilterName(),
                     Filter(m_ui->classFilterWidget, classFilterName(), this));
    m_filters.insert(objectFilterName(),
                     Filter(m_ui->objectFilterWidget, objectFilterName(), this));
    m_filters.insert(connectionFilterName(),
                     Filter(m_ui->connectionFilterWidget, connectionFilterName(), this));
    m_filters.insert(threadFilterName(),
                     Filter(m_ui->threadFilterWidget, threadFilterName(), this));
    // clang-format on

    //m_ui->gvWidget->setModel(m_connectionModel);
    //m_ui->vtkWidget->setModel(m_connectionModel);

    // connect(m_interface, &AcquisitionInterface::samplingDone, this, [this]() {
    //     m_ui->vtkWidget->updateGraph();
    // });
}

ConnectivityInspectorWidget::~ConnectivityInspectorWidget() = default;
