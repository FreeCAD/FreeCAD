/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef GUI_VIEW_H
#define GUI_VIEW_H

#include <Base/BaseClass.h>
#include <FCGlobal.h>

namespace App
{
class Document;
}

namespace Gui
{
class Document;
class ViewProvider;

/** Base class of all windows belonging to a document
 *  there are two ways of belonging to a document. The
 *  first way is to a fixed one. The second way is to always
 *  belonging to the Active document. that means switching every time
 *  the active document is changing. It also means that the view
 *  belongs sometimes to no document at all!
 *  @see TreeView
 *  @see Gui::Document
 *  @see Application
 *  @author Juergen Riegel
 */
class GuiExport BaseView : public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** View constructor
     * Attach the view to the given document. If the document is 0
     * the view will attach to the active document. Be aware! there isn't
     * always an active document!
     */
    BaseView(Gui::Document* pcDocument=nullptr);
    /** View destructor
     * Detach the view from the document, if attached!
     */
    ~BaseView() override;


    /** @name methods used by the Application and the GuiDocument
     */
    //@{
    /// sets the view to another document (called by Application)
    void setDocument(Gui::Document* pcDocument);
    /// is sent from the document in order to close the document
    void onClose();
    //@}

    /// returns the document the view is attached to
    Gui::Document* getGuiDocument() const {return _pcDocument;}
    /// returns the document the view is attached to
    App::Document* getAppDocument() const;
    /// indicates if the view is in passive mode
    bool isPassive() const {return bIsPassive;}

    /** @name methods to override
     */
    //@{
    /// get called when the document is updated
    virtual void onUpdate(){}
    /// get called when the document is relabeled (change of its user name)
    virtual void onRelabel(Gui::Document *){}
    /// get called when the document is renamed (change of its internal name)
    virtual void onRename(Gui::Document *){}
    /// returns the name of the view (important for messages)
    virtual const char *getName() const
    { return "Base view"; }
    /// Message handler
    virtual bool onMsg(const char* pMsg, const char** ppReturn)=0;
    /// Message handler test
    virtual bool onHasMsg(const char* pMsg) const=0;
    /// overwrite when checking on close state
    virtual bool canClose(){return true;}
    /// delete itself
    virtual void deleteSelf();
    //@}

protected:
    Gui::Document*  _pcDocument;
    bool bIsDetached{false};
    bool bIsPassive{false};
};

} // namespace Gui

#endif // GUI_VIEW_H
