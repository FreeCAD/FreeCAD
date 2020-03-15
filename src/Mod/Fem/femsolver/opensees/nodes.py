
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function


# Author(s): Andrew Liew (github.com/andrewliew)


__all__ = [
    'Nodes',
]


class Nodes(object):

    def __init__(self):
        pass

    def write_nodes(self):

        header = {
            'abaqus': '**\n*NODE, NSET=nset_all\n**',
            'opensees': '#',
            'ansys': '!',
        }

        self.prefix = {
            'abaqus': '',
            'opensees': 'node ',
            'ansys': '',
        }

        self.write_section('Nodes')
        self.write_line(header[self.software])

        for key in sorted(self.femnodes_mesh, key=int):

            self.write_node(key)

        # if self.software == 'opensees':
        #     self.blank_line()
        #     for key in sorted(self.structure.nodes, key=int):
        #         if self.structure.nodes[key].mass:
        #             self.write_mass(key)

        self.blank_line()
        self.blank_line()

    def write_node(self, key):

        prefix = self.prefix[self.software]
        spacer = self.spacer[self.software]
        vector = self.femnodes_mesh[key]
        x, y, z = vector.x, vector.y, vector.z

        line = '{0}{1}{2}{3:.3f}{2}{4:.3f}{2}{5:.3f}'.format(prefix, key, spacer, x, y, z)
        self.write_line(line)

    def write_mass(self, key):

        mr = '' if self.ndof == 3 else '0 0 0'
        line = 'mass {0} {1} {1} {1} {2}'.format(key + 1, self.structure.nodes[key].mass, mr)
        self.write_line(line)
