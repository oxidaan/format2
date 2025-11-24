#incude "format2.h"
#include <iostream>


int main(int, char **)
{
    std::cout << format2::Format("%0", "Hello!") << '\n';
    std::cout << format2::Format("%0", 12345) << '\n';
    std::cout << format2::Format("%0 %0", "Hello", "world!") << '\n';
    std::cout << format2::Format("%2 %1", "world!", "Hello") << '\n';
    std::cout << format2::Format("Hex: %0", std::hex, 12345) << '\n';
    std::cout << format2::Format("Hex: ", std::hex, 12345) << '\n';
    std::cout << format2::Format("Hello ", "world!") << '\n';
    std::cout << format2::Format(std::hex , 12345) << '\n';
}    
