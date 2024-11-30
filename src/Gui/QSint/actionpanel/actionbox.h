/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef ACTIONBOX_H
#define ACTIONBOX_H

#include "actionlabel.h"

#include <QLabel>
#include <QVBoxLayout>


namespace QSint
{


/**
    \brief Class representing a panel of actions similar to Windows Vista/7 control panel items.
    \since 0.2

    \image html ActionBox.png An example of ActionBox

    ActionBox normally consists of an icon, clickable header and a list of actions.
    Every action can have own icon as well, provide tooltips and status tips,
    be clickable and checkable etc. i.e. behave like a normal ActionLabel.

    ActionBox objects are easily customizable via CSS technology - you can get
    different look just by writing corresponding style sheet.

    <b>Usage of ActionBox in the application</b>

    1. Create ActionBox using constructor (or in Designer as Promoted Objects).
    Icon and header text can be passed to the constructor as well. For example:

    \code
    ActionBox *box1 = new ActionBox(":/icons/box1icon.png", "Header Text", this);
    \endcode

    2. ActionBox header itself is a clickable item (based on ActionLabel), so you
    can retrieve it and use, for example, to connect with a slot:

    \code
    connect(box1->header(), SIGNAL(clicked()), this, SLOT(header1clicked()));
    \endcode

    3. To create an action, use one of createItem() functions. For example:

    \code
    ActionLabel *action1 = box1->createItem(":/icons/action1icon.png", "Action1 Text");
    connect(action1, SIGNAL(clicked()), this, SLOT(action1clicked()));

    ActionLabel *action2 = box1->createItem(":/icons/action2icon.png", "Action2 Text");
    connect(action2, SIGNAL(clicked()), this, SLOT(action2clicked()));
    \endcode

    createItem() also allows to create an ActionLabel from already existing QAction:

    \code
    QAction myAction3(":/icons/action3icon.png", "Action3 Text");
    connect(myAction3, SIGNAL(clicked()), this, SLOT(action3clicked()));

    ActionLabel *action3 = box1->createItem(myAction3);
    \endcode

    4. By default, actions are arranged vertically, one per row. In order
    to have more than one actions in a row, first add horizontal layout item
    using createHBoxLayout() function, and then pass it to the createItem() to
    create actions.

    \code
    // create horizontal layout
    QLayout *hbl1 = box1->createHBoxLayout();
    // create actions using this layout
    ActionLabel *action3 = box1->createItem(":/icons/action3icon.png", "1st action in row", hbl1);
    ActionLabel *action4 = box1->createItem("2nd action in row", hbl1);
    \endcode

    5. Sometimes you would like to have a spacer between the items. Use createSpacer()
    function to insert an empty space into default or specified layout.

    \code
    // create a spacer after two actions added before
    box1->createSpacer(hbl1);
    // create another action which will be preceded by the empty space (i.e. right-aligned)
    ActionLabel *action5 = box1->createItem("3rd action in row", hbl1);
    \endcode

    6. You can insert arbitrary layout items and widgets into ActionBox using
    addLayout() and addWidgets() functions. Please note that ownership of these items
    transferred to ActionBox, so you must not delete them manually.

    <b>Customization of ActionBox via CSS</b>

    ActionBox items can be easily customized using CSS mechanism. Just create a new
    style and apply it to the ActionBox with setStyleSheet().

    See the following example of the complete ActionBox customization. Note that
    \a QSint--ActionBox is used as a main class name. Headers are ActionLabels with
    property \a class='header'. Actions are ActionLabels with
    property \a class='action'.

    \code
    // define a string representing CSS style
    const char* ActionBoxNewStyle =

        // customization of ActionBox
        "QSint--ActionBox {"
            "background-color: white;"
            "border: 1px solid white;"
            "border-radius: 3px;"
            "text-align: left;"
        "}"

        "QSint--ActionBox:hover {"
            "background-color: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, stop: 0 #F9FDFF, stop: 1 #EAF7FF);"
            "border: 1px solid #DAF2FC;"
        "}"

        // customization of ActionBox's header
        "QSint--ActionBox QSint--ActionLabel[class='header'] {"
            "text-align: left;"
            "font: 14px;"
            "color: #006600;"
            "background-color: transparent;"
            "border: none;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='header']:hover {"
            "color: #00cc00;"
            "text-decoration: underline;"
        "}"

        // customization of ActionBox's actions
        "QSint--ActionBox QSint--ActionLabel[class='action'] {"
            "background-color: transparent;"
            "border: none;"
            "color: #0033ff;"
            "text-align: left;"
            "font: 11px;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:hover {"
            "color: #0099ff;"
            "text-decoration: underline;"
        "}"

        "QSint--ActionBox QSint--ActionLabel[class='action']:on {"
            "background-color: #ddeeff;"
            "color: #006600;"
        "}"
    ;

    // apply the style
    box1->setStyleSheet(ActionBoxNewStyle);
    \endcode

*/
class QSINT_EXPORT ActionBox : public QFrame
{
    Q_OBJECT

