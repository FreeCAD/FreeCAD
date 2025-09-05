# 🟩 Core Essentials
*Features present or expected in a basic CAM package and necessary to perform the required tasks*

| Feature              | Description                                        | Assessment                                                                                                                     |
| -------------------- | -------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------ |
| Python/Macro Support | Allow scripting or automation via Python or macros | Limited.<br>Some CAM functions require the GUI to be loaded.  <br>No examples shipped with FreeCAD.<br>No examples on the wiki |

---

# 🟨 Professional Grade
*Features usually present or expected in the state-of-the art applications*

| Feature              | Description                                                  | Assessment                                                                                                                                                                                                                      |
| -------------------- | ------------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Parametric Workflows | Operations update automatically in response to model changes | Partial. <br>Recomputes are inconsistent. Sometimes triggered when not needed.  Sometimes require manual recompute.<br>Recomputes are inefficient.  Cannot, for example, recompute just the dressup or just the finishing pass. |

---

# 🟦 Next-Level CAM
*Features that would exceed industry standard*

| Feature                       | Description                                           | Assessment |
| ----------------------------- | ----------------------------------------------------- | ---------- |
| Headless Batch Jobs           | Run toolpath generation/export without UI interaction | Limited    |
| External Workflow Integration | Embed CAM pipeline into other automated systems       | None       |
