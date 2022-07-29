#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

class Util
{
    private:
        
    public:
        Util();
        static vector<string> ReadFile(string file);
        static string SplitString(string &text, string separator);
        ~Util();
};

#endif