/*!
 * \file    eventthread.h
 * \date    10.07.2009
 * \author  IDS Imaging Development Systems GmbH
 *
 * \brief   eventthread class (inherits QThread) declaration
 *          to wait on a uEye event and signal a connected slot
 *
 * \par Last modified
 *      on $Date:$ by $Author:$
 *
 * \li  05.02.2014 - Added define EVENTTHREAD_WAIT_TIMEOUT
 */

#ifndef EVENTTHREAD_H_
#define EVENTTHREAD_H_

#include <QObject>
#include <QThread>
#include <deque>
#include <ueye.h>
#include <mutex>
#include <QMap>

#ifndef FRAME_SIGNAL_LIMIT_RATE
#define FRAME_SIGNAL_LIMIT_RATE 2000
#endif

/*!
 * \defgroup EVENTTHREAD uEye events implementation
 * \{
 */

/*!
 * \brief Event thread implementation for camera list change handling
 *
 * eventthread class inherits QThread to use signal/slot mechanism to signal an
 * initialized uEye event to a connected slot. For each uEye event to wait for, should
 * be a seperate member of this eventthread class.
 * \note To use signal/slot mechanism no multiple inheritance from an Qt Object class
 * is allowed.
 */



#define EVENTTHREAD_WAIT_TIMEOUT    1000
class EventThread : public QThread
{
Q_OBJECT
    /* singleton class is maybe better? */
    class UserEvents
    {
    public:
        UserEvents()
        {
            std::generate_n(std::inserter(event_ids, event_ids.begin()), IS_SET_EVENT_USER_DEFINED_END - IS_SET_EVENT_USER_DEFINED_BEGIN, []() {
                static int i{IS_SET_EVENT_USER_DEFINED_BEGIN}; return i++;
            });
        }

        ~UserEvents() = default;

        UserEvents(const UserEvents&) = delete;
        UserEvents(UserEvents&&) = delete;
        UserEvents& operator=(const UserEvents&) = delete;
        UserEvents& operator=(UserEvents&&) = delete;

        int acquire_id()
        {
            if (event_ids.empty())
            {
                throw std::runtime_error("Too many events :D");
            }

            std::lock_guard<std::mutex> lock(mutex);
            auto id = event_ids.back();
            event_ids.pop_back();
            return id;
        }

        void release_id(unsigned int id)
        {
            std::lock_guard<std::mutex> lock(mutex);
            event_ids.push_back(id);
        }

    private:
        std::deque<unsigned int> event_ids;
        std::mutex mutex;
    };

public:
    /*!
     * \brief class standard constructor
     */
    explicit EventThread (QObject* parent = nullptr);
    /*!
     * \brief class standard destructor
     */
    ~EventThread () override;

    /*!
     * \brief starts waiting on a uEye event
     * \param hCam uEye camera handle
     * \param event waiting for
     */
    int start (HIDS hCam, unsigned int event);

    /*!
     * \brief starts waiting on a uEye event
     * \param hCam uEye camera handle
     * \param events waiting for (list)
     */
    int start(HIDS hCam, std::vector<unsigned int> events);

    /*!
     * \brief stops waiting on a uEye event
     */
    void stop ();

    uint32_t getCntFromEvent(uint32_t event);
    void resetCntFromEvent(uint32_t event);

signals:
    /*!
     * \brief Qt signal function to be emited, when the uEye event happens
     * \param event that happened
     */
    void eventhappen(unsigned int event);
    void frameEvent();

protected:
    /*!
     * \brief EventThread run function. Waits for the uEye event.
     */
    void run () override;

private:
    static UserEvents userEvents;
    /* camera handle for waiting event */
    HIDS m_hCam;
    /* event waiting for */
    std::vector<unsigned int> m_event;
    /* event run switch */
    volatile bool m_bRunEventThread;

    int terminateEvent;
    QMap<uint32_t, uint32_t> eventCnt;
    quint64 limitRate = FRAME_SIGNAL_LIMIT_RATE;

};

/*!
 * \}
 */ // end of doc group EVENTTHREAD

#endif /* EVENTTHREAD_H_ */
