#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include "Shell.hpp"
#include "Util.hpp"

Shell::Shell()
{
    this->USER = getenv("USER");

    vector<string> lines_profile = Util::ReadFile("/home/" + this->USER + "/.BRbshrc_profile");
    vector<string> lines_alias = Util::ReadFile("/home/" + this->USER + "/.BRshrc");

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
}
Shell::~Shell()
{
}

void Shell::MainLoop()
{
    // Main Loop
    string line_command = "";

    // Enquanto não digitar exit
    while (line_command != "exit")
    {
        cout << "BRsh-" << USER << "-" << get_current_dir_name() << ">";
        getline(cin, line_command);

        // Preparar e validar comando para execução
        if (line_command != "" && line_command != "exit")
            PrepareCommand(line_command);

        // Gerenciador de Backgrounds
        this->ManagerPids();
    }
}

void Shell::ManagerPids()
{
    // Se filho morreu, pai é avisado a cada rodada de Main Loop
    for (unsigned i = 0; i < this->manager_pids.size(); i++)
    {
        // Verifica se filho morreu, sem bloquear o pai (WHOHANG)
        int ret = waitpid(this->manager_pids[i], NULL, WNOHANG);

        // Se filho morreu, remove do gerenciador
        if (ret == -1)
        {
            this->manager_pids.erase(this->manager_pids.begin() + i);
            i = 0;
        }
    }
}

void Shell::ExecuteLote(string file_lote)
{
    // Lê o arquivo em lote
    vector<string> commands = Util::ReadFile(file_lote);

    // Para cada linha que não for comentário, tenta executar o comando.
    for (unsigned i = 0; i < commands.size(); i++)
    {
        if (commands[i][0] != '#')
        {
            this->PrepareCommand(commands[i]);
        }

        // Como não há Main Loop, é necessário verificar backgrounds
        this->ManagerPids();
    }

    // Somente encerra, se todos os filhos morrerem
    wait(NULL);
}

void Shell::ExecuteCommand(string command, vector<string> args)
{
    // Redirecionador
    for (unsigned i = 0; i < args.size(); i++)
    {
        // Redireciona a saída do comando para o arquivo especificado no argumento
        if (args[i] == ">")
        {
            int file = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC);
            dup2(file, 1);
            close(file);

            args.pop_back();
            args.pop_back();
        }
        // Flag append, concatena no final do arquivo
        else if (args[i] == ">>")
        {
            int file = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND);
            dup2(file, 1);
            close(file);

            args.pop_back();
            args.pop_back();
        }
        // Redireciona a leitura de dados do comando para o arquivo especificado
        else if (args[i] == "<")
        {
            int file = open(args[i + 1].c_str(), O_RDONLY);

            if (file != -1)
            {
                dup2(file, 0);
                close(file);

                args.pop_back();
                args.pop_back();
            }
            else
            {
                cout << "Arquivo não encontrado." << endl;
                return;
            }
        }
    }

    // Formata os argumentos do comando
    char *args_formated[args.size() + 2];
    args_formated[0] = const_cast<char*>("");
    for (unsigned j = 0; j < args.size(); j++)
    {
        args_formated[j + 1] = const_cast<char *>(args[j].c_str());
    }

    args_formated[args.size() + 1] = NULL;

    // Para cada path especificado no profile, tenta executar o comando
    for (unsigned i = 0; i < this->paths_profile.size(); i++)
    {
        string path = this->paths_profile[i] + command;
        args_formated[0] = const_cast<char *>(path.c_str());

        execv(path.c_str(), args_formated);
    }

    // Caso não tenha conseguido executar o comando, tenta executar um arquivo
    args_formated[0] = const_cast<char *>(command.c_str());
    execv(command.c_str(), args_formated);
    
    // Caso não tenha conseguido executar um comando ou arquivo, avisa o usuário
    cout << "Nao achei o comando: " << command << endl;
}

