#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    // TODO: Add your implementation

    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-Z" << endl;
    if (smash.current_job_pid != -1) {
        if (kill(smash.current_job_pid,SIGSTOP) < 0) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " <<  smash.current_job_pid <<" was stopped" << endl;
    }

}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash: got ctrl-C" << endl;
    if (smash.current_job_pid != -1) {
        if (kill(smash.current_job_pid,SIGKILL) < 0) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " <<  smash.current_job_pid <<" was killed" << endl;
    }
}

void alarmHandler(int sig_num) {
    // TODO: Add your implementation
    SmallShell& smash = SmallShell::getInstance();
    if(smash.current_job_pid == getpid()){
        return;
    }
    cout << "smash: got an alarm" << endl;
    TimeoutCommand last=smash.timed.back();
    smash.removeFinishedCommand();
    time_t cur = time(nullptr);
    for (unsigned int i = 0; i < smash.timed.size() ; ++i) {
        if(smash.timed[i].finish <= cur){
            char copy_cmd[COMMAND_ARGS_MAX_LENGTH];
            strcpy(copy_cmd, smash.timed[i].cmd);
            unsigned int max_index=0, index=0;
            bool last=false;
            for(index=0; index < string(copy_cmd).size(); index++){
                if(copy_cmd[index] == ' '){
                    max_index = index;
                    last = true;
                }
                else
                    last = false;
            }
            if(last) {
                copy_cmd[max_index] = '\0';
            }
            if(smash.timed[i].is_bg) {
                cout << "smash: " << copy_cmd << "&" << " timed out!" << endl;
            }
            else {
                cout << "smash: " << copy_cmd << " timed out!" << endl;
            }
            if(smash.current_job_pid == smash.timed[i].son_pid){
                smash.current_job_pid = -1;
            }
            killpg(smash.timed[i].son_pid, SIGKILL);
            smash.timed.erase(smash.timed.begin()+i);
        }
    }
    if(!smash.timed.empty()) smash.timed[0].setAlarm();  // eza m3 fork mn2em had
    smash.jobs_list->removeFinishedJobs();
}

