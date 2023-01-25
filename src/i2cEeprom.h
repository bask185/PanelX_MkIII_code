#include <Arduino.h>
#include "macros.h" 
#include <Wire.h>

class I2cEeprom
{
public:
    //I2cEeprom() ;
    I2cEeprom( uint8 ) ;

    void begin( uint8 ) ;

    void write( uint16_t eeAddress, uint8 data ) ;
    uint8 read( uint16_t eeAddress ) ;

    template<typename T>  
    uint8 put(uint16_t addr, T const & data)
    {
        return put_n(addr, &data, sizeof(data));
    }

    template<typename T> 
    uint8 get(uint16_t addr, T & data) // Note that I removed `const` here, const is immutable, since we need to write bytes to it, don't make it `const`
    {
        return get_n(addr, &data, sizeof(data));
    }


private: 
    uint8 put_n(uint16_t addr, void * src, int size);
    uint8 get_n(uint16_t addr, void * src, int size);

    uint8 i2cAddress ;
};
