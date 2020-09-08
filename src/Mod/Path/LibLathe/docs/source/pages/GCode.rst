GCode
=====

The fundamental purpose of LibLathe is to generate a program code or tool path
commonly refered to as `GCode`_.

.. _GCode: https://en.wikipedia.org/wiki/G-code

GCode is typically a sequential list of commands that explain to the machine
where to move the current tool in order to create the desired shape. 

   +-----+-----------------------------------+
   | Common Lathe GCodes                     |
   +=====+===================================+
   | G0  |  Rapid Motion                     |
   +-----+-----------------------------------+
   | G1  |  Linear Motion                    |
   +-----+-----------------------------------+
   | G2  |  Clockwise Arc                    |
   +-----+-----------------------------------+
   | G3  |  Anti-Clockwise Arc               |
   +-----+-----------------------------------+  
   | G18 |  Reference Plane XZ               |
   +-----+-----------------------------------+
   | G20 |  Inch Units                       |
   +-----+-----------------------------------+  
   | G21 |  Metric (mm) Units                |
   +-----+-----------------------------------+ 
   | G28 |  Return to Home Position          |
   +-----+-----------------------------------+
   | G32 |  Constant Lead Threading Cycle    |
   +-----+-----------------------------------+  
   | G70 |  Canned Finishing Cycle           |
   +-----+-----------------------------------+
   | G71 |  Canned Roughing Cycle            |
   +-----+-----------------------------------+
   | G72 |  Canned Facing Cycle              |
   +-----+-----------------------------------+
   | G73 |  Canned Pattern Cycle             |
   +-----+-----------------------------------+
   | G74 |  Canned Peck Drilling             |
   +-----+-----------------------------------+
   | G75 |  Canned Grooving Cycle            |
   +-----+-----------------------------------+
   | G76 |  Canned Threading Cycle           |
   +-----+-----------------------------------+
   | G98 |  Feedrate/Minute                  |
   +-----+-----------------------------------+ 
   | G99 |  Feedrate/Cycle                   |
   +-----+-----------------------------------+ 
   
   **Canned Cycle**: Canned cycles are preprogrammed operations that are created 
   from variables. Canned cycles usually perform repeative tasks such as peck
   drilling or threading where a number of repeat passes are made.   
   
   Example GCode File:

