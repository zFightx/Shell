#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Shell.hpp"
#include "Util.hpp"

Shell::Shell(string file_profiles, string file_aliases)
{
    vector<string> lines_profile = Util::ReadFile(file_profiles);
    vector<string> lines_alias = Util::ReadFile(file_aliases);

    this->USER = getenv("USER");

    // Leitura do Profile
    for (unsigned int i = 0; i < lines_profile.size(); i++)
    {
        if (lines_profile[i].find("PATH=") != string::npos)
        {
            string string_paths = lines_profile[i].substr(5, lines_profile[i].size() - 5);

            while (string_paths.find(";") != string::npos)
            {
                string path = Util::SplitString(string_paths, ";");
                if (path != "")
                    this->paths_profile.push_back(path);
            }

            // for (unsigned i = 0; i < paths_profile.size(); i++)
            // {
            //     cout << paths_profile[i] << endl;
            // }
        }
    }

    // Leitura dos Aliases
    for (unsigned int i = 0; i < lines_alias.size(); i++)
    {
        if (lines_alias[i].find("alias ") != string::npos)
        {
            string string_alias = lines_alias[i].substr(6, lines_alias[i].size() - 6);
            string original = Util::SplitString(string_alias, " ");

            string_alias.erase(string_alias.begin());
            string_alias.erase(string_alias.end() - 1);
            original.erase(original.begin());
            original.erase(original.end() - 1);

            this->aliases[string_alias] = original;
        }
    }

    // cout << "INICIANDO ALIASES" << endl;
    // for (pair<string,string> i : aliases)
    // {
    //     cout << i.first << " : " << i.second << endl;
    // }
}
Shell::~Shell()
{
}

void Shell::MainLoop()
{
    // Main Loop
    string line_command = "";

    while (line_command != "exit")
    {
        cout << "BRsh-" << USER << "-" << get_current_dir_name() << ">";
        getline(cin, line_command);

        if (line_command != "" && line_command != "exit")
            PrepareCommand(line_command);
    }
}

void Shell::ExecuteCommand(string command, vector<string> args)
{
    // sleep(5);
    for (unsigned i = 0; i < this->paths_profile.size(); i++)
    {
        int start = this->paths_profile[i].find("/" + command);
        if (start != string::npos)
        {
            if (this->paths_profile[i].size() - start == command.size() + 1)
            {
                char *args_formated[args.size() + 2];
                args_formated[0] = const_cast<char *>(this->paths_profile[i].c_str());

                for (unsigned j = 0; j < args.size(); j++)
                {
                    args_formated[j + 1] = const_cast<char *>(args[j].c_str());
                }

                args_formated[args.size() + 1] = NULL;

                execv(this->paths_profile[i].c_str(), args_formated);
                cout << "Nao consegui executar o comando [" << command << "]" << endl;
                cout << "Path=" << this->paths_profile[i] << endl;
            }
        }
    }

    cout << "Nao achei o comando" << endl;
}

string Shell::GetOriginalCommand(string alias)
{
    if (this->aliases.count(alias) > 0)
        return this->aliases[alias];

    return alias;
}

void Shell::ShowHistory()
{
    for (unsigned i = 0; i < this->history.size(); i++)
    {
        cout << i + 1 << " " << this->history[i] << endl;
    }
}

void Shell::PrepareCommand(string line_command)
{
    string command = "";
    string bkp_line_command = line_command;
    vector<string> args;

    // Separa comando de argumentos e busca na lista de alias
    if (line_command.find(" ") != string::npos)
    {
        command = GetOriginalCommand(Util::SplitString(line_command, " "));
    }
    else
    {
        command = GetOriginalCommand(line_command);
        line_command = "";
    }

    while (line_command.find(" ") != string::npos)
    {
        string str_arg = Util::SplitString(line_command, " ");
        if (str_arg != " ")
            args.push_back(str_arg);
    }

    if (!line_command.empty() && line_command != " ")
        args.push_back(line_command);

    // Se comando não vazio
    if (command != "" && command != "exit")
    {
        // Comando de histórico
        if (command == "historico")
        {
            if (args.size() > 0)
            {
                int num = stoi(args[0]);
                if (num <= this->history.size() && num >= 1)
                {
                    PrepareCommand(this->history[num - 1]);
                }
                else
                {
                    cout << "Comando fora do intervalo do historico" << endl;
                }
            }
            else
                ShowHistory();
        }
        // Comandos que não são do próprio shell
        else
        {
            // Insere no histórico
            this->history.insert(this->history.begin(), bkp_line_command);

            if (this->history.size() > 10)
                this->history.pop_back();

            if (bkp_line_command.find(" | ") != string::npos)
            {
                vector<vector<string>> all_args;
                vector<string> all_commands;

                int amount = 0;

                all_commands.push_back(command);
                all_args.push_back(vector<string>());

                for (unsigned i = 0; i < args.size(); i++)
                {
                    if (args[i] == "|")
                    {
                        amount++;
                        all_commands.push_back(args[i + 1]);
                        all_args.push_back(vector<string>());
                        i++;
                    }
                    else
                    {
                        all_args[amount].push_back(args[i]);
                    }
                }

                cout << "comandos: " << all_commands.size() << endl;
                cout << "args: " << all_args.size() << endl;

                int fd[all_commands.size() - 1][2];
                vector<int> pids = vector<int>(all_commands.size());

                for (unsigned i = 0; i < all_commands.size() - 1; i++)
                {
                    pipe(fd[i]);
                }

                if ((pids[0] = fork() == 0))
                {
                    for (unsigned i = 1; i < all_commands.size() - 1; i++)
                    {
                        close(fd[i][0]);
                        close(fd[i][1]);
                    }

                    close(fd[0][0]);
                    dup2(fd[0][1], 1);

                    close(fd[0][1]);

                    ExecuteCommand(all_commands[0], all_args[0]);
                    exit(0);
                }
                for (unsigned i = 1; i < all_commands.size(); i++)
                {
                    if ((pids[i] = fork() == 0))
                    {
                        for (unsigned j = 0; j < all_commands.size() - 1; j++)
                        {
                            if (i != j && j != i - 1)
                            {
                                close(fd[j][0]);
                                close(fd[j][1]);
                            }
                        }

                        close(fd[i - 1][1]);
                        dup2(fd[i - 1][0], 0);
                        close(fd[i - 1][0]);

                        if (i != all_commands.size() - 1)
                        {
                            close(fd[i][0]);
                            dup2(fd[i][1], 1);
                            close(fd[i][1]);
                        }

                        ExecuteCommand(all_commands[i], all_args[i]);
                        exit(0);
                    }
                }

                for (unsigned i = 0; i < all_commands.size()-1; i++)
                {
                    close(fd[i][0]);
                    close(fd[i][1]);
                }

                for (unsigned i = 0; i < all_commands.size(); i++)
                {
                    waitpid(pids[i], NULL, 0);
                }
            }
            else
            {
                // Cria um novo processo
                int pid = fork();
                if (pid == 0)
                {
                    ExecuteCommand(command, args);
                    exit(0); // Encerra processo, caso não encontre o comando
                }
                else
                {
                    cout << "AGUARDANDO " << pid << "..." << endl;
                    // WNOHANG
                    waitpid(pid, NULL, 0);
                    cout << "Recuperou..." << endl;
                }
            }
        }
    }
}