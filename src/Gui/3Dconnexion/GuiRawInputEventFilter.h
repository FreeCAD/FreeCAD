#ifndef GUIRAWNATIVEINPUTEVENTFILTER_H
#define GUIRAWNATIVEINPUTEVENTFILTER_H

namespace Gui
{
    class RawInputEventFilter : public QAbstractNativeEventFilter
    {
    public:
        typedef bool (*EventFilter)(void *message, long *result);
        RawInputEventFilter(EventFilter filter) : eventFilter(filter) {
        }
        virtual ~RawInputEventFilter() {
        }

        virtual bool nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result) {
            return eventFilter(message, result);
        }

    private:
        EventFilter eventFilter;
    };
} //namespace Gui

#endif //GUIRAWNATIVEINPUTEVENTFILTER_H
