// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Céleste Wouters <foss@elementw.net>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

// windows.h must be kept above commdlg.h and shlobj.h
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

#include <QDir>
#include <QFileInfo>
#include <QList>
#include <QPair>
#include <QString>

#include <Base/Console.h>

#include "FileDialogInternal.h"


/* The enum & structs below are part of Win32 private APIs, which are the only
 * way to force hiding the patterns on file dialog filters.
 * See https://elementw.net/posts/microsoft-bs-ifiledialog/ for the investigation
 * that led to the discovery of those.
 */

using FileDialogPrivateOptions = DWORD;
static constexpr const FileDialogPrivateOptions FDPO_DONT_FORCE_SHOW_EXTS = 0x10;

struct IFileDialogPrivate_W7Vtbl;
struct IFileDialogPrivate_W7
{
    IFileDialogPrivate_W7Vtbl* lpVtbl;
};
struct IFileDialogPrivate_W7Vtbl
{
    HRESULT (*QueryInterface)(IFileDialogPrivate_W7*, IID*, void**);
    ULONG (*AddRef)(IFileDialogPrivate_W7*);
    ULONG (*Release)(IFileDialogPrivate_W7*);
    HRESULT (*GetPrivateOptions)(IFileDialogPrivate_W7*, FileDialogPrivateOptions*);
    HRESULT (*SetPrivateOptions)(IFileDialogPrivate_W7*, FileDialogPrivateOptions);
};

struct IFileDialogPrivate_W10Vtbl;
struct IFileDialogPrivate_W10
{
    IFileDialogPrivate_W10Vtbl* lpVtbl;
};
struct IFileDialogPrivate_W10Vtbl
{
    HRESULT (*QueryInterface)(IFileDialogPrivate_W10*, IID*, void**);
    ULONG (*AddRef)(IFileDialogPrivate_W10*);
    ULONG (*Release)(IFileDialogPrivate_W10*);
    HRESULT (*HideControlsForHostedPickerProviderApp)(IFileDialogPrivate_W10*);
    HRESULT (*EnableControlsForHostedPickerProviderApp)(IFileDialogPrivate_W10*);
    HRESULT (*GetPrivateOptions)(IFileDialogPrivate_W10*, FileDialogPrivateOptions*);
    HRESULT (*SetPrivateOptions)(IFileDialogPrivate_W10*, FileDialogPrivateOptions);
};

#define INITGUID
#include <initguid.h>
// clang-format off
DEFINE_GUID(IID_IFileDialogPrivate_W7, 0xAC92FFC5, 0xF0E9, 0x455A, 0x90, 0x6B, 0x4A, 0x83, 0xE7, 0x4A, 0x80, 0x3B);
DEFINE_GUID(IID_IFileDialogPrivate_W10, 0x9EA5491C, 0x89C8, 0x4BEF, 0x93, 0xD3, 0x7F, 0x66, 0x5F, 0xB8, 0x2A, 0x33);
// clang-format on

using namespace Gui;
using namespace Gui::FileDialogInternal;

static std::unique_ptr<wchar_t[]> qStringToWCharArray(const QString& s, size_t reserveSize = 0)
{
    const size_t stringSize = s.size();
    wchar_t* result = new wchar_t[qMax(stringSize + 1, reserveSize)];
    s.toWCharArray(result);
    result[stringSize] = 0;
    return std::unique_ptr<wchar_t[]>(result);
}

/* The legacy Get{Open,Save}FileNameW() functions never change how the file filters are
 * displayed, inherently avoiding the problem in issue #23139.
 */
