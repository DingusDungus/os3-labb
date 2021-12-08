#include <iostream>

int main()
{
    uint8_t num1 = 0;
    uint8_t num2 = 0;
    uint16_t total = 0;
    while(true)
    {
        if (num1 < 255)
        {
            num1++;
            for (int i = 0;i < 255;i++)
            {
                num2++;
                total++;
            }
            num2 = 0;
        }
        else
        {
            break;
        }
    }
    std::cout << "Number of total permutations: " << total << std::endl;

    return 0;
}