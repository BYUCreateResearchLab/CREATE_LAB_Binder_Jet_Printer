/*!
 * \file    eventthread.cpp
 * \date    10.07.2009
 * \author  IDS Imaging Development Systems GmbH
 *
 * \brief   eventthread class (inherits QThread) implementation
 *          to wait on a uEye event and signal a connected slot
 *
 * \par Last modified
 *      on $Date:$ by $Author:$
 *
 * \li  05.02.2014 - Changed is_WaitEvent() timeout to prevent from losing events
 * \li  12.12.2014 - Add missing Windows events
 */

#include "eventthread.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QSettings>
#include <utility>

EventThread::UserEvents EventThread::userEvents;

EventThread::EventThread (QObject *parent) :
    QThread(parent)
{
    m_hCam = 0;
    m_event.clear();
    m_bRunEventThread = false;

    QSettings settings("IDS", "uEyeDemo");

    settings.beginGroup("eventThread");
    limitRate = settings.value("limitRate", limitRate).toULongLong();
    settings.endGroup();

    terminateEvent = EventThread::userEvents.acquire_id();
}

EventThread::~EventThread ()
{
    stop();
    wait();

    QSettings settings("IDS", "uEyeDemo");

    settings.beginGroup("eventThread");
    settings.setValue("limitRate", limitRate);
    settings.endGroup();

    EventThread::userEvents.release_id(terminateEvent);
}

int EventThread::start (HIDS hCam, unsigned int event)
{
    return start(hCam, std::vector<unsigned int>{event});
}

int EventThread::start(HIDS hCam, std::vector<unsigned int> events)
{
    int ret = 0;
    m_hCam = hCam;
    m_event = std::move(events);
    m_event.push_back(terminateEvent);

    // we could also create a vector here instead of looping. But has the same effort...
    IS_INIT_EVENT evInit{};

    for(auto ev: m_event)
    {
        evInit.nEvent = ev;
        evInit.bManualReset = FALSE;
        evInit.bInitialState = FALSE;

        ret = is_Event(m_hCam, IS_EVENT_CMD_INIT, &evInit, sizeof(evInit));
        if(ret != IS_SUCCESS)
        {
            qDebug() << "EvInit failed!" << ret << ev;
            return -1;
        }
        eventCnt[ev] = 0;
    }

    ret = is_Event(m_hCam, IS_EVENT_CMD_ENABLE, m_event.data(), static_cast<UINT>(sizeof(unsigned int) * m_event.size()));
    if (ret == 0)
    {
        m_bRunEventThread = true;
        QThread::start (); // start the thread run function
    }
    else
    {
       qDebug() << "EvEnable failed!" << ret;
    }

    return ret;
}

void EventThread::stop ()
{
    if (m_bRunEventThread)
    {
        m_bRunEventThread = false;

        int setEvent = terminateEvent;
        is_Event(m_hCam, IS_EVENT_CMD_SET, &setEvent, sizeof(setEvent));
    }
}

void EventThread::run ()
{
    IS_WAIT_EVENTS evWait{};
    evWait.pEvents = m_event.data();
    evWait.nCount = static_cast<UINT>(m_event.size());
    evWait.bWaitAll = FALSE;
    evWait.nTimeoutMilliseconds = INFINITE;

    QElapsedTimer t1;
    t1.start();

    while (m_bRunEventThread)
    {
        if(is_Event(m_hCam, IS_EVENT_CMD_WAIT, &evWait, sizeof(evWait)) == IS_SUCCESS && m_bRunEventThread)
        {
            eventCnt[evWait.nSignaled]++;

            if (evWait.nSignaled == IS_SET_EVENT_FRAME)
            {
                // QT signals have problems with the Event loop being filled with signals,
                // so limit it by 2000 signals/sec
                if (t1.nsecsElapsed() >= 1000000000LL / limitRate)
                {
                    emit eventhappen(evWait.nSignaled);
                    emit frameEvent();
                    t1.restart();
                }
            }
            else
            {
                emit eventhappen(evWait.nSignaled);
            }
        }
    }
    m_bRunEventThread = FALSE;
    is_Event(m_hCam, IS_EVENT_CMD_EXIT, m_event.data(), static_cast<UINT>(m_event.size() * sizeof(int)));
}

uint32_t EventThread::getCntFromEvent(uint32_t event)
{
    return eventCnt[event];
}

void EventThread::resetCntFromEvent(uint32_t event)
{
    eventCnt[event] = 0;
}

#include "moc_eventthread.cpp"
