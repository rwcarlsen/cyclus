// EventBackend.h
#if !defined(_EVENTBACKEND_H)
#define _EVENTBACKEND_H

#include <vector>
#include <boost/intrusive_ptr.hpp>

class Event;

typedef boost::intrusive_ptr<Event> event_ptr;
typedef std::vector<event_ptr> EventList;

/*!
An abstract base class for listeners (e.g. output databases) that want
to receive data generated by the simulation.
*/
class EventBackend {
  public:

    /// Used to pass a list of new/collected events
    virtual void notify(EventList events) = 0;

    /*!
    Used to uniquely identify a backend - particularly if there are more
    than one in a simulation.
    */
    virtual std::string name() = 0;

    /*!
    Used to notify the backend that no more events will be sent (i.e.
    the simulatoin is over).
    */
    virtual void close() = 0;
};

/*!
this allows files to use events without having to explicitly include
both EventManager.h and Event.h, while avoiding a circular include
dependency.
*/
#include "Event.h"

#endif
