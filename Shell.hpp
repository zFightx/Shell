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
        vector<int> manager_pids;
        string USER;

    public:
        Shell();
        void MainLoop();
        void ExecuteLote(string file);
        void ExecuteCommand(string command, vector<string> args);
        void ExecutePipe(string command, vector<string> args);
        string GetOriginalCommand(string alias);
        void ShowHistory();
        void ShowVersion();
        void PrepareCommand(string line_command);
        void ManagerPids();

        ~Shell();
};

#endif
