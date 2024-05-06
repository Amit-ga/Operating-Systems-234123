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

    //TODO: setup sig alarm handler
    //Consider setting up SIG_ALRM handler with sigaction instead of signal, and use SA_RESTART flag
    //set up new sigaction struct
    struct sigaction sig_action_alarm {};
    // add SA_RESTART to sigaction flags
    sig_action_alarm.sa_flags = SA_RESTART;
    //set handler
    sig_action_alarm.sa_handler = alarmHandler;
    if (sigaction(SIGALRM, &sig_action_alarm, NULL)) {
        perror("smash error: failed to set sig-alarm handler");
    }

    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        if (!smash.getIsRunning()) {
            break;
        }
        std::cout << smash.getPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        if (cmd_line.length() > 0) {
            smash.executeCommand(cmd_line.c_str());
        }
    }
    return 0;
}