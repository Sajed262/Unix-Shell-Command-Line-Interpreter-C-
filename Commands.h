#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <time.h>
#include <string.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define SMASH "smash> "
using std::string;

const string WHITESPACE = "\n\r\t\f\v";
string _trim(const std::string &s);

class Command {
// TODO: Add your data members
public:
    const char * cmd;
    bool is_bg;
    Command(const char *cmd,bool is_bg) : cmd(cmd) ,is_bg(is_bg){}
    virtual ~Command() {}
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    string left_command;
    string right_command;
    bool is_err;
    PipeCommand(const char* cmd_line , string l, string r, bool f);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    string out_file;
    string command;
    bool is_append;
    explicit RedirectionCommand(const char* cmd_line,string out, string cmd,bool append);
    virtual ~RedirectionCommand() {}
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    ChangeDirCommand(const char* cmd_line);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class ChPromptCommand : public BuiltInCommand {
public:
    ChPromptCommand(const char* cmd_line);
    virtual ~ChPromptCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class PwdCommand : public BuiltInCommand{
public:
    PwdCommand(const char* cmd_line);
    void execute() override;
    virtual ~PwdCommand() {}
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    JobsList* jobs;
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    public:
        int job_id;
        pid_t job_pid;
        time_t start_time;
        bool is_stopped;
        bool is_bg;
        string cmd;

    };
    // TODO: Add your data members
    std::vector<JobEntry> jobs;
    int size;
    int max_id;
public:
    JobsList();
    ~JobsList() = default;
    void addJob(const char *cmd_l ,pid_t pid,bool isStopped = false);
    void printJobsList();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob();
    JobEntry *getLastStoppedJob();
    int numOfStopped();
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList * jobs;
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList * jobs;
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class HeadCommand : public BuiltInCommand {
    string file;
    int n;
public:
    HeadCommand(const char* cmd_line,string f, int n);
    virtual ~HeadCommand() {}
    void execute() override;
};

class TimeoutCommand : public Command{
public:
    unsigned sec;
    string innerCmd = "";
    time_t finish;
    pid_t son_pid;

    TimeoutCommand(const char* cmd_line, bool bg, char** args, int num_args);
    void setAlarm();
    virtual ~TimeoutCommand() = default;
    void execute() override;

};

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
public:
    std::vector<TimeoutCommand> timed;
    string prompt;
    string cmd_l;
    string prev_dir;
    pid_t smash_pid;
    pid_t current_job_pid;
    JobsList *jobs_list;
    void removeFinishedCommand();
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_