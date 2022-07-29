#include <iostream>

#include "Shell.hpp"

using namespace std;

int main()
{
    Shell *shell = new Shell("/home/.BRbshrc_profile", "/home/.BRshrc");
    shell->MainLoop();

    return 0;
}