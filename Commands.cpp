#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include <algorithm>

using namespace std;

SmallShell &smash = SmallShell::getInstance();

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

//command



//built in command
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line,false) {}

// chprompt BIC
ChPromptCommand::ChPromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChPromptCommand::execute() {// A7a error
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd, args);
    if (args_num == 1)
        smash.prompt = SMASH;
    else
        smash.prompt = string(args[1]) + "> ";
    delete[] args;
}

//showPid BIC
ShowPidCommand::ShowPidCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << smash.smash_pid << endl;
}

//pwd BIC
PwdCommand::PwdCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void PwdCommand::execute() {
    char *path = getcwd(nullptr, 0);
    if (path)
        cout << path << endl;
    else
        perror("smash error: getcwd failed");
}

//cd command BIC
ChangeDirCommand::ChangeDirCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd, args);
    string prev = smash.prev_dir;
    if (args_num > 2) {
        delete[] args;
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    if (string(args[1]) == "-") {
        if (prev.empty()) {
            delete[] args;
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        }
        smash.prev_dir = string(getcwd(nullptr, 0));//bdeka
        if (chdir(prev.c_str()) != 0) {
            delete[] args;
            smash.prev_dir = prev;
            perror("smash error: chdir failed");
            return;
        }
    } else if (string(args[1]) == "..") {
        string current_dir = getcwd(nullptr, 0);
        string father_dir = current_dir;
        for (unsigned long i = father_dir.size() - 1; father_dir[father_dir.size() - 1] != '/'; --i) {
            father_dir = father_dir.substr(0, father_dir.size() - 1);
        }
        if (std::count(father_dir.begin(),father_dir.end(),'/') > 1)
            father_dir = father_dir.substr(0, father_dir.size() - 1); // remove the "/"
        smash.prev_dir = current_dir;
        if (chdir(father_dir.c_str()) != 0) {
            delete[] args;
            smash.prev_dir = prev;
            perror("smash error: chdir failed");
            return;
        }
    } else {
        smash.prev_dir = string(getcwd(nullptr, 0)); // bdeka n5shal
        if (chdir(args[1]) != 0) {
            delete[] args;
            smash.prev_dir = prev;
            perror("smash error: chdir failed");
            return;
        }
    }
    delete[] args;
}

// jobs command BIC
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void JobsCommand::execute() {
    jobs->printJobsList();
}

