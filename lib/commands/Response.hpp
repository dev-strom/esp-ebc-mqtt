#ifndef _RESPONSE_HPP_
#define _RESPONSE_HPP_

#include "Message.hpp"
#include <vector>



#define Mode_t uint8_t


class Response : public Message
{
    public:

        static const Mode_t InvalidMode = 0xff;
        static const size_t DATA_VALUES_LEN = 7;

        Response();

    protected:
        
        friend class EbcController;

        uint8_t GetId() const;
        std::vector<uint16_t> GetData() const;
        virtual void FillFromBytes();

    private:

        Mode_t mode; // == uint8_t
        std::vector<uint16_t> values;
        uint8_t id; // Controller id : 09 = EBC-A20
};

#endif // _RESPONSE_HPP_