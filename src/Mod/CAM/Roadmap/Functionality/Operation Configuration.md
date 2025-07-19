# Requirements
## 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

### All operations
- Configure key operation Parameters
	- spindle speed
	- feed rate
    - Allow user to set the start and end depth
    - Tool to use

### Pocketing
- provide helical and ramp entry
- Allow user to specify entry start point
- Allow user to configure the clearing strategy
    - zigzag
    - offset
    - Adaptive
- Allow the user to set remaining material to leave to be removed in a
  finishing pass
- Allow the user to control the amount of material removed per pass (stepover)

### Contouring (Profile)
- Specify entry/exit strategies
- Allow the user to set remaining material to leave to be removed in a
finishing pass
- Allow the user to add holding tags to keep the part retained

### Circular Holes
Capable of generating canned cycle drilling actions which correspond to

| Gcode | Canned Cycle                      |     |
|-------|-----------------------------------|-----|
| G81   | Drilling Cycle                    |     |
| G82   | Drilling Cycle, Dwell             |     |
| G83   | Peck Drilling Cycle               |     |
| G73   | Drilling Cycle with Chip Breaking |     |

- Capable of handling multiple drill targets in the same operation
- Allow the user to sort the drill target order
- Support drill targets in faces that are separated by an obstruction


### Engraving


## 🟨 Professional Grade
*Features usually present or expected in the state-of-the art applications*

### All operations
- Allow user to set relevent depths by clicking on features of the model or
  stock

### Pocketing
- Functionality to configure pocketing operations that target material left
  behind by previous operations.  (REST milling)

### Contour (profile)
- Permit multiple pass or kerf widening passes
- Allow user to customize the size and shape of holding tags

### Engraving

### Circular Holes
- Automatically optimize target order for efficiency
- Canned cycles should be as efficient as possible with minimal wasted travel


Capable of generating canned cycle drilling actions which correspond to
all basic canned cycles above and

#### Boring
| Gcode | Canned Cycle                               |   |
|-------|--------------------------------------------|---|
| G85   | Boring Cycle, Feed Out                     |   |
| G86   | Boring Cycle, Spindle Stop, Rapid Move Out |   |
| G87   | Back Boring Cycle                          |   |
| G88   | Boring Cycle, Spindle Stop, Manual Out     |   |
| G89   | Boring Cycle, Dwell, Feed Out              |   |

#### Tapping

| Gcode | Canned Cycle                       |     |
|-------|-----------------------------------|-----|
| G33.1 | Rigid Tapping                      |     |
| G74   | Left-hand Tapping Cycle with Dwell |     |
| G84   | Right-hand Tapping Cycle, Dwell    |     |
|       |                                    |     |

### threadmilling

### Vcarving

### Deburring

### Helical clearing

### Adaptive clearing

### Lathe operations

### Indexed Multi-Axis operations

### Continuous 4th and 5th Axis

## 🟦 Next-Level CAM
*Features that would exceed industry standard*
