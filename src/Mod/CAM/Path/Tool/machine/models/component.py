from typing import Dict, List, Optional, Type, TypeVar


T = TypeVar("T", bound="MachineComponent")


class MachineComponent(object):
    """
    Base class for all machine components. Can be assembled as a tree, with
    parent/child relationships.
    """

    def __init__(self, name: str, label: Optional[str] = None, icon: Optional[str] = None):
        self.name: str = name
        self._label: str = label or name
        self._parent: Optional[MachineComponent] = None
        self._children: List[MachineComponent] = []
        self._icon = icon

    @property
    def label(self) -> str:
        return self._label

    @label.setter
    def label(self, value: str):
        self._label = value

    @property
    def parent(self) -> Optional["MachineComponent"]:
        return self._parent

    @property
    def children(self) -> List["MachineComponent"]:
        return self._children

    @children.setter
    def children(self, value: List["MachineComponent"]):
        self._children = value

    def add(self, component: "MachineComponent"):
        """
        Adds a child component to this component.
        Manages the parent-child relationship.
        """
        if component._parent is not None:
            component._parent.remove(component)
        self._children.append(component)
        component._parent = self

    def remove(self, component: "MachineComponent"):
        """
        Removes a child component from this component.
        Breaks the parent-child relationship.
        """
        if component in self._children:
            self._children.remove(component)
            component._parent = None

    def get_child_by_name(self, name: str) -> Optional["MachineComponent"]:
        """
        Retrieves a child component by its name.
        """
        for child in self._children:
            if child.name == name:
                return child
        return None

    def find_child_by_name(self, name: str) -> Optional["MachineComponent"]:
        """
        Recursively retrieves a child component by its name from the entire hierarchy.
        """
        for child in self._children:
            if child.name == name:
                return child
            found_in_child = child.find_child_by_name(name)
            if found_in_child:
                return found_in_child
        return None

    def get_children_by_type(self, component_type: Type[T]) -> List[T]:
        """
        Retrieves all child components of a specific type.
        """
        return [child for child in self._children if isinstance(child, component_type)]

    def find_children_by_type(self, component_type: Type[T]) -> List[T]:
        """
        Recursively retrieves all child components of a specific type from the entire hierarchy.
        """
        found_children = []
        for child in self._children:
            if isinstance(child, component_type):
                found_children.append(child)
            found_children.extend(child.find_children_by_type(component_type))
        return found_children

    @property
    def icon(self) -> Optional[str]:
        """Returns the component's icon name."""
        return self._icon

    @icon.setter
    def icon(self, value: Optional[str]):
        self._icon = value

    def validate(self):
        """
        Validates the component and its children.
        """
        for child in self._children:
            child.validate()

    def dump(self, do_print: bool = True, indent: int = 0) -> str:
        """
        Dumps the component's data into a string for display, including children.
        """
        output = self._dump_self(" " * indent)
        for child in self._children:
            output += child.dump(do_print=False, indent=indent + 2)
        if do_print:
            print(output)
        return output

    def _dump_self(self, indent_str="") -> str:
        """
        Abstract method to dump the component's own data.
        """
        output = f"{indent_str}{self.__class__.__name__}: {self.name}\n"
        output += f"{indent_str}  Icon: {self.icon}\n"
        return output

    def to_dict(self) -> Dict:
        """
        Serializes the component to a dictionary.
        """
        data = {
            "name": self.name,
            "label": self.label,
            "icon": self.icon,
            "type": self.__class__.__name__,
            "children": [child.to_dict() for child in self.children],
        }
        return data

    @classmethod
    def from_dict(cls, data: Dict) -> "MachineComponent":
        """
        Deserializes a dictionary to a MachineComponent.
        """
        component = cls._from_dict_self(data)
        component.children = []

        # Recursively add children
        for child_data in data.get("children", []):
            component_type_name = child_data.get("type")
            if not component_type_name:
                raise ValueError("Component type not found in child data")

            component_class = None
            # Recursively find the component class
            for subclass in cls._get_all_subclasses(MachineComponent):
                if subclass.__name__ == component_type_name:
                    component_class = subclass
                    break

            if not component_class:
                raise ValueError(f"Unknown component type: {component_type_name}")

            child_component = component_class.from_dict(child_data)
            component.add(child_component)

        return component

    @classmethod
    def _get_all_subclasses(cls, base_class: Type) -> List[Type]:
        """
        Recursively gets all subclasses of a given base class.
        """
        all_subclasses = [MachineComponent]
        for subclass in base_class.__subclasses__():
            all_subclasses.append(subclass)
            all_subclasses.extend(cls._get_all_subclasses(subclass))
        return all_subclasses

    @classmethod
    def _from_dict_self(cls, data: Dict) -> "MachineComponent":
        """
        Abstract class method to deserialize a dictionary to a component,
        handling its own specific attributes.
        """
        instance = cls(data["name"])
        instance.label = data.get("label", data["name"])
        instance.icon = data.get("icon")
        return instance
