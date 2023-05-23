#***************************************************************************
#*   Copyright (c) 2015 Yorik van Havre <yorik@uncreated.net>              *
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__title__  = "FreeCAD GbXml exporter"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

import FreeCAD
import Draft

if FreeCAD.GuiUp:
    from draftutils.translate import translate
else:
    # \cond
    def translate(ctx,txt):
        return txt
    # \endcond

## @package importGBXML
#  \ingroup ARCH
#  \brief GBXML file format exporter
#
#  This module provides tools to export GBXML files.

def export(objectslist,filename):

    if len(objectslist) != 1:
        FreeCAD.Console.PrintError(translate("Arch","This exporter can currently only export one site object")+"\n")
        return
    site = objectslist[0]
    if Draft.getType(site) != "Site":
        FreeCAD.Console.PrintError(translate("Arch","This exporter can currently only export one site object")+"\n")
        return

    filestream = pyopen(filename,"wb")

    # header
    filestream.write( '<?xml version="1.0"?>\n' )
    filestream.write( '<!-- Exported by FreeCAD %s -->\n' % FreeCAD.Version()[0]+FreeCAD.Version()[1]+FreeCAD.Version()[2] )
    filestream.write( '<gbXML\n' )
    filestream.write( '  xmlns="http://www.gbxml.org/schema"\n' )
    filestream.write( '  xmlns:xhtml="http://www.w3.org/1999/xhtml"\n' )
    filestream.write( '  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"\n' )
    filestream.write( '  xsi:schemaLocation="http://www.gbxml.org/schema"\n ' )
    filestream.write( '  temperatureUnit="C"\n' )
    filestream.write( '  lengthUnit="Meters"\n' )
    filestream.write( '  areaUnit="SquareMeters"\n' )
    filestream.write( '  volumeUnit="CubicMeters"\n' )
    filestream.write( '  useSIUnitsForResults="false" >\n' )
    filestream.write( '\n' )

    # campus
    filestream.write( '<Campus id="%s">\n' % site.Name )
    filestream.write( '<Location>\n' )
    filestream.write( '    <ZipcodeOrPostalCode>%s</ZipcodeOrPostalCode>\n' % site.PostalCode )
    filestream.write( '    <Longitude>%f</Longitude>\n' % site.Longitude )
    filestream.write( '    <Latitude>%f</Latitude>\n' % site.Latitude )
    filestream.write( '    <Elevation>%f/Elevation>\n' % site.Elevation.Value )
    filestream.write( '    <Name>%s</Name>\n' % site.Label )
    #filestream.write( '    <CADModelAzimuth>0</CADModelAzimuth>\n' )
    #filestream.write( '    <StationId IDType="WMO">53158_2004</StationId>\n' )
    filestream.write( '</Location>\n' )

    # buildings
    for building in site.Group:
        if Draft.getType(building) == "Building":
            filestream.write( '    <Building id="$s" buildingType="$s">\n' % (building.Name,building.BuildingType) )
            filestream.write( '        <Area>$f</Area>\n' % str(building.Area.getValueAs("m^2")) )

            # spaces
            for space in Draft.getObjectsOfType(Draft.get_group_contents(building.Group, addgroups=True),
                                                "Space"):
                if not space.Zone:
                    FreeCAD.Console.PrintError(translate("Arch","Error: Space '%s' has no Zone. Aborting.") % space.Label + "\n")
                    return
                filestream.write( '        <Space id="%s" spaceType="%s" zoneIdRef="%s" conditionType="%f">\n' % (space.Name, space.SpaceType, space.Zone.Name, space.Conditioning) )
                #filestream.write( '            <CADObjectId>%s</CADObjectId>\n' % space.Name ) # not sure what this is used for?
                filestream.write( '            <Name>%s</Name>\n' % space.Label )
                filestream.write( '            <Description>%s</Description>\n' % space.Description )
                filestream.write( '            <PeopleNumber unit="NumberOfPeople">%i</PeopleNumber>\n' % space.NumberOfPeople)
                filestream.write( '            <LightPowerPerArea unit="WattPerSquareMeter">%f</LightPowerPerArea>\n' % space.LightingPower/space.Area.getValueAs("m^2") )
                filestream.write( '            <EquipPowerPerArea unit="WattPerSquareMeter">%f</EquipPowerPerArea>\n' % space.EquipmentPower/space.Area.getValueAs("m^2") )
                filestream.write( '            <Area>$f</Area>\n' % space.Area.getValueAs("m^2") )
                filestream.write( '            <Volume>$f</Volume>\n' % FreeCAD.Units.Quantity(space.Shape.Volume,FreeCAD.Units.Volume).getValueAs("m^3") )
                filestream.write( '            <ShellGeometry id="%s_geometry">\n' % space.Name )

                # shells
                for solid in space.Shape.Solids:
                    filestream.write( '                <ClosedShell>\n' )
                    for face in solid.Faces:
                        filestream.write( '                    <PolyLoop>\n' )
                        for v in face.OuterWire.Vertexes:
                            filestream.write( '                        <CartesianPoint>\n' )
                            filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.x )
                            filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.y )
                            filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.z )
                            filestream.write( '                        </CartesianPoint>\n' )
                        filestream.write( '                    </PolyLoop>\n' )
                    filestream.write( '                </ClosedShell>\n' )
                filestream.write( '            </ShellGeometry>\n' )
                filestream.write( '        </Space>\n' )

                # surfaces
                for i,face in enumerate(space.Shape.Faces):
                    filestream.write( '            <SpaceBoundary isSecondLevelBoundary="false" surfaceIdRef="%s_Face%i"\n' % space.Name, i )
                    filestream.write( '                <PlanarGeometry>\n' )
                    filestream.write( '                    <PolyLoop>\n' )
                    for v in face.OuterWire.Vertexes:
                        filestream.write( '                        <CartesianPoint>\n' )
                        filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.x )
                        filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.y )
                        filestream.write( '                            <Coordinate>%f</Coordinate>\n' % v.Point.z )
                        filestream.write( '                        </CartesianPoint>\n' )
                    filestream.write( '                    </PolyLoop>\n' )
                    filestream.write( '                </PlanarGeometry>\n' )
                    filestream.write( '            </SpaceBoundary>\n' )

                filestream.write( '        </Space>\n' )

            filestream.write( '    </Building>\n' )

    filestream.write( '</Campus>\n' )

    filestream.write( '</gbXML>' )

