# STATUS:

Active

# Why it is a priority

The adaptive operation is an important function for users to make constant load
tool paths. The current implementation has missing features and bugs that make
it difficult for users to generate adaptive tool paths effectively.

# Scope



| In  | Out |
| --- | --- |
| Algorithm fixes to reliably generate toolpaths with small stepover                                                     | Rewrite the adaptive algorithm based on new princples to produces arcs instead of lines (too much of a research project; not ready) |
| Improved automatic diameter selection for entry helix, to never fail if it is possible to enter at the chosen point    |  |
| Implement rest machining for adaptive                                                                                  |  |
| Fix adaptive "profile" mode regression                                                                                 |  |
| Refactor operation code to convert adaptive into a pocket/profile fill strategy                                        |  |

# Related Epics
