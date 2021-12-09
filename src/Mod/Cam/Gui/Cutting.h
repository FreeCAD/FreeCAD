/***************************************************************************
 *   Copyright (c) 2007 Joachim Zettler <Joachim.Zettler@gmx.de>           *
 *   Copyright (c) 2007 Werner Mayer <wmayer@users.sourceforge.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef CAMGUI_CUTTING_H
#define CAMGUI_CUTTING_H

#include "ui_Cutting.h"

#include <Base/Vector3D.h>
#include <QProcess>
#include <Mod/Mesh/App/Core/Grid.h>
#include <Mod/Cam/App/cutting_tools.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Mod/Cam/App/mergedata.h>
#include <zlib.h>

class best_fit;
class cutting_tools;
class path_simulate;
class SpringbackCorrection;
class UniGridApprox;
class TopoDS_Shape;
class Approximate;
class Deviation;
class MergeData;

namespace CamGui
{

class Cutting : public QDialog, public Ui_Cutting
{
    Q_OBJECT

public:
    Cutting(QWidget* parent,Qt::WFlags = 0);
    ~Cutting();
protected Q_SLOTS:
    void on_CalculateZLevel_clicked();
    void on_CalculateFeatureBased_clicked();
    void on_CalculateSpiralBased_clicked();
    void on_select_shape_z_level_button_clicked();
    void on_select_shape_feature_based_button_clicked();
    void on_select_shape_spiral_based_button_clicked();
    void on_toolpath_calculation_highest_level_button_clicked();
    void on_toolpath_calculation_middle_level_button_clicked();
    void on_toolpath_calculation_lowest_level_button_clicked();
    void on_toolpath_calculation_go_button_clicked();
    void on_GenSimOut_clicked();
    void on_GenRobotOut_clicked();
    void on_adaptdynainput_clicked();
    void on_start_simulation_clicked();
    void on_BestFitButton_clicked();
    void on_SpringbackButton_clicked();
    void on_Approximate_button_clicked();
    void on_best_fit_cad_button_clicked();
    void on_best_fit_mesh_button_clicked();
	void on_best_fit_mesh2_button_clicked();
    void on_SelectFace_button_clicked();
    void on_best_fit_go_button_clicked();
	void on_Deviation_button_clicked();
	void on_deviation_geometry1_button_clicked();
	void on_deviation_geometry2_button_clicked();
	void on_deviation_go_button_clicked();
	void on_error_accumulation_go_button_clicked();
	void on_error_accumulation_select_files_button_clicked();


    void selectShape();
    void selectMesh();
    bool getProcessOutput();
    const CuttingToolsSettings& getSettings();
    void setFace(const TopoDS_Shape &aFace, const float , const float,const float);

private:
    static void zLevelCallback(void * ud, SoEventCallback * n);
    void DisplayCAMOutput();
    void DisplayMeshOutput(const MeshCore::MeshKernel &mesh);
    void DisplayShapeOutput();


private:

    SpringbackCorrection *m_Spring;
    cutting_tools        *m_CuttingAlgo;  //Instanz von der cutting-klasse auf dem Heap erzeugen
    path_simulate        *m_PathSimulate;
    best_fit             *m_BestFit;
    UniGridApprox        *m_Approx;
	Approximate          *m_App;
	Deviation	         *m_Deviation;
	MergeData			 *m_MergeData;

    CuttingToolsSettings m_Settings;

    QProcess *m_Process;
    TopoDS_Shape m_Shape;
    MeshCore::MeshKernel m_Mesh;
    MeshCore::MeshKernel m_MeshOut;
    MeshCore::MeshKernel m_MeshCad;
    bool m_timer;
    //1 means Standard, 2 means Feature Based, 3 means Spiral Based
    unsigned int m_Mode;

    enum support_selection
    {
        BestFit,
        Springback,
        Approx,
        ToolpathCalculation
    };
    support_selection m_selection;


};

}

#endif