//kill command
KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void KillCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd, args);
    if (args_num != 3) {
        delete[] args;
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    string sig = args[1];
    string job = args[2];
    if (sig[0] == '-') {
        sig = sig.substr(1);
        int signum = atoi(sig.c_str());
        int job_id = atoi(job.c_str());
        if (signum < 1 || job_id == 0 ) {
            delete[] args;
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }

        JobsList::JobEntry *jb = jobs->getJobById(job_id);
        if (jb == nullptr) {
            delete[] args;
            cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
            return;
        }
        if (kill(jb->job_pid, signum) != 0) {
            delete[] args;
            perror("smash error: kill failed");
            return;
        }
        cout << "signal number " << signum << " was sent to pid " << jb->job_pid << endl;
        if (signum == SIGSTOP) {
            jb->is_stopped = true;

        }
        if (signum == SIGCONT)
            jb->is_stopped = false;
    }
    else {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    delete[] args;
}

//fg command BIC
ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void ForegroundCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd, args);
    if (args_num > 2) {
        delete[] args;
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry *job = nullptr;
    if (args_num == 1) {
        if (jobs->jobs.empty()) {
            delete[] args;
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
        job = jobs->getLastJob();
    } else if (args_num == 2) {
        int x = atoi(args[1]);
        if (x == 0) {
            delete[] args;
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
        job = jobs->getJobById(x);
        if (job == nullptr) {
            delete[] args;
            cerr << "smash error: fg: job-id " << x << " does not exist" << endl;
            return;
        }

    }
    job->is_stopped = false;
    cout << job->cmd << " : " << job->job_pid << endl;
    if (kill(job->job_pid, SIGCONT) != 0) {
        delete[] args;
        perror("smash error: kill failed");
        return;
    }
    if (job->is_bg) {
        _removeBackgroundSign((char *) cmd);
        job->is_bg = false;
    }
    int status;
    smash.current_job_pid = job->job_pid;
    if (waitpid(job->job_pid, &status, WUNTRACED) == -1) {
        delete[] args;
        perror("smash error: waitpid failed");
        return;
    }
    smash.current_job_pid = -1;
    if (WIFSTOPPED(status)) {
        job->is_stopped = true;
        job->start_time=time(nullptr);
        delete[] args;
        return;
    }
    jobs->removeJobById(job->job_id);

    delete[] args;
}

//bg command BIC
BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void BackgroundCommand::execute() {
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd, args);
    if (args_num > 2) {
        delete[] args;
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    JobsList::JobEntry *job = nullptr;
    if (args_num == 1) {
        if (jobs->numOfStopped() == 0) {
            delete[] args;
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
        job = jobs->getLastStoppedJob();
    } else if (args_num == 2) {
        int x = atoi(args[1]);
        if (x == 0) {
            delete[] args;
            cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }
        job=jobs->getJobById(x);
        if (job == nullptr) {
            delete[] args;
            cerr << "smash error: bg: job-id " << x << " does not exist" << endl;
            return;
        }
    }
    if (!job->is_stopped) {
        delete[] args;
        cerr << "smash error: bg: job-id " << job->job_id << " is already running in the background" << endl;
        return;
    }
    job->is_stopped = false;
    cout << job->cmd << " : " << job->job_pid << endl;
    if (kill(job->job_pid, SIGCONT) != 0) {
        delete[] args;
        perror("smash error: kill failed");
        return;
    }
    delete[] args;
}

//quit command BIC
QuitCommand::QuitCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {}

void QuitCommand::execute() {

    char **args = new char *[COMMAND_MAX_ARGS + 1];
    _parseCommandLine(cmd, args);

    if (args[1] && string(args[1]) == "kill") {
        cout << "smash: sending SIGKILL signal to " << jobs->size << " jobs:" << endl;
        for (int i = 0; i < jobs->size; ++i) {
            JobsList::JobEntry job = jobs->jobs[i];
            cout << job.job_pid << ": " << job.cmd << endl;
            kill(job.job_pid, SIGKILL);
        }
    }
    delete[] args;
    exit(0);

}

//head command BIC
HeadCommand::HeadCommand(const char *cmd_line,string f, int n) : BuiltInCommand(cmd_line), file(f),n(n) {}

void HeadCommand::execute() {
    int fd = open(file.c_str(),O_RDWR,0666);
    if (fd < 0) {
        perror("smash error: open failed");
        return;
    }
    int count = 0;
    char c;
    while (read(fd, &c, 1) > 0) {
        if (count == n)
            break;
        if (c == '\n')
            count++;
        write(1, &c, 1);
    }

}

//external commands
ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line, false) {}

void ExternalCommand::execute() {
    bool back_ground = false;
    string command = cmd;
    if (_isBackgroundComamnd(command.c_str())) {
        _removeBackgroundSign((char *) command.c_str());
        back_ground = true;
    }
    char *argv[] = {(char *) "/bin/bash", (char *) "-c", (char *) command.c_str(), nullptr};
    pid_t p = fork();
    if (p < 0) {
        perror("smash error: fork failed");
        return;
    } else if (p == 0) { // son's process
        if (setpgrp() < 0) {
            perror("smash error: setpgrp failed");
            return;
        }
        execv(argv[0], argv);
        perror("smash error: setpgrp failed");
        return;
    } else { // father's process
        if (back_ground)
            smash.jobs_list->addJob(cmd, p, false);
        else {
            int status;
            smash.current_job_pid = p;
            if (waitpid(p, &status, WUNTRACED) < 0) {
                perror("smash error: waitpid failed");
                return;
            }
            smash.current_job_pid = -1;
            if (WIFSTOPPED(status)) {
                smash.jobs_list->addJob(cmd, p, true);
            }
        }
    }
}

//pipe command
PipeCommand::PipeCommand(const char *cmd_line, string l, string r, bool f) : Command(cmd_line,false), left_command(l),
                                                                             right_command(r), is_err(f) {}

void PipeCommand::execute() {
    Command * f = smash.CreateCommand(left_command.c_str());
    Command * s = smash.CreateCommand(right_command.c_str());
    int pip[2];
    if (pipe(pip) < 0) {
        perror("smash error: pipe failed");
        return;
    }
    int fd = -1;
    pid_t pid = fork();
    if (pid < 0) {
        perror("smash error: fork failed");
        return;
    }
    else if (pid == 0){ // thle5 bn
        if (close(pip[1]) < 0) {
            perror("smash error: close failed");
            return;
        }
        fd = dup(0);
        if (fd < 0) {
            perror("smash error: dup failed");
            return;
        }
        if (dup2(pip[0],0) < 0) {
            perror("smash error: dup2 failed");
            return;
        }
        s->execute();
        if (close(pip[0]) < 0) {
            perror("smash error: close failed");
            return;
        }
        if (dup2(fd,0) < 0) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd) < 0) {
            perror("smash error: close failed");
            return;
        }
        exit(0);
    } else { // thle5 av
        if (close(pip[0]) < 0) {
            perror("smash error: close failed");
            return;
        }
        int x = is_err ? 2 : 1;
        fd = dup(x);
        if (fd < 0) {
            perror("smash error: dup failed");
            return;
        }
        if (dup2(pip[1],x) < 0) {
            perror("smash error: dup2 failed");
            return;
        }
        f->execute();
        if (close(pip[1]) < 0) {
            perror("smash error: close failed");
            return;
        }
        if (dup2(fd,x) < 0) {
            perror("smash error: dup2 failed");
            return;
        }
        if (close(fd) < 0) {
            perror("smash error: close failed");
            return;
        }
        if (wait(nullptr) < 0) {
            perror("smash error: wait failed");
            return;
        }
    }
}

