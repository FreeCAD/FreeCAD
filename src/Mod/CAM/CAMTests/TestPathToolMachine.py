import unittest
import FreeCAD
from Path.Tool.machine.models.machine import Machine


class TestPathToolMachine(unittest.TestCase):
    def setUp(self):
        self.default_machine = Machine()

    def test_initialization_defaults(self):
        self.assertEqual(self.default_machine.label, "Machine")
        self.assertAlmostEqual(self.default_machine.max_power.getValueAs("W").Value, 2000)
        self.assertAlmostEqual(self.default_machine.get_min_rpm_value(), 3000)
        self.assertAlmostEqual(self.default_machine.get_max_rpm_value(), 60000)
        self.assertAlmostEqual(self.default_machine.min_feed.getValueAs("mm/min").Value, 1)
        self.assertAlmostEqual(self.default_machine.max_feed.getValueAs("mm/min").Value, 2000)
        expected_peak_torque_rpm = 60000 / 3
        self.assertAlmostEqual(
            self.default_machine.get_peak_torque_rpm_value(),
            expected_peak_torque_rpm,
        )
        expected_max_torque_nm = 2000 * 9.5488 / expected_peak_torque_rpm
        self.assertAlmostEqual(
            self.default_machine.max_torque.getValueAs("Nm").Value,
            expected_max_torque_nm,
        )
        self.assertIsNotNone(self.default_machine.id)

    def test_initialization_custom_values(self):
        custom_machine = Machine(
            label="Custom Machine",
            max_power=5,
            min_rpm=1000,
            max_rpm=20000,
            max_torque=50,
            peak_torque_rpm=15000,
            min_feed=10,
            max_feed=5000,
            id="custom-id",
        )
        self.assertEqual(custom_machine.label, "Custom Machine")
        self.assertAlmostEqual(custom_machine.max_power.getValueAs("W").Value, 5000)
        self.assertAlmostEqual(custom_machine.get_min_rpm_value(), 1000)
        self.assertAlmostEqual(custom_machine.get_max_rpm_value(), 20000)
        self.assertAlmostEqual(custom_machine.max_torque.getValueAs("Nm").Value, 50)
        self.assertAlmostEqual(custom_machine.get_peak_torque_rpm_value(), 15000)
        self.assertAlmostEqual(custom_machine.min_feed.getValueAs("mm/min").Value, 10)
        self.assertAlmostEqual(custom_machine.max_feed.getValueAs("mm/min").Value, 5000)
        self.assertEqual(custom_machine.id, "custom-id")

    def test_initialization_custom_torque_quantity(self):
        custom_torque_machine = Machine(max_torque=FreeCAD.Units.Quantity(100, "Nm"))
        self.assertAlmostEqual(custom_torque_machine.max_torque.getValueAs("Nm").Value, 100)

    def test_validate_valid(self):
        try:
            self.default_machine.validate()
        except AttributeError as e:
            self.fail(f"Validation failed unexpectedly: {e}")

    def test_validate_missing_label(self):
        self.default_machine.label = ""
        with self.assertRaisesRegex(AttributeError, "Machine name is required"):
            self.default_machine.validate()

    def test_validate_peak_torque_rpm_greater_than_max_rpm(self):
        self.default_machine.set_peak_torque_rpm(70000)
        with self.assertRaisesRegex(AttributeError, "Peak Torque RPM.*must be less than max RPM"):
            self.default_machine.validate()

    def test_validate_max_rpm_less_than_min_rpm(self):
        self.default_machine = Machine()
        self.default_machine.set_min_rpm(4000)  # min_rpm = 4000 RPM
        self.default_machine.set_peak_torque_rpm(1000)  # peak_torque_rpm = 1000 RPM
        self.default_machine._max_rpm = 2000 / 60.0  # max_rpm = 2000 RPM (33.33 1/s)
        self.assertLess(
            self.default_machine.get_max_rpm_value(),
            self.default_machine.get_min_rpm_value(),
        )
        with self.assertRaisesRegex(AttributeError, "Max RPM must be larger than min RPM"):
            self.default_machine.validate()

    def test_validate_max_feed_less_than_min_feed(self):
        self.default_machine.set_min_feed(1000)
        self.default_machine._max_feed = 500
        with self.assertRaisesRegex(AttributeError, "Max feed must be larger than min feed"):
            self.default_machine.validate()

    def test_get_torque_at_rpm(self):
        torque_below_peak = self.default_machine.get_torque_at_rpm(10000)
        expected_peak_torque_rpm = 60000 / 3
        expected_max_torque_nm = 2000 * 9.5488 / expected_peak_torque_rpm
        expected_torque_below_peak = expected_max_torque_nm / expected_peak_torque_rpm * 10000
        self.assertAlmostEqual(torque_below_peak, expected_torque_below_peak)

        torque_at_peak = self.default_machine.get_torque_at_rpm(
            self.default_machine.get_peak_torque_rpm_value()
        )
        self.assertAlmostEqual(
            torque_at_peak,
            self.default_machine.max_torque.getValueAs("Nm").Value,
        )

        torque_above_peak = self.default_machine.get_torque_at_rpm(50000)
        expected_torque_above_peak = 2000 * 9.5488 / 50000
        self.assertAlmostEqual(torque_above_peak, expected_torque_above_peak)

    def test_set_label(self):
        self.default_machine.label = "New Label"
        self.assertEqual(self.default_machine.label, "New Label")

    def test_set_max_power(self):
        self.default_machine = Machine()
        self.default_machine.set_max_power(5, "hp")
        self.assertAlmostEqual(
            self.default_machine.max_power.getValueAs("W").Value,
            5 * 745.7,
            places=4,
        )
        with self.assertRaisesRegex(AttributeError, "Max power must be positive"):
            self.default_machine.set_max_power(0)

    def test_set_min_rpm(self):
        self.default_machine = Machine()
        self.default_machine.set_min_rpm(5000)
        self.assertAlmostEqual(self.default_machine.get_min_rpm_value(), 5000)
        with self.assertRaisesRegex(AttributeError, "Min RPM cannot be negative"):
            self.default_machine.set_min_rpm(-100)
        self.default_machine = Machine()
        self.default_machine.set_min_rpm(70000)
        self.assertAlmostEqual(self.default_machine.get_min_rpm_value(), 70000)
        self.assertAlmostEqual(self.default_machine.get_max_rpm_value(), 70001)

    def test_set_max_rpm(self):
        self.default_machine = Machine()
        self.default_machine.set_max_rpm(50000)
        self.assertAlmostEqual(self.default_machine.get_max_rpm_value(), 50000)
        with self.assertRaisesRegex(AttributeError, "Max RPM must be positive"):
            self.default_machine.set_max_rpm(0)
        self.default_machine = Machine()
        self.default_machine.set_max_rpm(2000)
        self.assertAlmostEqual(self.default_machine.get_max_rpm_value(), 2000)
        self.assertAlmostEqual(self.default_machine.get_min_rpm_value(), 1999)
        self.default_machine = Machine()
        self.default_machine.set_max_rpm(0.5)
        self.assertAlmostEqual(self.default_machine.get_max_rpm_value(), 0.5)
        self.assertAlmostEqual(self.default_machine.get_min_rpm_value(), 0)

    def test_set_min_feed(self):
        self.default_machine = Machine()
        self.default_machine.set_min_feed(500, "inch/min")
        self.assertAlmostEqual(
            self.default_machine.min_feed.getValueAs("mm/min").Value,
            500 * 25.4,
            places=4,
        )
        with self.assertRaisesRegex(AttributeError, "Min feed cannot be negative"):
            self.default_machine.set_min_feed(-10)
        self.default_machine = Machine()
        self.default_machine.set_min_feed(3000)
        self.assertAlmostEqual(self.default_machine.min_feed.getValueAs("mm/min").Value, 3000)
        self.assertAlmostEqual(self.default_machine.max_feed.getValueAs("mm/min").Value, 3001)

    def test_set_max_feed(self):
        self.default_machine = Machine()
        self.default_machine.set_max_feed(3000)
        self.assertAlmostEqual(self.default_machine.max_feed.getValueAs("mm/min").Value, 3000)
        with self.assertRaisesRegex(AttributeError, "Max feed must be positive"):
            self.default_machine.set_max_feed(0)
        self.default_machine = Machine()
        self.default_machine.set_min_feed(600)
        self.default_machine.set_max_feed(500)
        self.assertAlmostEqual(self.default_machine.max_feed.getValueAs("mm/min").Value, 500)
        self.assertAlmostEqual(self.default_machine.min_feed.getValueAs("mm/min").Value, 499)
        self.default_machine = Machine()
        self.default_machine.set_max_feed(0.5)
        self.assertAlmostEqual(self.default_machine.max_feed.getValueAs("mm/min").Value, 0.5)
        self.assertAlmostEqual(self.default_machine.min_feed.getValueAs("mm/min").Value, 0)

    def test_set_peak_torque_rpm(self):
        self.default_machine = Machine()
        self.default_machine.set_peak_torque_rpm(40000)
        self.assertAlmostEqual(self.default_machine.get_peak_torque_rpm_value(), 40000)
        with self.assertRaisesRegex(AttributeError, "Peak torque RPM cannot be negative"):
            self.default_machine.set_peak_torque_rpm(-100)

    def test_set_max_torque(self):
        self.default_machine = Machine()
        self.default_machine.set_max_torque(200, "in-lbf")
        self.assertAlmostEqual(
            self.default_machine.max_torque.getValueAs("Nm").Value,
            200 * 0.112985,
            places=4,
        )
        with self.assertRaisesRegex(AttributeError, "Max torque must be positive"):
            self.default_machine.set_max_torque(0)

    def test_dump(self):
        try:
            self.default_machine.dump(False)
        except Exception as e:
            self.fail(f"dump() method failed unexpectedly: {e}")


if __name__ == "__main__":
    unittest.main()
