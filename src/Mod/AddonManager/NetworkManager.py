# -*- coding: utf-8 -*-

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2022 FreeCAD Project Association                        *
# *                                                                         *
# *   This program is free software; you can redistribute it and/or modify  *
# *   it under the terms of the GNU Lesser General Public License (LGPL)    *
# *   as published by the Free Software Foundation; either version 2 of     *
# *   the License, or (at your option) any later version.                   *
# *   for detail see the LICENSE text file.                                 *
# *                                                                         *
# *   This program is distributed in the hope that it will be useful,       *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this program; if not, write to the Free Software   *
# *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
# *   USA                                                                   *
# *                                                                         *
# ***************************************************************************


#############################################################################
#
# ABOUT NETWORK MANAGER
#
# A wrapper around QNetworkAccessManager providing proxy-handling
# capabilities, and simplified access to submitting requests from any
# application thread.
#
#
# USAGE
#
# Once imported, this file provides access to a global object called
# AM_NETWORK_MANAGER. This is a QObject running on the main thread, but
# designed to be interacted with from any other application thread. It
# provides two principal methods: submit_unmonitored_get() and
# submit_monitored_get(). Use the unmonitored version for small amounts of
# data (suitable for caching in RAM, and without a need to show a progress
# bar during download), and the monitored version for larger amounts of data.
# Both functions take a URL, and return an integer index. That index allows
# tracking of the completed request by attaching to the signals completed(),
# progress_made(), and progress_complete(). All three provide, as the first
# argument to the signal, the index of the request the signal refers to.
# Code attached to those signals should filter them to look for the indices
# of the requests they care about. Requests may complete in any order.
#
# A secondary blocking interface is also provided, for very short network
# accesses: the blocking_get() function blocks until the network transmission
# is complete, directly returning a QByteArray object with the received data.
# Do not run on the main GUI thread!


try:
    import FreeCAD

    if FreeCAD.GuiUp:
        import FreeCADGui

    HAVE_FREECAD = True
    translate = FreeCAD.Qt.translate
except Exception:
    # For standalone testing support working without the FreeCAD import
    HAVE_FREECAD = False

import threading
from PySide2 import QtCore

import os
import queue
import itertools
import tempfile
from typing import Dict, List, Optional

# This is the global instance of the NetworkManager that outside code
# should access
AM_NETWORK_MANAGER = None

HAVE_QTNETWORK = True
try:
    from PySide2 import QtNetwork
except Exception:
    if HAVE_FREECAD:
        FreeCAD.Console.PrintError(
            translate(
                "AddonsInstaller",
                "Could not import QtNetwork -- it does not appear to be installed on your system. Please install the package 'python3-pyside2.qtnetwork' on your system and if possible contact your FreeCAD package maintainer to alert them to the missing dependency. The Addon Manager will not be available.",
            )
            + "\n"
        )
    else:
        print(
            "Could not import QtNetwork, unable to test this file. Try installing the python3-pyside2.qtnetwork package."
        )
        exit(1)
    HAVE_QTNETWORK = False