//redirection command
RedirectionCommand::RedirectionCommand(const char *cmd_line, string out, string cmd, bool append) : Command(cmd_line,false),
                                                                                                    out_file(*&out),
                                                                                                    command(*&cmd),
                                                                                                    is_append(append) {}

void RedirectionCommand::execute() {
    int fd = dup(1);
    if (fd < 0) {
        perror("smash error: dup failed");
        return;
    }
    if (close(1) < 0) {
        perror("smash error: close failed");
        return;
    }
    int x = -1;
    x = open(out_file.c_str(), O_CREAT | O_RDWR | (is_append ? O_APPEND : O_TRUNC), 0666);
    if (x < 0) {
        perror("smash error: open failed");
        if (dup2(fd, 1) < 0) {
            perror("smash error: dup2 failed");
            return;
        }
        return;
    }
    Command *new_command = smash.CreateCommand(command.c_str());

    new_command->execute();
    if (close(1) < 0) {
        perror("smash error: close failed");
        return;
    }
    if (dup2(fd, 1) < 0) {
        perror("smash error: dup2 failed");
        return;
    }
    if (close(fd) < 0) {
        perror("smash error: close failed");
        return;
    }
}

TimeoutCommand::TimeoutCommand(const char *cmd_line, bool bg, char ** args, int num_args) : Command(cmd_line,bg){
    this->sec = atoi((args[1]));
    for (int i = 2; i < num_args ; ++i) {
        innerCmd = innerCmd + string(args[i]) + " ";
    }
    this->finish = time(nullptr) + sec;
}

