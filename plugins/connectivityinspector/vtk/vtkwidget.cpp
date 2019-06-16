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

#include "vtkwidget.h"
#include "connectionmodel.h"
#include "vtkgraphadapter.h"

#include "common/objectid.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QElapsedTimer>
#include <QHash>
#include <QMouseEvent>
#include <QTimer>

#include <vtkAnnotationLink.h>
#include <vtkArrowSource.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkDataRepresentation.h>
#include <vtkDataSetAttributes.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGlyph3D.h>
#include <vtkGlyphSource2D.h>
#include <vtkGraphLayout.h>
#include <vtkGraphLayoutView.h>
#include <vtkGraphToGlyphs.h>
#include <vtkGraphToPolyData.h>
#include <vtkInteractorStyleRubberBand2D.h>
#include <vtkInteractorStyleRubberBand3D.h>
#include <vtkInteractorStyleRubberBandZoom.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkPickingManager.h>
#include <vtkPolyDataMapper.h>
#include <vtkPropPicker.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderedGraphRepresentation.h>
#include <vtkRenderer.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkUnsignedLongLongArray.h>
#include <vtkViewUpdater.h>

// TODO: move to common, along with theme and stereo
#include <vtkAssignCoordinatesLayoutStrategy.h>
#include <vtkAttributeClustering2DLayoutStrategy.h>
#include <vtkCircularLayoutStrategy.h>
#include <vtkClustering2DLayoutStrategy.h>
#include <vtkCommunity2DLayoutStrategy.h>
#include <vtkConeLayoutStrategy.h>
#include <vtkConstrained2DLayoutStrategy.h>
#include <vtkCosmicTreeLayoutStrategy.h>
#include <vtkFast2DLayoutStrategy.h>
#include <vtkForceDirectedLayoutStrategy.h>
#include <vtkGraphLayoutView.h>
#include <vtkRandomLayoutStrategy.h>
#include <vtkRenderWindow.h>
#include <vtkSimple2DLayoutStrategy.h>
#include <vtkSimple3DCirclesStrategy.h>
#include <vtkSpanTreeLayoutStrategy.h>
#include <vtkTreeLayoutStrategy.h>
#include <vtkTreeOrbitLayoutStrategy.h>

#include <vtkViewTheme.h>

#include <iostream>
#include <limits>

// https://vtk.org/Wiki/VTK/Examples/Python/Graphs/SelectedVerticesAndEdges
// https://itk.org/Wiki/VTK/Examples/Python/Infovis/SelectedGraphIDs
// https://vtk.org/Wiki/VTK/Examples/Cxx/Graphs/SelectedVerticesAndEdges
// https://vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/Modelling/Python/SpherePuzzle.py
// https://vtk.org/Wiki/VTK/Examples/Cxx/Picking/CellPicking

using namespace GammaRay;

VtkWidget::VtkWidget(QWidget *parent)
    // : QVTKWidget2(new QGLContext(QGLFormat()), parent)
    : QVTKWidget(parent)
{
    m_graphAdapter = new vtkGraphAdapter(this);
    connect(m_graphAdapter, &vtkGraphAdapter::graphChanged, this, [this]() {
        m_inputHasChanged = true;
        updateGraph();
    });

    m_layout = vtkSmartPointer<vtkGraphLayout>::New();
    m_layoutStrategy->GlobalWarningDisplayOn();
    m_layout->SetInputDataObject(m_graphAdapter->graph());
    m_inputHasChanged = true;

    m_updater = vtkSmartPointer<vtkViewUpdater>::New();
    m_updater->GlobalWarningDisplayOn();
    m_layoutView = vtkSmartPointer<vtkGraphLayoutView>::New();
    m_layoutView->GlobalWarningDisplayOn();
    m_layoutView->AddRepresentationFromInputConnection(m_layout->GetOutputPort());
    //createArrowDecorator(m_layout->GetOutputPort());
    m_updater->AddView(m_layoutView);

    m_layoutView->SetVertexLabelArrayName("VertexLabel"); // FIXME: graph adapter
    m_layoutView->SetEdgeLabelArrayName("EdgeLabel");     // FIXME: graph adapter
    m_layoutView->SetEdgeColorArrayName("centrality");
    m_layoutView->SetVertexColorArrayName("VertexDegree"); // magic value

    m_layoutView->SetColorVertices(true);
    m_layoutView->SetVertexLabelVisibility(true);
    m_layoutView->SetGlyphType(vtkGraphToGlyphs::SPHERE);
    m_layoutView->SetColorEdges(true);
    m_layoutView->SetEdgeLabelVisibility(true);
    //m_layoutView->SetEdgeVisibility(false);
    m_layoutView->SetLayoutStrategyToPassThrough();
    m_layoutView->SetHideVertexLabelsOnInteraction(true);
    m_layoutView->SetHideEdgeLabelsOnInteraction(true);

    auto representation = m_layoutView->GetRepresentation(0);
    representation->SetSelectionType(vtkSelectionNode::PEDIGREEIDS);
    m_annotationLink = vtkSmartPointer<vtkAnnotationLink>::New();
    m_annotationLink->GlobalWarningDisplayOn();
    representation->SetAnnotationLink(m_annotationLink);
    m_graphAdapter->setAnnotationLink(m_annotationLink);
    m_updater->AddAnnotationLink(m_annotationLink);

    // 2D/3D: m_layoutView->SetInteractionModeTo[2,3]D();
    // We wight still want to switch to box zoom
    m_interactorZoomStyle = vtkSmartPointer<vtkInteractorStyleRubberBandZoom>::New();

    m_layoutView->SetInteractor(GetInteractor());
    GetRenderWindow()->AddRenderer(m_layoutView->GetRenderer());
    //SetRenderWindow(m_layoutView->GetRenderWindow());

    //    auto timer = new QTimer(this);
    //    timer->start(1000);
    //    connect(timer, &QTimer::timeout, this, [this]() {
    //        m_inputHasChanged = true;
    //        m_graphAdapter->recompute();
    //        updateGraph();
    //    });
}