if HAVE_QTNETWORK:

    class QueueItem:
        def __init__(
            self, index: int, request: QtNetwork.QNetworkRequest, track_progress: bool
        ):
            self.index = index
            self.request = request
            self.original_url = request.url()
            self.track_progress = track_progress

    class NetworkManager(QtCore.QObject):
        """A single global instance of NetworkManager is instantiated and stored as
        AM_NETWORK_MANAGER. Outside threads should send GET requests to this class by
        calling the submit_unmonitored_request() or submit_monitored_request() function,
        as needed. See the documentation of those functions for details."""

        # Connect to complete for requests with no progress monitoring (e.g. small amounts of data)
        completed = QtCore.Signal(
            int, int, QtCore.QByteArray
        )  # Index, http response code, received data (if any)

        # Connect to progress_made and progress_complete for large amounts of data, which get buffered into a temp file
        # That temp file should be deleted when your code is done with it
        progress_made = QtCore.Signal(
            int, int, int
        )  # Index, bytes read, total bytes (may be None)

        progress_complete = QtCore.Signal(
            int, int, os.PathLike
        )  # Index, http response code, filename

        __request_queued = QtCore.Signal()

        def __init__(self):
            super().__init__()

            self.counting_iterator = itertools.count()
            self.queue = queue.Queue()
            self.__last_started_index = 0
            self.__abort_when_found: List[int] = []
            self.replies: Dict[int, QtNetwork.QNetworkReply] = {}
            self.file_buffers = {}

            # We support an arbitrary number of threads using synchronous GET calls:
            self.synchronous_lock = threading.Lock()
            self.synchronous_complete: Dict[int, bool] = {}
            self.synchronous_result_data: Dict[int, QtCore.QByteArray] = {}

            # Make sure we exit nicely on quit
            QtCore.QCoreApplication.instance().aboutToQuit.connect(self.__aboutToQuit)

            # Create the QNAM on this thread:
            self.QNAM = QtNetwork.QNetworkAccessManager()
            self.QNAM.proxyAuthenticationRequired.connect(self.__authenticate_proxy)
            self.QNAM.authenticationRequired.connect(self.__authenticate_resource)

            qnam_cache = QtCore.QStandardPaths.writableLocation(
                QtCore.QStandardPaths.CacheLocation
            )
            os.makedirs(qnam_cache, exist_ok=True)
            self.diskCache = QtNetwork.QNetworkDiskCache()
            self.diskCache.setCacheDirectory(qnam_cache)
            self.QNAM.setCache(self.diskCache)

            self.monitored_connections: List[int] = []

            # Set up the proxy, if necesssary:
            noProxyCheck = True
            systemProxyCheck = False
            userProxyCheck = False
            proxy_string = ""
            if HAVE_FREECAD:
                pref = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Addons")
                noProxyCheck = pref.GetBool("NoProxyCheck", noProxyCheck)
                systemProxyCheck = pref.GetBool("SystemProxyCheck", systemProxyCheck)
                userProxyCheck = pref.GetBool("UserProxyCheck", userProxyCheck)
                proxy_string = pref.GetString("ProxyUrl", "")

                # Add some error checking to the proxy setup, since for historical reasons they
                # are independent booleans, rather than an enumeration:
                count = [noProxyCheck, systemProxyCheck, userProxyCheck].count(True)
                if count != 1:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            "Parameter error: mutually exclusive proxy options set. Resetting to default.",
                        )
                        + "\n"
                    )
                    noProxyCheck = True
                    systemProxyCheck = False
                    userProxyCheck = False
                    pref.SetBool("NoProxyCheck", noProxyCheck)
                    pref.SetBool("SystemProxyCheck", systemProxyCheck)
                    pref.SetBool("UserProxyCheck", userProxyCheck)

                if userProxyCheck and not proxy_string:
                    FreeCAD.Console.PrintWarning(
                        translate(
                            "AddonsInstaller",
                            "Parameter error: user proxy indicated, but no proxy provided. Resetting to default.",
                        )
                        + "\n"
                    )
                    noProxyCheck = True
                    userProxyCheck = False
                    pref.SetBool("NoProxyCheck", noProxyCheck)
                    pref.SetBool("UserProxyCheck", userProxyCheck)

            else:
                print("Please select a proxy type:")
                print("1) No proxy")
                print("2) Use system proxy settings")
                print("3) Custom proxy settings")
                result = input("Choice: ")
                if result == "1":
                    pass
                elif result == "2":
                    noProxyCheck = False
                    systemProxyCheck = True
                elif result == "3":
                    noProxyCheck = False
                    userProxyCheck = True
                    proxy_string = input("Enter your proxy server (host:port): ")
                else:
                    print(f"Got {result}, expected 1, 2, or 3.")
                    app.quit()

            if noProxyCheck:
                pass
            elif systemProxyCheck:
                query = QtNetwork.QNetworkProxyQuery(
                    QtCore.QUrl("https://github.com/FreeCAD/FreeCAD")
                )
                proxy = QtNetwork.QNetworkProxyFactory.systemProxyForQuery(query)
                if proxy and proxy[0]:
                    self.QNAM.setProxy(
                        proxy[0]
                    )  # This may still be QNetworkProxy.NoProxy
            elif userProxyCheck:
                host, _, port_string = proxy_string.rpartition(":")
                port = 0 if not port_string else int(port_string)
                # For now assume an HttpProxy, but eventually this should be a parameter
                proxy = QtNetwork.QNetworkProxy(
                    QtNetwork.QNetworkProxy.HttpProxy, host, port
                )
                self.QNAM.setProxy(proxy)

            # A helper connection for our blocking interface
            self.completed.connect(self.__synchronous_process_completion)

            # Set up our worker connection
            self.__request_queued.connect(self.__setup_network_request)

        def __aboutToQuit(self):
            pass

        def __setup_network_request(self):
            try:
                item = self.queue.get_nowait()
                if item:
                    if item.index in self.__abort_when_found:
                        self.__abort_when_found.remove(item.index)
                        return  # Do not do anything with this item, it's been aborted...
                    if item.track_progress:
                        self.monitored_connections.append(item.index)
                    self.__launch_request(item.index, item.request)
            except queue.Empty:
                pass

        def __launch_request(
            self, index: int, request: QtNetwork.QNetworkRequest
        ) -> None:
            reply = self.QNAM.get(request)
            self.replies[index] = reply

            self.__last_started_index = index
            reply.finished.connect(self.__reply_finished)
            reply.redirected.connect(self.__follow_redirect)
            reply.sslErrors.connect(self.__on_ssl_error)
            if index in self.monitored_connections:
                reply.readyRead.connect(self.__ready_to_read)
                reply.downloadProgress.connect(self.__download_progress)

        def submit_unmonitored_get(self, url: str) -> int:
            """Adds this request to the queue, and returns an index that can be used by calling code
            in conjunction with the completed() signal to handle the results of the call. All data is
            kept in memory, and the completed() call includes a direct handle to the bytes returned. It
            is not called until the data transfer has finished and the connection is closed."""

            current_index = next(self.counting_iterator)  # A thread-safe counter
            # Use a queue because we can only put things on the QNAM from the main event loop thread
            self.queue.put(
                QueueItem(
                    current_index, self.__create_get_request(url), track_progress=False
                )
            )
            self.__request_queued.emit()
            return current_index

        def submit_monitored_get(self, url: str) -> int:
            """Adds this request to the queue, and returns an index that can be used by calling code
            in conjunction with the progress_made() and progress_completed() signals to handle the
            results of the call. All data is cached to disk, and progress is reported periodically
            as the underlying QNetworkReply reports its progress. The progress_completed() signal
            contains a path to a temporary file with the stored data. Calling code should delete this
            file when done with it (or move it into its final place, etc.)."""

            current_index = next(self.counting_iterator)  # A thread-safe counter
            # Use a queue because we can only put things on the QNAM from the main event loop thread
            self.queue.put(
                QueueItem(
                    current_index, self.__create_get_request(url), track_progress=True
                )
            )
            self.__request_queued.emit()
            return current_index

        def blocking_get(self, url: str) -> Optional[QtCore.QByteArray]:
            """Submits a GET request to the QNetworkAccessManager and block until it is complete"""

            current_index = next(self.counting_iterator)  # A thread-safe counter
            with self.synchronous_lock:
                self.synchronous_complete[current_index] = False

            self.queue.put(
                QueueItem(
                    current_index, self.__create_get_request(url), track_progress=False
                )
            )
            self.__request_queued.emit()
            while True:
                if QtCore.QThread.currentThread().isInterruptionRequested():
                    return None
                QtCore.QCoreApplication.processEvents()
                with self.synchronous_lock:
                    if self.synchronous_complete[current_index]:
                        break

            with self.synchronous_lock:
                self.synchronous_complete.pop(current_index)
                if current_index in self.synchronous_result_data:
                    return self.synchronous_result_data.pop(current_index)
                else:
                    return None

        def __synchronous_process_completion(
            self, index: int, code: int, data: QtCore.QByteArray
        ) -> None:
            with self.synchronous_lock:
                if index in self.synchronous_complete:
                    if code == 200:
                        self.synchronous_result_data[index] = data
                    else:
                        FreeCAD.Console.PrintWarning(
                            translate(
                                "AddonsInstaller",
                                "Addon Manager: Unexpected {} response from server",
                            ).format(code)
                            + "\n"
                        )
                    self.synchronous_complete[index] = True

        def __create_get_request(self, url: str) -> QtNetwork.QNetworkRequest:
            request = QtNetwork.QNetworkRequest(QtCore.QUrl(url))
            request.setAttribute(
                QtNetwork.QNetworkRequest.RedirectPolicyAttribute,
                QtNetwork.QNetworkRequest.UserVerifiedRedirectPolicy,
            )
            request.setAttribute(
                QtNetwork.QNetworkRequest.CacheSaveControlAttribute, True
            )
            request.setAttribute(
                QtNetwork.QNetworkRequest.CacheLoadControlAttribute,
                QtNetwork.QNetworkRequest.PreferNetwork,
            )
            return request

        def abort_all(self):
            """Abort ALL network calls in progress, including clearing the queue"""
            for reply in self.replies:
                if reply.isRunning():
                    reply.abort()
            while True:
                try:
                    self.queue.get()
                    self.queue.task_done()
                except queue.Empty:
                    break

        def abort(self, index: int):
            if index in self.replies and self.replies[index].isRunning():
                self.replies[index].abort()
            elif index < self.__last_started_index:
                # It's still in the queue. Mark it for later destruction.
                self.__abort_when_found.append(index)

        def __authenticate_proxy(
            self,
            reply: QtNetwork.QNetworkProxy,
            authenticator: QtNetwork.QAuthenticator,
        ):
            if HAVE_FREECAD and FreeCAD.GuiUp:
                proxy_authentication = FreeCADGui.PySideUic.loadUi(
                    os.path.join(os.path.dirname(__file__), "proxy_authentication.ui")
                )
                proxy_authentication.setWindowFlag(QtCore.Qt.WindowStaysOnTopHint, True)
                # Show the right labels, etc.
                proxy_authentication.labelProxyAddress.setText(
                    f"{reply.hostName()}:{reply.port()}"
                )
                if authenticator.realm():
                    proxy_authentication.labelProxyRealm.setText(authenticator.realm())
                else:
                    proxy_authentication.labelProxyRealm.hide()
                    proxy_authentication.labelRealmCaption.hide()
                result = proxy_authentication.exec()
                if result == QtWidgets.QDialogButtonBox.Ok:
                    authenticator.setUser(proxy_authentication.lineEditUsername.text())
                    authenticator.setPassword(
                        proxy_authentication.lineEditPassword.text()
                    )
            else:
                username = input("Proxy username: ")
                import getpass

                password = getpass.getpass()
                authenticator.setUser(username)
                authenticator.setPassword(password)

        def __authenticate_resource(
            self,
            _reply: QtNetwork.QNetworkReply,
            _authenticator: QtNetwork.QAuthenticator,
        ):
            pass

        def __follow_redirect(self, url):
            sender = self.sender()
            if sender:
                for index, reply in self.replies.items():
                    if reply == sender:
                        current_index = index
                        break

                sender.abort()
                self.__launch_request(current_index, self.__create_get_request(url))

        def __on_ssl_error(self, reply: str, errors: List[str]):
            if HAVE_FREECAD:
                FreeCAD.Console.PrintWarning(
                    translate("AddonsInstaller", "Error with encrypted connection")
                    + "\n:"
                )
                FreeCAD.Console.PrintWarning(reply)
                for error in errors:
                    FreeCAD.Console.PrintWarning(error)
            else:
                print("Error with encrypted connection")
                for error in errors:
                    print(error)

        def __download_progress(self, bytesReceived: int, bytesTotal: int) -> None:
            sender = self.sender()
            if not sender:
                return
            for index, reply in self.replies.items():
                if reply == sender:
                    self.progress_made.emit(index, bytesReceived, bytesTotal)
                    return

        def __ready_to_read(self) -> None:
            sender = self.sender()
            if not sender:
                return

            for index, reply in self.replies.items():
                if reply == sender:
                    self.__data_incoming(index, reply)
                    return

        def __data_incoming(self, index: int, reply: QtNetwork.QNetworkReply) -> None:
            if not index in self.replies:
                # We already finished this reply, this is a vestigial signal
                return
            buffer = reply.readAll()
            if not index in self.file_buffers:
                f = tempfile.NamedTemporaryFile("wb", delete=False)
                self.file_buffers[index] = f
            else:
                f = self.file_buffers[index]
            try:
                f.write(buffer.data())
            except Exception as e:
                if HAVE_FREECAD:
                    FreeCAD.Console.PrintError(
                        f"Network Manager internal error: {str(e)}"
                    )
                else:
                    print(f"Network Manager internal error: {str(e)}")

        def __reply_finished(self) -> None:
            reply = self.sender()
            if not reply:
                print(
                    "Network Manager Error: __reply_finished not called by a Qt signal"
                )
                return

            if (
                reply.error()
                == QtNetwork.QNetworkReply.NetworkError.OperationCanceledError
            ):
                # Silently do nothing
                return

            index = None
            for key, value in self.replies.items():
                if reply == value:
                    index = key
                    break
            if index is None:
                print(f"Lost net request for {reply.url()}")
                return

            response_code = reply.attribute(
                QtNetwork.QNetworkRequest.HttpStatusCodeAttribute
            )
            self.queue.task_done()
            if reply.error() == QtNetwork.QNetworkReply.NetworkError.NoError:
                if index in self.monitored_connections:
                    # Make sure to read any remaining data
                    self.__data_incoming(index, reply)
                    self.monitored_connections.remove(index)
                    f = self.file_buffers[index]
                    f.close()
                    self.progress_complete.emit(index, response_code, f.name)
                else:
                    data = reply.readAll()
                    self.completed.emit(index, response_code, data)
            else:
                if index in self.monitored_connections:
                    self.progress_complete.emit(index, response_code, "")
                else:
                    self.completed.emit(index, response_code, None)
            self.replies.pop(index)

