class RotaryToolBitMixin:
    """
    Mixin class for rotary tool bits.
    Provides methods for accessing diameter and length from the shape.
    """

    def can_rotate(self) -> bool:
        return True

    def get_diameter(self) -> float:
        """
        Get the diameter of the rotary tool bit from the shape.
        """
        # Assuming the shape object has a get_parameter method
        # and the parameter name for diameter is "Diameter"
        if hasattr(self, "_tool_bit_shape") and self._tool_bit_shape:
            return self._tool_bit_shape.get_parameter("Diameter")
        return 0.0

    def set_diameter(self, diameter: float):
        """
        Set the diameter of the rotary tool bit on the shape.
        """
        # Assuming the shape object has a set_parameter method
        # and the parameter name for diameter is "Diameter"
        if hasattr(self, "_tool_bit_shape") and self._tool_bit_shape:
            self._tool_bit_shape.set_parameter("Diameter", diameter)

    def get_length(self) -> float:
        """
        Get the length of the rotary tool bit from the shape.
        """
        # Assuming the shape object has a get_parameter method
        # and the parameter name for length is "Length"
        if hasattr(self, "_tool_bit_shape") and self._tool_bit_shape:
            return self._tool_bit_shape.get_parameter("Length")
        return 0.0

    def set_length(self, length: float):
        """
        Set the length of the rotary tool bit on the shape.
        """
        # Assuming the shape object has a set_parameter method
        # and the parameter name for length is "Length"
        if hasattr(self, "_tool_bit_shape") and self._tool_bit_shape:
            self._tool_bit_shape.set_parameter("Length", length)