VtkWidget::~VtkWidget() {}

vtkGraphAdapter *VtkWidget::graphAdapter()
{
    return m_graphAdapter;
}

void VtkWidget::updateGraph()
{
    updateSatus("Updating graph");
    if (!(m_inputHasChanged || m_configHasChanged))
        return;

    if (m_is3dLayout) {
        m_layoutView->SetInteractionModeTo3D();
    } else {
        m_layoutView->SetInteractionModeTo2D();
    }
    m_layout->Update();
    m_layoutView->Render(); // can get The BB from renderer
    m_layoutView->ResetCamera();

    updateSatus("Done");
    m_inputHasChanged = false;
    m_configHasChanged = false;
}

// FIXME: Arrow size need to scale with graph representation size.
void VtkWidget::createArrowDecorator(vtkAlgorithmOutput *input)
{
    vtkSmartPointer<vtkGraphToPolyData> graphToPoly = vtkSmartPointer<vtkGraphToPolyData>::New();
    graphToPoly->SetInputConnection(input);
    graphToPoly->EdgeGlyphOutputOn();
    graphToPoly->SetEdgeGlyphPosition(0.6);
    auto arrowSource = vtkSmartPointer<vtkArrowSource>::New();
    // arrowSource->SetTipLength();
    // arrowSource->SetTipRadius();
    // arrowSource->SetTipResolution();
    // arrowSource->SetShaftRadius(1.0);
    // arrowSource->SetShaftResolution();
    arrowSource->Update();
    auto arrowGlyph = vtkSmartPointer<vtkGlyph3D>::New();
    arrowGlyph->SetInputConnection(0, graphToPoly->GetOutputPort(1));
    arrowGlyph->SetInputConnection(1, arrowSource->GetOutputPort());
    //arrowGlyph->SetScaling(1);
    auto arrowMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    arrowMapper->SetInputConnection(arrowGlyph->GetOutputPort());    
    m_arrowDecorator = vtkSmartPointer<vtkActor>::New();
    m_arrowDecorator->SetMapper(arrowMapper);
}

void VtkWidget::updateSatus(const QString &state)
{
    const auto status = QStringLiteral("State: %1, Render: %4 ms").arg(state).arg(m_renderDuration);
    emit statusChanged(status);
}

