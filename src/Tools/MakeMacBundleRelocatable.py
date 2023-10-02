import os
import sys
from subprocess import check_call, check_output
import re
import logging

# This script is intended to help copy dynamic libraries used by FreeCAD into
# a Mac application bundle and change dyld commands as appropriate.  There are
# two key items that this currently does differently from other similar tools:
#
#  * @rpath is used rather than @executable_path because the libraries need to
#    be loadable through a Python interpreter and the FreeCAD binaries.
#  * We need to be able to add multiple rpaths in some libraries.

# Assume any libraries in these paths don't need to be bundled
systemPaths = [
    "/System/",
    "/usr/lib/",
    "/Library/Frameworks/3DconnexionClient.framework/",
]

# If a library is in these paths, but not systemPaths, a warning will be
# issued and it will NOT be bundled.  Generally, libraries installed by
# MacPorts or Homebrew won't end up in /Library/Frameworks, so we assume
# that libraries found there aren't meant to be bundled.
warnPaths = ["/Library/Frameworks/"]


class LibraryNotFound(Exception):
    pass


class Node:
    """
    self.path should be an absolute path to self.name
    """

    def __init__(self, name, path="", children=None):
        self.name = name
        self.path = path
        if not children:
            children = list()
        self.children = children
        self._marked = False

    def __eq__(self, other):
        if not isinstance(other, Node):
            return False
        return self.name == other.name

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.name)

    def __str__(self):
        return self.name + " path: " + self.path + " num children: " + str(len(self.children))


class DepsGraph:
    graph = {}

    def in_graph(self, node):
        return node.name in list(self.graph)

    def add_node(self, node):
        self.graph[node.name] = node

    def get_node(self, name):
        if name in self.graph:
            return self.graph[name]
        return None

    def visit(self, operation, op_args=[]):
        """ "
        Perform a depth first visit of the graph, calling operation
        on each node.
        """
        stack = []

        for k in list(self.graph):
            self.graph[k]._marked = False

        for k in list(self.graph):
            if not self.graph[k]._marked:
                stack.append(k)
                while stack:
                    node_key = stack.pop()
                    self.graph[node_key]._marked = True
                    for ck in self.graph[node_key].children:
                        if not self.graph[ck]._marked:
                            stack.append(ck)
                    operation(self, self.graph[node_key], *op_args)


def is_macho(path):
    return b"Mach-O" in check_output(["file", path])


def get_token(txt, delimiter=" (", first=True):
    result = txt.decode().split(delimiter)
    if first:
        return result[0]
    else:
        return result


def is_system_lib(lib):
    for p in systemPaths:
        if lib.startswith(p):
            return True
    for p in warnPaths:
        if lib.startswith(p):
            logging.warning("WARNING: library %s will not be bundled!" % lib)
            logging.warning("See MakeMacRelocatable.py for more information.")
            return True
    return False


def get_path(name, search_paths):
    for path in search_paths:
        if os.path.isfile(os.path.join(path, name)):
            return path
    return None


def list_install_names(path_macho):
    output = check_output(["otool", "-L", path_macho])
    lines = output.split(b"\t")
    libs = []

    # first line is the filename, and if it is a library, the second line
    # is the install name of it
    if path_macho.endswith(os.path.basename(get_token(lines[1]))):
        lines = lines[2:]
    else:
        lines = lines[1:]

    for line in lines:
        lib = get_token(line)
        if not is_system_lib(lib):
            libs.append(lib)
    return libs


def library_paths(install_names, search_paths):
    paths = []
    for name in install_names:
        path = os.path.dirname(name)
        lib_name = os.path.basename(name)

        if path == "" or name[0] == "@":
            # not absolute -- we need to find the path of this lib
            path = get_path(lib_name, search_paths)

        paths.append(os.path.join(path, lib_name))

    return paths


def create_dep_nodes(install_names, search_paths):
    """
    Return a list of Node objects from the provided install names.
    """
    nodes = []
    for lib in install_names:
        install_path = os.path.dirname(lib)
        lib_name = os.path.basename(lib)

        # even if install_path is absolute, see if library can be found by
        # searching search_paths, so that we have control over what library
        # location to use
        path = get_path(lib_name, search_paths)

        if install_path != "" and lib[0] != "@":
            # we have an absolute path install name
            if not path:
                path = install_path

        if not path:
            logging.error("Unable to find LC_DYLD_LOAD entry: " + lib)
            raise LibraryNotFound(lib_name + " not found in given search paths")

        nodes.append(Node(lib_name, path))

    return nodes


def paths_at_depth(prefix, paths, depth):
    filtered = []
    for p in paths:
        dirs = os.path.join(prefix, p).strip("/").split("/")
        if len(dirs) == depth:
            filtered.append(p)
    return filtered


def should_visit(prefix, path_filters, path):
    s_path = path.strip("/").split("/")
    filters = []
    # we only want to use filters if they have the same parent as path
    for rel_pf in path_filters:
        pf = os.path.join(prefix, rel_pf)
        if os.path.split(pf)[0] == os.path.split(path)[0]:
            filters.append(pf)
    if not filters:
        # no filter that applies to this path
        return True

    for pf in filters:
        s_filter = pf.strip("/").split("/")
        length = len(s_filter)
        matched = 0
        for i in range(len(s_path)):
            if s_path[i] == s_filter[i]:
                matched += 1
            if matched == length or matched == len(s_path):
                return True

    return False


