import os
import sys
import subprocess
import pprint

SYS_PATHS = ["/System/","/usr/lib/"]

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
        
class DepsGraph:
    graph = {}

    def in_graph(self, node):
        return node.name in self.graph.keys()
    
    def add_node(self, node):
        self.graph[node.name] = node

    def get_node(self, name):
        if self.graph.has_key(name):
            return self.graph[name]
        return None

    def visit(self, operation, op_args=[]):
        """"
        Preform a depth first visit of the graph, calling operation
        on each node.
        """
        stack = []
          
        for k in self.graph.keys():
            self.graph[k]._marked = False
            
        for k in self.graph.keys():
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
    output = subprocess.check_output(["file",path])
    if output.count("Mach-O") != 0:
        return True
    
    return False

def is_system_lib(lib):
    for p in SYS_PATHS:
        if lib.startswith(p):
            return True
    return False

def get_path(name, search_paths):
    for path in search_paths:
        if os.path.isfile(os.path.join(path, name)):
            return path
    return None

def list_install_names(path_macho):
    output = subprocess.check_output(["otool", "-L", path_macho])
    lines = output.split("\t")
    libs = []

    #first line is the the filename, and if it is a library, the second line
    #is the install name of it
    if path_macho.endswith(os.path.basename(lines[1].split(" (")[0])):
        lines = lines[2:]
    else:
        lines = lines[1:]

    for line in lines:
        lib = line.split(" (")[0]
        if not is_system_lib(lib):
            libs.append(lib)
    return libs

def library_paths(install_names, search_paths):
    paths = []
    for name in install_names:
        path = os.path.dirname(name)
        lib_name = os.path.basename(name)
            
        if path == "" or name[0] == "@":
            #not absolute -- we need to find the path of this lib
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
        
        #even if install_path is absolute, see if library can be found by 
        #searching search_paths, so that we have control over what library
        #location to use 
        path = get_path(lib_name, search_paths)

        if install_path != "" and lib[0] != "@":
            #we have an absolte path install name
            if not path:
                path = install_path
       
        if not path:
            raise LibraryNotFound("{0} not found in given paths".format(lib_name))

        nodes.append(Node(lib_name, path))
    
    return nodes

def paths_at_depth(prefix, paths, depth):
    filtered = []
    for p in paths:
        dirs = os.path.join(prefix, p).strip('/').split('/')
        if len(dirs) == depth:
            filtered.append(p)
    return filtered
    

def should_visit(prefix, path_filters, path):
    s_path = path.strip('/').split('/')
    filters = []
    #we only want to use filters if they have the same parent as path 
    for rel_pf in path_filters:
        pf = os.path.join(prefix, rel_pf)
        #print(path)
        #print(pf)
        #print(os.path.split(pf)[0] == os.path.split(path)[0])
        #print('----')
        if os.path.split(pf)[0] == os.path.split(path)[0]:
            filters.append(pf)
    if not filters:
        #no filter that applies to this path
        return True

    for pf in filters:
        s_filter = pf.strip('/').split('/')
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
    #make a local copy since we add to it
    s_paths = list(search_paths)
    
    visited = {}

    for root, dirs, files in os.walk(bundle_path):
        if dirs_filter != None:
            dirs[:] = [d for d in dirs if should_visit(bundle_path, dirs_filter, 
                                                       os.path.join(root, d))]

        s_paths.insert(0, root)
        
        for f in files:
            fpath = os.path.join(root, f)
            ext = os.path.splitext(f)[1]
            if (ext == "" and is_macho(fpath)) or ext == ".so" or ext == ".dylib":
                visited[fpath] = False

    stack = []
    for k in visited.keys():
        if not visited[k]:
            stack.append(k)
            while stack:
                k2 = stack.pop()
                visited[k2] = True

                node = Node(os.path.basename(k2), os.path.dirname(k2))
                if not graph.in_graph(node):
                    graph.add_node(node)
                 
                deps = create_dep_nodes(list_install_names(k2), s_paths)
                for d in deps:
                    if d.name not in node.children:
                        node.children.append(d.name)

                    dk = os.path.join(d.path, d.name)
                    if dk not in visited.keys():
                        visited[dk] = False
                    if not visited[dk]:
                        stack.append(dk)

                

def in_bundle(lib, bundle_path):
    if lib.startswith(bundle_path):
        return True
    return False

def copy_into_bundle(graph, node, bundle_path):
    if not in_bundle(node.path, bundle_path):
        subprocess.check_call(["cp","-L",os.path.join(node.path,node.name), 
                               os.path.join(bundle_path,"lib",node.name)])
        node.path = os.path.join(bundle_path,"lib")
        
        #fix permissions
        subprocess.check_call(["chmod", "a+w", os.path.join(bundle_path,"lib",node.name)])
 
def add_rpaths(graph, node, bundle_path):
    if node.children:
        lib = os.path.join(node.path, node.name)
        if in_bundle(lib, bundle_path):
            install_names = list_install_names(lib)
            rpaths = []

            for install_name in install_names:
                name = os.path.basename(install_name)
                #change install names to use rpaths
                subprocess.check_call(["install_name_tool","-change", 
                    install_name, "@rpath/" + name, lib]) 
                  
                dep_node = node.children[node.children.index(name)]
                rel_path = os.path.relpath(graph.get_node(dep_node).path, node.path)
                rpath = ""
                if rel_path == ".":
                    rpath = "@loader_path/"
                else:
                    rpath = "@loader_path/" + rel_path + "/"
                if rpath not in rpaths:
                    rpaths.append(rpath)
            for path in rpaths:
                subprocess.call(["install_name_tool", "-add_rpath", path, lib])

def print_node(graph, node):
    print(os.path.join(node.path, node.name))
    
def main():
    path = sys.argv[1]
    bundle_path = os.path.abspath(os.path.join(path, "Contents"))
    graph = DepsGraph()
    dir_filter = ["bin","lib", "Mod","Mod/PartDesign", 
                  "lib/python2.7/site-packages",
                  "lib/python2.7/lib-dynload"]
    search_paths = [bundle_path + "/lib", "/usr/local/lib", "/usr/local/Cellar/freetype/2.5.4/lib"]
    
    build_deps_graph(graph, bundle_path, dir_filter, search_paths)

    #graph.visit(print_node)
    graph.visit(copy_into_bundle, [bundle_path])
    graph.visit(add_rpaths, [bundle_path])

if __name__ == "__main__":
    main()
