## Generator

Generators are low-level functions that compute the cutting moves for a single target.
They have no user interface and are implemented as pure functions.  
Generators should have 100% test coverage at all times because they may be called from multiple operation contexts.

For example, creating a helical move may be done
- By the helix operation
- As an entry move to a pocketing operation

### Current Generators
|                 |                                          |
| ---------       |                   ---                    |
| Drill Generator | Generates canned cycle drilling commands |
| Helix           |              helical paths               |
| Rotation        |  Generates rotation moves of A & B axes  |
| Tapping         |                                          |
| threadmilling   |                                          |
| toolchange      |                                          |
| dogboneII       | used by dogbone dressup                  |

### Needed Generators
|                   |                                              |
| ---------         | ---                                          |
| Pocket (zigzag)   |                                              |
| Pocket (adaptive) |                                              |
| Pocket (offset)   |                                              |
| Line              |                                              |
| Linking           | Generate linking moves between cutting paths |
| vcarve            |                                              |
|                   |                                              |
