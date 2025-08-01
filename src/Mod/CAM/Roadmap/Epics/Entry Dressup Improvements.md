# STATUS:
PROPOSED

# Why it is a priority

*(Give some justification why this is important)*
- Entry dressups are a frequent source of complaints.  The behavior is inconsistent and buggy
- Entry actions are necessary on most pocket operations and most profiles.  The current implementation of the workflow is cumbersome and slow
- Ramp and leadin/out are implemented as separate dressup. This is sometimes confusing to users.  Potentially,  these could be consolidated into a single dressup with multiple strategies possible.
- Ramp lacks a UI task panel.
- Ramp also has a helical entry strategy that duplicates code from helix and from adaptive operation.  Adaptive operation has its own helix entry method.  The result is a confusing jumble of functionality and code that is hard to work on
# Scope

*(think about what's in and out to keep the scope narrow)*

| In                                                                           | Out |
| ---------------------------------------------------------------------------- | --- |
| Evaluation of feasibility to add entry dressups directly in parent operation |     |
| Evaluation of feasibility to consolidate ramp and leadin/out                 |     |
| Ramp task panel                                                              |     |
|                                                                              |     |

# Related Epics
*(list any other epics (draft, active, delayed) that relate)*
- helix
- pocket improvements
