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

#include "PreCompiled.h"

#include "Cutting.h"
#include <Mod/Cam/App/ChangeDyna.h> //Only for Testing
#include <Mod/Cam/App/best_fit.h>
#include <Mod/Cam/App/path_simulate.h>
#include <Mod/Cam/App/SpringbackCorrection.h>
#include <Mod/Cam/App/UniGridApprox.h>
#include <Mod/Cam/App/Approx.h>
#include <Mod/Cam/App/deviation.h>
#include <QTimer>
#include <QByteArray>


#include <Base/Vector3D.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include <Gui/ViewProvider.h>
#include <Gui/Selection.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <App/Document.h>
#include <Gui/Document.h>

#include <TopExp_Explorer.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <Mod/Cam/App/cutting_tools.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Mesh/App/MeshFeature.h>
#include <Mod/Part/Gui/ViewProvider.h>

#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GCPnts_QuasiUniformAbscissa.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <Geom2d_Curve.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom_Surface.hxx>
#include <Mod/Mesh/App/Core/Grid.h>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/SoPickedPoint.h>

#include <QMessageBox>
#include <QFileDialog>

using namespace CamGui;

Cutting::Cutting(QWidget* parent,Qt::WFlags fl)
        :QDialog(parent,fl),m_Process(NULL),
		m_PathSimulate(NULL),m_CuttingAlgo(NULL),
		m_BestFit(NULL),m_MergeData(NULL),m_Deviation(NULL)
{
    this->setupUi(this);
    m_timer= false;
}

Cutting::~Cutting()
{
    delete m_CuttingAlgo;
    delete m_PathSimulate;
    delete m_Process;
    delete m_BestFit;
	delete m_Deviation;
	delete m_MergeData;
}

bool Cutting::getProcessOutput()
{
    QByteArray result = m_Process->readAll();
    if (result.contains("Error"))
    {
        m_Process->kill();
        QMessageBox::critical(this, tr("FreeCAD CamWorkbench"),
                              tr("Fehler bei der Erzeugung\n"),
                              QMessageBox::Ok, QMessageBox::NoButton);
    }
    else if (result.contains("N o r m a l    t e r m i n a t i o n"))
    {
        QMessageBox::information(this, tr("FreeCAD CamWorkbench"),
                                 tr("Dyna-Job finished well\n"),
                                 QMessageBox::Ok, QMessageBox::NoButton);
    }

    return true;
}
void Cutting::on_adaptdynainput_clicked()
{
    //First we have to select the LS-Dyna Masterfile and the current working dir
    QString filename, path, program;
    QStringList arguments;
    QString strcheck("dyna.str");
    filename = QFileDialog::getOpenFileName( this, "Open Dyna.Str or Master-Key File",filename,"Ls-Dyna Keywords (*.k) (*.str)" );
    if (filename.isNull())
        return;
    QFileInfo aFileInfo(filename);
    path = aFileInfo.absolutePath();
    QDir::setCurrent(path);
    program = "c:/Program Files/lsdyna/ls971d";
    //As the new Versions already account for a proper curve file we no longer have to adapt the input
    //if we already have a str File we will not step into the next if-case
    //if(!aFileInfo.fileName().contains("dyna.str"))
    //{
    //    arguments << " i="<< aFileInfo.fileName();
    //    m_Process = new QProcess(this);
    //    m_Process->start(program, arguments);
    //    //Now we check if the output is written correctly
    //    m_Process->waitForFinished(50000);
    //    aFileInfo.setFile("dyna.str");
    //    if (aFileInfo.size() == 0) //the file does not exist
    //    {
    //        QMessageBox::critical(this, tr("FreeCAD CamWorkbench"),
    //                          tr("Fehler bei der Erzeugung vom Struct File\n"),
    //                          QMessageBox::Ok, QMessageBox::NoButton);
    //        return;
    //    }
    //    else
    //    {
    //        QMessageBox::information(this, tr("FreeCAD CamWorkbench"),
    //                             tr("Structured-Dyna gut erzeugt\n"),
    //                             QMessageBox::Ok, QMessageBox::NoButton);
    //    }
    //}
    //ChangeDyna aFileChanger;
    //if (aFileChanger.Read("dyna.str"))
    // start_simulation->show();
    //else{
    //        QMessageBox::critical(this, tr("FreeCAD CamWorkbench"),
    //                          tr("Error while parsing the str File\n"),
    //                          QMessageBox::Ok, QMessageBox::NoButton);
    //        return;
    //}

}

