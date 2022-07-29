#ifndef __SHELL_HPP__
#define __SHELL_HPP__

#include <iostream>
#include <vector>
#include <map>

using namespace std;

class Shell
{
    private:
        vector<string> paths_profile;
        map<string, string> aliases;
        vector<string> history;
        string USER;

    public:
        Shell(string file_profiles, string file_aliases);
        void MainLoop();
        void ExecuteCommand(string command, vector<string> args);
        string GetOriginalCommand(string alias);
        void ShowHistory();
        void PrepareCommand(string line_command);

        ~Shell();
};

#endif
