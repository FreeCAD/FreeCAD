# STATUS:
PROPOSED

# Why it is a priority

*(Give some justification why this is important)*
- Helices are used in at least three different places; Helix Op, Ramp entry, and Adaptive clearing.
- This is a lot of duplicated code with inconsistent features. Centralizing the logic to a helix generator would allow us to first put it under unit tests and then incrementally replace the duplicated logic in each place where it is used.
- This will provide a more robust solution that is easier to extend and maintain and also give the users a consistent set of features related to helices everywhere
# Scope

*(think about what's in and out to keep the scope narrow)*

| In                                            | Out                    |
| --------------------------------------------- | ---------------------- |
| Add multiple passes to helix generator        | Spiral pocket strategy |
| Replace adaptive helical entry with generator |                        |
| Replace helix ramp entry code with generator  |                        |
|                                               |                        |

# Related Epics
*(list any other epics (draft, active, delayed) that relate)*
- LeadIn