string Shell::GetOriginalCommand(string alias)
{
    // Retorna o comando original a partir do apelido
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

void Shell::ShowVersion()
{
    cout << "=========================================================" << endl;
    cout << "| \tShell BRsh \t\t\t\t\t|" << endl;
    cout << "| \tAutor: Alexandre Souza Costa Oliveira \t\t|" << endl;
    cout << "| \tVersão: 1.8.2 \t\t\t\t\t|" << endl;
    cout << "| \tÚltima Atualização: 31/07/2022 \t\t\t|" << endl;
    cout << "=========================================================" << endl;
}

void Shell::ExecutePipe(string command, vector<string> args)
{
    // vetor de comandos e vetor de argumento para cada comando
    vector<vector<string>> all_args;
    vector<string> all_commands;

    int amount = 0;

    all_commands.push_back(command);
    all_args.push_back(vector<string>());

    // Montagem do vetor de comandos e argumentos
    for (unsigned i = 0; i < args.size(); i++)
    {
        if (args[i] == "|")
        {
            amount++;
            all_commands.push_back(this->GetOriginalCommand(args[i + 1]));
            all_args.push_back(vector<string>());
            i++;
        }
        else
        {
            all_args[amount].push_back(args[i]);
        }
    }

    // Fd's de cada pipe que será usado
    int fd[all_commands.size() - 1][2];
    vector<int> pids = vector<int>(all_commands.size());

    for (unsigned i = 0; i < all_commands.size() - 1; i++)
    {
        pipe(fd[i]);
    }

    // Comando inicial do pipe
    if ((pids[0] = fork() == 0))
    {
        // Fecha todos os fds que não será utilizado para esse processo
        for (unsigned i = 1; i < all_commands.size() - 1; i++)
        {
            close(fd[i][0]);
            close(fd[i][1]);
        }

        // Fecha leitura, pois o primeiro não irá ler.
        close(fd[0][0]);
        // Redireciona a escrita, para o fd[1]
        dup2(fd[0][1], 1);
        // Fecha o FD de escrita para esse processo.
        close(fd[0][1]);
        // Executa o comando.
        ExecuteCommand(all_commands[0], all_args[0]);
        // Encerra o processo caso falhe na execução do comando.
        exit(0);
    }

    // Para cada comando secundário
    for (unsigned i = 1; i < all_commands.size(); i++)
    {
        if ((pids[i] = fork() == 0))
        {
            // Fecha os fds que não serão usados
            for (unsigned j = 0; j < all_commands.size() - 1; j++)
            {
                if (i != j && j != i - 1)
                {
                    close(fd[j][0]);
                    close(fd[j][1]);
                }
            }

            // fecha a conexão de escrita com o comando anterior
            close(fd[i - 1][1]);
            // redireciona a leitura de dados do comando, com a saída do comando anterior
            dup2(fd[i - 1][0], 0);
            // fecha o fd de leitura com o comando anterior
            close(fd[i - 1][0]);

            // Se não for último comando, redireciona a saída pro próximo comando ler
            if (i != all_commands.size() - 1)
            {
                close(fd[i][0]);
                dup2(fd[i][1], 1);
                close(fd[i][1]);
            }

            // Executa o comando
            ExecuteCommand(all_commands[i], all_args[i]);
            exit(0);
        }
    }

    // Fecha todos os fds para o pai
    for (unsigned i = 0; i < all_commands.size() - 1; i++)
    {
        close(fd[i][0]);
        close(fd[i][1]);
    }

    // Aguarda todos os comandos acabarem
    for (unsigned i = 0; i < all_commands.size(); i++)
    {
        waitpid(pids[i], NULL, 0);
    }
}

void Shell::PrepareCommand(string line_command)
{
    string command = "";
    string bkp_line_command = line_command;
    vector<string> args;
    bool is_background = false;

    if (line_command.find("&") == line_command.size() - 1)
    {
        line_command.resize(line_command.size() - 1);
        is_background = true;
    }

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
                    cout << "BRsh-" << USER << "-" << get_current_dir_name() << ">" << this->history[num - 1] << endl;
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
        else if (command == "ver")
        {
            ShowVersion();
        }
        else if (command == "cd")
        {
            if (args.size() >= 1)
                chdir(args[0].c_str());
        }
        // Comandos que não são do próprio shell
        else
        {
            // Insere no histórico
            this->history.insert(this->history.begin(), bkp_line_command);

            if (this->history.size() > 10)
                this->history.pop_back();

            // Tratamento de PIPEs
            if (bkp_line_command.find(" | ") != string::npos)
            {
                if (is_background)
                {
                    int background_pid = fork();

                    if (background_pid == 0)
                    {
                        this->ExecutePipe(command, args);
                        exit(0);
                    }
                    manager_pids.push_back(background_pid);
                }
                else
                {
                    this->ExecutePipe(command, args);
                }
            }

            // Tratamento de comandos únicos
            else
            {
                if (is_background)
                {
                    int background_pid = fork();

                    if (background_pid == 0)
                    {
                        // Cria um novo processo
                        int pid = fork();
                        if (pid == 0)
                        {
                            this->ExecuteCommand(command, args);
                            exit(0); // Encerra processo, caso não encontre o comando
                        }
                        else
                        {
                            waitpid(pid, NULL, 0);
                            cout << "Processo em background [" << pid << "][executado] " << bkp_line_command << endl;
                            exit(0);
                        }
                    }

                    manager_pids.push_back(background_pid);
                }
                else
                {
                    // Cria um novo processo
                    int pid = fork();
                    if (pid == 0)
                    {
                        this->ExecuteCommand(command, args);
                        exit(0); // Encerra processo, caso não encontre o comando
                    }
                    else
                    {
                        // cout << "AGUARDANDO " << pid << "..." << endl;
                        // WNOHANG
                        waitpid(pid, NULL, 0);
                        // cout << "Recuperou... " << endl;
                    }
                }
            }
        }
    }
}