'''
        <Area>18000.00000</Area>
        <Space id="sp1_LabandCorridor_Labcorridor" spaceType="LaboratoryOffice" zoneIdRef="z1_LabandCorridor">
            <Name>Lab corridor</Name>
            <Description/>
            <PeopleNumber unit="NumberOfPeople">1.00000</PeopleNumber>
            <LightPowerPerArea unit="WattPerSquareFoot">1.50000</LightPowerPerArea>
            <EquipPowerPerArea unit="WattPerSquareFoot">0.00000</EquipPowerPerArea>
            <Area>800.00000</Area>
            <Volume>6400.00000</Volume>
            <ShellGeometry id="geo_sp1_LabandCorridor_Labcorridor">
                <ClosedShell>
                    <PolyLoop>
                        <CartesianPoint>
                            <Coordinate>0.00000</Coordinate>
                            <Coordinate>1200.00000</Coordinate>
                            <Coordinate>0.00000</Coordinate>
                        </CartesianPoint>
                        <CartesianPoint>
                            <Coordinate>0.00000</Coordinate>
                            <Coordinate>1200.00000</Coordinate>
                            <Coordinate>96.00000</Coordinate>
                        </CartesianPoint>
                        <CartesianPoint>
                            <Coordinate>480.00000</Coordinate>
                            <Coordinate>1200.00000</Coordinate>
                            <Coordinate>96.00000</Coordinate>
                        </CartesianPoint>
                        <CartesianPoint>
                            <Coordinate>480.00000</Coordinate>
                            <Coordinate>1200.00000</Coordinate>
                            <Coordinate>0.00000</Coordinate>
                        </CartesianPoint>
                    </PolyLoop>

                    ... repeat

                </ClosedShell>
            </ShellGeometry>

            <SpaceBoundary isSecondLevelBoundary="false" surfaceIdRef="aim1095">
              <PlanarGeometry>
                <PolyLoop>
                  <CartesianPoint>
                    <Coordinate>9.981497</Coordinate>
                    <Coordinate>-31.19363</Coordinate>
                    <Coordinate>0</Coordinate>
                  </CartesianPoint>
                  <CartesianPoint>
                    <Coordinate>9.981497</Coordinate>
                    <Coordinate>-5.193626</Coordinate>
                    <Coordinate>0</Coordinate>
                  </CartesianPoint>
                  <CartesianPoint>
                    <Coordinate>9.981497</Coordinate>
                    <Coordinate>-5.193626</Coordinate>
                    <Coordinate>100</Coordinate>
                  </CartesianPoint>
                  <CartesianPoint>
                    <Coordinate>9.981497</Coordinate>
                    <Coordinate>-31.19363</Coordinate>
                    <Coordinate>100</Coordinate>
                  </CartesianPoint>
                </PolyLoop>
              </PlanarGeometry>
            </SpaceBoundary>




            <CADObjectId>21E2</CADObjectId>
        </Space>

        ... repeat

    </Building>



    <Surface id="su1_Floor" surfaceType="UndergroundSlab" constructionIdRef="construction-1">
        <Name>Floor</Name>
        <AdjacentSpaceId spaceIdRef="sp1_LabandCorridor_Labcorridor"/>
        <RectangularGeometry>
            <Azimuth>90.00</Azimuth>
            <CartesianPoint>
                <Coordinate>0.00000</Coordinate>
                <Coordinate>1320.00000</Coordinate>
                <Coordinate>0.00000</Coordinate>
            </CartesianPoint>
            <Tilt>180.00</Tilt>
            <Height>480.00000</Height>
            <Width>240.00000</Width>
        </RectangularGeometry>
        <PlanarGeometry>
            <PolyLoop>
                <CartesianPoint>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>1320.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>480.00000</Coordinate>
                    <Coordinate>1320.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>960.00000</Coordinate>
                    <Coordinate>1320.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>960.00000</Coordinate>
                    <Coordinate>1200.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>480.00000</Coordinate>
                    <Coordinate>1200.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>1200.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
            </PolyLoop>
        </PlanarGeometry>
    </Surface>



    <Surface id="su44_Surface4" surfaceType="ExteriorWall" constructionIdRef="construction-3">
        <Name>Surface 4</Name>
        <AdjacentSpaceId spaceIdRef="sp7_Office_Office6"/>
        <RectangularGeometry>
            <Azimuth>180.00</Azimuth>
            <CartesianPoint>
                <Coordinate>960.00000</Coordinate>
                <Coordinate>0.00000</Coordinate>
                <Coordinate>0.00000</Coordinate>
            </CartesianPoint>
            <Tilt>90.00</Tilt>
            <Height>114.00000</Height>
            <Width>480.00000</Width>
        </RectangularGeometry>
        <PlanarGeometry>
            <PolyLoop>
                <CartesianPoint>
                    <Coordinate>960.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>1440.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>1440.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>114.00000</Coordinate>
                </CartesianPoint>
                <CartesianPoint>
                    <Coordinate>960.00000</Coordinate>
                    <Coordinate>0.00000</Coordinate>
                    <Coordinate>114.00000</Coordinate>
                </CartesianPoint>
            </PolyLoop>
        </PlanarGeometry>
        <Opening id="su44-op1_Opening1" openingType="OperableWindow" windowTypeIdRef="windowType-1">
            <Name>Opening1</Name>
            <RectangularGeometry>
                <CartesianPoint>
                    <Coordinate>96.00000</Coordinate>
                    <Coordinate>24.00000</Coordinate>
                </CartesianPoint>
                <Height>72.00000</Height>
                <Width>48.00000</Width>
            </RectangularGeometry>
            <PlanarGeometry>
                <PolyLoop>
                    <CartesianPoint>
                        <Coordinate>1056.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1104.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1104.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1056.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                </PolyLoop>
            </PlanarGeometry>
        </Opening>
        <Opening id="su44-op2_Opening2" openingType="OperableWindow" windowTypeIdRef="windowType-1">
            <Name>Opening2</Name>
            <RectangularGeometry>
                <CartesianPoint>
                    <Coordinate>216.00000</Coordinate>
                    <Coordinate>24.00000</Coordinate>
                </CartesianPoint>
                <Height>72.00000</Height>
                <Width>48.00000</Width>
            </RectangularGeometry>
            <PlanarGeometry>
                <PolyLoop>
                    <CartesianPoint>
                        <Coordinate>1176.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1224.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1224.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1176.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                </PolyLoop>
            </PlanarGeometry>
        </Opening>
        <Opening id="su44-op3_Opening3" openingType="OperableWindow" windowTypeIdRef="windowType-1">
            <Name>Opening3</Name>
            <RectangularGeometry>
                <CartesianPoint>
                    <Coordinate>336.00000</Coordinate>
                    <Coordinate>24.00000</Coordinate>
                </CartesianPoint>
                <Height>72.00000</Height>
                <Width>48.00000</Width>
            </RectangularGeometry>
            <PlanarGeometry>
                <PolyLoop>
                    <CartesianPoint>
                        <Coordinate>1296.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1344.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>24.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1344.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                    <CartesianPoint>
                        <Coordinate>1296.00000</Coordinate>
                        <Coordinate>0.00000</Coordinate>
                        <Coordinate>96.00000</Coordinate>
                    </CartesianPoint>
                </PolyLoop>
            </PlanarGeometry>
        </Opening>
    </Surface>


    ... repeat

</Campus>



<Construction id="construction-1">
    <Name>Standard</Name>
    <Description/>
</Construction>
<Construction id="construction-2">
    <Name>Standard</Name>
    <Description/>
</Construction>
<Construction id="construction-3">
    <Name>Standard</Name>
    <Description/>
</Construction>
<WindowType id="windowType-1">
    <Name>Standard</Name>
    <Description/>
</WindowType>



<Zone id="z1_LabandCorridor">
    <Name>Lab and Corridor</Name>
    <Description/>
    <AirChangesPerHour>0"</AirChangesPerHour>
    <FlowPerArea unit="CFMPerSquareFoot">0.00000</FlowPerArea>
    <FlowPerPerson unit="CFM">0.00000</FlowPerPerson>
    <OAFlowPerArea unit="CFMPerSquareFoot">2.37037</OAFlowPerArea>
    <OAFlowPerPerson unit="CFM">812.69841</OAFlowPerPerson>
    <DesignHeatT>72.00000</DesignHeatT>
    <DesignCoolT>75.00000</DesignCoolT>
</Zone>
<Zone id="z2_Office">
    <Name>Office</Name>
    <Description/>
    <AirChangesPerHour>1"</AirChangesPerHour>
    <FlowPerArea unit="CFMPerSquareFoot">0.13333</FlowPerArea>
    <FlowPerPerson unit="CFM">20.00000</FlowPerPerson>
    <OAFlowPerArea unit="CFMPerSquareFoot">0.05333</OAFlowPerArea>
    <OAFlowPerPerson unit="CFM">8.00000</OAFlowPerPerson>
    <DesignHeatT>72.00000</DesignHeatT>
    <DesignCoolT>75.00000</DesignCoolT>
</Zone>
<Zone id="z3_Warehouse">
    <Name>Warehouse</Name>
    <Description/>
    <AirChangesPerHour>5/32"</AirChangesPerHour>
    <FlowPerArea unit="CFMPerSquareFoot">0.05000</FlowPerArea>
    <FlowPerPerson unit="CFM">25.71429</FlowPerPerson>
    <OAFlowPerArea unit="CFMPerSquareFoot">0.00000</OAFlowPerArea>
    <OAFlowPerPerson unit="CFM">0.00000</OAFlowPerPerson>
    <DesignHeatT>60.00000</DesignHeatT>
    <DesignCoolT>80.00000</DesignCoolT>
</Zone>

<DocumentHistory>
    <ProgramInfo id="adesk-rvt-1">
        <CompanyName>Autodesk, Inc.</CompanyName>
        <ProductName>Autodesk Project Vasari CEA</ProductName>
        <Version>TP2.0 20110514_1800</Version>
        <Platform>Microsoft Windows XP</Platform>
    </ProgramInfo>
</DocumentHistory>

<Results xmlns="" id="sp3_LabandCorridor_Lab1" objectIdRef="sp3_LabandCorridor_Lab1" resultsType="CoolingLoad" unit="BtuPerHour">
    <ObjectId>sp3_LabandCorridor_Lab1</ObjectId>
    <Value>5534.837890625</Value>
    <Description>Space Cooling Roof Cond</Description>
    <CADObjectId>21E3</CADObjectId>
</Results>

... repeat

</gbXML>'''
