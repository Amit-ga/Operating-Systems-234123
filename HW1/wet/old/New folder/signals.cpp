#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
    //get access to shell
    SmallShell& sh = SmallShell::getInstance();
    int to_terminate = 0;
    // let user know we received his input
    cout << "smash: got ctrl-Z" << endl;
    //check if there are any commands that are currently running in the fg
    //if smash itself - do nothing
    if (sh.getRunningFgCmd()) {
        //get command pid
        to_terminate = sh.getRunningFgCmd()->getPID();
        //terminate currently running fg command by sending SIGSTOP
        if ((kill(to_terminate, SIGSTOP)) < 0) {
            perror("smash error: kill failed");
            return;
        }
        //assert: killing successful
        cout << "smash: process " << to_terminate << " was stopped" << endl;
        //add command to list of stopped jobs
        sh.insertJob(sh.getRunningFgCmd(), true);
        //update currently running fg command to null
        sh.setRunningFgCmd(nullptr);
    }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    //get access to shell
    SmallShell& sh = SmallShell::getInstance();
    int to_terminate = 0;
    // let user know we received his input
    cout << "smash: got ctrl-C" << endl;
    //check if there are any commands that are currently running in the fg
    if (sh.getRunningFgCmd()) {
        //get command pid
        to_terminate = sh.getRunningFgCmd()->getPID();
        if (!waitpid(to_terminate, nullptr, WNOHANG)) {
            //terminate currently running fg command by sending SIGKILL
            if ((kill(to_terminate, SIGKILL))) {
                perror("smash error: kill failed");
                return;
            }
            cout << "smash: process " << to_terminate << " was killed" << endl;
            //update currently running fg command to null
            sh.setRunningFgCmd(nullptr);
        }
        else {
            //update currently running fg command to null
            sh.setRunningFgCmd(nullptr);
            return;
        }

    }
    else {
        sh.setRunningFgCmd(nullptr);
        return;
    }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
    //get access to shell
    SmallShell& sh = SmallShell::getInstance();
    pid_t timed_out = sh.getMinTimeoutPID();
    string timed_out_c = sh.getMinTimeoutCMD();
    cout << "smash: got an alarm" << endl;
    //check if there are any commands that are currently running in the fg
    if (sh.getRunningFgCmd() && waitpid(timed_out, NULL, WNOHANG)) {
            sh.removeJob(timed_out, false);
            sh.removeJob(timed_out, true);
    }
    else {
        //terminate currently running fg by sending SIGKILL
        if (kill(timed_out, SIGKILL)) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: " << timed_out_c << " timed out!" << endl;
        sh.removeJob(timed_out, true);
    }
}

