# FreeCAD Path Workbench GCode documentation

This is a documentation of all GCodes used by the FreeCAD Path Workbench

| Command      | Description                                                       | Supported parameters                   |
| ---          | ---                                                               | ---                                    |
| G0, G00      | Rapid move                                                        | X,Y,Z,A,B,C                            |
| G1, G01      | Normal interpolated move                                          | X,Y,Z,A,B,C                            |
| G2, G02      | Clockwise arc                                                     | X,Y,Z,A,B,C,I,J,K                      |
| G3, G03      | Counterclockwise arc                                              | X,Y,Z,A,B,C,I,J,K                      |
| G40          | Turn off tool radius compensation                                 | Radius compensation is done in FreeCAD |
| G41          | Tool radius compensation value                                    | Radius compensation is done in FreeCAD |
| G42          | Tool radius compensation value                                    | Radius compensation is done in FreeCAD |
| G43          | Tool length offset                                                |                                        |
| G44          | Tool length offset                                                |                                        |
| G53          | Machine coordinate system fixture                                 | ???                                    |
| G54          | Scratchpad coordinate system fixture                              | ???                                    |
| G55 - G59.9  | Machine specific work offset fixtures relative to homing switches | ???                                    |
| G73          | Machine specific drill operation                                  | X,Y,Z,R,Q                              |
| G81          | Machine specific drill operation                                  | X,Y,Z,R,Q                              |
| G82          | Machine specific drill operation                                  | X,Y,Z,R,Q                              |
| G83          | Machine specific drill operation                                  | X,Y,Z,R,Q                              |
| G90          | Absolute coordinates                                              |                                        |
| G91          | Relative coordinates                                              |                                        |
| G98          | Return to initial Z level in canned cycle                         |                                        |
| G99          | Return to R level in canned cycle                                 |                                        |
| M0, M00      | Compulsory stop                                                   |                                        |
| M1, M01      | Optional stop                                                     |                                        |
| M3, M03      | Spindle on (clockwise rotation)                                   | S\<rounds per minute\>                 |
| M4, M04      | Spindle on (counterclockwise rotation)                            | S\<rounds per minute\>                 |
| M6, M06      | Tool change                                                       | T\<tool number\>                       |
| (\<String\>) | comment                                                           |                                        |