void Cutting::on_start_simulation_clicked()
{
    //check if the initial process is already killed
    m_Process->kill();
    QString program;
    QStringList arguments;
    program = "c:/Program Files/lsdyna/ls971d";
    arguments << " i=dyna2.str";
    connect(m_Process,SIGNAL(readyReadStandardError()),this,SLOT(getProcessOutput()));
    connect(m_Process,SIGNAL(readyReadStandardOutput()),this,SLOT(getProcessOutput()));
    m_Process->start(program, arguments);
}

void Cutting::on_Deviation_button_clicked()
{
	m_Deviation = new Deviation();
	deviation_geometry1_button->setEnabled(true);
	deviation_geometry2_button->setEnabled(true);
	deviation_go_button->setEnabled(true);
}

void Cutting::on_deviation_geometry1_button_clicked()
{
	selectShape();
}

void Cutting::on_deviation_geometry2_button_clicked()
{
	selectMesh();
}

void Cutting::on_deviation_go_button_clicked()
{
	QString current_filename = QFileDialog::getSaveFileName(this,"Select Deviation Files","","*.txt");
	m_Deviation->ImportGeometry(m_Shape, m_Mesh);
	m_Deviation->Compute();
	
	
	m_Deviation->WriteOutput(current_filename);
	
}

void Cutting::on_error_accumulation_select_files_button_clicked()
{
	m_MergeData = new MergeData();

	QStringList m_dateinamen = QFileDialog::getOpenFileNames(
		this,
		"Select one or more files to open",
		"c:",
		"Deviation Files (*.txt)");
	
	if (!m_dateinamen.isEmpty())
	{
		if (!m_MergeData->Einlesen(m_dateinamen))
		{
			QMessageBox::information(this, tr("FreeCAD CamWorkbench"),
				tr("Alles i.O. Output kann erzeugt werden\n"),
				QMessageBox::Ok, QMessageBox::NoButton);
		}
	
		
	}
	error_accumulation_go_button->setEnabled(true);
	
}

void Cutting::on_error_accumulation_go_button_clicked()
{
		QString current_filename = QFileDialog::getSaveFileName(this,"Select Output File","","*.txt");

	m_MergeData->WriteOutput(current_filename);

}

void Cutting::selectShape()
{
    if (!m_timer)
    {

        int check_box1=0,check_box2=0;
        if (!m_Shape.IsNull())
        {
            check_box1 = QMessageBox::question(this, tr("FreeCAD CamWorkbench"),
                                               tr("You have already selected a CAD-Shape.\n"
                                                  "Do you want to make a new Selection?"),
                                               QMessageBox::Yes, QMessageBox::No);
        }
        else
        {
            check_box2 = QMessageBox::information(this, tr("FreeCAD CamWorkbench"),
                                                  tr("You have to select a CAD-Shape.\n"),
                                                  QMessageBox::Ok, QMessageBox::Cancel);
        }
        if ((check_box1 == QMessageBox::Yes) || (check_box2 == QMessageBox::Ok))
        {
            //First, remove the old selection from the Gui, so that we do not directly have the same CAD once again.
            Gui::Selection().clearCompleteSelection();
            //to make a Selection more easy, hide the dialog
            this->hide();
            QTimer::singleShot(100,this,SLOT(selectShape()));
            m_timer = true;
        }
    }
    else
    {
        std::vector<App::DocumentObject*> fea = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
        if ( fea.size() == 1)
        {
            m_Shape = static_cast<Part::Feature*>(fea.front())->Shape.getValue();
            //std::vector<Gui::SelectionSingleton::SelObj> aSelection = Gui::Selection().getSelection();
            this->show();
            CalculateZLevel->setEnabled(true);
            CalculateFeatureBased->setEnabled(true);
            CalculateSpiralBased->setEnabled(true);
            m_timer = false;
        }
        else
        {
            QTimer::singleShot(100,this,SLOT(selectShape()));
            m_timer = true;
        }
    }
}