void TimeoutCommand::setAlarm() {
    SmallShell& smash = SmallShell::getInstance();
    if(smash.timed.empty())
        return;
    time_t curr = time(nullptr);
    time_t min_time =  smash.timed[0].finish - curr;
    for (unsigned int i = 1; i < smash.timed.size() ; ++i) {
        if(smash.timed[i].finish - curr < min_time){
            min_time = smash.timed[i].finish - curr;
        }
    }
    alarm(min_time);
}

void TimeoutCommand::execute() {

    if(finish == 0) return;
    SmallShell& smash = SmallShell::getInstance();
    smash.timed.push_back(*this);
    this->setAlarm();
    if(_isBackgroundComamnd(cmd))
        smash.executeCommand((this->innerCmd + "&").c_str());
    else
        smash.executeCommand(this->innerCmd.c_str());
}

void SmallShell::removeFinishedCommand() {
    int size = this->timed.size();
    for (int i = 0; i < size ; ++i) {
        if(waitpid(timed[i].son_pid, nullptr,WNOHANG) != 0 ){
            timed.erase(timed.begin()+i);
        }
    }
}

//JobsList
JobsList::JobsList() {
    jobs = vector<JobEntry>();
    size = 0;
    max_id = 0;
}

void JobsList::addJob(const char *cmd_l, pid_t pid, bool isStopped) {
    removeFinishedJobs();
    JobEntry *job = new JobsList::JobEntry();
    string cmd_s = string(cmd_l);
    cmd_s = _trim(string(cmd_l));
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd_l, args);
    job->job_id = max_id + 1;
    job->job_pid = pid;

    job->is_stopped = isStopped;
    if (string(args[args_num - 1]).find('&') == 0) {
        job->is_bg = true;
    } else
        job->is_bg = false;
    job->cmd = cmd_s;
    job->start_time = time(nullptr);
    jobs.push_back(*job);
    max_id += 1;
    size = int(jobs.size());
}

void JobsList::printJobsList() {

    time_t end_time = time(nullptr);
    removeFinishedJobs();
    for (unsigned int i = 0; i < jobs.size(); ++i) {
        JobsList::JobEntry job = jobs[i];

        double elapsed_Seconds = difftime(end_time, job.start_time);
        if (job.is_stopped) {
            cout << "[" << job.job_id << "] " << job.cmd << " : " << job.job_pid << " " << elapsed_Seconds << " secs "
                 << "(stopped)"
                 << endl;
        } else
            cout << "[" << job.job_id << "] " << job.cmd << " : " << job.job_pid << " " << elapsed_Seconds << " secs "
                 << endl;
    }
}

void JobsList::removeFinishedJobs() {//7zra m3 external

    for (unsigned int i = 0; i < jobs.size(); i++) {
        JobEntry job = jobs[i];
        int status;
        pid_t p = waitpid(job.job_pid, &status, WNOHANG);
        if (p < 0) {
            perror("smash error: waitpid failed");

            return;
        } else if (p == job.job_pid) {
            jobs.erase(jobs.begin() + i);
            i--;
        }
    }
    if (jobs.empty()) {
        max_id = 0;
        size = int(jobs.size());
    } else {
        max_id = jobs.back().job_id;
        size = (int) jobs.size();
    }


}

void JobsList::removeJobById(int jobId) {
    for (int i = 0; i < size; ++i) {
        JobEntry job = jobs[i];
        if (job.job_id == jobId) {
            jobs.erase(jobs.begin() + i);
            return;
        }
    }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
    for (unsigned int i = 0; i < jobs.size(); ++i) {
        JobEntry *job = &jobs[i];
        if (job->job_id == jobId)
            return job;
    }
    return nullptr;
}

JobsList::JobEntry *JobsList::getLastJob() {
    if (jobs.empty())
        return nullptr;
    return &jobs.back();
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
    for (unsigned long i = jobs.size() - 1; i >= 0; --i) {
        JobEntry *job = &jobs[i];
        if (job->is_stopped)
            return job;
    }
    return nullptr;
}

