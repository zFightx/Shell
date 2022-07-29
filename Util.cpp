#include "Util.hpp"

Util::Util()
{

}

Util::~Util()
{
    
}

vector<string> Util::ReadFile(string file)
{
    ifstream my_file(file.c_str(), ios_base::in);
    string my_text;
    vector<string> lines;

    while (getline(my_file, my_text))
    {
        lines.push_back(my_text);
    }

    my_file.close();
    return lines;
}

string Util::SplitString(string &text, string separator)
{
    if (text.find(separator) != string::npos)
    {
        string sub = text.substr(0, text.find(separator));
        text.replace(0, sub.size() + separator.size(), "");

        return sub;
    }

    return "";
}