import math
from typing import Dict, Optional
from FreeCAD import Units
from .component import MachineComponent


class Spindle(MachineComponent):
    """
    Represents a spindle component in a machine.
    """

    def __init__(
        self,
        name: str,
        label: Optional[str] = None,
        max_power: Units.Quantity = Units.Quantity("2 kW"),
        min_rpm: float = 3000.0,
        max_rpm: float = 60000.0,
        max_torque: float = 10.0,
        peak_torque_rpm: float = 20000.0,
        icon: Optional[str] = None,
    ):
        super().__init__(name=name, label=label, icon=icon)
        self._max_power = max_power
        self._min_rpm = min_rpm
        self._max_rpm = max_rpm
        self._max_torque = max_torque
        self._peak_torque_rpm = peak_torque_rpm

    @property
    def max_power(self) -> Units.Quantity:
        return self._max_power

    @max_power.setter
    def max_power(self, value: Units.Quantity) -> None:
        self._max_power = value

    @property
    def min_rpm(self) -> float:
        return self._min_rpm

    @min_rpm.setter
    def min_rpm(self, value: float) -> None:
        self._min_rpm = value

    @property
    def max_rpm(self) -> float:
        return self._max_rpm

    @max_rpm.setter
    def max_rpm(self, value: float) -> None:
        self._max_rpm = value

    @property
    def max_torque(self) -> float:
        return self._max_torque

    @max_torque.setter
    def max_torque(self, value: float) -> None:
        self._max_torque = value

    @property
    def peak_torque_rpm(self) -> float:
        return self._peak_torque_rpm

    @peak_torque_rpm.setter
    def peak_torque_rpm(self, value: float) -> None:
        self._peak_torque_rpm = value

    def get_torque_at_rpm(self, rpm: float) -> float:
        """
        Calculates torque at a given RPM.

        Args:
            rpm: RPM value (e.g., 5000.0).

        Returns:
            Torque at the given RPM (e.g., 5.0).
        """
        max_torque = self.max_torque
        peak_torque_rpm = self.peak_torque_rpm
        max_power = self.max_power

        # Convert RPM to Hz for power calculation (1 RPM = 1/60 Hz)
        rpm_hz = rpm / 60.0
        peak_rpm_hz = peak_torque_rpm / 60.0

        if rpm_hz <= peak_rpm_hz:
            torque_nm = max_torque * (rpm_hz / peak_rpm_hz)
        else:
            # Power (W) = Torque (Nm) * Angular Velocity (rad/s)
            # Angular Velocity (rad/s) = 2 * pi * RPM / 60
            # Torque (Nm) = Power (W) / (2 * pi * RPM / 60)
            torque_nm = (
                max_power.getValueAs("W").Value / (2 * math.pi * rpm_hz)
                if rpm_hz > 0
                else float("inf")
            )
            torque_nm = min(max_torque, torque_nm)

        return torque_nm

    def validate(self) -> None:
        """
        Validate the spindle properties.
        """
        if self.min_rpm >= self.max_rpm:
            raise AttributeError("Max RPM must be larger than min RPM")
        if self.peak_torque_rpm > self.max_rpm:
            raise AttributeError("Peak Torque RPM must be less than max RPM")

        super().validate()

    def _dump_self(self, indent_str="") -> str:
        output = f"{indent_str}Spindle: {self.name}\n"
        output += f"{indent_str}  Max Power: {self.max_power.UserString}\n"
        output += f"{indent_str}  Min RPM: {self.min_rpm}\n"
        output += f"{indent_str}  Max RPM: {self.max_rpm}\n"
        output += f"{indent_str}  Max Torque: {self.max_torque}\n"
        output += f"{indent_str}  Peak Torque RPM: {self.peak_torque_rpm}\n"
        return output

    def to_dict(self) -> Dict:
        data = super().to_dict()
        data.update(
            {
                "max_power": self.max_power.UserString,
                "min_rpm": self.min_rpm,
                "max_rpm": self.max_rpm,
                "max_torque": self.max_torque,
                "peak_torque_rpm": self.peak_torque_rpm,
            }
        )
        return data

    @classmethod
    def _from_dict_self(cls, data: Dict) -> "Spindle":
        instance = cls(name=data["name"], label=data.get("label"))
        instance.icon = data.get("icon", instance.icon)
        instance.max_power = Units.Quantity(data.get("max_power", instance.max_power))
        instance.min_rpm = data.get("min_rpm", instance.min_rpm)
        instance.max_rpm = data.get("max_rpm", instance.max_rpm)
        instance.max_torque = data.get("max_torque", instance.max_torque)
        instance.peak_torque_rpm = data.get("peak_torque_rpm", instance.peak_torque_rpm)
        return instance
