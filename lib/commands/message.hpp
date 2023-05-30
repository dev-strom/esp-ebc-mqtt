#ifndef _MESSAGE_HPP_
#define _MESSAGE_HPP_

#include <Arduino.h>
#include <vector>

class Message
{
    public:
        Message(size_t length);

        bool Read(Stream& stream);
        bool Send(Stream& stream) const;

        String ToHexString() const;

    private:

        int failCounter;
        bool crcCheckDisabled;

    protected:
        std::vector<uint8_t> buffer;

        uint8_t startTag;
        uint8_t crc;
        uint8_t endTag;

        virtual void   FillFromBytes() {};
        virtual void   FillBytes() {};
        void           CalcCRC();
        bool           CheckCRC();
};

#endif // _MESSAGE_HPP_