void Cutting::selectMesh()
{
    if (!m_timer)
    {
        int check_box1=0,check_box2=0;
        if (m_Mesh.CountPoints() > 0)
        {
            check_box1 = QMessageBox::question(this, tr("FreeCAD CamWorkbench"),
                                               tr("You have already selected a Mesh.\n"
                                                  "Do you want to make a new Selection?"),
                                               QMessageBox::Yes, QMessageBox::No);
        }
        else
        {
            check_box2 = QMessageBox::information(this, tr("FreeCAD CamWorkbench"),
                                                  tr("You have to select a Mesh.\n"),
                                                  QMessageBox::Ok, QMessageBox::Cancel);
        }
        if ((check_box1 == QMessageBox::Yes) || (check_box2 == QMessageBox::Ok))
        {
            //First, remove the old selection from the Gui, so that we do not directly have the same CAD once again.
            Gui::Selection().clearCompleteSelection();
            //to make a Selection more easy, hide the dialog
            this->hide();
            QTimer::singleShot(100,this,SLOT(selectMesh()));
            m_timer = true;
        }
    }
    else
    {
        std::vector<App::DocumentObject*> fea = Gui::Selection().getObjectsOfType(Mesh::Feature::getClassTypeId());
        if ( fea.size() == 1)
        {
            m_Mesh = static_cast<Mesh::Feature*>(fea.front())->Mesh.getValue().getKernel();
            //std::vector<Gui::SelectionSingleton::SelObj> aSelection = Gui::Selection().getSelection();
            this->show();
            m_timer = false;
        }
        else
        {
            QTimer::singleShot(100,this,SLOT(selectMesh()));
            m_timer = true;
        }
    }
}


void Cutting::setFace(const TopoDS_Shape& aShape, const float x, const float y, const float z)
{
    //check if a Shape is selected
    std::vector<App::DocumentObject*> fea = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    if ( fea.size() == 1)
    {
        int test = aShape.ShapeType();
        //get Hash Code of Selected Face inside the selected Shape and also the Coordinates of the click
        if (aShape.ShapeType() != TopAbs_FACE)
        {
            QMessageBox::information(this, tr("FreeCAD CamWorkbench"), tr("You have to select a Face!!\n"));
            return;
        }

        TopoDS_Face tempFace = TopoDS::Face(aShape);
        //Now search for the Hash-Code in the m_Shape
        TopExp_Explorer anExplorer;
        TopoDS_Face aselectedFace;

        //pickPoint.Set(x,y,z);
        for (anExplorer.Init(m_Shape,TopAbs_FACE);anExplorer.More();anExplorer.Next())
        {
            if (tempFace.HashCode(IntegerLast()) == anExplorer.Current().HashCode(IntegerLast()))
            {
                if (m_selection == Springback)
                    (m_Spring->m_FixFaces).push_back(TopoDS::Face(anExplorer.Current()));
                else if (m_selection == BestFit)
                    (m_BestFit->m_LowFaces).push_back(TopoDS::Face(anExplorer.Current()));
                else if (m_selection == ToolpathCalculation)
                    m_CuttingAlgo->SetMachiningOrder(TopoDS::Face(anExplorer.Current()),x,y,z);
                break;
            }
        }
    }
}

void Cutting::on_CalculateZLevel_clicked()
{
    //Cutting-Klasse instanzieren
    if (m_CuttingAlgo == NULL)
        m_CuttingAlgo = new cutting_tools(m_Shape);
    else
    {
        delete m_CuttingAlgo;
        m_CuttingAlgo = new cutting_tools(m_Shape);
    }
    m_Mode = 1;
    CalculateFeatureBased->setEnabled(false);
    CalculateSpiralBased->setEnabled(false);
    toolpath_calculation_highest_level_button->setEnabled(true);
    m_selection = ToolpathCalculation;
}

void Cutting::on_CalculateFeatureBased_clicked()
{
    if (m_CuttingAlgo == NULL)
        m_CuttingAlgo = new cutting_tools(m_Shape);
    else
    {
        delete m_CuttingAlgo;
        m_CuttingAlgo = new cutting_tools(m_Shape);
    }
    m_Mode = 2;
    toolpath_calculation_highest_level_button->setEnabled(true);
    m_selection = ToolpathCalculation;
    CalculateZLevel->setEnabled(false);
    CalculateSpiralBased->setEnabled(false);
}