def build_deps_graph(graph, bundle_path, dirs_filter=None, search_paths=[]):
    """
    Walk bundle_path and build a graph of the encountered Mach-O binaries
    and there dependencies
    """
    # make a local copy since we add to it
    s_paths = list(search_paths)

    visited = {}

    for root, dirs, files in os.walk(bundle_path):
        if dirs_filter is not None:
            dirs[:] = [
                d for d in dirs if should_visit(bundle_path, dirs_filter, os.path.join(root, d))
            ]

        s_paths.insert(0, root)

        for f in files:
            fpath = os.path.join(root, f)
            ext = os.path.splitext(f)[1]
            if (ext == "" and is_macho(fpath)) or ext == ".so" or ext == ".dylib":
                visited[fpath] = False

    stack = []
    for k in list(visited):
        if not visited[k]:
            stack.append(k)
            while stack:
                k2 = stack.pop()
                visited[k2] = True

                node = Node(os.path.basename(k2), os.path.dirname(k2))
                if not graph.in_graph(node):
                    graph.add_node(node)

                try:
                    deps = create_dep_nodes(list_install_names(k2), s_paths)
                except Exception:
                    logging.error("Failed to resolve dependency in " + k2)
                    raise

                for d in deps:
                    if d.name not in node.children:
                        node.children.append(d.name)

                    dk = os.path.join(d.path, d.name)
                    if dk not in list(visited):
                        visited[dk] = False
                    if not visited[dk]:
                        stack.append(dk)


def in_bundle(lib, bundle_path):
    if lib.startswith(bundle_path):
        return True
    return False


def copy_into_bundle(graph, node, bundle_path):
    if not in_bundle(node.path, bundle_path):
        source = os.path.join(node.path, node.name)
        target = os.path.join(bundle_path, "lib", node.name)
        logging.info("Bundling {}".format(source))

        check_call(["cp", "-L", source, target])

        node.path = os.path.dirname(target)

        # fix permissions
        check_call(["chmod", "a+w", target])


def get_rpaths(library):
    "Returns a list of rpaths specified within library"

    out = check_output(["otool", "-l", library])

    pathRegex = r"^path (.*) \(offset \d+\)$"
    expectingRpath = False
    rpaths = []
    for line in get_token(out, "\n", False):
        line = line.strip()

        if "cmd LC_RPATH" in line:
            expectingRpath = True
        elif "Load command" in line:
            expectingRpath = False
        elif expectingRpath:
            m = re.match(pathRegex, line)
            if m:
                rpaths.append(m.group(1))
                expectingRpath = False

    return rpaths


def add_rpaths(graph, node, bundle_path):
    lib = os.path.join(node.path, node.name)

    if in_bundle(lib, bundle_path):
        logging.debug(lib)

        # Remove existing rpaths that could take precedence
        for rpath in get_rpaths(lib):
            logging.debug(" - rpath: " + rpath)
            check_call(["install_name_tool", "-delete_rpath", rpath, lib])

        if node.children:
            install_names = list_install_names(lib)
            rpaths = []

            for install_name in install_names:
                name = os.path.basename(install_name)
                # change install names to use rpaths
                logging.debug(" ~ rpath: " + name + " => @rpath/" + name)
                check_call(
                    [
                        "install_name_tool",
                        "-change",
                        install_name,
                        "@rpath/" + name,
                        lib,
                    ]
                )

                dep_node = node.children[node.children.index(name)]
                rel_path = os.path.relpath(graph.get_node(dep_node).path, node.path)
                rpath = ""
                if rel_path == ".":
                    rpath = "@loader_path/"
                else:
                    rpath = "@loader_path/" + rel_path + "/"
                if rpath not in rpaths:
                    rpaths.append(rpath)

            for rpath in rpaths:
                # Ensure that lib has rpath set
                if not rpath in get_rpaths(lib):
                    logging.debug(" + rpath: " + rpath)
                    check_call(["install_name_tool", "-add_rpath", rpath, lib])


def change_libid(graph, node, bundle_path):
    lib = os.path.join(node.path, node.name)

    logging.debug(lib)

    if in_bundle(lib, bundle_path):
        logging.debug(" ~ id: " + node.name)
        try:
            check_call(["install_name_tool", "-id", node.name, lib])
        except Exception:
            logging.warning("Failed to change bundle id {} in lib {}".format(node.name, lib))


def print_child(graph, node, path):
    logging.debug("  >" + str(node))


def print_node(graph, node, path):
    logging.debug(node)
    graph.visit(print_child, [node])


def main():
    if len(sys.argv) < 2:
        print("Usage " + sys.argv[0] + " path [additional search paths]")
        quit()

    path = sys.argv[1]
    bundle_path = os.path.abspath(os.path.join(path, "Contents"))
    graph = DepsGraph()
    dir_filter = ["MacOS", "lib", "Mod"]
    search_paths = [bundle_path + "/lib"] + sys.argv[2:]

    # change to level to logging.DEBUG for diagnostic messages
    logging.basicConfig(
        stream=sys.stdout, level=logging.INFO, format="-- %(levelname)s: %(message)s"
    )

    logging.info("Analyzing bundle dependencies...")
    build_deps_graph(graph, bundle_path, dir_filter, search_paths)

    if logging.getLogger().getEffectiveLevel() == logging.DEBUG:
        graph.visit(print_node, [bundle_path])

    logging.info("Copying external dependencies to bundle...")
    graph.visit(copy_into_bundle, [bundle_path])

    logging.info("Updating dynamic loader paths...")
    graph.visit(add_rpaths, [bundle_path])

    logging.info("Setting bundled library IDs...")
    graph.visit(change_libid, [bundle_path])

    logging.info("Done.")


if __name__ == "__main__":
    main()
