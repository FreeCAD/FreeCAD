"""
For optimizing sequences of Path.Commands
"""

from collections.abc import Iterator

import Constants
import Path
from Path.Base.MachineState import MachineState


def modal_axis(command: Path.Command, previous_command: Path.Command) -> Path.Command | None:
    """
    Removes redundant axis parameters
    See modal_gcode()
    """

    def axis_modal_fn(previous_command, command):
        # make a new dict of not-the-same axis
        new_params = {}

        # They are barriers
        if command.Name in Constants.NOT_PARAMETER_MODAL:
            new_command = command
            next_previous_command = None

        else:
            # `dont_axis_modal` never comes into play because NOT_PARAMETER_MODAL are total barriers
            dont_axis_modal = Constants.NOT_PARAMETER_MODAL.get(command.Name, [])

            for parameter, value in command.Parameters.items():
                if parameter not in Constants.PARAMETER_MODAL or parameter in dont_axis_modal:
                    new_params[parameter] = value
                elif previous_command is None or parameter not in previous_command.Parameters:
                    new_params[parameter] = value
                elif previous_command.Parameters[parameter] != value:
                    new_params[parameter] = value
                # else don't copy

            new_command = Path.Command(command.Name, new_params, command.Annotations)

            # elide if empty
            if (
                new_command.Name == ""
                and new_command.Parameters == {}
                and new_command.Annotations == {}
            ):
                new_command = None

            # need to keep a running-state of ALL previous axis parameters
            if previous_command is not None:
                next_previous_parameters = {**previous_command.Parameters}
            else:
                next_previous_parameters = {}
            next_previous_parameters.update(command.Parameters)
            next_previous_command = Path.Command(
                command.Name, next_previous_parameters, command.Annotations
            )

        return next_previous_command, new_command

    return _deduplicate(axis_modal_fn, command, previous_command)


def modal_gcode(command: Path.Command, previous_command: Path.Command) -> Path.Command | None:
    """
    Removes the command word from consecutive commands of the same type,
    keeping only the parameters. This is modal G-code behavior.

    Usage:
        new_list = []
        previous_command = None
        for command in some_command_list:
            previous_command, deduped_command = modal_gcode(command, previous_command)
            if deduped_command:
                new_list.append(deduped_command)

    Example:
        G1 X10 Y20
        G1 X30 Y40  -> X30 Y40 (G1 removed)
        G1 X50 Y60  -> X50 Y60 (G1 removed)
        G0 Z5       -> G0 Z5 (different command, kept)

    Returns:
        (next_previous_command, deduped_command)
        next_previous_command might be None (on a "modal barrier")
        deduped_command _might_ be a new Path.Command, or the original command
            If it is None, you should elide it.
    """

    def modal_fn(previous_command, command):
        # Same .Name as previously?
        if previous_command and command.Name == previous_command.Name:
            new_command = Path.Command("", command.Parameters, command.Annotations)

            # we only signal "eliding" for a deduped situation
            if (
                new_command.Name == ""
                and new_command.Parameters == {}
                and new_command.Annotations == {}
            ):
                new_command = None
        else:
            new_command = command

        # Tolerate Path.Command.Name == ""
        next_previous_command = (
            command if (command.Name != "" and command.Name[0] != "(") else previous_command
        )

        return next_previous_command, new_command

    return _deduplicate(modal_fn, command, previous_command)


def _deduplicate(
    modal_fn, command: Path.Command, previous_command: Path.Command
) -> Path.Command | None:
    """Common dedup skeleton.
    Handles previous logic, barriers, etc.

    Just do your change in:

        modal_fn(previous_command, command):
            next_previous_command = calculate this

            if needs to be changed
                return next_previous_command, Path.Command( changes )
            else:
                return next_previous_command, command

    See modal_gcode() for overall behavior
    """

    # comments unchanged
    if command.Name == "(":
        return previous_command, command

    # Things that are modal barriers:
    if previous_command and (
        previous_command.Name in Constants.MCODE_TOOL_CHANGE
        or previous_command.Annotations.get(Constants.ANNOT_MODAL_BARRIER, False)
    ):
        previous_command = None

    next_previous_command, new_command = modal_fn(previous_command, command)

    return next_previous_command, new_command


