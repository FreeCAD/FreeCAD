// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <App/PropertyLinks.h>

namespace App
{
class DocumentObject;
class DocumentObjectGroup;
}  // namespace App

namespace MbDFEM
{

bool appendUnique(App::PropertyLinkList& list, App::DocumentObject* object);
bool removeAll(App::PropertyLinkList& list, App::DocumentObject* object);

void addChildToListFolderAndGeoGroup(App::DocumentObject* owner,
                                     App::PropertyLinkList& list,
                                     App::DocumentObjectGroup* folder,
                                     App::DocumentObject* child);
void removeChildFromListFolderAndGeoGroup(App::DocumentObject* owner,
                                          App::PropertyLinkList& list,
                                          App::DocumentObjectGroup* folder,
                                          App::DocumentObject* child);
void removeChildFromMbDFEMSemanticOwners(App::DocumentObject* child,
                                         App::DocumentObject* exceptOwner = nullptr);

}  // namespace MbDFEM
