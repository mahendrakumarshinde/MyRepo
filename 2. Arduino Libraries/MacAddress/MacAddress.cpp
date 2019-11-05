#include <Arduino.h>
#include "MacAddress.h"

MacAddress::MacAddress()
{
    _address.dword = 0;
}

MacAddress::MacAddress(uint8_t octet0, uint8_t octet1, uint8_t octet2,
                       uint8_t octet3, uint8_t octet4, uint8_t octet5)
{
    _address.bytes[0] = 0;
    _address.bytes[1] = 0;
    _address.bytes[2] = octet0;
    _address.bytes[3] = octet1;
    _address.bytes[4] = octet2;
    _address.bytes[5] = octet3;
    _address.bytes[6] = octet4;
    _address.bytes[7] = octet5;
}

MacAddress::MacAddress(uint64_t address)
{
    _address.dword = address;
}

MacAddress::MacAddress(const uint8_t *address)
{
    _address.bytes[0] = 0;
    _address.bytes[1] = 0;
    for (int i = 0; i < 6; i++)
    {
        _address.bytes[i + 2] = address[i];
    }
}

bool MacAddress::fromString(const char *address)
{

    uint16_t acc = 0; // Accumulator
    uint8_t semicolons = 0;
    while (*address)
    {
        char c = *address++;
        if (c >= '0' && c <= '9')
        {
            acc = acc * 16 + (c - '0');
            if (acc > 255) {
                // Value out of [0..255] range
                return false;
            }
        }
        else if (c >= 'A' && c <= 'F')
        {
            acc = acc * 16 + 10 + (c - 'A');
            if (acc > 255) {
                // Value out of [0..255] range
                return false;
            }
        }
        else if (c == ':')
        {
            if (semicolons == 5) {
                // Too much semicolons (there must be 5 semicolons)
                return false;
            }
            _address.bytes[semicolons + 2] = acc;
            semicolons++;
            acc = 0;
        }
        else
        {
            // Invalid char
            return false;
        }
    }

    if (semicolons != 5) {
        // Too few dots (there must be 5 dots)
        return false;
    }
    _address.bytes[7] = acc;
    return true;
}

MacAddress& MacAddress::operator=(const uint8_t *address)
{
    memcpy(_address.bytes, address, sizeof(_address.bytes));
    return *this;
}

MacAddress& MacAddress::operator=(uint64_t address)
{
    _address.dword = address;
    return *this;
}

bool MacAddress::operator==(const uint8_t* addr) const
{
    return memcmp(addr, _address.bytes, sizeof(_address.bytes)) == 0;
}

size_t MacAddress::printTo(Print& p) const
{
    size_t n = 0;
    for (int i = 2; i < 7; i++)
    {
        if (_address.bytes[i] < 16)
        {
            n += p.print('0');
        }
        n += p.print(_address.bytes[i], HEX);
        n += p.print(':');
    }
    if (_address.bytes[7] < 16)
    {
        n += p.print('0');
    }
    n += p.print(_address.bytes[7], HEX);
    return n;
}

String MacAddress::toString()
{
    char temp[18] = "";
    sprintf(temp,"%02x:%02x:%02x:%02x:%02x:%02x",
            _address.bytes[2], _address.bytes[3], _address.bytes[4],
            _address.bytes[5], _address.bytes[6], _address.bytes[7]);
    String tmpStr = String(temp);
    tmpStr.toUpperCase();
    return tmpStr;
}
