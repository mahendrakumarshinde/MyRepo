#include "CharBufferSendingQueue.h"


/* =============================================================================
    Constructors and destructor
============================================================================= */

CharBufferSendingQueue::CharBufferSendingQueue() :
    m_head(0),
    m_tail(0)

{
    for (uint8_t i = 0; i < maxCount; i++) {
        m_nodes[i] = CharBufferNode(i);
    }
}


/* =============================================================================
    Accessors
============================================================================= */

/**
 * Returns the  next buffer to write: either the next available, or oldest one
 * that has not been sent (it will be overwritten).
 */
CharBufferNode* CharBufferSendingQueue::getNextBufferToWrite()
{
    // Return next available buffer
    CharBufferNode *node_to_return = &m_nodes[m_head];
    m_head = (m_head + 1) % maxCount;
    if (node_to_return->getState() != CharBufferNode::AVAILABLE_FOR_WRITE) {
        node_to_return->reset();
    }
    // If no more available buffer, do some house keeping
    if (m_head == m_tail) {
        m_nodes[m_tail].reset();
        m_tail = (m_tail + 1) % maxCount;
    }
    return node_to_return;
}

/**
 * Returns the oldest buffer that is ready to be sent, or NULL if no buffer is
 * ready.
 */
CharBufferNode* CharBufferSendingQueue::getNextBufferToSend()
{
    if (m_tail < m_head) {
        for (uint8_t i = m_tail; i < m_head; i++) {
            if (m_nodes[i].getState() == CharBufferNode::READY_TO_SEND) {
                return &m_nodes[i];
            }
        }
    } else if (m_tail > m_head) {
        for (uint8_t i = m_tail; i < maxCount; i++) {
            if (m_nodes[i].getState() == CharBufferNode::READY_TO_SEND) {
                return &m_nodes[i];
            }
        }
        for (uint8_t i = 0; i < m_head; i++) {
            if (m_nodes[i].getState() == CharBufferNode::READY_TO_SEND) {
                return &m_nodes[i];
            }
        }
    }
    return NULL;
}


/* =============================================================================
    Core
============================================================================= */

/**
 *
 */
void CharBufferSendingQueue::maintain()
{
    // Remove expired nodes and nodes that have been sent
    while (m_tail != m_head) {
        if (m_nodes[m_tail].getState() >= CharBufferNode::READY_TO_SEND &&
            m_nodes[m_tail].isExpired())
        {
            if (loopDebugMode) {
                debugPrint(F("Streaming buffer #"), false);
                debugPrint(m_tail, false);
                debugPrint(F("is expired"));
            }
            m_nodes[m_tail].reset();
        }
        if (m_tail != m_head &&
            m_nodes[m_tail].getState() == CharBufferNode::AVAILABLE_FOR_WRITE)
        {
            m_tail = (m_tail + 1) % maxCount;
        } else {
            break;
        }
    }
    // Mark nodes that are taking too long to receive the "sent" confirmation
    // as ready to be sent again.
    if (m_tail < m_head) {
        for (uint8_t i = m_tail; i < m_head; i++) {
            if (m_nodes[i].getState() == CharBufferNode::BEING_SENT &&
                m_nodes[i].sendingHasTimedOut())
            {
                m_nodes[i].markDoneWriting();
            }
        }
    } else if (m_tail > m_head) {
        for (uint8_t i = m_tail; i < maxCount; i++) {
            if (m_nodes[i].getState() == CharBufferNode::BEING_SENT &&
                m_nodes[i].sendingHasTimedOut())
            {
                m_nodes[i].markDoneWriting();
            }
        }
        for (uint8_t i = 0; i < m_head; i++) {
            if (m_nodes[i].getState() == CharBufferNode::BEING_SENT &&
                m_nodes[i].sendingHasTimedOut())
            {
                m_nodes[i].markDoneWriting();
            }
        }
    }
}

/**
 *
 */
void CharBufferSendingQueue::startedWriting(uint8_t idx)
{
    m_nodes[idx].markStartedWriting();
}

/**
 *
 */
void CharBufferSendingQueue::finishedWriting(uint8_t idx)
{
    m_nodes[idx].markDoneWriting();
}

/**
 *
 */
void CharBufferSendingQueue::attemptingToSend(uint8_t idx)
{
    m_nodes[idx].markAsBeingSent();
}

/**
 *
 */
void CharBufferSendingQueue::confirmSuccessfullSend(uint8_t idx)
{
    if (m_nodes[idx].getState() == CharBufferNode::BEING_SENT) {
        m_nodes[idx].reset();
    }
}
