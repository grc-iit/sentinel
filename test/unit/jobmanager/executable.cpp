#include <fstream>
#include <string>
#include <iostream>

int main()
{
    std::string input;
    std::cin >> input;
    std::ofstream out("output.txt");
    out << input;
    out.close();
    return 0;
}