static QStringList legacyNativeFileDialog(
    NativeFileDialogMode mode,
    QWidget* parent,
    const QString& caption,
    const QString& startPath,
    const FileDialog::FilterList& filters,
    qsizetype& selectedFilterIndex,
    FileDialog::Options options
)
{
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    if (parent) {
        ofn.hwndOwner = HWND(parent->winId());
    }

    // Populate file filters
    const bool showPatterns = getPreferShowFilterPatterns();
    QString flatFilter;
    for (const auto& filter : filters) {
        flatFilter += getFilterDisplayName(filter, showPatterns);
        flatFilter += QLatin1Char('\0');
        flatFilter += filter.patterns.join(QLatin1Char(';'));
        flatFilter += QLatin1Char('\0');
    }
    flatFilter += QLatin1Char('\0');
    auto ofnFilter = qStringToWCharArray(flatFilter);
    ofn.lpstrFilter = ofnFilter.get();

    // Select active filter
    if (selectedFilterIndex >= 0) {
        ofn.nFilterIndex = selectedFilterIndex + 1;  // OPENFILENAMEW index is 1-based
    }

    // Pre-select file name, if any
    constexpr const DWORD SelectionBufferSize = 65535;
    auto selectedFile = std::make_unique<wchar_t[]>(SelectionBufferSize);

    const QFileInfo startPathInfo(startPath);
    startPathInfo.fileName().toWCharArray(selectedFile.get());
    selectedFile[startPathInfo.fileName().size()] = L'\0';

    ofn.nMaxFile = SelectionBufferSize;
    ofn.lpstrFile = selectedFile.get();

    // Select starting directory, if any
    auto initialDir = qStringToWCharArray(QDir::toNativeSeparators(startPath));
    ofn.lpstrInitialDir = initialDir.get();

    // Set title
    auto title = qStringToWCharArray(caption);
    ofn.lpstrTitle = title.get();

    // Mode flags
    ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_EXPLORER | OFN_PATHMUSTEXIST;
    if (mode == NativeFileDialogMode::OpenSingle || mode == NativeFileDialogMode::OpenMultiple) {
        ofn.Flags |= OFN_FILEMUSTEXIST;
    }

    // Show the dialog
    BOOL ok = FALSE;
    if (mode == NativeFileDialogMode::OpenSingle) {
        ok = ::GetOpenFileNameW(&ofn);
    }
    else if (mode == NativeFileDialogMode::OpenMultiple) {
        ofn.Flags |= OFN_ALLOWMULTISELECT;
        ok = ::GetOpenFileNameW(&ofn);
    }
    else /* (mode == NativeFileDialogMode::Save) */ {
        ok = ::GetSaveFileNameW(&ofn);
    }

    QStringList selected;
    // Always return a selected filter index >= 0, but
    // ofn.nFilterIndex, while 1-based, might be 0 if the user cancelled.
    selectedFilterIndex = ofn.nFilterIndex == 0 ? 0 : (ofn.nFilterIndex - 1);
    if (ok != FALSE) {
        const QString dir = QDir::cleanPath(QString::fromWCharArray(ofn.lpstrFile));
        selected += dir;
        if (ofn.Flags & OFN_ALLOWMULTISELECT) {
            const wchar_t* ptr = ofn.lpstrFile + dir.size() + 1;
            if (*ptr) {
                selected.clear();
                const QString path = dir + L'/';
                while (*ptr) {
                    const QString fileName = QString::fromWCharArray(ptr);
                    selected += path + fileName;
                    ptr += fileName.size() + 1;
                }
            }
        }
    }
    return selected;
}

struct ComRuntime
{
    ComRuntime()
    {
        CoInitialize(nullptr);
    }
    ~ComRuntime()
    {
        CoUninitialize();
    }
};

// WRL ComPtr-like wrapper, without the need for WRL
template<typename T>
struct ComPointer
{
    T* ptr;
    ComPointer()
        : ptr(nullptr)
    {}
    ~ComPointer()
    {
        if (ptr) {
            ptr->Release();
        }
    }
    T* operator->() const
    {
        return ptr;
    }
    operator bool() const
    {
        return ptr != nullptr;
    }
    operator T*() const
    {
        return ptr;
    }
    void** ppvObject()
    {
        return reinterpret_cast<void**>(&ptr);
    }
    // An `operator T**` would exist here if type conversion rules to bool
    // did not make them ambiguous even in the presence of `operator bool`.
};

