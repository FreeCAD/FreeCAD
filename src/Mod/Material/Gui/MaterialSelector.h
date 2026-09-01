// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <QPushButton>

#include <vector>

#include <App/DocumentObserver.h>
#include <Mod/Material/MaterialGlobal.h>

namespace MatGui
{

/** A compact material picker for one or more document objects.
 *
 * The selector displays the common material name, up to two distinct names,
 * or an ellipsis for a larger mixed selection. Clicking it opens the material
 * tree and assigns the chosen material to every writable ShapeMaterial
 * property in one undoable transaction per document.
 */
class MatGuiExport MaterialSelector: public QPushButton
{
    Q_OBJECT
    Q_PROPERTY(bool requirePhysical READ requirePhysical WRITE setRequirePhysical)

public:
    explicit MaterialSelector(QWidget* parent = nullptr);

    void setObjects(const std::vector<App::DocumentObject*>& objects);
    void clearObjects();
    void refresh();

    bool requirePhysical() const;
    void setRequirePhysical(bool required);

Q_SIGNALS:
    void materialChanged();

private Q_SLOTS:
    void chooseMaterial();

private:
    std::vector<App::DocumentObjectT> assignedObjects;
    bool physicalRequired = false;
};

}  // namespace MatGui
