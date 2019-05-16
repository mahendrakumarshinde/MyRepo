#ifndef ASSOCIATIVEARRAY_H
#define ASSOCIATIVEARRAY_H

/**
 * An implementation of an ordered associative array / ordered hashmap.
 *
 * Insertion, deletion and access are all in O(n).
 * When creating an instance, the user can specify a comparator function that
 * evaluates the equality of 2 keys, and must define a nullValue that will be
 * returned if the requested key is not found.
 */
template <typename KeyClass, typename ValueClass, uint8_t maxLength>
class AssociativeArray
{
    public:

        /***** Key comparison functions *****/

        typedef bool (*comparatorSignature)(KeyClass, KeyClass);
        static bool defaultComparator(KeyClass k0, KeyClass k1)
            { return k0 == k1; }

        /***** Core *****/

        AssociativeArray(ValueClass nullValue, comparatorSignature comparator) :
            m_length(0), m_nullValue(nullValue), m_comparator(comparator) {}

        AssociativeArray(ValueClass nullValue) :
            AssociativeArray(nullValue, defaultComparator) {}

        AssociativeArray() :
            AssociativeArray(NULL, NULL) {}


        /***** Associative functions *****/

        /**
         * Check if the key is already in the associative array.
         */
        bool hasKey(KeyClass key) {
            if (m_comparator != NULL) {
                for (uint8_t i = 0; i < m_length; i++) {
                    if (m_comparator(m_keys[i] , key)) {
                        return true;
                    }
                }
            }
            return false;
        }

        /**
         * Returns the index of key if the key is present, else -1.
         */
        int getIndex(KeyClass key) {
            if (m_comparator != NULL) {
                for (uint8_t i = 0; i < m_length; i++) {
                    if (m_comparator(m_keys[i] , key)) {
                        return i;
                    }
                }
            }
            return -1;
        }

        /**
         * Add a (key, value) pair to the associative array.
         *
         * @return true if the (key, value) pair was succesfully added, else
         *  false (eg: if the AssociativeArray instance is full).
         */
        bool setItem(KeyClass key, ValueClass value) {
            int idx = getIndex(key);
            if (idx < 0) {
                if (m_length >= maxLength) {
                    return false;  // associative array is full
                }
                m_keys[m_length] = key;
                m_values[m_length] = value;
                m_length++;
                return true;
            } else {
                m_keys[uint8_t(idx)] = key;
                m_values[uint8_t(idx)] = value;
                return true;
            }
        }

        void deleteItem(KeyClass key) {
            int idx = getIndex(key);
            if (idx >= 0) {
                for (uint8_t i = uint8_t(idx); i < m_length; i++) {
                    m_keys[i] = m_keys[i + 1];
                    m_values[i] = m_values[i + 1];
                }
                m_length--;
            }
        }

        ValueClass getItem(KeyClass key) {
            int idx = getIndex(key);
            if (idx < 0) {
                return m_nullValue;
            } else {
                return m_values[uint8_t(idx)];
            }
        }

        uint8_t length() { return m_length; }


        /***** Ordered functions *****/

        KeyClass getKey(uint8_t idx) { return m_keys[idx]; }

        ValueClass getValue(uint8_t idx) { return m_values[idx]; }

    protected:
        KeyClass m_keys[maxLength];
        ValueClass m_values[maxLength];
        uint8_t m_length;
        ValueClass m_nullValue;
        comparatorSignature m_comparator;
};

bool stringComparator(const char* s0, const char* s1)
{
    return strcmp(s0, s1) == 0;
}



#endif // ASSOCIATIVEARRAY_H
