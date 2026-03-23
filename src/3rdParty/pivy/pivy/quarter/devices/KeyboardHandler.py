###
# Copyright (c) 2002-2008 Kongsberg SIM
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

from pivy.qt import QtCore

from pivy import coin

from .DeviceHandler import DeviceHandler


class KeyboardHandler(DeviceHandler):
    """The KeyboardHandler class provides translation of keyboard
    events on the QuarterWidget. It is registered with the DeviceManager
    by default."""

    def __init__(self):
        self._keyboard = coin.SoKeyboardEvent()
        self._keyboardmap, self._keypadmap = self.initKeyMap()

    def translateEvent(self, qevent):
        """Translates from QKeyEvents to coin.SoKeyboardEvents"""
        if qevent.type() in (QtCore.QEvent.KeyPress, QtCore.QEvent.KeyRelease):
            return self.keyEvent(qevent)
        else:
            return None

    def debugKeyEvents(self):
        pass
        # FIXME jkg: implement using os.ev
        #const char * env = coin_getenv("QUARTER_DEBUG_KEYEVENTS");
        #return env && (atoi(env) > 0);

    def keyEvent(self, qevent):
        modifiers = qevent.modifiers()

        pos = self.manager.getLastMousePosition()
        self._keyboard.setPosition(pos)
        self.setModifiers(self._keyboard, qevent)

        if qevent.type() == QtCore.QEvent.KeyPress:
            self._keyboard.setState(coin.SoButtonEvent.DOWN)
        else:
            self._keyboard.setState(coin.SoButtonEvent.UP)

        qkey = qevent.key()

        sokey = None
        if modifiers & QtCore.Qt.KeypadModifier:
            sokey = self._keypadmap[qkey]
        else:
            sokey = self._keyboardmap[qkey]
        try:
            printable = str(qevent.text().toAscii())
        except AttributeError:      # python3
            printable = str(qevent.text())
        self._keyboard.setPrintableCharacter(printable)
        self._keyboard.setKey(sokey)

