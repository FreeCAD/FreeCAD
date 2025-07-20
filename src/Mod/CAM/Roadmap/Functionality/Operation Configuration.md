# Requirements

## 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

### All Operations

| Feature | Description | Assessment |
|--------|-------------|------------|
| Operation Parameters | Configure spindle speed, feed rate, start/end depth, tool selection | |

### Pocketing

| Feature | Description | Assessment |
|--------|-------------|------------|
| Entry Strategies | Provide helical and ramp entry | |
| Entry Start Point | Allow user to specify entry point | |
| Clearing Strategies | zigzag, offset, adaptive | |
| Stock to Leave | Set material to leave for finishing pass | |
| Stepover Control | Set amount of material removed per pass | |

### Contouring (Profile)

| Feature | Description | Assessment |
|--------|-------------|------------|
| Entry/Exit Strategies | Specify entry/exit paths | |
| Stock to Leave | Set material to leave for finishing | |
| Holding Tags | Add tags to retain part | |

### Circular Holes

| Feature | Description | Assessment |
|--------|-------------|------------|
| Canned Cycles | Support G81, G82, G83, G73 | |
| Multiple Targets | Handle multiple drill locations in one op | |
| Drill Ordering | Allow user-defined target order | |
| Obstructed Faces | Support targets across obstructions | |

| Gcode | Canned Cycle                      | Assessment |
|-------|-----------------------------------|------------|
| G81   | Drilling Cycle                    | |
| G82   | Drilling Cycle, Dwell             | |
| G83   | Peck Drilling Cycle               | |
| G73   | Chip Breaking Drill               | |

### Engraving

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

---

## 🟨 Professional Grade
*Features usually present or expected in the state-of-the-art applications*

### All Operations

| Feature | Description | Assessment |
|--------|-------------|------------|
| Model-Based Depth Selection | Set relevant depths by clicking features of model or stock | |

### Pocketing

| Feature | Description | Assessment |
|--------|-------------|------------|
| REST Milling | Target leftover material from previous ops | |

### Contouring (Profile)

| Feature | Description | Assessment |
|--------|-------------|------------|
| Multiple Passes | Support multi-pass or kerf-widening | |
| Holding Tag Shape | Customize size/shape of holding tabs | |

### Engraving

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Circular Holes

| Feature | Description | Assessment |
|--------|-------------|------------|
| Order Optimization | Automatically sort drill order for efficiency | |
| Efficient Cycles | Minimize wasted travel in canned cycles | |

#### Boring Cycles

| Gcode | Canned Cycle                               | Assessment |
|-------|--------------------------------------------|------------|
| G85   | Boring Cycle, Feed Out                     | |
| G86   | Spindle Stop, Rapid Out                    | |
| G87   | Back Boring Cycle                          | |
| G88   | Spindle Stop, Manual Out                   | |
| G89   | Dwell, Feed Out                            | |

#### Tapping Cycles

| Gcode | Canned Cycle                                | Assessment |
|-------|---------------------------------------------|------------|
| G33.1 | Rigid Tapping                               | |
| G74   | Left-hand Tapping with Dwell                | |
| G84   | Right-hand Tapping with Dwell               | |

### Threadmilling

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### V-Carving

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Deburring

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Helical Clearing

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Adaptive Clearing

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Lathe Operations

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Indexed Multi-Axis

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

### Continuous 4th and 5th Axis

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(To be defined)* | |

---

## 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature | Description | Assessment |
|--------|-------------|------------|
| *(placeholder)* | *(Add high-end, futuristic CAM ideas here)* | |
