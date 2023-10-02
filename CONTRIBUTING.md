# FreeCAD Contribution Process (FCP)

FreeCAD's contribution process is inspired by the Collective Code Construction Contract which itself is an evolution of the github.com Fork and Pull Model.

The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT", "SHOULD", "SHOULD NOT", "RECOMMENDED",  "MAY", and "OPTIONAL" in this document are to be interpreted as described in RFC 2119.


## 0. Status

FreeCAD is in a transition period. The following are to be regarded as GUIDELINES for contribution submission and acceptance. For historical reasons, the actual process MAY diverge from this process during the transition.  Such deviations SHOULD be noted and discussed whenever possible.

## 1. Goals

The FreeCAD Contribution Process is expressed here with the following specific goals in mind:

1. To provide transparency and fairness in the contribution process.
2. To allow contributions to be included as quickly as possible.
3. To preserve and improve the code quality while encouraging appropriate experimentation and risk-taking.
4. To minimize dependence on individual Contributors by encouraging a large pool of active Contributors.
5. To be inclusive of many viewpoints and to harness a diverse set of skills.
6. To provide an encouraging environment where Contributors learn and improve their skills.
7. To protect the free and open nature of the FreeCAD project.

## 2. Fundamentals

1. FreeCAD uses the git distributed revision control system.
2. Source code for the main application and related subprojects is hosted on github.com in the FreeCAD organization.
3. Problems are discrete, well-defined limitations or bugs.
4. FreeCAD uses GitHub's issue-tracking system to track problems and contributions. For help requests and general discussions, use the project forum. 
5. Contributions are sets of code changes that resolve a single problem.
6. FreeCAD uses the Pull Request workflow for evaluating and accepting contributions.

## 3. Roles
1. "User": A member of the wider FreeCAD community who uses the software.
2. "Contributor": A person who submits a contribution that resolves a previously identified problem. Contributors do not have commit access to the repository unless they are also Maintainers. Everyone, without distinction or discrimination, SHALL have an equal right to become a Contributor.
3. "Maintainer": A person who merges contributions. Maintainers may or may not be Contributors. Their role is to enforce the process. Maintainers have commit access to the repository.
4. "Administrator": Administrators have additional authority to maintain the list of designated Maintainers.

