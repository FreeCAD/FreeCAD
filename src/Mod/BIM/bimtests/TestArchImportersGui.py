# SPDX-FileNotice: Part of the FreeCAD project.

import FreeCAD as App
from bimtests.TestArchBaseGui import TestArchBaseGui


class TestArchImportersGui(TestArchBaseGui):

    def testImportSH3D(self):
        """Import a SweetHome 3D file"""
        import BIM.importers.importSH3DHelper

        importer = BIM.importers.importSH3DHelper.SH3DImporter(None)
        importer.import_sh3d_from_string(SH3D_HOME)

        assert App.ActiveDocument.Site
        assert App.ActiveDocument.BuildingPart.Label == "Building"
        assert App.ActiveDocument.BuildingPart001.Label == "Level"
        assert App.ActiveDocument.Wall


SH3D_HOME = """<?xml version='1.0'?>
<home version='7200' name='0-JoinWall.sh3d' camera='topCamera' wallHeight='250.0'>
  <property name='com.eteks.sweethome3d.SweetHome3D.CatalogPaneDividerLocation' value='327'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ColumnWidths' value='100,84,82,85,84'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameHeight' value='576'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameMaximized' value='true'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameWidth' value='1092'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameX' value='50'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.FrameY' value='87'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.MainPaneDividerLocation' value='441'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanPaneDividerLocation' value='263'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanScale' value='0.21343713'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanViewportX' value='0'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.PlanViewportY' value='0'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ScreenHeight' value='720'/>
  <property name='com.eteks.sweethome3d.SweetHome3D.ScreenWidth' value='1366'/>
  <furnitureVisibleProperty name='NAME'/>
  <furnitureVisibleProperty name='WIDTH'/>
  <furnitureVisibleProperty name='DEPTH'/>
  <furnitureVisibleProperty name='HEIGHT'/>
  <furnitureVisibleProperty name='VISIBLE'/>
  <environment groundColor='FFB78744' skyColor='00CCE4FC' lightColor='00D0D0D0' ceillingLightColor='00D0D0D0' photoWidth='400' photoHeight='300' photoAspectRatio='VIEW_3D_RATIO' photoQuality='0' videoWidth='320' videoAspectRatio='RATIO_4_3' videoQuality='0' videoFrameRate='25'>
    <texture attribute='skyTexture' name='Cloudy' creator='eTeks' catalogId='eTeks#cloudy' width='100.0' height='27.6' image='0'/>
  </environment>
  <compass x='-100.0' y='50.0' diameter='100.0' northDirection='0.0' longitude='-0.06428629' latitude='0.70511305' timeZone='Europe/Madrid'/>
  <observerCamera attribute='observerCamera' lens='PINHOLE' x='50.0' y='50.0' z='170.0' yaw='5.4977875' pitch='0.19634955' fieldOfView='1.0995575' time='1729080000000'/>
  <camera attribute='topCamera' lens='PINHOLE' x='1304.082' y='1936.5889' z='1526.6199' yaw='8.98363' pitch='0.7049999' fieldOfView='1.0995575' time='1729080000000'/>
  <wall id='wall0' wallAtEnd='wall1' xStart='0.0' yStart='0.0' xEnd='100.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall1' wallAtStart='wall0' wallAtEnd='wall2' xStart='100.0' yStart='0.0' xEnd='200.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall2' wallAtStart='wall1' xStart='200.0' yStart='0.0' xEnd='300.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall3' wallAtEnd='wall4' xStart='0.0' yStart='50.0' xEnd='100.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall4' wallAtStart='wall3' wallAtEnd='wall5' xStart='100.0' yStart='100.0' xEnd='200.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall5' wallAtStart='wall4' xStart='200.0' yStart='100.0' xEnd='300.0' yEnd='50.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall6' wallAtEnd='wall7' xStart='0.0' yStart='200.0' xEnd='100.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall7' wallAtStart='wall6' wallAtEnd='wall8' xStart='100.0' yStart='300.0' xEnd='200.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall8' wallAtStart='wall7' xStart='200.0' yStart='300.0' xEnd='300.0' yEnd='200.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall9' wallAtEnd='wall10' xStart='100.0' yStart='400.0' xEnd='100.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall10' wallAtStart='wall9' wallAtEnd='wall11' xStart='100.0' yStart='500.0' xEnd='200.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall11' wallAtStart='wall10' xStart='200.0' yStart='500.0' xEnd='200.0' yEnd='400.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall12' wallAtStart='wall14' wallAtEnd='wall13' xStart='150.0' yStart='600.0' xEnd='100.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall13' wallAtStart='wall12' wallAtEnd='wall14' xStart='100.0' yStart='700.0' xEnd='200.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall14' wallAtStart='wall13' wallAtEnd='wall12' xStart='200.0' yStart='700.0' xEnd='150.0' yEnd='600.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall15' wallAtEnd='wall16' xStart='400.0' yStart='150.0' xEnd='500.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall16' wallAtStart='wall15' wallAtEnd='wall17' xStart='500.0' yStart='100.0' xEnd='600.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall17' wallAtStart='wall16' xStart='600.0' yStart='100.0' xEnd='700.0' yEnd='50.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall18' wallAtEnd='wall19' xStart='400.0' yStart='400.0' xEnd='500.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall19' wallAtStart='wall18' wallAtEnd='wall20' xStart='500.0' yStart='300.0' xEnd='600.0' yEnd='300.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall20' wallAtStart='wall19' xStart='600.0' yStart='300.0' xEnd='700.0' yEnd='200.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall21' wallAtEnd='wall22' xStart='400.0' yStart='600.0' xEnd='400.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall22' wallAtStart='wall21' wallAtEnd='wall23' xStart='400.0' yStart='500.0' xEnd='600.0' yEnd='500.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall23' wallAtStart='wall22' xStart='600.0' yStart='500.0' xEnd='600.0' yEnd='400.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall24' wallAtEnd='wall25' xStart='600.0' yStart='800.0' xEnd='500.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall25' wallAtStart='wall24' wallAtEnd='wall26' xStart='500.0' yStart='700.0' xEnd='600.0' yEnd='700.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall26' wallAtStart='wall25' xStart='600.0' yStart='700.0' xEnd='500.0' yEnd='600.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall27' wallAtStart='wall30' wallAtEnd='wall28' xStart='800.0' yStart='0.0' xEnd='1000.0' yEnd='0.0' height='250.0' thickness='10.0' arcExtent='1.0471976' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall28' wallAtStart='wall27' wallAtEnd='wall29' xStart='1000.0' yStart='0.0' xEnd='1000.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall29' wallAtStart='wall28' wallAtEnd='wall30' xStart='1000.0' yStart='100.0' xEnd='800.0' yEnd='100.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall30' wallAtStart='wall29' wallAtEnd='wall27' xStart='800.0' yStart='100.0' xEnd='800.0' yEnd='0.0' height='250.0' thickness='10.0' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall31' wallAtEnd='wall32' xStart='800.0' yStart='400.0' xEnd='1000.0' yEnd='200.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall32' wallAtStart='wall31' wallAtEnd='wall33' xStart='1000.0' yStart='200.0' xEnd='1200.0' yEnd='400.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall33' wallAtStart='wall32' wallAtEnd='wall34' xStart='1200.0' yStart='400.0' xEnd='1000.0' yEnd='600.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall34' wallAtStart='wall33' xStart='1000.0' yStart='600.0' xEnd='800.0' yEnd='400.0' height='250.0' thickness='10.0' arcExtent='1.5707964' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall35' wallAtEnd='wall36' xStart='800.0' yStart='800.0' xEnd='900.0' yEnd='900.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall36' wallAtStart='wall35' wallAtEnd='wall37' xStart='900.0' yStart='900.0' xEnd='1000.0' yEnd='800.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall37' wallAtStart='wall36' wallAtEnd='wall38' xStart='1000.0' yStart='800.0' xEnd='900.0' yEnd='700.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
  <wall id='wall38' wallAtStart='wall37' xStart='900.0' yStart='700.0' xEnd='800.0' yEnd='800.0' height='250.0' thickness='10.0' arcExtent='-3.1415927' pattern='hatchUp' topColor='FF0000FF' leftSideColor='FF00FF00' rightSideColor='FFFF0000'/>
</home>
"""
