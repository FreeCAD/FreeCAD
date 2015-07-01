#ifndef ACTIONLABEL_H
#define ACTIONLABEL_H

#include <QToolButton>


namespace QSint
{


/**
    \brief Class representing an action similar to Windows Vista/7 control panel item.

    \image html ActionLabel.png An example of ActionLabel

    ActionLabel normally consists of an icon and text.
    It also can have tooltip and status tip,
    be clickable and checkable etc. i.e. behave like a normal QToolButton.

    <b>Customization of ActionLabel via CSS</b>

    ActionLabel objects are easily customizable via CSS technology - you can get
    different look just by writing corresponding style sheet and applying it with setStyleSheet().

    See the following example of the complete ActionLabel customization. Note that
    \a QSint--ActionLabel is used as a main class name.

    \code
    // define a string representing CSS style
    const char* ActionLabelNewStyle =

        "QSint--ActionLabel[class='action'] {"
            "background-color: transparent;"
            "border: 1px solid transparent;"
            "color: #0033ff;"
            "text-align: left;"
            "font: 11px;"
        "}"

        "QSint--ActionLabel[class='action']:hover {"
            "color: #0099ff;"
            "text-decoration: underline;"
        "}"

        "QSint--ActionLabel[class='action']:focus {"
            "border: 1px dotted black;"
        "}"

        "QSint--ActionLabel[class='action']:on {"
            "background-color: #ddeeff;"
            "color: #006600;"
        "}"
    ;

    // apply the style
    label1->setStyleSheet(ActionLabelNewStyle);

    \endcode
*/
class ActionLabel : public QToolButton
{
    Q_OBJECT

public:
    /** Constructor.
      */
    explicit ActionLabel(QWidget *parent = 0);

    /** Constructor. Creates ActionLabel from the \a action.
      \since 0.2
      */
    explicit ActionLabel(QAction *action, QWidget *parent = 0);

    virtual ~ActionLabel() {}

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

protected:
    void init();
};


} // namespace

#endif // ACTIONLABEL_H