## 4. Licensing, Ownership, and Credit
1. FreeCAD is distributed under the Lesser General Public License, version 2, or superior (LGPL2+).  Additional details can be found in the LICENSE file.
2. All contributions to FreeCAD MUST use a compatible license.
3. All contributions are owned by their authors unless assigned to another.
4. FreeCAD does not have a mandatory copyright assignment policy.
5. A Contributor who wishes to be identified in the Credits section of the application "About" dialog is responsible for identifying themselves. They should modify the Contributors file and submit a PR with a single commit for this modification only. The contributors file is found at https://github.com/FreeCAD/FreeCAD/blob/master/src/Doc/CONTRIBUTORS
6. A contributor who does not wish to assume the copyright of their contribution MAY choose to assign it to the [FreeCAD project association](https://fpa.freecad.org) by mentioning **Copyright (c) 2022 The FreeCAD project association <fpa@freecad.org>** in the file's license code block.

## 5. Contribution Requirements

1. Contributions are submitted in the form of Pull Requests (PR).
2. Maintainers and Contributors MUST have a GitHub account and SHOULD use their real names or a well-known alias. 
3. If the GitHub username differs from the username on the FreeCAD Forum, effort SHOULD be taken to avoid confusion.
4. A PR SHOULD be a minimal and accurate answer to exactly one identified and agreed-on problem.
5. A PR SHOULD refrain from adding additional dependencies to the FreeCAD project unless no other option is available.
6. Code submissions MUST adhere to the code style guidelines of the project if these are defined.
7. If a PR contains multiple commits, each commit MUST compile cleanly when merged with all previous commits of the same PR. Each commit SHOULD add value to the history of the project. Checkpoint commits SHOULD be squashed.
8. A PR SHALL NOT include non-trivial code from other projects unless the Contributor is the original author of that code.
9. A PR MUST compile cleanly and pass project self-tests on all target platforms.
10. Each commit message in a PR MUST succinctly explain what the commit achieves. The commit message SHALL follow the suggestions in the `git commit --help` documentation, section DISCUSSION.
11. The PR message MUST consist of a single short line, the PR Title, summarizing the problem being solved, followed by a blank line and then the proposed solution in the Body. If a PR consists of more than one commit, the PR Title MUST succinctly explain what the PR achieves. The Body MAY be as detailed as needed.
12. A “Valid PR” is one which satisfies the above requirements.

## 6. Process

1. Change on the project follows the pattern of accurately identifying problems and applying minimal, accurate solutions to these problems.
2. To request changes, a User logs an issue on the project GitHub issue tracker.
3. The User or Contributor SHOULD write the issue by describing the problem they face or observe. Links to the forum or other resources are permitted but the issue SHOULD be complete and accurate and SHOULD NOT require the reader to visit the forum or any other platform to understand what is being described.
4. Issue authors SHOULD strive to describe the minimum acceptable condition.
5. Issue authors SHOULD focus on User tasks and avoid comparisons to other software solutions.
6. The User or Contributor SHOULD seek consensus on the accuracy of their observation and the value of solving the problem.
7. To submit a solution to a problem, a Contributor SHALL create a pull request back to the project.
8. Contributors and Maintainers SHALL NOT commit changes directly to the target branch.
9. To discuss a proposed solution, Users MAY comment on the Pull Request in GitHub. Forum conversations regarding the solution SHOULD be discouraged and conversation redirected to the Pull Request or the related issue.
10. To accept or reject a Pull Request, a Maintainer SHALL use GitHub's interface.
11. Maintainers SHOULD NOT merge their own PRs except:
    1. in exceptional cases, such as non-responsiveness from other Maintainers for an extended period.
    2. If the Maintainer is also the primary developer of the workbench or subsystem.

12. Maintainers SHALL merge valid PRs from other Contributors rapidly.
13. Maintainers MAY, at their discretion merge PRs that have not met all criteria to be considered valid to:
    1. end fruitless discussions
    2. capture toxic contributions in the historical record
    3. engage with the Contributor on improving their contribution quality.
14. Maintainers SHALL NOT make value judgments on correct contributions.
15. Any Contributor who has value judgments on a PR SHOULD express these via their own PR.
16. The User who created an issue SHOULD close the issue after checking the PR is successful.
17. Maintainers SHOULD close issues that are left open without action or update for an unreasonable period.

## 7. Branches and Releases

1. The project SHALL have one branch (“master”) that always holds the latest in-progress version and SHOULD always build.
2. The project SHALL NOT use topic branches for any reason. Personal forks MAY use topic branches.
3. To make a stable release a Maintainer SHALL tag the repository. Stable releases SHALL always be released from the repository master.

## 8. Project Administration

1. Project Administrators are those individuals who are members of the FreeCAD Github organization and have the role of 'owner'.  They have the task of administering the organization including adding and removing individuals from various teams.
2. Project Administrator is a technical role necessitated by the GitHub platform. Except for the specific exceptions listed below, the Project Administrators do not make the decision about individual team members. Rather, they carry out the collective wishes of the Maintainers team. Project Administrators will be selected from the Maintainers team by the Maintainers themselves.
3. To ensure continuity there SHALL be at least four Project Administrators at all times.
4. The project Administrators will manage the set of project Maintainers.  They SHALL maintain a sufficiently large pool of Maintainers to ensure their succession and permit timely review of contributions. If the pool of Maintainers is insufficient, the Project Administrators will request that the Maintainers select additional individuals to add.
5. Contributors who have a history of successful PRs and have demonstrated continued professionalism should be invited to be Maintainers.
6. Administrators SHOULD remove Maintainers who are inactive for an extended period, or who repeatedly fail to apply this process accurately.
7. The list of Maintainers SHALL be publicly accessible and reflective of current activity on the project.
8. Administrators SHALL act expediently to protect the FreeCAD infrastructure and resources.
9. Administrators SHOULD block or ban “bad actors” who cause stress, animosity, or confusion to others in the project. This SHOULD be done after public discussion, with a chance for all parties to speak. A bad actor is someone who repeatedly ignores the rules and culture of the project, who is hostile or offensive, who impedes the productive exchange of information, and who is unable to self-correct their behavior when asked to do so by others.
