# SPDX-License-Identifier: LGPL-2.1-or-later

"""Animation playback helpers for MbDFEM result series."""

from bisect import bisect_right
from math import degrees

import FreeCAD as App


_POSITION_PROPERTIES = ("xs", "ys", "zs")
_BRYANT_PROPERTIES = ("bryxs", "bryys", "bryzs")


class AnimationController:
    """Scrub and play solved MbDFEM result series on document object placements."""

    def __init__(self, assembly, objects=None, length_scale=1000.0):
        if assembly is None:
            raise ValueError("AnimationController expects an MbDFEM assembly")

        self.assembly = assembly
        self.length_scale = length_scale
        self.current_time = 0.0
        self.current_frame = 0
        self.is_playing = False
        self._timer = None
        self._targets = list(objects) if objects is not None else self._default_targets()

    @property
    def targets(self):
        return list(self._targets)

    @property
    def times(self):
        frames = self._playback_frames()
        if frames:
            return [time for _, time in frames]

        count = self.frame_count
        frame_rate = self.frame_rate
        return [index / frame_rate for index in range(count)]

    @property
    def frame_count(self):
        counts = []
        for target in self._targets:
            for name in _POSITION_PROPERTIES:
                values = list(getattr(target, name, []))
                if values:
                    counts.append(max(len(values) - 1, 0))
        assembly_times = self._playback_frames()
        if assembly_times:
            counts.append(len(assembly_times))
        return min(counts) if counts else 0

    @property
    def duration(self):
        times = self.times
        return times[-1] if times else 0.0

    @property
    def frame_rate(self):
        parameters = self._animation_parameters()
        value = getattr(parameters, "frameRate", 30) if parameters else 30
        return max(float(value), 1.0)

    @property
    def playback_speed(self):
        parameters = self._animation_parameters()
        return float(getattr(parameters, "playbackSpeed", 1.0)) if parameters else 1.0

    @property
    def loop_enabled(self):
        parameters = self._animation_parameters()
        return bool(getattr(parameters, "loop", True)) if parameters else True

    @property
    def interpolate_frames(self):
        parameters = self._animation_parameters()
        return bool(getattr(parameters, "interpolateFrames", True)) if parameters else True

    def setTime(self, seconds):
        """Apply the animation state at *seconds* and return the selected time."""
        times = self.times
        if not times:
            self.current_time = 0.0
            self.current_frame = 0
            return self.current_time

        self.current_time = self._bounded_time(float(seconds))
        sample = self._sample_for_time(self.current_time)
        self.current_frame = sample[3]
        self._apply_sample(sample)
        self._recompute_document()
        return self.current_time

    def setFrame(self, index):
        """Apply the animation state at frame *index*."""
        times = self.times
        if not times:
            self.current_frame = 0
            self.current_time = 0.0
            return self.current_frame

        self.current_frame = max(0, min(int(index), len(times) - 1))
        self.setTime(times[self.current_frame])
        return self.current_frame

    def stepForward(self):
        return self.setFrame(self.current_frame + 1)

    def stepBackward(self):
        return self.setFrame(self.current_frame - 1)

    def play(self):
        """Start timer-driven playback when Qt is available."""
        if self.frame_count == 0:
            return False

        timer = self._ensure_timer()
        if timer is None:
            self.is_playing = True
            return False

        interval = max(int(1000.0 / self.frame_rate), 1)
        self.is_playing = True
        timer.start(interval)
        return True

    def pause(self):
        if self._timer is not None:
            self._timer.stop()
        self.is_playing = False

    def stop(self):
        self.pause()
        self.setFrame(0)

    def tick(self, delta_seconds=None):
        """Advance playback by one tick; useful for tests and non-Qt callers."""
        if delta_seconds is None:
            delta_seconds = 1.0 / self.frame_rate

        next_time = self.current_time + float(delta_seconds) * self.playback_speed
        duration = self.duration
        if duration > 0.0 and next_time > duration:
            if self.loop_enabled:
                next_time = next_time % duration
            else:
                next_time = duration
                self.pause()
        return self.setTime(next_time)

    def _animation_parameters(self):
        getter = getattr(self.assembly, "getAnimationParameters", None)
        if getter is None:
            return None
        try:
            return getter()
        except Exception:
            return None

    def _default_targets(self):
        targets = []
        for name in ("parts", "fixedparts"):
            for obj in list(getattr(self.assembly, name, [])):
                if obj is not None and obj not in targets and self._has_result_series(obj):
                    targets.append(obj)
        return targets

    @staticmethod
    def _has_result_series(obj):
        return any(list(getattr(obj, name, [])) for name in _POSITION_PROPERTIES)

    def _playback_frames(self):
        raw_times = list(getattr(self.assembly, "times", []))
        return [(index, time) for index, time in enumerate(raw_times) if index > 0]

    def _bounded_time(self, seconds):
        times = self.times
        if not times:
            return 0.0
        if self.loop_enabled and times[-1] > times[0] and seconds > times[-1]:
            return times[0] + ((seconds - times[0]) % (times[-1] - times[0]))
        return max(times[0], min(seconds, times[-1]))

    def _sample_for_time(self, seconds):
        frames = self._playback_frames()
        if not frames:
            frames = [(index + 1, time) for index, time in enumerate(self.times)]
        times = [time for _, time in frames]
        if len(frames) == 1:
            return frames[0][0], frames[0][0], 0.0, 0
        upper = bisect_right(times, seconds)
        if upper <= 0:
            return frames[0][0], frames[0][0], 0.0, 0
        if upper >= len(frames):
            index = len(frames) - 1
            return frames[index][0], frames[index][0], 0.0, index

        lower = upper - 1
        span = times[upper] - times[lower]
        if span <= 0.0 or not self.interpolate_frames:
            return frames[lower][0], frames[lower][0], 0.0, lower
        return frames[lower][0], frames[upper][0], (seconds - times[lower]) / span, lower

    def _apply_sample(self, sample):
        for target in self._targets:
            placement = App.Placement(target.Placement)
            placement.Base = App.Vector(*self._position(target, sample))
            rotation = self._rotation(target, sample)
            if rotation is not None:
                placement.Rotation = rotation
            target.Placement = placement

    def _position(self, target, sample):
        values = []
        for name in _POSITION_PROPERTIES:
            series = list(getattr(target, name, []))
            values.append(self._sample_value(series, sample) * self.length_scale)
        return values

    def _rotation(self, target, sample):
        series = [list(getattr(target, name, [])) for name in _BRYANT_PROPERTIES]
        if not all(series):
            return None

        x_angle, y_angle, z_angle = [degrees(self._sample_value(values, sample)) for values in series]
        x_rotation = App.Rotation(App.Vector(1, 0, 0), x_angle)
        y_rotation = App.Rotation(App.Vector(0, 1, 0), y_angle)
        z_rotation = App.Rotation(App.Vector(0, 0, 1), z_angle)
        return z_rotation.multiply(y_rotation).multiply(x_rotation)

    @staticmethod
    def _sample_value(values, sample):
        if not values:
            return 0.0

        lower, upper, ratio = sample[:3]
        lower = max(0, min(lower, len(values) - 1))
        upper = max(0, min(upper, len(values) - 1))
        if lower == upper:
            return float(values[lower])
        return float(values[lower]) + (float(values[upper]) - float(values[lower])) * ratio

    def _ensure_timer(self):
        if self._timer is not None:
            return self._timer
        try:
            from PySide import QtCore
        except Exception:
            return None

        self._timer = QtCore.QTimer()
        self._timer.timeout.connect(self.tick)
        return self._timer

    def _recompute_document(self):
        document = getattr(self.assembly, "Document", None)
        if document is not None:
            document.recompute()


def controller(assembly, objects=None, length_scale=1000.0):
    """Create an :class:`AnimationController` for *assembly*."""
    return AnimationController(assembly, objects=objects, length_scale=length_scale)
