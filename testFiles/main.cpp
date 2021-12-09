#include <iostream>

void convert32to8(uint32_t num, uint8_t *result)
{
    unsigned long CurrentPosition = num;

    result[0] = (num & 0xff000000UL) >> 24;
    result[1] = (num & 0x00ff0000UL) >> 16;
    result[2] = (num & 0x0000ff00UL) >>  8;
    result[3] = (num & 0x000000ffUL)      ;
}

int main()
{   
    uint32_t num = 7654321;
    uint8_t result[4];
    convert32to8(num, result);
    for (int i = 0;i < 4;i++)
    {
        std::cout << result[i] << std::endl;
    }

    uint32_t myLongNum = (result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3];
    std::cout << myLongNum << std::endl;

    return 0;
}