# FIXME jkg: implement
#if QUARTER_DEBUG
#  if (KeyboardHandlerP.debugKeyEvents()) {
#    SbString s;
#    self._keyboard.enumToString(self._keyboard.getKey(), s);
#    SoDebugError.postInfo("KeyboardHandlerP.keyEvent",
#                           "enum: '%s', pos: <%i %i>, printable: '%s'",
#                           s.getString(),
#                           pos[0], pos[1],
#                           printable);
        return self._keyboard


    def initKeyMap(self):

        # FIXME jkg: move out?
        class QMap:
            def __init__(self):
                self.map = {}
            def __getitem__(self, key):
                try:
                    return self.map[key]
                except KeyError:
                    # FIXME jkg: use logging module to output warning
                    return coin.SoKeyboardEvent.ANY

            def insert(self, key, value):
                self.map[key] = value

        keyboardmap = QMap()

        # keyboard
        keyboardmap.insert(QtCore.Qt.Key_Shift,   coin.SoKeyboardEvent.LEFT_SHIFT)
        keyboardmap.insert(QtCore.Qt.Key_Alt,     coin.SoKeyboardEvent.LEFT_ALT)
        keyboardmap.insert(QtCore.Qt.Key_Control, coin.SoKeyboardEvent.LEFT_CONTROL)
        keyboardmap.insert(QtCore.Qt.Key_0, coin.SoKeyboardEvent.NUMBER_0)
        keyboardmap.insert(QtCore.Qt.Key_1, coin.SoKeyboardEvent.NUMBER_1)
        keyboardmap.insert(QtCore.Qt.Key_2, coin.SoKeyboardEvent.NUMBER_2)
        keyboardmap.insert(QtCore.Qt.Key_3, coin.SoKeyboardEvent.NUMBER_3)
        keyboardmap.insert(QtCore.Qt.Key_4, coin.SoKeyboardEvent.NUMBER_4)
        keyboardmap.insert(QtCore.Qt.Key_5, coin.SoKeyboardEvent.NUMBER_5)
        keyboardmap.insert(QtCore.Qt.Key_6, coin.SoKeyboardEvent.NUMBER_6)
        keyboardmap.insert(QtCore.Qt.Key_7, coin.SoKeyboardEvent.NUMBER_7)
        keyboardmap.insert(QtCore.Qt.Key_8, coin.SoKeyboardEvent.NUMBER_8)
        keyboardmap.insert(QtCore.Qt.Key_9, coin.SoKeyboardEvent.NUMBER_9)

        keyboardmap.insert(QtCore.Qt.Key_A, coin.SoKeyboardEvent.A)
        keyboardmap.insert(QtCore.Qt.Key_B, coin.SoKeyboardEvent.B)
        keyboardmap.insert(QtCore.Qt.Key_C, coin.SoKeyboardEvent.C)
        keyboardmap.insert(QtCore.Qt.Key_D, coin.SoKeyboardEvent.D)
        keyboardmap.insert(QtCore.Qt.Key_E, coin.SoKeyboardEvent.E)
        keyboardmap.insert(QtCore.Qt.Key_F, coin.SoKeyboardEvent.F)
        keyboardmap.insert(QtCore.Qt.Key_G, coin.SoKeyboardEvent.G)
        keyboardmap.insert(QtCore.Qt.Key_H, coin.SoKeyboardEvent.H)
        keyboardmap.insert(QtCore.Qt.Key_I, coin.SoKeyboardEvent.I)
        keyboardmap.insert(QtCore.Qt.Key_J, coin.SoKeyboardEvent.J)
        keyboardmap.insert(QtCore.Qt.Key_K, coin.SoKeyboardEvent.K)
        keyboardmap.insert(QtCore.Qt.Key_L, coin.SoKeyboardEvent.L)
        keyboardmap.insert(QtCore.Qt.Key_M, coin.SoKeyboardEvent.M)
        keyboardmap.insert(QtCore.Qt.Key_N, coin.SoKeyboardEvent.N)
        keyboardmap.insert(QtCore.Qt.Key_O, coin.SoKeyboardEvent.O)
        keyboardmap.insert(QtCore.Qt.Key_P, coin.SoKeyboardEvent.P)
        keyboardmap.insert(QtCore.Qt.Key_Q, coin.SoKeyboardEvent.Q)
        keyboardmap.insert(QtCore.Qt.Key_R, coin.SoKeyboardEvent.R)
        keyboardmap.insert(QtCore.Qt.Key_S, coin.SoKeyboardEvent.S)
        keyboardmap.insert(QtCore.Qt.Key_T, coin.SoKeyboardEvent.T)
        keyboardmap.insert(QtCore.Qt.Key_U, coin.SoKeyboardEvent.U)
        keyboardmap.insert(QtCore.Qt.Key_V, coin.SoKeyboardEvent.V)
        keyboardmap.insert(QtCore.Qt.Key_W, coin.SoKeyboardEvent.W)
        keyboardmap.insert(QtCore.Qt.Key_X, coin.SoKeyboardEvent.X)
        keyboardmap.insert(QtCore.Qt.Key_Y, coin.SoKeyboardEvent.Y)
        keyboardmap.insert(QtCore.Qt.Key_Z, coin.SoKeyboardEvent.Z)

        keyboardmap.insert(QtCore.Qt.Key_Home,     coin.SoKeyboardEvent.HOME)
        keyboardmap.insert(QtCore.Qt.Key_Left,     coin.SoKeyboardEvent.LEFT_ARROW)
        keyboardmap.insert(QtCore.Qt.Key_Up,       coin.SoKeyboardEvent.UP_ARROW)
        keyboardmap.insert(QtCore.Qt.Key_Right,    coin.SoKeyboardEvent.RIGHT_ARROW)
        keyboardmap.insert(QtCore.Qt.Key_Down,     coin.SoKeyboardEvent.DOWN_ARROW)
        keyboardmap.insert(QtCore.Qt.Key_PageUp,   coin.SoKeyboardEvent.PAGE_UP)
        keyboardmap.insert(QtCore.Qt.Key_PageDown, coin.SoKeyboardEvent.PAGE_DOWN)
        keyboardmap.insert(QtCore.Qt.Key_End,      coin.SoKeyboardEvent.END)

        keyboardmap.insert(QtCore.Qt.Key_F1,  coin.SoKeyboardEvent.F1)
        keyboardmap.insert(QtCore.Qt.Key_F2,  coin.SoKeyboardEvent.F2)
        keyboardmap.insert(QtCore.Qt.Key_F3,  coin.SoKeyboardEvent.F3)
        keyboardmap.insert(QtCore.Qt.Key_F4,  coin.SoKeyboardEvent.F4)
        keyboardmap.insert(QtCore.Qt.Key_F5,  coin.SoKeyboardEvent.F5)
        keyboardmap.insert(QtCore.Qt.Key_F6,  coin.SoKeyboardEvent.F6)
        keyboardmap.insert(QtCore.Qt.Key_F7,  coin.SoKeyboardEvent.F7)
        keyboardmap.insert(QtCore.Qt.Key_F8,  coin.SoKeyboardEvent.F8)
        keyboardmap.insert(QtCore.Qt.Key_F9,  coin.SoKeyboardEvent.F9)
        keyboardmap.insert(QtCore.Qt.Key_F10, coin.SoKeyboardEvent.F10)
        keyboardmap.insert(QtCore.Qt.Key_F11, coin.SoKeyboardEvent.F11)
        keyboardmap.insert(QtCore.Qt.Key_F12, coin.SoKeyboardEvent.F12)

        keyboardmap.insert(QtCore.Qt.Key_Backspace,  coin.SoKeyboardEvent.BACKSPACE)
        keyboardmap.insert(QtCore.Qt.Key_Tab,        coin.SoKeyboardEvent.TAB)
        keyboardmap.insert(QtCore.Qt.Key_Return,     coin.SoKeyboardEvent.RETURN)
        keyboardmap.insert(QtCore.Qt.Key_Enter,      coin.SoKeyboardEvent.ENTER)
        keyboardmap.insert(QtCore.Qt.Key_Pause,      coin.SoKeyboardEvent.PAUSE)
        keyboardmap.insert(QtCore.Qt.Key_ScrollLock, coin.SoKeyboardEvent.SCROLL_LOCK)
        keyboardmap.insert(QtCore.Qt.Key_Escape,     coin.SoKeyboardEvent.ESCAPE)
        keyboardmap.insert(QtCore.Qt.Key_Delete,     coin.SoKeyboardEvent.DELETE)
        keyboardmap.insert(QtCore.Qt.Key_Print,      coin.SoKeyboardEvent.PRINT)
        keyboardmap.insert(QtCore.Qt.Key_Insert,     coin.SoKeyboardEvent.INSERT)
        keyboardmap.insert(QtCore.Qt.Key_NumLock,    coin.SoKeyboardEvent.NUM_LOCK)
        keyboardmap.insert(QtCore.Qt.Key_CapsLock,   coin.SoKeyboardEvent.CAPS_LOCK)

        keyboardmap.insert(QtCore.Qt.Key_Space,        coin.SoKeyboardEvent.SPACE)
        keyboardmap.insert(QtCore.Qt.Key_Apostrophe,   coin.SoKeyboardEvent.APOSTROPHE)
        keyboardmap.insert(QtCore.Qt.Key_Comma,        coin.SoKeyboardEvent.COMMA)
        keyboardmap.insert(QtCore.Qt.Key_Minus,        coin.SoKeyboardEvent.MINUS)
        keyboardmap.insert(QtCore.Qt.Key_Period,       coin.SoKeyboardEvent.PERIOD)
        keyboardmap.insert(QtCore.Qt.Key_Slash,        coin.SoKeyboardEvent.SLASH)
        keyboardmap.insert(QtCore.Qt.Key_Semicolon,    coin.SoKeyboardEvent.SEMICOLON)
        keyboardmap.insert(QtCore.Qt.Key_Equal,        coin.SoKeyboardEvent.EQUAL)
        keyboardmap.insert(QtCore.Qt.Key_BracketLeft,  coin.SoKeyboardEvent.BRACKETLEFT)
        keyboardmap.insert(QtCore.Qt.Key_BracketRight, coin.SoKeyboardEvent.BRACKETRIGHT)
        keyboardmap.insert(QtCore.Qt.Key_Backslash,    coin.SoKeyboardEvent.BACKSLASH)
        keyboardmap.insert(QtCore.Qt.Key_Agrave,       coin.SoKeyboardEvent.GRAVE)

        # keypad

        # on Mac OS X, the keypad modifier will also be set when an arrow
        # key is pressed as the arrow keys are considered part of the
        # keypad

        keypadmap = QMap()

        keypadmap.insert(QtCore.Qt.Key_Left,     coin.SoKeyboardEvent.LEFT_ARROW)
        keypadmap.insert(QtCore.Qt.Key_Up,       coin.SoKeyboardEvent.UP_ARROW)
        keypadmap.insert(QtCore.Qt.Key_Right,    coin.SoKeyboardEvent.RIGHT_ARROW)
        keypadmap.insert(QtCore.Qt.Key_Down,     coin.SoKeyboardEvent.DOWN_ARROW)

        keypadmap.insert(QtCore.Qt.Key_Enter,    coin.SoKeyboardEvent.PAD_ENTER)
        keypadmap.insert(QtCore.Qt.Key_F1,       coin.SoKeyboardEvent.PAD_F1)
        keypadmap.insert(QtCore.Qt.Key_F2,       coin.SoKeyboardEvent.PAD_F2)
        keypadmap.insert(QtCore.Qt.Key_F3,       coin.SoKeyboardEvent.PAD_F3)
        keypadmap.insert(QtCore.Qt.Key_F4,       coin.SoKeyboardEvent.PAD_F4)
        keypadmap.insert(QtCore.Qt.Key_0,        coin.SoKeyboardEvent.PAD_0)
        keypadmap.insert(QtCore.Qt.Key_1,        coin.SoKeyboardEvent.PAD_1)
        keypadmap.insert(QtCore.Qt.Key_2,        coin.SoKeyboardEvent.PAD_2)
        keypadmap.insert(QtCore.Qt.Key_3,        coin.SoKeyboardEvent.PAD_3)
        keypadmap.insert(QtCore.Qt.Key_4,        coin.SoKeyboardEvent.PAD_4)
        keypadmap.insert(QtCore.Qt.Key_5,        coin.SoKeyboardEvent.PAD_5)
        keypadmap.insert(QtCore.Qt.Key_6,        coin.SoKeyboardEvent.PAD_6)
        keypadmap.insert(QtCore.Qt.Key_7,        coin.SoKeyboardEvent.PAD_7)
        keypadmap.insert(QtCore.Qt.Key_8,        coin.SoKeyboardEvent.PAD_8)
        keypadmap.insert(QtCore.Qt.Key_9,        coin.SoKeyboardEvent.PAD_9)
        keypadmap.insert(QtCore.Qt.Key_Plus,     coin.SoKeyboardEvent.PAD_ADD)
        keypadmap.insert(QtCore.Qt.Key_Minus,    coin.SoKeyboardEvent.PAD_SUBTRACT)
        keypadmap.insert(QtCore.Qt.Key_multiply, coin.SoKeyboardEvent.PAD_MULTIPLY)
        keypadmap.insert(QtCore.Qt.Key_division, coin.SoKeyboardEvent.PAD_DIVIDE)
        keypadmap.insert(QtCore.Qt.Key_Tab,      coin.SoKeyboardEvent.PAD_TAB)
        keypadmap.insert(QtCore.Qt.Key_Space,    coin.SoKeyboardEvent.PAD_SPACE)
        keypadmap.insert(QtCore.Qt.Key_Insert,   coin.SoKeyboardEvent.PAD_INSERT)
        keypadmap.insert(QtCore.Qt.Key_Delete,   coin.SoKeyboardEvent.PAD_DELETE)
        keypadmap.insert(QtCore.Qt.Key_Period,   coin.SoKeyboardEvent.PAD_PERIOD)

#    #if 0 // FIXME: don't know what to do with these (20070306 frodo)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.RIGHT_SHIFT)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.RIGHT_CONTROL)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.RIGHT_ALT)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.PRIOR)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.NEXT)
#        keyboardmap.insert(QtCore.Qt., coin.SoKeyboardEvent.SHIFT_LOCK)
#    #endif

        return keyboardmap, keypadmap
