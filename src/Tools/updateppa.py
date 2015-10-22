#!/usr/bin/python
# -*- coding: utf-8 -*-
# (c) 2012 Werner Mayer LGPL

import sys
from os import chdir
from os import path
from tempfile import gettempdir

from bzrlib.transport import get_transport
from bzrlib.branch import Branch
from bzrlib.workingtree import WorkingTree

def runUpdate(filename):
    branch = "versioning.git"
    REMOTE_URL="bzr+ssh://bazaar.launchpad.net/~freecad-maintainers/freecad/%s" % (branch)
    PUSHTO_URL="bzr+ssh://bazaar.launchpad.net/~freecad-maintainers/freecad/%s" % (branch)
    LOCAL_BRANCH=path.join(gettempdir(),branch)

    # Location of branch on Launchpad
    remote_branch = Branch.open(REMOTE_URL)

    # Location of branch on local system
    local_branch = remote_branch.bzrdir.sprout(LOCAL_BRANCH).open_branch()

    # Change a file in the local branch
    try:
        wf = open(LOCAL_BRANCH + "/src/Build/Version.h", 'w')
        rf = open(filename, 'r')
    except IOError, error:
        raise IOError, error
    else:
        wf.write(rf.read())
        wf.close()

    # Commit the change
    tree = WorkingTree.open(LOCAL_BRANCH)
    tree.commit("Update version number")

    # Push back to Launchpad
    #transport = get_transport(PUSHTO_URL)
    #local_branch.create_clone_on_transport(transport)

def main():
    runUpdate(sys.argv[1])
        
if __name__ == "__main__":
    main()


