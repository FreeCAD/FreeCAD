# -*- coding: utf-8 -*-
import uuid

class Library(object):
    API_VERSION = 1

    def __init__(self, label, id=None):
        self.id = id or str(uuid.uuid1())
        self.label = label
        self.tools = []
        self.tool_nos = {}  # Maps tool_no number to tool

    def __str__(self):
        return '{} "{}"'.format(self.id, self.label)

    def __eq__(self, other):
        return self.id == other.id

    def __iter__(self):
        return self.tools.__iter__()

    def get_next_tool_no(self):
        tool_nolist = sorted(self.tool_nos, reverse=True)
        return tool_nolist[0]+1 if tool_nolist else 1

    def get_tool_no_from_tool(self, tool):
        for tool_no, thetool in self.tool_nos.items():
            if tool == thetool:
                return tool_no
        return None

    def assign_new_tool_no(self, tool, tool_no=None):
        if tool not in self.tools:
            return

        # If no specific tool_no was requested, assign a new one.
        if tool_no is None:
            tool_no = self.get_next_tool_no()
        elif self.tool_nos.get(tool_no) == tool:
            return

        # Otherwise, add the tool. Since the requested tool_no may already
        # be in use, we need to account for that. In this case, we will
        # add the removed tool into a new tool_no.
        old_tool = self.tool_nos.pop(tool_no, None)
        old_tool_no = self.get_tool_no_from_tool(tool)
        if old_tool_no:
            del self.tool_nos[old_tool_no]
        self.tool_nos[tool_no] = tool
        if old_tool:
            self.assign_new_tool_no(old_tool)
        return tool_no

    def add_tool(self, tool, tool_no=None):
        if tool not in self.tools:
            self.tools.append(tool)
        self.assign_new_tool_no(tool, tool_no)

    def get_tools(self):
        return self.tools

    def has_tool(self, tool):
        for t in self.tools:
            if tool.id == t.id:
                return True
        return False

    def remove_tool(self, tool):
        self.tools = [t for t in self.tools if t.id != tool.id]
        self.tool_nos = {k: v for (k, v) in self.tool_nos.items() if v.id != tool.id}

    def serialize(self, serializer, filename=None):
        return serializer.serialize_library(self, filename=filename)

    @classmethod
    def deserialize(cls, serializer, id):
        return serializer.deserialize_library(id)

    def dump(self, summarize=False):
        title = 'Library "{}" ({}) (instance {})'.format(self.label, self.id, id(self))
        print("-"*len(title))
        print(title)
        print("-"*len(title))
        for tool in self.tools:
            tool.dump(summarize=summarize)
            print()