void VtkWidget::setLayoutStrategy(Vtk::LayoutStrategy strategy)
{
    switch (strategy) {
    case Vtk::LayoutStrategy::Cone: {
        auto *strategy = vtkConeLayoutStrategy::New();
        // strategy->SetSpacing(1.0);
        // strategy->SetCompactness(1.0);
        // strategy->SetCompression(1);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::Tree: {
        auto strategy = vtkTreeLayoutStrategy::New();
        // strategy->SetRotation(0.0);
        // strategy->SetLeafSpacing(1.0);
        // strategy->SetLogSpacingValue(1.0);
        // strategy->SetDistanceArrayName("distance");
        strategy->SetRadial(true);
        strategy->SetAngle(360);
        strategy->SetLogSpacingValue(1);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::Fast2D: {
        auto *strategy = vtkFast2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Random: {
        auto *strategy = vtkRandomLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetGraphBounds(...);
        // strategy->SetThreeDimensionalLayout(1);
        // strategy->SetAutomaticBoundsComputation(1);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false; // FIXME: can choose
        break;
    }
    case Vtk::LayoutStrategy::Circular: {
        auto *strategy = vtkCircularLayoutStrategy::New();
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Simple2D: {
        auto *strategy = vtkSimple2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetJitter(true);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::SpanTree: {
        auto *strategy = vtkSpanTreeLayoutStrategy::New();
        // strategy->SetDepthFirstSpanningTree(true);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::Community2D: {
        auto *strategy = vtkCommunity2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        // strategy->SetCommunityStrength(1.0);
        strategy->SetCommunityArrayName("VertexClusterId");
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Clustering2D: {
        auto *strategy = vtkClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance(1.0);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected2D: {
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRandomInitialPoints(1);
        // strategy->SetGraphBounds(...);
        // strategy->SetThreeDimensionalLayout(1);
        // strategy->SetAutomaticBoundsComputation(1);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::ForceDirected3D: {
        // same as above
        auto *strategy = vtkForceDirectedLayoutStrategy::New();
        strategy->SetThreeDimensionalLayout(true);
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::AssignCoordinates: {
        auto *strategy = vtkAssignCoordinatesLayoutStrategy::New();
        // strategy->SetXCoordArrayName();
        // strategy->SetYCoordArrayName();
        // strategy->SetZCoordArrayName();
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    case Vtk::LayoutStrategy::AttributeClustering2D: {
        auto *strategy = vtkAttributeClustering2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        // strategy->SetRestDistance();
        strategy->SetVertexAttribute("VertexClusterId"); // FIXME
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;
        break;
    }
    case Vtk::LayoutStrategy::Constrained2D: {
        auto *strategy = vtkConstrained2DLayoutStrategy::New();
        strategy->SetRandomSeed(0);
        //strategy->SetInputArrayName(s_connWeightArrayName); // FIXME [0:1]
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = false;

        break;
    }
    case Vtk::LayoutStrategy::Simple3DCircles: {
        auto *strategy = vtkSimple3DCirclesStrategy::New();
        // strategy->SetHeight();
        // strategy->SetMethod();
        // strategy->SetOrigin();
        // strategy->SetRadius();
        // strategy->SetDirection();
        // strategy->SetAutoHeight();
        // strategy->SetMarkedValue();
        // strategy->SetMinimumDegree();
        // strategy->SetMinimumRadian();
        // strategy->SetHierarchicalOrder();
        // strategy->SetHierarchicalLayers();
        // strategy->SetMarkedStartVertices();
        // strategy->SetForceToUseUniversalStartPointsFinder();
        m_layoutStrategy.TakeReference(strategy);
        m_is3dLayout = true;
        break;
    }
    }

    m_layoutStrategy->SetWeightEdges(true);
    m_layoutStrategy->SetEdgeWeightField("EdgeWeight");
    m_layoutStrategy->DebugOn();
    m_layoutStrategy->GlobalWarningDisplayOn();
    m_layout->SetLayoutStrategy(m_layoutStrategy);
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setStereoMode(Vtk::StereoMode mode)
{
    GetRenderWindow()->SetStereoType(static_cast<int>(mode));
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setThemeType(Vtk::ThemeType themeType)
{
    vtkViewTheme *viewTheme = nullptr;
    switch (themeType) {
    case Vtk::ThemeType::Neon:
        viewTheme = vtkViewTheme::CreateNeonTheme();
        break;
    case Vtk::ThemeType::Ocean:
        viewTheme = vtkViewTheme::CreateOceanTheme();
        break;
    case Vtk::ThemeType::Mellow:
        viewTheme = vtkViewTheme::CreateMellowTheme();
        break;
    }
    Q_ASSERT(viewTheme != nullptr);
    viewTheme->SetLineWidth(0.5);
    viewTheme->SetPointSize(15);
    // viewTheme->SetOutlineColor();
    // viewTheme->SetPointOpacity(1.0);
    viewTheme->SetCellOpacity(0.3);
    // viewTheme->SetSelectedPointOpacity(0.3);
    // viewTheme->SetSelectedPointColor(0.0, 0.5, 1.0);
    // viewTheme->SetSelectedCellOpacity();
    // viewTheme->SetSelectedCellColor();

    m_layoutView->ApplyViewTheme(viewTheme);
    viewTheme->Delete();
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setShowNodeLabel(bool show)
{
    if (m_layoutView->GetVertexLabelVisibility() == show)
        return;
    m_layoutView->SetVertexLabelVisibility(show);
    m_configHasChanged = true;
    updateGraph();
}

void VtkWidget::setShowEdgeLabel(bool show)
{
    if (m_layoutView->GetEdgeLabelVisibility() == show)
        return;
    m_layoutView->SetEdgeLabelVisibility(show);
    m_configHasChanged = true;
    updateGraph();
}

/* See:
 * https://vtk.org/Wiki/VTK/Examples/Cxx/Graphs/VisualizeDirectedGraph
 * https://lorensen.github.io/VTKExamples/site/Cxx/GeometricObjects/Arrow/
 */
void VtkWidget::setShowEdgeArrow(bool show)
{
    if (m_showEdgeArrow == show)
        return;
    m_showEdgeArrow = show;
    if (m_showEdgeArrow) {
        m_layoutView->GetRenderer()->AddActor(m_arrowDecorator);
        m_layoutView->SetEdgeLayoutStrategyToPassThrough();
        // m_layoutView->SetScaledGlyphs(true); // only applies to nodes
    } else {
        m_layoutView->GetRenderer()->RemoveActor(m_arrowDecorator);
        m_layoutView->SetEdgeLayoutStrategyToArcParallel();
        // m_layoutView->SetScaledGlyphs(false);  // only applies to nodes
    }
    m_configHasChanged = true;
    updateGraph();
}
