
# STATUS:
PROPOSED

# Why it is a priority
- Moving between circular hole targets has complexity not present in other operations.  Addressing the shortcomings of linking moves for circular hole operations will allow this logic to be reused in other operations.
- Handling of target selection get be generalized to other operations and has overlapping functionality with Sanity
- Circular holes generates more complex path commands that need special handling in the simulator
- Refactoring Post-processor logic should consider centralizing the handling and decomposition of circular hole commands
# Scope

| In                                                      | Out                                                      |
| ------------------------------------------------------- | -------------------------------------------------------- |
| Create a linking move generator                         | Adding new circular hole operation (Bore,  reaming, etc) |
| Improved target selection and management                |  Improve simulator to visualize circular hole processing |
| refactor circular hole base to use linking generator    |                                                          |
| Improve Base post-processors to generate canned cycles  |                                                          |
| Improve feedrate calculation for helix and threadmill   |                                                          |
| Add target order column to target list                  |                                                          |


# Related Epics
- Helix Work
