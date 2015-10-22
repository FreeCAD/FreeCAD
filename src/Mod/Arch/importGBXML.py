#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2015                                                    *  
#*   Yorik van Havre <yorik@uncreated.net>                                 *  
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

__title__=   "FreeCAD GbXml exporter"
__author__ = "Yorik van Havre"
__url__ =    "http://www.freecadweb.org"

import os,FreeCAD,Draft

if FreeCAD.GuiUp:
    from DraftTools import translate
else:
    def translate(ctx,txt):
        return txt

if open.__module__ == '__builtin__':
    pyopen = open # because we'll redefine open below
    
    
def export(objectslist,filename):
    
    if len(objectslist) != 1:
        FreeCAD.Console.PrintError(translate("Arch","This exporter can currently only export one site object"))
        return
    site = objectslist[0]
    if Draft.getType(site) != "Site":
        FreeCAD.Console.PrintError(translate("Arch","This exporter can currently only export one site object"))
        return
        
    filestream = pyopen(filename,"wb")
        
    # header
    filestream.write( '<?xml version="1.0"?>\n' )
    filestream.write( '<!-- Exported by FreeCAD %s -->\n' % FreeCAD.Version()[0]+FreeCAD.Version()[1]+FreeCAD.Version()[2] )
    filestream.write( '<gbXML xmlns="http://www.gbxml.org/schema" xmlns:xhtml="http://www.w3.org/1999/xhtml" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.gbxml.org/schema" temperatureUnit="C" lengthUnit="Millimeters" areaUnit="SquareMeters" volumeUnit="CubicMeters" useSIUnitsForResults="false">\n' )
    filestream.write( '\n' )

    # campus
    filestream.write( '<Campus id="%s">\n' % site.Name )
    filestream.write( '<Location>\n' )
    filestream.write( '    <ZipcodeOrPostalCode>%s</ZipcodeOrPostalCode>\n' % site.PostalCode )
    filestream.write( '</Location>\n' )

    # building
    for building in site.Group:
        if Draft.getType(building) == "Building":
            area = 10000.0 # TODO calculate
            filestream.write( '    <Building id="$s" buildingType="$s">\n' % (building.Name,building.BuildingType) )
            filestream.write( '        <Area>$f</Area>\n' % area )
            
            # space
            for space in Draft.getGroupContents(building):
                if Draft.getType(space) == "Space":
                    zone = "BLABLA" # TODO build values
                    filestream.write( '        <Space id="%s" spaceType="%s" zoneIdRef="%s">\n' % (space.Name, space.SpaceType, zone) )
                    filestream.write( '            <Name>%s</Name>\n' % space.Label )
                    filestream.write( '            <Description>%s</Description>\n' % space.Description )
                    filestream.write( '            <Name>%s</Name>\n' % space.Label )
                    #filestream.write( '            <PeopleNumber unit="NumberOfPeople">1.00000</PeopleNumber>\n' )
                    #filestream.write( '            <LightPowerPerArea unit="WattPerSquareFoot">1.50000</LightPowerPerArea>\n' )
                    #filestream.write( '            <EquipPowerPerArea unit="WattPerSquareFoot">0.00000</EquipPowerPerArea>\n' )
                    filestream.write( '            <Area>$f</Area>\n' % space.Area
            
            
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


<Results xmlns="" id="sp3_LabandCorridor_Lab1" objectIdRef="sp3_LabandCorridor_Lab1" resultsType="CoolingLoad" unit="BtuPerHour">
    <ObjectId>sp3_LabandCorridor_Lab1</ObjectId>
    <Value>5534.837890625</Value>
    <Description>Space Cooling Roof Cond</Description>
    <CADObjectId>21E3</CADObjectId>
</Results>

... repeat

</gbXML>'''




