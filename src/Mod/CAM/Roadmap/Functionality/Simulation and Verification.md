# 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

| Feature                | Description                                            | Assessment                                                                                                                                                          |
| ---------------------- | ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Toolpath Visualization | Display toolpath in context of model and stock         | Done.  Arrow indicators on segments are too small to be useful.                                                                                                     |
| Toolpath Inspection    | Allow user to inspect/explore segments of the toolpath | Done. <br>Inspection shows the raw internal comands for the segments.  These commands are not in the correct unit schema and do not reflect the postprocessed code. |
|                        |                                                        |                                                                                                                                                                     |

---

# 🟨 Professional Grade
*Features usually present or expected in the state-of-the art applications*

| Feature                      | Description                                                                                                                                                                                   | Assessment                                                            |
| ---------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------- |
| Job Origin Indicator         | Visual reference showing origin of the job                                                                                                                                                    | Shows in the 3D scene if enabled.<br>Does not show in the simulators. |
| Simulation: Tool & Model     | Realistically render tool, stock, and models                                                                                                                                                  | Partially done. <br>Tool representation is incomplete.                |
| Simulation: Tool Motion      | Show representative tool movement.<br>Movement is sufficient to verify the tool path accuracy but does not reflect every movement.  For example, may not show every peck movement in a cycle. | DONE                                                                  |
| Simulation: Material Removal | Show realistic material removal in simulation                                                                                                                                                 | DONE                                                                  |
| Simulation Speed Control     | Speed up or slow down simulation for inspection                                                                                                                                               | DONE                                                                  |
| Collision Detection          | Detect collisions between tool, stock, and model                                                                                                                                              | NO                                                                    |
| Error Detection              | Identify common errors like gouging, overcutting, missed areas                                                                                                                                | NO                                                                    |
| Consistent UI/UX             | The UI scene navigation (pan, zoom, rotate) should follow the application mouse model                                                                                                         | Behavior is inconsistent                                              |

---

# 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature            | Description                                                         | Assessment |
| ------------------ | ------------------------------------------------------------------- | ---------- |
| Machine Simulation | Simulation accurately reflects actual machine behavior and movement |            |
| Job estimates      | Estimates of tool path completion time are accurate to within 1%    |            |
|                    |                                                                     |            |
