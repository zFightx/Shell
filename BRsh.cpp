#include <iostream>

#include "Shell.hpp"

using namespace std;

int main(int argc, char *argv[])
{
    string file_lote = "";
    
    if(argc >= 2)
        file_lote = argv[1];

    Shell *shell = new Shell();

    if(!file_lote.empty())
        shell->ExecuteLote(file_lote);
    else
        shell->MainLoop();
    

    return 0;
}