def collapse_g0(commands: list[Path.Command]) -> Iterator[Path.Command]:
    """Collapse a chain of G0's if we travel _along_ the same single axis
    (at the same speed).
    That means, either direction (up, down), as well as same (up, up).

    This was wanted to clean up at the boundaries of operations, where
    the last move might be a rapid-move up to some height,
    and the next operation moves up/down again to a new height,
    e.g. safe->clearance->safe.
    Thus, all along just the Z axis.
    Generalized here to any single XYZABC axis.
    Possible to generalize to a vector (hint, dot-product), but not done here.

    Note that we need: (starting-point, G0#1, G0#2)
        Because we need to figure out the single-axis that G0#1 moves in,
        so we need a starting point,
        and we need to determine if G0#2 _continues_
        in the same axis as G0#1 moved.
    This means that a G0 before any other move can never be "chained".

    We respect Constants.ANNOT_NO_COLLAPSE_G0: don't collapse G0's w/this
    annotation. E.g. drill-cycle-expand should mark the retracts with this.

    "Along the same axis" means that only a single axis changes value.
    The implementation does not tolerate axis-modal. e.g. "G0 X1 Y2" -> "G0 X2"
        is counted as changing 2 axis, because Y is missing (treated as None).
        It could, by merging in MachineState when checking.
        But, we assume that Operations generate all axis in a group (XYZ, or ABC).
    Anything other than a G0 breaks the chain (including empty commands and comments).
    Implemented as a generator.

    We have to keep track of the current MachineState (for move#0),
    and the one-axis that changed.
    """

    def only_one_axis(g0, vs_parameters):
        # Only diff in one (or zero) of the axis
        # By allowing different-in-zero, we collapse redundant G0's as well
        # BUT, we are looking at the explicit Parameters, so "G0 X1" and "G0 X1 Y2" are 2-axis different
        # Returns
        #   the axis, if 1 is different
        #   "", if 0 are different
        #   None, if >1 are different

        # cmd can actually be a machine_state

        if vs_parameters is not None:
            diff = [a for a in "XYZABC" if g0.Parameters.get(a, None) != vs_parameters.get(a, None)]
            return diff[0] if len(diff) == 1 else ("" if len(diff) == 0 else None)
        else:
            # No vs_parameters means no previous position, so can't know
            return False

    def compatible_F(g0, cmd):
        # F's have to be close-enough
        g0_f = g0.Parameters.get("F", None)
        cmd_f = cmd.Parameters.get("F", None)
        if g0_f is None and cmd_f is not None:
            # we don't know what the first one's F is
            return False
        elif cmd_f is None:
            # F carries over from g0
            return True
        else:
            return math.isclose(g0_f, cmd_f, 1e-4)

    def start_chain(cmd):
        # we know cmd is a g0
        # we are before start of chain.
        # Return info for starting-new-chain, or for yielding command
        #   (new g0, new chain_axis, None | value-to-emit)

        emit = None
        g0 = None

        one_axis = only_one_axis(
            cmd, machine_state.getState() if machine_state else None
        )  # false or an axis

        # We only allow to change 1-axis vs previous command
        if one_axis is None:
            emit = cmd

        # any annotated-non-collapsible ends a chain
        elif cmd.Annotations.get(Constants.ANNOT_NO_COLLAPSE_G0, False):
            emit = cmd

        # we can be collapsed into
        else:
            g0 = cmd

        return g0, None if one_axis is False else one_axis, emit

    # Collapse a sequence of G0's
    # so, we only need a "stack" depth of 1
    # but we need the machinestate for the first g0
    # and what axis this chain is
    g0 = None
    machine_state = None  # set at bottom of loop first time
    chain_axis = None

    for cmd in commands:
        # Only a sequence of G0's can be collapsed
        if cmd.Name in Constants.GCODE_MOVE_RAPID:

            # 1st g0 of a chain: might start a chain
            if g0 is None:
                g0, chain_axis, to_emit = start_chain(cmd)
                if to_emit is not None:
                    yield to_emit

            # Next g0, might chain
            elif (
                (a := only_one_axis(g0, cmd.Parameters)) is not None
                and a == chain_axis
                and compatible_F(g0, cmd)
                and g0.Annotations == cmd.Annotations
            ):
                g0 = cmd
                # same chain_axis

            # G0 isn't in this chain, might start next chain
            else:

                # flush
                yield g0

                # might start new chain (in a different axis)
                g0, chain_axis, to_emit = start_chain(cmd)
                if to_emit is not None:
                    yield to_emit

        # Wasn't a G0, so emit (w/saved g0)
        else:
            if g0 is not None:
                yield g0
                g0 = None
                chain_axis = None

            yield cmd

        # track the consumed cmd
        if machine_state is None:
            machine_state = MachineState(None)
        machine_state.addCommand(cmd)

    # final emit of last g0
    if g0 is not None:
        yield g0