void Cutting::on_CalculateSpiralBased_clicked()
{
    if (m_CuttingAlgo == NULL)
        m_CuttingAlgo = new cutting_tools(m_Shape);
    else
    {
        delete m_CuttingAlgo;
        m_CuttingAlgo = new cutting_tools(m_Shape);
    }
    m_Mode = 3;//
    toolpath_calculation_highest_level_button->setEnabled(true);
    m_selection = ToolpathCalculation;
    CalculateZLevel->setEnabled(false);
    CalculateFeatureBased->setEnabled(false);

}

void Cutting::on_select_shape_z_level_button_clicked()
{
    selectShape();
}

void Cutting::on_select_shape_feature_based_button_clicked()
{
    selectShape();
}

void Cutting::on_select_shape_spiral_based_button_clicked()
{
    selectShape();
}

void Cutting::on_toolpath_calculation_highest_level_button_clicked()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view)
    {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, this);
        QMessageBox::information(this, tr("FreeCAD CamWorkbench"), tr("You have to pick a point.\n"));
        this->hide();
    }
    toolpath_calculation_middle_level_button->setEnabled(true);
    toolpath_calculation_lowest_level_button->setEnabled(true);
}

void Cutting::on_toolpath_calculation_middle_level_button_clicked()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view)
    {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, this);
        QMessageBox::information(this, tr("FreeCAD CamWorkbench"), tr("You have to pick a point.\n"));
        this->hide();
    }
}

void Cutting::on_toolpath_calculation_lowest_level_button_clicked()
{
    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view)
    {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, this);
        QMessageBox::information(this, tr("FreeCAD CamWorkbench"), tr("You have to pick a point.\n"));
        this->hide();
    }
    toolpath_calculation_go_button->setEnabled(true);
}

void Cutting::on_toolpath_calculation_go_button_clicked()
{
    //Do the actual Cut
    //First transfer the settings to the Cutting_tools class
    m_CuttingAlgo->m_UserSettings = getSettings();

    if (!m_CuttingAlgo->arrangecuts_ZLEVEL())
    {
        std::cout << "Konnte nicht sauber schneiden" << std::endl;
    }

    bool ok = true;
    try
    {
        switch (m_Mode)
        {
        case 1:
            ok = m_CuttingAlgo->OffsetWires_Standard();
            break;
        case 2:
            ok = m_CuttingAlgo->OffsetWires_FeatureBased();
            break;
        case 3:
            ok = m_CuttingAlgo->OffsetWires_Spiral();
            break;
        }
    }
    catch (...)
    {
        std::cout<<"Fehler"<<std::endl;
    }
    if (!ok)
    {
        QMessageBox::critical(this, tr("FreeCAD CamWorkbench"),
                              tr("Irgendwas stimmt nicht. Nochmal alles neu versuchen\n"),
                              QMessageBox::Ok, QMessageBox::NoButton);
        delete m_CuttingAlgo;
        m_CuttingAlgo = new cutting_tools(m_Shape);
        return;
    }
    DisplayCAMOutput();
    GenRobotOut->setEnabled(true);
    GenSimOut->setEnabled(true);
}

void Cutting::on_GenSimOut_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Simulation-Path Output-Directory"),"d:/",
                  QFileDialog::ShowDirsOnly
                  | QFileDialog::DontResolveSymlinks);
    if (dir.isNull())
        return;
    QDir::setCurrent(dir);
    if (m_PathSimulate != NULL) delete m_PathSimulate;//If it exists already
    m_PathSimulate = new path_simulate(*(m_CuttingAlgo->getOutputhigh()),*(m_CuttingAlgo->getOutputlow()),m_CuttingAlgo->m_UserSettings);
    switch (m_Mode)
    {
    case 1:
        if (m_PathSimulate->MakePathSimulate())
            adaptdynainput->setEnabled(true);
        break;
    case 2:
        if (m_PathSimulate->MakePathSimulate_Feat(m_CuttingAlgo->getFlatAreas(),0))
            adaptdynainput->setEnabled(true);
        break;
     case 3:
        if (m_PathSimulate->MakePathSimulate_Feat(m_CuttingAlgo->getFlatAreas(),1))
            adaptdynainput->setEnabled(true);
    break;
    }

}

