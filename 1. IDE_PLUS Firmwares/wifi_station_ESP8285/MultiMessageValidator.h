#ifndef MULTIMESSAGEVALIDATOR_H
#define MULTIMESSAGEVALIDATOR_H

#include <Arduino.h>
#include "Logger.h"


template<uint8_t N>
class MultiMessageValidator
{
    public:
        /***** *****/
        MultiMessageValidator() : MultiMessageValidator(0) { }
        MultiMessageValidator(uint32_t timeout) :
            m_timeout(timeout), m_startTime(0) {
            for (uint8_t i = 0; i < N; ++i)
            {
                m_messageReceived[i] = false;
            }
        }
        virtual ~MultiMessageValidator();
        /***** *****/
        void setTimeout(uint32_t timeout) { m_timeout = timeout; }
        void reset();
        bool started();
        bool completed();
        void receivedMessage(uint8_t index);
        bool hasTimedOut();

    protected:
        bool m_messageReceived[N];
        uint32_t m_timeout;
        uint32_t m_startTime;

};

template<uint8_t N>
MultiMessageValidator<N>::reset()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        m_messageReceived[i] = false;
    }
}

template<uint8_t N>
MultiMessageValidator<N>::started()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        if (m_messageReceived[i])
        {
            return true;
        }
    }
    return false;
}

template<uint8_t N>
MultiMessageValidator<N>::completed()
{
    for (uint8_t i = 0; i < N; ++i)
    {
        if (!m_messageReceived[i])
        {
            return false;
        }
    }
    return true;
}

template<uint8_t N>
MultiMessageValidator<N>::receivedMessage(uint8_t index)
{
    if (index >= N)
    {
        if (debugMode)
        {
            raiseException("Index out of bound: N=", false);
            debugPrint(N, false);
            debugPrint(", k=", false);
            debugPrint(index);
        }
        return;
    }
    if (!started())
    {
        m_startTime = millis();
    }
    m_messageReceived[index] = true;
}

template<uint8_t N>
MultiMessageValidator<N>::hasTimedOut()
{
    uint32_t current = millis();
    return (m_timeout > 0 && current - m_startTime > m_timeout);
}

#endif // MULTIMESSAGEVALIDATOR_H
