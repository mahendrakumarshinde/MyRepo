#ifndef TEST_ASSOCIATIVEARRAY_H_INCLUDED
#define TEST_ASSOCIATIVEARRAY_H_INCLUDED


#include <ArduinoUnit.h>

test(aa_test0)
{
    // Initialization
    int notFoundValue = -1;
    AssociativeArray<const char*, int, 5> aa(notFoundValue, stringComparator);
    assertEqual(aa.length(), 0);
    // Key insertion and look up
    const char key1[4] = "ABC";
    aa.setItem(key1, 3);
    const char key2[12] = "easy as 123";
    aa.setItem(key2, 12);
    assertEqual(aa.length(), 2);
    assertEqual(aa.getItem(key1), 3);
    assertEqual(aa.getItem(key2), 12);
    assertEqual(aa.getItem("ABC"), 3);
    // Non-existing key
    assertEqual(aa.getItem("ABCD"), notFoundValue);
    // Key replacement
    aa.setItem("ABC", 143);
    assertEqual(aa.getItem(key1), 143);
    // Key deletion
    aa.deleteItem(key1);
    assertEqual(aa.length(), 1);
    assertEqual(aa.getItem("ABC"), notFoundValue);
}

#endif // TEST_ASSOCIATIVEARRAY_H_INCLUDED