int JobsList::numOfStopped() {
    int counter = 0;
    for (unsigned int i = 0; i < jobs.size(); ++i) {
        JobsList::JobEntry job = jobs[i];
        if (job.is_stopped) {
            counter++;
        }
    }
    return counter;
}


//smash
SmallShell::SmallShell() {
// TODO: add your implementation
    prompt = SMASH;
    prev_dir = "";
    smash_pid = getpid();
    current_job_pid = -1;
    jobs_list = new JobsList();
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete jobs_list;
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    char **args = new char *[COMMAND_MAX_ARGS + 1];
    int args_num = _parseCommandLine(cmd_s.c_str(), args);
    if (*args == nullptr) {
        return nullptr;
    }
    if (cmd_s.find("timeout") == 0) {
        return new TimeoutCommand(cmd_line, _isBackgroundComamnd(cmd_line), args, args_num);
    }
    if (cmd_s.find('|') != string::npos) {
        if (_isBackgroundComamnd(cmd_line)) {
            _removeBackgroundSign((char *) cmd_line);
        }
        bool flag = cmd_s.find('&') != string::npos;
        string left = cmd_s.substr(0, cmd_s.find('|'));
        int tmp = flag ? cmd_s.find_last_of('&') : cmd_s.find_last_of('|');
        string right = cmd_s.substr(tmp + 1);
        return new PipeCommand(cmd_line, left, right, flag);
    } else if (cmd_s.find('>') != string::npos || cmd_s.find(">>") != string::npos) {
        if (_isBackgroundComamnd(cmd_line)) {
            _removeBackgroundSign((char *) cmd_line);
        }
        bool append = (cmd_s.find(">>") != string::npos);
        string cmd = cmd_s.substr(0, cmd_s.find('>'));
        string out = cmd_s.substr(cmd_s.find_last_of('>') + 1);
        while (out.c_str()[0] == ' ')
            out = out.substr(1);
        while (out.c_str()[out.size() - 1] == ' ')
            out = out.substr(0, out.size() - 1);
        return new RedirectionCommand(cmd_line, out, cmd, append);
    }

    if (string(args[0]).find("chprompt") == 0) {
        return new ChPromptCommand(cmd_line);
    } else if (string(args[0]).find("showpid") == 0) {
        return new ShowPidCommand(cmd_line);
    } else if (string(args[0]).find("pwd") == 0) {
        return new PwdCommand(cmd_line);
    } else if (string(args[0]).find("cd") == 0) {
        return new ChangeDirCommand(cmd_line);
    } else if (string(args[0]).find("jobs") == 0) {
        return new JobsCommand(cmd_line, jobs_list);
    } else if (string(args[0]).find("kill") == 0) {
        return new KillCommand(cmd_line, jobs_list);
    } else if (string(args[0]).find("fg") == 0) {
        return new ForegroundCommand(cmd_line, jobs_list);
    } else if (string(args[0]).find("bg") == 0) {
        return new BackgroundCommand(cmd_line, jobs_list);
    } else if (string(args[0]).find("quit") == 0) {
        return new QuitCommand(cmd_line, jobs_list);
    } else if (string(args[0]).find("head") == 0) {
        if (args_num == 2)
            return new HeadCommand(cmd_line,string(args[1]),10);
        else if (args_num == 3) {
            int n = atoi((string(args[1]).substr(1)).c_str());
            return new HeadCommand(cmd_line,string(args[2]),n);
        }
        else
            cout << "smash error: head: not enough arguments" << endl;
        return nullptr;
    }
    return new ExternalCommand(cmd_line);
}


void SmallShell::executeCommand(const char *cmd_line) {

    Command *command = CreateCommand(cmd_line);
    if (command) {
        command->execute();
    }
    delete command;
}