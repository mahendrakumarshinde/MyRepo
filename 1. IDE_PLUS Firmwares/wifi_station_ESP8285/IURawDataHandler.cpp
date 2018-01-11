#include "IURawDataHandler.h"


/* =============================================================================
    Core
============================================================================= */

IURawDataHandler::IURawDataHandler(uint8_t partcount) :
    m_partcount(partCount)
{
    resetPayload();
}

/**
 *
 */
void IURawDataHandler::resetPayload()
{
    m_payload = "";
    m_payloadCounter = 0;
    m_payloadStartTime = 0;
    for (uint8_t i = 0; i < MAX_PART_COUNT; ++i)
    {
        m_partAdded[i] = false;
    }
}


/**
 * Reset the buffer if too much time passed since last reception
 *
 * Time since last reception is compared to m_dataReceptionTimeout.
 * @return true if a timeout happened and that the buffer needs to be reset,
 * else false.
 */
bool IUSerial::hasTimedOut()
{
    if (m_payloadStartTime == 0)
    {
        return false;
    }
    if (m_bufferIndex > 0 && (millis() -
            m_payloadStartTime > TIMEOUT))
    {
        m_bufferIndex = 0;
        return true;
    }
    return false;
}
