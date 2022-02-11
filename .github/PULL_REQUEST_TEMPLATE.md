Thank you for creating a pull request to contribute to FreeCAD! To ease integration, we ask you to conform to the following items. Pull requests which don't satisfy all the items below might be rejected. If you are in doubt with any of the items below, don't hesitate to ask for help in the [FreeCAD forum](https://forum.freecadweb.org/viewforum.php?f=10)!

- [ ]  Your pull request is confined strictly to a single module. That is, all the files changed by your pull request are either in `App`, `Base`, `Gui` or one of the `Mod` subfolders. If you need to make changes in several locations, make several pull requests and wait for the first one to be merged before submitting the next ones
- [ ]  In case your pull request does more than just fixing small bugs, make sure you discussed your ideas with other developers on the FreeCAD forum
- [ ]  Your branch is [rebased](https://git-scm.com/docs/git-rebase) on latest master `git pull --rebase upstream master`
- [ ]  All FreeCAD unit tests are confirmed to pass by running `./bin/FreeCAD --run-test 0`
- [ ]  All commit messages are [well-written](https://chris.beams.io/posts/git-commit/) ex: `Fixes typo in Draft Move command text`
- [ ]  Your pull request is well written and has a good description, and its title starts with the module name, ex: `Draft: Fixed typos`
- [ ]  Commit messages include `issue #<id>` or `fixes #<id>` where `<id>` is the issue ID number from our [Issues database](https://github.com/FreeCAD/FreeCAD/issues) in case a particular commit solves or is related to an existing issue. Ex: `Draft: fix typos - fixes #4805`

And please remember to update the Wiki with the features added or changed once this PR is merged.  
**Note**: If you don't have wiki access, then please mention your contribution on the [0.20 Changelog Forum Thread](https://forum.freecadweb.org/viewtopic.php?f=10&t=56135).

---
