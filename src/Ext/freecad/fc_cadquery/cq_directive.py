"""
A special directive for including a cq object.

"""

import traceback
from cadquery import *
from cadquery import cqgi
import io
from docutils.parsers.rst import directives

template = """

.. raw:: html

    <div class="cq" style="text-align:%(txt_align)s;float:left;">
        %(out_svg)s
    </div>
    <div style="clear:both;">
    </div>

"""
template_content_indent = '      '


def cq_directive(name, arguments, options, content, lineno,
                 content_offset, block_text, state, state_machine):
    # only consider inline snippets
    plot_code = '\n'.join(content)

    # Since we don't have a filename, use a hash based on the content
    # the script must define a variable called 'out', which is expected to
    # be a CQ object
    out_svg = "Your Script Did not assign call build_output() function!"

    try:
        _s = io.StringIO()
        result = cqgi.parse(plot_code).build()

        if result.success:
            exporters.exportShape(result.first_result.shape, "SVG", _s)
            out_svg = _s.getvalue()
        else:
            raise result.exception

    except Exception:
        traceback.print_exc()
        out_svg = traceback.format_exc()

    # now out
    # Now start generating the lines of output
    lines = []

    # get rid of new lines
    out_svg = out_svg.replace('\n', '')

    txt_align = "left"
    if "align" in options:
        txt_align = options['align']

    lines.extend((template % locals()).split('\n'))

    lines.extend(['::', ''])
    lines.extend(['    %s' % row.rstrip()
                  for row in plot_code.split('\n')])
    lines.append('')

    if len(lines):
        state_machine.insert_input(
            lines, state_machine.input_lines.source(0))

    return []


def setup(app):
    setup.app = app
    setup.config = app.config
    setup.confdir = app.confdir

    options = {'height': directives.length_or_unitless,
               'width': directives.length_or_percentage_or_unitless,
               'align': directives.unchanged
               }

    app.add_directive('cq_plot', cq_directive, True, (0, 2, 0), **options)
