#ifndef CHARBUFFERSENDINGQUEUE_H
#define CHARBUFFERSENDINGQUEUE_H

#include <Arduino.h>

#include <IUDebugger.h>


// Forward decalartion for frienship
class CharBufferSendingQueue;

/**
 * A node of a CharBufferSendingQueue.
 *
 * A CharBufferNode contains a char buffer that can be written to, and later be
 * sent (not handled by this class).
 * After being sent, the node must receive a confirmation that the sending was
 * successfull.
 * The node will expire after a while. It has 2 timers:
 * - sendingMaxDuration: the max allowed duration after the node was sent to
 * receive the successfull sending confirmation. After that timeout, the sending
 * is considered failed and can be retried.
 * - maxRetryDuration: After being written, the node is considered valid for
 * that duration. After that, it will be considered expired and reset.
 *
 * Each CharBufferNode has 1 of the following state:
 * - AVAILABLE_FOR_WRITE: ready to be written to
 * - IN_WRITING: started writing
 * - READY_TO_SEND: writing done, timer for maxRetryDuration is started when the
 * node enter this state.
 * - BEING_SENT: attempting to send, timer for sendingMaxDuration is started
 * when the node enter this state.
 */
class CharBufferNode
{
    friend class CharBufferSendingQueue;

    public:
        /***** Preset values and default settings *****/
        enum bufferStateOptions : uint8_t {AVAILABLE_FOR_WRITE,
                                           IN_WRITING,
                                           READY_TO_SEND,
                                           BEING_SENT};
        static const uint16_t bufferSize = 700;
        static const uint32_t sendingMaxDuration = 2000;  // ms
        static const uint32_t maxRetryDuration = 30000;  // ms

        /***** Public Attribute *****/
        char buffer[bufferSize];
        uint8_t idx;

        /***** Core *****/

        CharBufferNode(uint8_t bufferIdx=0) : idx(bufferIdx) { reset(); }

        virtual ~CharBufferNode() { }

        bufferStateOptions getState() { return m_bufferState; }

        bool sendingHasTimedOut() {
            return ((millis() - m_lastSent) > maxRetryDuration);
        }

        bool isExpired() {
            return ((millis() - m_completed) > maxRetryDuration);
        }

    protected:

        /***** Protected Attribute *****/
        bufferStateOptions m_bufferState;
        uint32_t m_completed;
        uint32_t m_lastSent;

        /***** Buffer state management *****/

        void reset() {
            m_bufferState = AVAILABLE_FOR_WRITE;
            for (uint16_t i = 0; i < bufferSize; i++) {
                buffer[i] = 0;
            }
            m_completed = 0;
            m_lastSent = 0;
            if (loopDebugMode) {
                debugPrint(F("Char buffer #"), false);
                debugPrint(idx, false);
                debugPrint(F(": AVAILABLE_FOR_WRITE"));
            }
        }

        void markStartedWriting() {
            if (loopDebugMode) {
                debugPrint(F("Char buffer #"), false);
                debugPrint(idx, false);
                debugPrint(F(": IN_WRITING"));
            }
            m_bufferState = IN_WRITING;
        }

        void markDoneWriting() {
            if (loopDebugMode) {
                debugPrint(F("Char buffer #"), false);
                debugPrint(idx, false);
                debugPrint(F(": READY_TO_SEND"));
            }
            m_bufferState = READY_TO_SEND;
            m_completed = millis();
        }

        void markAsBeingSent() {
            if (loopDebugMode) {
                debugPrint(F("Char buffer #"), false);
                debugPrint(idx, false);
                debugPrint(F(": BEING_SENT"));
            }
            m_bufferState = BEING_SENT;
            m_lastSent = millis();
        }
};

/**
 *
 *
 * A queue (represented as an array to avoid linked list and dynamic memory
 * allocation) of CharBufferNode.
 */
class CharBufferSendingQueue
{
    public:
        /***** Preset values and default settings *****/
        static const uint8_t maxCount = 20;
        /***** Constructors and destructor *****/
        CharBufferSendingQueue();
        virtual ~CharBufferSendingQueue() {}
        /***** Accessors *****/
        CharBufferNode* getBuffer(uint8_t idx) { return &m_nodes[idx]; }
        CharBufferNode* getNextBufferToWrite();
        CharBufferNode* getNextBufferToSend();
        /***** Core *****/
        void maintain();
        void startedWriting(uint8_t idx);
        void finishedWriting(uint8_t idx);
        void attemptingToSend(uint8_t idx);
        void confirmSuccessfullSend(uint8_t idx);

    protected:
        CharBufferNode m_nodes[maxCount];
        uint8_t m_head;
        uint8_t m_tail;
};

#endif // CHARBUFFERSENDINGQUEUE_H
