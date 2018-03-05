#ifndef MacAddress_h
#define MacAddress_h

#include <stdint.h>
#include "Printable.h"
#include "WString.h"

/**
 * A class to make it easier to handle and pass around MAC addresses
 *
 * Built on the Model of Arduino IPAddress class.
 */
class MacAddress  : public Printable {
private:
    // 6 bytes address => union dword needs to be uint64_t, so 8 bytes.
    // The first 2 bytes will then always be 0.
    union {
    uint8_t bytes[8];  // 6bytes / 48bits MAC address
    uint64_t dword;
    } _address;

// Access the raw byte array containing the address.  Because this returns a pointer
// to the internal structure rather than a copy of the address this function should only
// be used when you know that the usage of the returned uint8_t* will be transient and not
// stored.
uint8_t* raw_address() { return _address.bytes; };

public:
    // Constructors
    MacAddress();
    MacAddress(uint8_t octet0, uint8_t octet1, uint8_t octet2, uint8_t octet3,
               uint8_t octet4, uint8_t octet5);
    MacAddress(uint64_t address);
    MacAddress(const uint8_t *address);

    bool fromString(const char *address);
    bool fromString(const String &address) { return fromString(address.c_str()); }

    // Overloaded cast operator to allow IPAddress objects to be used where a pointer
    // to a 6-byte uint8_t array is expected
    operator uint64_t() const { return _address.dword; };
    bool operator==(const MacAddress& addr) const { return _address.dword == addr._address.dword; };
    bool operator==(const uint8_t* addr) const;

    // Overloaded index operator to allow getting and setting individual octets of the address
    uint8_t operator[](int index) const { return _address.bytes[index + 2]; };
    uint8_t& operator[](int index) { return _address.bytes[index + 2]; };

    // Overloaded copy operators to allow initialisation of IPAddress objects from other types
    MacAddress& operator=(const uint8_t *address);
    MacAddress& operator=(uint64_t address);

    virtual size_t printTo(Print& p) const;

};

#endif // MacAddress_h