void Cutting::on_GenRobotOut_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select File Output-Directory"),"d:/",
                  QFileDialog::ShowDirsOnly
                  | QFileDialog::DontResolveSymlinks);
    QDir::setCurrent(dir);
    if (m_PathSimulate != NULL) delete m_PathSimulate;
    m_PathSimulate = new path_simulate(*(m_CuttingAlgo->getOutputhigh()),*(m_CuttingAlgo->getOutputlow()),m_CuttingAlgo->m_UserSettings);

    switch (m_Mode)
    {
    case 1:
        m_PathSimulate->MakePathRobot();
        break;
    case 2:
        m_PathSimulate->MakePathRobot_Feat(m_CuttingAlgo->getFlatAreas());
        break;
    }
}


const CuttingToolsSettings& Cutting::getSettings()
{
    //First transfer the settings to the Cutting_tools class
    m_Settings.cad_radius = cad_radius_box->value();
    m_Settings.correction_factor = correction_factor_box->value();
    m_Settings.level_distance = level_distance_box->value();
    m_Settings.limit_angle = limit_angle_box->value();
    m_Settings.sheet_thickness = sheet_thickness_box->value();
    m_Settings.slave_radius = slave_radius_box->value();
    m_Settings.master_radius = master_radius_box->value();
    m_Settings.max_Vel = max_vel->value();
    m_Settings.max_Acc = max_acc->value();
    m_Settings.spring_pretension = spring_pretension->value();
    m_Settings.x_offset_robot = xoffset_box->value();
    m_Settings.y_offset_robot = yoffset_box->value();
    m_Settings.clockwise = clockwise_checkbox->isChecked();
    m_Settings.error_tolerance = error_tolerance->value();

    return m_Settings;
}


void Cutting::on_BestFitButton_clicked()
{






    m_selection = BestFit;
    m_BestFit = new best_fit();


	// Best-Fit based on Point-Clouds
	m_BestFit->Initialize_Mesh_Geometrie_1();
	m_BestFit->Initialize_Mesh_Geometrie_2();
	m_BestFit->Perform_PointCloud();
	m_BestFit->output_best_fit_mesh();


    best_fit_cad_button->setEnabled(true);
}

void Cutting::on_SpringbackButton_clicked()
{
    m_Spring = new SpringbackCorrection();
    m_selection = Springback;
    best_fit_cad_button->setEnabled(true);

}

void Cutting::on_Approximate_button_clicked()
{
    m_selection = Approx;
    best_fit_mesh_button->setEnabled(true);
}

void Cutting::on_best_fit_cad_button_clicked()
{
    selectShape();
    best_fit_mesh_button->setEnabled(true);
}

void Cutting::on_best_fit_mesh_button_clicked()
{
if(m_selection == Springback)
	{
		best_fit_mesh2_button->setEnabled(true);
		m_Spring->Load(m_Shape);
	}
    selectMesh();
    best_fit_go_button->setEnabled(true);
    SelectFace_button->setEnabled(true);
if(m_selection == Springback)
		best_fit_mesh2_button->setEnabled(true);
}
void Cutting::on_best_fit_mesh2_button_clicked()
{
	m_Spring->Load(m_Mesh);
    selectMesh();
}

void Cutting::on_SelectFace_button_clicked()
{


    Gui::Document* doc = Gui::Application::Instance->activeDocument();
    Gui::View3DInventor* view = static_cast<Gui::View3DInventor*>(doc->getActiveView());
    if (view)
    {
        Gui::View3DInventorViewer* viewer = view->getViewer();
        viewer->setEditing(true);
        viewer->addEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, this);
        QMessageBox::information(this, tr("FreeCAD CamWorkbench"), tr("You have to pick a face.\n"));
        this->hide();
    }
}

