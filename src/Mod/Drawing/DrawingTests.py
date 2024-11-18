import Part, Drawing

Part.open("D:/_Projekte/FreeCAD/FreeCADData/Schenkel.stp")
App.activeDocument().addObject("Drawing::FeaturePage", "Page")
App.activeDocument().Page.Template = (
    "D:/_Projekte/FreeCAD/FreeCAD_0.9_LibPack7/Mod/Drawing/Templates/A3_Landscape.svg"
)
App.activeDocument().addObject("Drawing::FeatureViewPart", "View")
App.activeDocument().View.Source = App.activeDocument().Schenkel
App.activeDocument().View.Direction = (0.0, 1.0, 0.0)
App.activeDocument().View.X = 30.0
App.activeDocument().View.Y = 30.0
App.activeDocument().View.Scale = 1.0
App.activeDocument().Page.addObject(App.activeDocument().View)

App.activeDocument().addObject("Drawing::FeatureViewPart", "View1")
App.activeDocument().View1.Source = App.activeDocument().Schenkel
App.activeDocument().View1.Direction = (0.0, 0.0, 1.0)
App.activeDocument().View1.X = 70.0
App.activeDocument().View1.Y = 30.0
App.activeDocument().View1.Scale = 1.0
App.activeDocument().Page.addObject(App.activeDocument().View1)

App.activeDocument().addObject("Drawing::FeatureViewPart", "View2")
App.activeDocument().View2.Source = App.activeDocument().Schenkel
App.activeDocument().View2.Direction = (1.0, 0.0, 0.0)
App.activeDocument().View2.X = 70.0
App.activeDocument().View2.Y = 200.0
App.activeDocument().View2.Rotation = 90.0
App.activeDocument().View2.Scale = 1.0
App.activeDocument().Page.addObject(App.activeDocument().View2)

App.activeDocument().addObject("Drawing::FeatureViewPart", "View3")
App.activeDocument().View3.Source = App.activeDocument().Schenkel
App.activeDocument().View3.Direction = (1.0, 1.0, 1.0)
App.activeDocument().View3.X = 280.0
App.activeDocument().View3.Y = 90.0
App.activeDocument().View3.Scale = 1.0
App.activeDocument().Page.addObject(App.activeDocument().View3)

App.activeDocument().recompute()
