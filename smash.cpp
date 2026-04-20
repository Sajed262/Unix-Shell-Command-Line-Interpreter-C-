#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction sig_alarm;
    sig_alarm.sa_handler = &alarmHandler;
    sig_alarm.sa_flags = SA_RESTART;
    if(sigaction(SIGALRM,&sig_alarm, nullptr) < 0){
        perror("smash error: failed to set alarm handler");
    }
    //TODO: setup sig alarm handler
    SmallShell& smash = SmallShell::getInstance();

    while(true) {
        std::cout << smash.prompt;
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        string pash_mkplt = _trim(cmd_line);
        if (pash_mkplt.empty())
            continue;
        smash.jobs_list->removeFinishedJobs();
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}