void Cutting::on_best_fit_go_button_clicked()
{
	bool out = 1;  //gibt an ob beim springback NUR die Fehlervektoren ausgegeben werden solle
    getSettings(); //First transfer the settings to the Cutting_tools class (-> m_Settings)
        

    switch (m_selection)
    {

    case BestFit:

		
        m_BestFit->Load(m_Mesh,m_Shape);
        m_BestFit->Perform();

        best_fit_cad_button ->setEnabled(false);
        best_fit_mesh_button->setEnabled(false);
        best_fit_go_button  ->setEnabled(false);

        m_MeshOut = m_BestFit->m_MeshWork;
        m_MeshCad = m_BestFit->m_CadMesh;
        DisplayMeshOutput(m_MeshOut);
        //DisplayMeshOutput(m_MeshCad);

		


        break;

    case Springback:

      	m_Spring->Load(m_Mesh);
        m_Spring->Init_Setting(m_Settings);
        m_Spring->Init();
        m_Spring->Perform(m_Settings.limit_angle,out);
        m_MeshCad = m_Spring->m_CadMesh;



        best_fit_cad_button ->setEnabled(false);
        best_fit_mesh_button->setEnabled(false);
        best_fit_go_button  ->setEnabled(false);

		if(out==0)
		{
			DisplayMeshOutput(m_MeshCad);
			DisplayMeshOutput(m_Spring->m_Mesh_vis);
			DisplayMeshOutput(m_Spring->m_Mesh_vis2);
		}
best_fit_mesh2_button->setEnabled(true);
        break;

    case Approx:
        /*MeshCore::MeshPointArray pnts   = m_Mesh.GetPoints();  // file "kleines.stl" hat spitze über der ebene ... nicht kompatibel mit diesem Algo
        MeshCore::MeshFacetArray facets = m_Mesh.GetFacets();

        for(int i=0; i<pnts.size(); ++i)
        {
         if(pnts[i].z >0.0)
          pnts[i].z *= -1;
        }

        m_Mesh.Assign(pnts,facets);*/

		std::vector<double> CtrlPnts, U_knot, V_knot;
		int degU,degV;	
		m_App = new Approximate(m_Mesh,CtrlPnts,U_knot,V_knot,degU,degV,m_Settings.error_tolerance);

        //m_Approx = new UniGridApprox(m_Mesh, 1);
        //m_Approx->Perform(0.1);

        BRepBuilderAPI_MakeFace  Face(m_App->aAdaptorSurface.Surface());
        m_Shape = Face.Face();
		
		ofstream anOutputFile;
	    anOutputFile.open("c:/approx_log.txt");
		anOutputFile << "face zugewiesen" << endl;
		
		DisplayMeshOutput(m_App->MeshParam);
        DisplayShapeOutput();
        break;
    }
}

void Cutting::DisplayMeshOutput(const MeshCore::MeshKernel &mesh)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    App::DocumentObject* obj = doc->addObject("Mesh::Feature","Best_Fit-Mesh");

    Mesh::Feature* part1 = static_cast<Mesh::Feature*>(obj);
    part1->Mesh.setValue(mesh);

    //doc->recompute();
}

void Cutting::DisplayShapeOutput()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    App::DocumentObject* obj = doc->addObject("Part::Feature","Output-Shape");

    Part::Feature* part1 = static_cast<Part::Feature*>(obj);
    part1->Shape.setValue(m_Shape);

    //doc->recompute();
}