else:  # HAVE_QTNETWORK is false:

    class NetworkManager(QtCore.QObject):
        """A dummy class to enable an offline mode when the QtNetwork package is not yet installed"""

        completed = QtCore.Signal(
            int, int, bytes
        )  # Emitted as soon as the request is made, with a connection failed error
        progress_made = QtCore.Signal(
            int, int, int
        )  # Never emitted, no progress is made here
        progress_complete = QtCore.Signal(
            int, int, os.PathLike
        )  # Emitted as soon as the request is made, with a connection failed error

        def __init__(self):
            super().__init__()
            self.monitored_queue = queue.Queue()
            self.unmonitored_queue = queue.Queue()

        def submit_unmonitored_request(self, _) -> int:
            current_index = next(itertools.count())
            self.unmonitored_queue.put(current_index)
            return current_index

        def submit_monitored_request(self, _) -> int:
            current_index = next(itertools.count())
            self.monitored_queue.put(current_index)
            return current_index

        def blocking_get(self, _: str) -> QtCore.QByteArray:
            return None

        def abort_all(
            self,
        ):
            pass  # Nothing to do

        def abort(self, _):
            pass  # Nothing to do


def InitializeNetworkManager():
    global AM_NETWORK_MANAGER
    if AM_NETWORK_MANAGER is None:
        AM_NETWORK_MANAGER = NetworkManager()


if __name__ == "__main__":

    app = QtCore.QCoreApplication()

    InitializeNetworkManager()

    count = 0

    # For testing, create several network requests and send them off in quick succession:
    # (Choose small downloads, no need for significant data)
    urls = [
        "https://api.github.com/zen",
        "http://climate.ok.gov/index.php/climate/rainfall_table/local_data",
        "https://tigerweb.geo.census.gov/arcgis/rest/services/TIGERweb/AIANNHA/MapServer",
    ]

    def handle_completion(index: int, code: int, data):
        global count
        if code == 200:
            print(
                f"For request {index+1}, response was {data.size()} bytes.", flush=True
            )
        else:
            print(
                f"For request {index+1}, request failed with HTTP result code {code}",
                flush=True,
            )

        count += 1
        if count >= len(urls):
            print(f"Shutting down...", flush=True)
            AM_NETWORK_MANAGER.requestInterruption()
            AM_NETWORK_MANAGER.wait(5000)
            app.quit()

    AM_NETWORK_MANAGER.completed.connect(handle_completion)
    for url in urls:
        AM_NETWORK_MANAGER.submit_unmonitored_get(url)

    app.exec_()

    print("Done with all requests.")