    Q_PROPERTY(QPixmap icon READ icon WRITE setIcon) // clazy:exclude=qproperty-without-notify
    Q_PROPERTY(ActionLabel header READ header) // clazy:exclude=qproperty-without-notify

public:
    /** Constructor.
      */
    explicit ActionBox(QWidget *parent = nullptr);
    /** Constructor.
      */
    explicit ActionBox(const QString & headerText, QWidget *parent = nullptr);
    /** Constructor.
      */
    explicit ActionBox(const QPixmap & icon, const QString & headerText, QWidget *parent = nullptr);

    /** Sets icon of the ActionBox to \a icon.
      */
    void setIcon(const QPixmap & icon);
    /** Returns icon of the ActionBox.
      */
    QPixmap icon() const;// { return iconLabel->pixmap(); }

    /** Returns header item of the ActionBox.
      */
    inline ActionLabel* header() const { return headerLabel; }

    /** Creates action item from the \a action and returns it.

      By default, action is added to the default vertical layout, i.e. subsequent
      calls of this function will create several actions arranged vertically,
      one below another.

      You can add action to the specified layout passing it as \a l parameter.
      This allows to do custom actions arrangements, i.e. horizontal etc.

      \since 0.2
      */
    ActionLabel* createItem(QAction * action, QLayout * l = nullptr);

    /** Creates action items from the \a actions list and returns the list of action items.
      \since 0.2
      */
    QList<ActionLabel*> createItems(QList<QAction*> actions);

    /** Adds an action with \a text to the ActionBox and returns action item.
      */
    ActionLabel* createItem(const QString & text = QString(), QLayout * l = nullptr);
    /** Adds an action with \a icon and \a text to the ActionBox and returns action item.

      This function acts just like previous one. See the description above.
      */
    ActionLabel* createItem(const QPixmap & icon, const QString & text, QLayout * l = nullptr);

    /** Adds a spacer and returns spacer item.

      By default, a spacer is added to the default vertical layout.
      You can add a spacer to the specified layout passing it as \a l parameter.
      */
    QSpacerItem* createSpacer(QLayout * l = nullptr);

    /** Creates empty horizontal layout.

      Use this function to arrange action items into a row.
      */
    QLayout* createHBoxLayout();

    /** Returns default layout used for actions (typically it's QVBoxLayout).
      */
    inline QLayout* itemLayout() const { return dataLayout; }

    /** Adds layout \a l to the default layout.
      */
    void addLayout(QLayout * l);
    /** Adds widget \a w to the layout.

      By default, widget is added to the default vertical layout.
      You can add widget to the specified layout passing it as \a l parameter.
      */
    void addWidget(QWidget * w, QLayout * l = nullptr);

    QSize minimumSizeHint() const override;

protected:
    void init();

    QVBoxLayout *dataLayout;
    QLabel *iconLabel;
    ActionLabel *headerLabel;
};


} // namespace

#endif // ACTIONBOX_H