/* Try using the Vista+ IFileDialog APIs first with the private interfaces,
 * falling back to legacy functions if said private APIs aren't available.
 *
 * Note neither IFileDialog or the legacy functions are valid for UWP WinRT,
 * for which Windows::Storage::Pickers::FileOpenPicker will have to be used instead.
 */
QStringList FileDialogInternal::nativeFileDialog(
    NativeFileDialogMode mode,
    QWidget* parent,
    const QString& caption,
    const QString& startPath,
    const FileDialog::FilterList& filters,
    qsizetype& selectedFilterIndex,
    FileDialog::Options options
)
{
    ComRuntime com;
    const bool isOpen = mode == NativeFileDialogMode::OpenSingle
        || mode == NativeFileDialogMode::OpenMultiple;

    ComPointer<IFileDialog> fileDialog;
    if (FAILED(CoCreateInstance(
            isOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog,
            nullptr,
            CLSCTX_INPROC_SERVER,
            isOpen ? IID_IFileOpenDialog : IID_IFileSaveDialog,
            fileDialog.ppvObject()
        ))) {
        Base::Console().warning("Failed to create IFileDialog, falling back on GetOpen/SaveFileNameW");
        return legacyNativeFileDialog(mode, parent, caption, startPath, filters, selectedFilterIndex, options);
    }

    // These aren't wrapped interfaces, can't use ComPointer on them
    IFileDialogPrivate_W10* w10;
    IFileDialogPrivate_W7* w7;
    // Set the private flag to hide filter patterns
    if (SUCCEEDED(
            fileDialog->QueryInterface(IID_IFileDialogPrivate_W10, reinterpret_cast<void**>(&w10))
        )) {
        FileDialogPrivateOptions flags;
        w10->lpVtbl->GetPrivateOptions(w10, &flags);
        w10->lpVtbl->SetPrivateOptions(w10, flags | FDPO_DONT_FORCE_SHOW_EXTS);
        w10->lpVtbl->Release(w10);
    }
    else if (
        SUCCEEDED(fileDialog->QueryInterface(IID_IFileDialogPrivate_W7, reinterpret_cast<void**>(&w7)))
    ) {
        FileDialogPrivateOptions flags;
        w7->lpVtbl->GetPrivateOptions(w7, &flags);
        w7->lpVtbl->SetPrivateOptions(w7, flags | FDPO_DONT_FORCE_SHOW_EXTS);
        w7->lpVtbl->Release(w7);
    }
    else {
        // No private API, fall back to legacy and emit warning
        Base::Console()
            .warning("FileDialog fell back on legacy GetOpen/SaveFileNameW, the IFileDialog private API needs to be updated");
        return legacyNativeFileDialog(mode, parent, caption, startPath, filters, selectedFilterIndex, options);
    }

    // Populate file filters
    const bool showPatterns = getPreferShowFilterPatterns();
    std::vector<std::pair<QString, QString>> qstringFilterSpecs;
    qstringFilterSpecs.reserve(filters.size());
    qsizetype totalStringLength = 0;
    for (const auto& filter : filters) {
        const auto display = getFilterDisplayName(filter, showPatterns);
        totalStringLength += display.size() + 1;
        const auto patterns = filter.patterns.join(QLatin1Char(';'));
        totalStringLength += patterns.size() + 1;
        qstringFilterSpecs.emplace_back(std::move(display), std::move(patterns));
    }
    auto comFilterSpecs = std::make_unique<COMDLG_FILTERSPEC[]>(filters.size());
    auto comFilterBuffer = std::make_unique<wchar_t[]>(totalStringLength);
    auto ptr = comFilterBuffer.get();
    for (qsizetype i = 0; i < filters.size(); ++i) {
        comFilterSpecs[i].pszName = ptr;
        ptr += qstringFilterSpecs[i].first.toWCharArray(ptr);
        *ptr++ = 0;
        comFilterSpecs[i].pszSpec = ptr;
        ptr += qstringFilterSpecs[i].second.toWCharArray(ptr);
        *ptr++ = 0;
    }
    fileDialog->SetFileTypes(filters.size(), comFilterSpecs.get());

    // Enable appending extension of currently selected filter
    fileDialog->SetDefaultExtension(L"");

    // Select active filter
    if (selectedFilterIndex >= 0) {
        fileDialog->SetFileTypeIndex(selectedFilterIndex + 1);  // FileTypeIndex is 1-based
    }

    // Pre-select file name, if any
    const QFileInfo startPathInfo(startPath);
    const auto startFileName = qStringToWCharArray(startPathInfo.fileName());
    fileDialog->SetFileName(startFileName.get());

    // Select starting directory, if any
    if (startPathInfo.isAbsolute()) {
        const auto startFolder = qStringToWCharArray(startPathInfo.absoluteDir().path());
        ComPointer<IShellItem> shellStartFolder;
        if (SUCCEEDED(SHCreateItemFromParsingName(
                startFolder.get(),
                nullptr,
                IID_IShellItem,
                shellStartFolder.ppvObject()
            ))) {
            fileDialog->SetFolder(shellStartFolder);
        }
    }

    // Set title
    auto title = qStringToWCharArray(caption);
    fileDialog->SetTitle(title.get());

    // Mode flags
    FILEOPENDIALOGOPTIONS fos = FOS_NOCHANGEDIR | FOS_PATHMUSTEXIST;
    if (mode == NativeFileDialogMode::OpenSingle) {
        fos |= FOS_FILEMUSTEXIST;
    }
    else if (mode == NativeFileDialogMode::OpenMultiple) {
        fos |= FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT;
    }
    else /* (mode == NativeFileDialogMode::Save) */ {
        fos |= FOS_NOREADONLYRETURN | FOS_OVERWRITEPROMPT;
    }
    fileDialog->SetOptions(fos);

    // Show the dialog
    if (FAILED(fileDialog->Show(HWND(parent ? parent->winId() : 0)))) {
        return {};
    }

    // Always return a selected filter index >= 0, but
    // FileTypeIndex, while 1-based, might be 0 if the user cancelled.
    UINT fileTypeIndex;
    fileDialog->GetFileTypeIndex(&fileTypeIndex);
    selectedFilterIndex = fileTypeIndex == 0 ? 0 : (fileTypeIndex - 1);

    QStringList selectedFiles;
    if (isOpen) {
        const auto openFileDialog = reinterpret_cast<IFileOpenDialog*>(fileDialog.ptr);
        ComPointer<IShellItemArray> results;
        if (SUCCEEDED(openFileDialog->GetResults(&results.ptr)) && results) {
            DWORD itemCount;
            if (FAILED(results->GetCount(&itemCount))) {
                Base::Console().error("Failed to enumerate IShellItemArray");
                return selectedFiles;
            }
            for (DWORD i = 0; i < itemCount; ++i) {
                ComPointer<IShellItem> item;
                if (FAILED(results->GetItemAt(i, &item.ptr))) {
                    Base::Console().error("Failed to get IShellItemArray item #%d", i);
                    return selectedFiles;
                }
                PWSTR itemPath;
                if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &itemPath))) {
                    Base::Console().error("Failed to get path of IShellItemArray item #%d", i);
                    return selectedFiles;
                }
                selectedFiles.append(QDir::cleanPath(QString::fromWCharArray(itemPath)));
                CoTaskMemFree(itemPath);
            }
        }
        else {
            Base::Console().error("Failed to get file picker IShellItemArray results");
        }
    }
    else {
        ComPointer<IShellItem> item;
        if (SUCCEEDED(fileDialog->GetResult(&item.ptr)) && item) {
            PWSTR itemPath = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &itemPath))) {
                selectedFiles.append(QDir::cleanPath(QString::fromWCharArray(itemPath)));
                CoTaskMemFree(itemPath);
            }
            else {
                Base::Console().error("Failed to get path of IShellItem");
            }
        }
        else {
            Base::Console().error("Failed to get file picker IShellItem result");
        }
    }

    return selectedFiles;
}