void Cutting::DisplayCAMOutput()
{
    BRep_Builder BB;
    TopoDS_Compound aCompound1,aCompound2;
    BB.MakeCompound(aCompound1);
    BB.MakeCompound(aCompound2);
    TopoDS_Edge anEdge;
    const std::vector<Handle_Geom_BSplineCurve>* topCurves;
    const std::vector<Handle_Geom_BSplineCurve>* botCurves;
    std::vector<Handle_Geom_BSplineCurve>::const_iterator an_it1;
    topCurves = m_CuttingAlgo->getOutputhigh();
    botCurves = m_CuttingAlgo->getOutputlow();
    for (an_it1 = topCurves->begin();an_it1!=topCurves->end();an_it1++)
    {
        BB.MakeEdge(anEdge,*an_it1,0.01);
        BB.Add(aCompound1,anEdge);

    }
    for (an_it1 = botCurves->begin();an_it1!=botCurves->end();an_it1++)
    {
        BB.MakeEdge(anEdge,*an_it1,0.01);
        BB.Add(aCompound2,anEdge);
    }

    App::Document* doc = App::GetApplication().getActiveDocument();
    App::DocumentObject* obj = doc->addObject("Part::Feature","Master-Tool");
    App::DocumentObject* obj1 = doc->addObject("Part::Feature","Slave-Tool");

    Part::Feature* part1 = static_cast<Part::Feature*>(obj);
    Part::Feature* part2 = static_cast<Part::Feature*>(obj1);
	part1->Shape.setValue(aCompound1);
	part2->Shape.setValue(aCompound2);






    //
    //      for (unsigned int i=0;i<aTestOutput.size();++i)
    //      {
    //          BB.Add(aCompound,anEdge);
    //      }

    //anewCuttingEnv.OffsetWires_Standard(10.0);

    //std::vector<Handle_Geom_BSplineCurve> topCurves;
    //std::vector<Handle_Geom_BSplineCurve> botCurves;
    //std::vector<Handle_Geom_BSplineCurve>::iterator an_it;
    //topCurves = *(anewCuttingEnv.getOutputhigh());
    //botCurves = *(anewCuttingEnv.getOutputlow());
  
	ofstream anoutput;
	ofstream anoutput2;
	anoutput.open("c:/topCurves.txt");
	anoutput2.open("c:/botCurves.txt");
    for (an_it1 = topCurves->begin();an_it1!=topCurves->end();an_it1++)
    {
		GeomAdaptor_Curve aCurveAdaptor(*an_it1);
        GCPnts_QuasiUniformDeflection aPointGenerator(aCurveAdaptor,0.1);
        for (int t=1;t<=aPointGenerator.NbPoints();++t)
        {
            anoutput << (aPointGenerator.Value(t)).X() <<","<< (aPointGenerator.Value(t)).Y() <<","<<(aPointGenerator.Value(t)).Z()<<std::endl;
        }
    }
    for (an_it1 = botCurves->begin();an_it1!=botCurves->end();an_it1++)
    {
        GeomAdaptor_Curve aCurveAdaptor(*an_it1);
        GCPnts_QuasiUniformDeflection aPointGenerator(aCurveAdaptor,0.1);
        for (int t=1;t<=aPointGenerator.NbPoints();++t)
        {
            anoutput2 << (aPointGenerator.Value(t)).X() <<","<< (aPointGenerator.Value(t)).Y() <<","<<(aPointGenerator.Value(t)).Z()<<std::endl;
        }
    }
    anoutput.close();
    anoutput2.close();

}



void Cutting::zLevelCallback(void * ud, SoEventCallback * n)
{
    const SoMouseButtonEvent * mbe = (SoMouseButtonEvent *)n->getEvent();
    Gui::View3DInventorViewer* view  = reinterpret_cast<Gui::View3DInventorViewer*>(n->getUserData());
    Cutting* that = reinterpret_cast<Cutting*>(ud);

    // Mark all incoming mouse button events as handled, especially, to deactivate the selection node
    n->getAction()->setHandled();
    if (mbe->getButton() == SoMouseButtonEvent::BUTTON2 && mbe->getState() == SoButtonEvent::UP)
    {
        n->setHandled();
        view->setEditing(false);
        view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, that);
        that->show();
    }
    else if (mbe->getButton() == SoMouseButtonEvent::BUTTON1 && mbe->getState() == SoButtonEvent::DOWN)
    {
        const SoPickedPoint * point = n->getPickedPoint();
        if (point == NULL)
        {
            QMessageBox::warning(Gui::getMainWindow(),"z level", "No shape picked!");
            return;
        }

        n->setHandled();

        // By specifying the indexed mesh node 'pcFaceSet' we make sure that the picked point is
        // really from the mesh we render and not from any other geometry
        Gui::ViewProvider* vp = static_cast<Gui::ViewProvider*>(view->getViewProviderByPath(point->getPath()));
        if (!vp || !vp->getTypeId().isDerivedFrom(PartGui::ViewProviderPart::getClassTypeId()))
            return;
        PartGui::ViewProviderPart* vpp = static_cast<PartGui::ViewProviderPart*>(vp);
        std::string element = vpp->getElement(point->getDetail());
        TopoDS_Shape sh = static_cast<Part::Feature*>(vpp->getObject())->
            Shape.getShape().getSubShape(element.c_str());
        if (!sh.IsNull())
        {
            // ok a shape was picked
            n->setHandled();
            view->setEditing(false);
            view->removeEventCallback(SoMouseButtonEvent::getClassTypeId(), zLevelCallback, that);
            that->show();
            SbVec3f pt = point->getPoint();
            that->setFace(sh, pt[0],pt[1],pt[2]);
        }
    }
}



#include "moc_Cutting.cpp"
