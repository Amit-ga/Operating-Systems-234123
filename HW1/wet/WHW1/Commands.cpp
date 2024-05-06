#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <algorithm>
#include <sched.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <climits>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
#define LAST_ADDED_JOB -1

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

/// <summary>
/// removes all whitespaces from start and end of string
/// </summary>
/// <param name="s">does not change string</param>
/// <returns>"       kill    -9   1      &        " => "kill    -9   1      &"</returns>
string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}


/// <summary>
/// Example: "   kill    -9   1      &     " => res = 4, args=[kill,-9,1,&];
/// </summary>
/// <param name="cmd_line">remains unchanged</param>
/// <param name="args">contains all commands parameters, including command itself</param>
/// <returns> number of arguments to command (including command itself)</returns>
int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

/// <summary>
/// removes & an whitespaces after it, if it appears last
/// </summary>
/// <param name="cmd_line">changed by function:    kill    -9   1      &     " =>
///                                                     "   kill    -9   1      " =></param>
void _removeBackgroundSign(char* cmd_line) {
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

/* our helper functions */

/// <summary>
/// free segel's malloc allocated resources 
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
void freeParsedCommandLine(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
    {
        free(argv[i]);
    }
}

/// <summary>
/// check if a non-empty string contains only digits
/// </summary>
/// <param name="s"></param>
/// <returns></returns>
bool isValidId(const string& s)
{
    return !s.empty() && find_if(s.begin(),
        s.end(), [](unsigned char c) { return !isdigit(c); }) == s.end();
}

// TODO: Add your implementation for classes in Commands.h 

/* Command */

Command::Command(const char* cmd_line) {
    this->argv = new char* [COMMAND_MAX_ARGS];
    if (!this->argv) {
        this->success = false;
        return;
    }
    this->argc = _parseCommandLine(cmd_line, argv);
    this->is_fg = !_isBackgroundComamnd(cmd_line);
    this->cmd = new char[string(cmd_line).length() + 1];
    strcpy(this->cmd, cmd_line);
    this->success = true;
}

Command::~Command() {
    freeParsedCommandLine(this->argc,this->argv);
    if (this->cmd) {
        delete[] this->cmd;
    }
}

void Command::setPID(pid_t pid)
{
    this->pid = pid;
}

pid_t Command::getPID() {
    return this->pid;
}

char* Command::getCmd() {
    return this->cmd;
}

bool Command::getStatus()
{
    return this->success;
}

bool Command::get_fg()
{
    return this->is_fg;
}

/* BuiltInCommand */

BuiltInCommand::BuiltInCommand(const char* cmd_line) : 
    Command(cmd_line) 
{}

/* ShowPid */

ShowPidCommand::ShowPidCommand(const char* cmd_line) : 
    BuiltInCommand(cmd_line) 
{}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

/* pwd */

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : 
    BuiltInCommand(cmd_line) 
{}

void GetCurrDirCommand::execute() {
    char buffer[COMMAND_ARGS_MAX_LENGTH];
    char* cwd = getcwd(buffer, sizeof(buffer));
    if (!cwd) {
        perror("smash error: getcwd failed");
        return;
    }
    cout << cwd << endl;
}

/* cd */
ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : 
    BuiltInCommand(cmd_line)
{
    if (plastPwd) {
        this->plastPwd = new char[COMMAND_ARGS_MAX_LENGTH];
        strcpy(this->plastPwd, *plastPwd);
    }
    //check input validity
    if (this->argc > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        this->success = false;
        return;
    }

    if (!strcmp(this->argv[1], "-")) {
        //no dirs left in the dirs tree
        if (!this->plastPwd) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            this->success = false;
            return;
        }
    }
}

void ChangeDirCommand::execute() {
    //first, save current wd
    SmallShell& sh = SmallShell::getInstance();
    char buffer[COMMAND_ARGS_MAX_LENGTH];
    char* cwd = getcwd(buffer, COMMAND_ARGS_MAX_LENGTH);
    if (!cwd) {
        perror("smash error: getcwd failed");
        return;
    }
    //go back to last wd
    if (!strcmp(this->argv[1], "-")) {
        if (chdir(this->plastPwd)) {
            perror("smash error: chdir failed");
            return;
        }
    }
    else if (chdir(string(this->argv[1]).c_str())) {
        perror("smash error: chdir failed");
        return;
    }
    //update shell last wd
    sh.setLastWD(cwd);
}

/* jobs */

//command
JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs) : 
    BuiltInCommand(cmd_line),
    jobs(jobs) 
{}

void JobsCommand::execute() {
    this->jobs->printJobsList();
}

//list
//entry
JobsList::JobEntry::JobEntry(Command* cmd, bool isStopped, int id, time_t insert_time) :
    cmd(cmd),
    isStopped(isStopped),
    id(id),
    pid(0),
    insert_time(insert_time),
    diff_time(0)
{}
Command* JobsList::JobEntry::getJobCommand() {
    return this->cmd;
}
bool JobsList::JobEntry::getIsJobStopped(){
    return this->isStopped;
}
int JobsList::JobEntry::geJobtId() {
    return this->id;
}
time_t JobsList::JobEntry::getJobInsertTime() {
    return this->insert_time;
}
void JobsList::JobEntry::setIsJobStopped(bool is_stopped) {
    this->isStopped = is_stopped;
}
//end of entry class

JobsList::JobsList() : 
    jobs(new list<JobEntry*>()),
    max_job_id(0),
    size(0)
{}

void JobsList::addJob(Command* cmd, bool isStopped) {
    //assign the new job a new id
    int new_job_id = 0;
    //get current max
    if (this->jobs->size() == 0) {
        this->max_job_id = 0;
    }
    else {
        auto max_e = max_element(this->jobs->begin(), this->jobs->end(),
            [](JobEntry* je1, JobEntry* je2) {
                return je1->geJobtId() < je2->geJobtId();
            });
        this->max_job_id = (*max_e)->geJobtId();
    }
    this->max_job_id = this->max_job_id + 1;

    if (this->size > 0) {
        new_job_id = this->max_job_id;
    }
    else {
        new_job_id = 1;
    }
    //create new job
    JobEntry* to_insert = new JobEntry(cmd, isStopped, new_job_id, time(0));
    this->jobs->push_back(to_insert);
    this->size++;
    this->jobs->sort([](JobEntry* je1, JobEntry* je2) { return je1->geJobtId() < je2->geJobtId(); });
    }

    void JobsList::printJobsList() {
        for (auto const& i : *this->jobs) {
            //printing format: [<job-id>] <command> : <process id> <seconds elapsed> (stopped)
                // [<job-id>] <command> : <process id> <seconds elapsed>
            cout << "[" << i->geJobtId() << "] ";
            cout << string(i->getJobCommand()->getCmd()) << " : ";
            cout << i->getJobCommand()->getPID() << " ";
            cout << difftime(time(0), i->getJobInsertTime()) << " secs";
            if (i->getIsJobStopped()) {
                cout << " (stopped)";
            }
            cout << endl;
        }
    }

    void JobsList::killAllJobs() {
        for (auto const& i : *this->jobs) {
            pid_t to_kill = i->getJobCommand()->getPID();
            if (kill(to_kill, SIGKILL)) {
                perror("smash error: kill failed");
            }
            cout << i->getJobCommand()->getPID() << " : " << i->getJobCommand()->getCmd() << endl;
        }
    }

    void JobsList::removeFinishedJobs() {
        //check list contains elements
        if (this->jobs && this->size > 0) {
            int run_times = this->size;
            //create list iterator
            for (int i = 0; i < run_times; i++)
            {
                auto iter = this->jobs->begin();
                while (iter != this->jobs->end()) {
                    //check if current job is finished, use WNOHANG flag for immidiate result
                    pid_t res = waitpid((*iter)->getJobCommand()->getPID(), nullptr, WNOHANG);
                    if (res < 0) {
                        if (errno == ECHILD) {
                            this->jobs->erase(iter);
                            this->size--;
                            break;
                        }
                        else {
                            perror("smash error: waitpid failed");
                            return;
                        }
                    }
                    else if (res > 0) {
                        this->jobs->erase(iter);
                        this->size--;
                        break;
                    }
                    else {
                        iter++;
                    }
                }
            }

            //update max id in updated list
            if(this->jobs->size() == 0) {
                this->max_job_id = 0;
            }
            else{
                auto max_e = max_element(this->jobs->begin(), this->jobs->end(),
                    [](JobEntry* je1, JobEntry* je2) {
                        return je1->geJobtId() < je2->geJobtId();
                    });
                this->max_job_id = (*max_e)->geJobtId();
            }

            this->jobs->sort([](JobEntry* je1, JobEntry* je2) { return je1->geJobtId() < je2->geJobtId(); });
        }
    }

    JobsList::JobEntry* JobsList::getJobById(int jobId) {
        if (!isValidId(to_string(jobId))){
            return nullptr;
        }

        for (auto const& i : *this->jobs) {
            if (i->geJobtId() == jobId) {
                return i;
            }
        }
        return nullptr;
    }

    JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {

        if (this->jobs && this->jobs->size() > 0)
        {
            JobEntry* last_stopped_job = nullptr;
            auto iter = this->jobs->begin();
            while (iter != this->jobs->end()) {
                if ((*iter)->getIsJobStopped()) {
                    last_stopped_job = (*iter);
                }
                iter++;
            }
            if (last_stopped_job) {
                *jobId = last_stopped_job->geJobtId();
            }
            return last_stopped_job;
        }
        return nullptr;
    }

    JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
        if (this->jobs && this->jobs->size() > 0) {
            //Asuumoptions: 
                //last job is the job with max id
                //list is sorted in an ascending order
            *lastJobId = this->jobs->back()->geJobtId();
            return this->jobs->back();
        }
        return nullptr;
    }

    void JobsList::removeJobById(int jobId) {
        //check list contains elements
        if (this->jobs && this->jobs->size() > 0){
            for (auto it = this->jobs->begin(); it != this->jobs->end(); it++) {
                if ((*it)->geJobtId() == jobId) {
                    this->jobs->erase(it);
                    this->size--;
                    break;
                }
            }
            //update max id in updated list if needed
            if (this->jobs->size() == 0){
                this->max_job_id = 0;
            }
            else {
                auto max_e = max_element(this->jobs->begin(), this->jobs->end(),
                    [](JobEntry* je1, JobEntry* je2) {
                        return je1->geJobtId() < je2->geJobtId();
                    });
                this->max_job_id = (*max_e)->geJobtId();
            }
            this->jobs->sort([](JobEntry* je1, JobEntry* je2) { return je1->geJobtId() < je2->geJobtId(); });
        }
    }
    
    void JobsList::removeJobByPId(pid_t pid) {
        if (this->jobs && this->jobs->size()>0) {
            for (auto it = this->jobs->begin(); it != this->jobs->end(); it++) {
                if ((*it)->getJobCommand()->getPID() == pid) {
                    this->jobs->erase(it);
                    this->size--;
                    break;
                }
            }

            //update max id in updated list if needed
            if (this->jobs->size() == 0)
            {
                this->max_job_id = 0;
            }
            else {
                auto max_e = max_element(this->jobs->begin(), this->jobs->end(),
                    [](JobEntry* je1, JobEntry* je2) {
                        return je1->geJobtId() < je2->geJobtId();
                    });
                this->max_job_id = (*max_e)->geJobtId();
            }
            this->jobs->sort([](JobEntry* je1, JobEntry* je2) { return je1->geJobtId() < je2->geJobtId(); });
        }
    }

    /* fg */
    ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs) :
        BuiltInCommand(cmd_line),
        jobs(jobs)
    {
        if (this->argc >= 2)
        {
         try{
              this->fg_id = stoi(this->argv[1]);
            }
            catch(...){
              cerr << "smash error: fg: job-id " << this->argv[1] << " does not exist" << endl;
              this->success = false;
              return;
            } 
        }
        // no arguments were passed to the command
        else if (this->argc == 1) {
            this->fg_id = LAST_ADDED_JOB;
        }
        //should not get here
        else {
            cerr << "smash error: fg: invalid arguments" << endl;
            this->success = false;
            return;
        }
    }

    void ForegroundCommand::execute() {
        SmallShell& sh = SmallShell::getInstance();
        JobsList::JobEntry* to_fg_job = nullptr;
        if (this->argc >= 2)
        {
            to_fg_job = this->jobs->getJobById(this->fg_id);
            if (!to_fg_job) {
                cerr << "smash error: fg: job-id " << this->fg_id << " does not exist" << endl;
                return;
            }
        }
        else if (this->fg_id == LAST_ADDED_JOB)
        {
            if (!this->jobs->size) {
                cerr << "smash error: fg: jobs list is empty" << endl;
                return;
            }
            to_fg_job = this->jobs->getLastJob(new int);
            if (!to_fg_job) {
                cerr << "smash error: fg: jobs list is empty" << endl;
                return;
            }
        }
        //if we did not exit in the previous if, that means the jobid exists, so now check number of arguments
        if (this->argc > 2) {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }

        //assert: to_fg_job conatins a valid existing job
        cout << to_fg_job->getJobCommand()->getCmd() << " : " << to_fg_job->getJobCommand()->getPID() << endl;
        sh.setRunningFgCmd(to_fg_job->getJobCommand());
        if (kill(to_fg_job->getJobCommand()->getPID(), SIGCONT))
        {
            perror("smash error: kill failed");
            return;
        }
        to_fg_job->setIsJobStopped(false);
        pid_t res = waitpid(sh.getRunningFgCmd()->getPID(), nullptr, WUNTRACED);
        if (res < 0) {
            perror("smash error: waitpid failed");
            return;
        }
        if (!to_fg_job->getIsJobStopped()) {
            this->jobs->removeJobById(to_fg_job->geJobtId());
        }
        sh.setRunningFgCmd(nullptr);
    }

    /* bg */
    BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs) :
        BuiltInCommand(cmd_line), 
        jobs(jobs)
    {
        // check input validity
        if (this->argc >= 2)
        {
            this->bg_id = atoi(this->argv[1]);
        }

        // no arguments were passed to the command
        else if (this->argc == 1) {
            this->bg_id = LAST_ADDED_JOB;
        }

        //should not get here
        else {
            cerr << "smash error: bg: invalid arguments" << endl;
            this->success = false;
            return;
        }
    }

    void BackgroundCommand::execute() {
        JobsList::JobEntry* to_bg_job = nullptr;
        if (this->argc >= 2) {
            to_bg_job = this->jobs->getJobById(this->bg_id);
            if (!this->jobs->size || !to_bg_job) {
                cerr << "smash error: bg: job-id " << this->bg_id << " does not exist" << endl;
                return;
            }
            else if (!to_bg_job->getIsJobStopped())
            {
                cerr << "smash error: bg: job-id " << this->bg_id << " is already running in the background" << endl;
                return;
            }
        }

        else if (this->bg_id == LAST_ADDED_JOB) {
            to_bg_job = this->jobs->getLastStoppedJob(new int);
            if (to_bg_job && !to_bg_job->getIsJobStopped())
            {
                cerr << "smash error: bg: job-id " << this->bg_id << " is already running in the background" << endl;
                return;
            }
            else if (!to_bg_job)
            {
                cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                return;
            }
        }

        //if we did not exit in the previous if, that means the jobid exist, so now check number of arguments
        if (this->argc > 2) {
            cerr << "smash error: bg: invalid arguments" << endl;
            return;
        }

        //Side effects: After resuming a stopped job in the background, the stopped mark should be removed from the jobs list.
        cout << to_bg_job->getJobCommand()->getCmd() << " : " << to_bg_job->getJobCommand()->getPID() << endl;
        to_bg_job->setIsJobStopped(false);
        if (kill(to_bg_job->getJobCommand()->getPID(), SIGCONT)) {
            perror("smash error: kill failed");
            return;
        }
    }

    /* quit */
    QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : 
        BuiltInCommand(cmd_line), 
        jobs(jobs)
    {}

    void QuitCommand::execute() {
        if (this->argc >= 2 && strcmp(this->argv[1], "kill") == 0) {
            cout << "smash: sending SIGKILL signal to " << this->jobs->size << " jobs:" << endl;
            this->jobs->killAllJobs();
        }
    }

    /* kill */
    KillCommand::KillCommand(const char* cmd_line, JobsList* jobs) : 
        BuiltInCommand(cmd_line), 
        jobs(jobs) 
    {
        string abs_sig_num = "";
        //check input validity
        if (this->argc >= 2) {
            abs_sig_num = string(argv[1]).substr(1);
        }
        if (this->argc >= 3) {
            //check if job exists in hobs list
            try{
              this->to_sig = this->jobs->getJobById(stoi(string(this->argv[2])));
              if (!to_sig) {
                  cerr << "smash error: kill: job-id " << this->argv[2] << " does not exist" << endl;
                  this->success = false;
                  return;
              }
            }
            catch(...){
              cerr << "smash error: kill: invalid arguments" << endl;
              this->success = false;
              return;
            }
        }
        //job exsits, now check number of arguments
        if (this->argc != 3 
            || this->argv[1][0] != '-' || !isValidId(abs_sig_num) || stoi(abs_sig_num) < 1 || stoi(abs_sig_num) > 63
                || !isValidId(argv[2])) {
            cerr << "smash error: kill: invalid arguments" << endl;
            this->success = false;
            return;
        }
    }

    void KillCommand::execute() {
        SmallShell& sh = SmallShell::getInstance();
        int sig = stoi(string(this->argv[1]).substr(1));
        bool is_sigstop = sig == SIGSTOP;
        bool is_sigcont = sig == SIGCONT;
        bool is_sigkill = sig == SIGKILL;
        //assert: job exists, signal num is valid
        if (kill(to_sig->getJobCommand()->getPID(), sig))
        {
            perror("smash error: kill failed");
            return;
        }
        //check if sent signal changes commands status
        if (is_sigstop) {
            to_sig->setIsJobStopped(true);
        }
        if (is_sigcont) {
            to_sig->setIsJobStopped(false);
        }
        cout << "signal number " << (string(this->argv[1]).substr(1)) << " was sent to pid " << to_sig->getJobCommand()->getPID() << endl;

        if (is_sigkill) {
            sh.removeJob(0, false, to_sig->geJobtId());
        }
    }

    /* external */
    ExternalCommand::ExternalCommand(const char* cmd_line) :
        Command(cmd_line) {
        this->is_special = (string(this->cmd).find("*") != string::npos
            || string(this->cmd).find("?") != string::npos);
        this->non_special_trimmed_cmd = nullptr;
    }

    void ExternalCommand::ExternalCommand::execute() {
        int res = 0;

        //check if special command
        if (this->is_special) {
            _removeBackgroundSign(this->cmd);
            res = execlp("/bin/bash", "/bin/bash", "-c", this->cmd, NULL);
        }
        else {
            //simple external commands
            //clear & in case it stands on its own, e.g: sleep 5    &
            if (!string(this->argv[this->argc - 1]).compare("&")) {
                delete[] this->argv[this->argc - 1];
                this->argv[this->argc - 1] = nullptr;
            }
            else {
                //clear & in case its adjacent to time, e.g: sleep 5&
                _removeBackgroundSign(this->argv[this->argc - 1]);
                string cmd_s = _trim(this->argv[this->argc - 1]);
                this->non_special_trimmed_cmd = new char[cmd_s.length() + 1];
                strcpy(this->non_special_trimmed_cmd, cmd_s.c_str());
                this->argv[this->argc - 1] = this->non_special_trimmed_cmd;
            }

            res = execvp(this->argv[0], this->argv);
        }
        if (res < 0) {
            perror("smash error: exec failed");
            return;
        }
    }

    /* pipe */
    PipeCommand::PipeCommand(const char* cmd_line) :
        Command(cmd_line)
    {}

    void PipeCommand::execute() {
        SmallShell& sh = SmallShell::getInstance();
        int pipe_fd[2];
        if (pipe(pipe_fd) < 0) {
           perror("smash error: pipe failed");
           return;
        }
        //p1 will change the pipe writing channel and excute the first command
        pid_t pid1 = fork();
        if (pid1 == -1) {
            perror("smash error: fork failed");
            return;
        }
        else if (pid1 == 0) {
            //write = 1 , read = 0
            dup2(pipe_fd[1], this->redirection_channel);
            close(pipe_fd[0]); //close readind end
            close(pipe_fd[1]); //close writing end
            sh.executeCommand(this->cmd1.c_str());
            exit(0);
        }
        //p2 will change the pipe reading channel and excute the second command
        pid_t pid2 = fork();
        if (pid2 == -1) {
            perror("smash error: fork failed");
            return;
        }
        else if (pid2 == 0) {
            dup2(pipe_fd[0], fileno(stdin));
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            sh.executeCommand(this->cmd2.c_str());
            exit(0);
        }
        //close pipe in the parent
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        //wait until both p1 and p2 are done before returning
        while (true) {
            pid_t terminated_process = waitpid(P_ALL, nullptr, 0);
            if (terminated_process == -1) {
                break;
            }
        }
    }

    void PipeCommand::setCommand1(string cmd)
    {
        this->cmd1 = cmd;
    }
    void PipeCommand::setCommand2(string cmd2)
    {
        this->cmd2 = cmd2;
    }
    void PipeCommand::setCommand1Rd(int ch)
    {
        this->redirection_channel = ch;
    }

    /* rd */
    RedirectionCommand::RedirectionCommand(const char* cmd_line) :
        Command(cmd_line)
    {}

    void RedirectionCommand::execute(){
        SmallShell& sh = SmallShell::getInstance();
        // duplicate the file descriptor for standard output 
            //and assign the new file descriptor to redirection command stdout_fd
        this->stdout_fd = dup(fileno(stdout));
        if (this->stdout_fd == -1) {
            perror("smash error: dup failed");
            return;
        }
        //open destination file
        this->dest_fd = open((this->dest_path).c_str(), this->syscall_flags, 0655);
        if (this->dest_fd == -1) {
            perror("smash error: open failed");
            return;
        }
        // duplicate the file descriptor for standard output, to the file descriptor stdout_fd
            //close the latter if it's already open.
         // duplicate the file descriptor for dest_fd, to the file descriptor of standard output
            //close the latter if it's already open.
        if (dup2(fileno(stdout), this->stdout_fd) == -1 || dup2(dest_fd, fileno(stdout)) == -1) {
            perror("smash error: dup2 failed");
            return;
        }

        //execute command
        sh.executeCommand(this->src_path.c_str());

        if (dup2(this->stdout_fd, fileno(stdout)) == -1) {
            perror("smash error: dup2 failed");
            return;
        }
        //close detination file
        if (close(this->dest_fd) == -1) {
            perror("smash error: close failed");
            return;
        }
    }

    void RedirectionCommand::setSrcPath(string src_path) {
        this->src_path = src_path;
    }
    void RedirectionCommand::setDestPath(string dest_path) {
        this->dest_path = dest_path;
    }
    void RedirectionCommand::setSyscallFlags(int syscall_flags) {
        this->syscall_flags = syscall_flags;
    }

    /* setcore */

    SetcoreCommand::SetcoreCommand(const char* cmd_linem, JobsList* jobs) :
        BuiltInCommand(cmd_linem),
        jobs(jobs) {

        //check if job exists in hobs list
        try{
          this->to_change_core = this->jobs->getJobById(stoi(string(this->argv[1])));
        }
        catch(...){
          cerr << "smash error: setcore: job-id " << this->argv[1] << " does not exist" << endl;
          this->success = false;
          return;
        }
        if (!to_change_core) {
            cerr << "smash error: setcore: job-id " << this->argv[1] << " does not exist" << endl;
            this->success = false;
            return;
        }

        //check input validity
        long num_processors = sysconf(_SC_NPROCESSORS_ONLN);
        this->core_num = atoi(this->argv[2]);
        if (!isValidId(this->argv[2]) || core_num >= num_processors) {
            cerr << "smash error: setcore: invalid core number" << endl;
            this->success = false;
            return;
        }

        if (num_processors == -1) {
            this->success = false;
            perror("smash error: sysconf failed");
        }
       
        if (this->argc != 3 || !isValidId(this->argv[1])) {
            cerr << "smash error: setcore: invalid arguments" << endl;
            this->success = false;
            return;
        }

    }

    void SetcoreCommand::execute() {
        //assert: job exists, core num is valid
        cpu_set_t cpu_set;
        CPU_ZERO(&cpu_set);
        CPU_SET(this->core_num, &cpu_set);
        if (sched_setaffinity(this->to_change_core->getJobCommand()->getPID(), sizeof(cpu_set_t), &cpu_set) == -1) {
            perror("smash error: sched_setaffinity failed");
            return;
        }
    }

    /* file type*/

    GetFileTypeCommand::GetFileTypeCommand(const char* cmd_line) :
        BuiltInCommand(cmd_line){
        //check input validity
        if (this->argc != 2) {
            cerr << "smash error: getfiletype: invalid arguments" << endl;
            this->success = false;
            return;
        }

        istringstream iss(this->argv[1]);
        if (!(iss >> this->filepath)) {
            cerr << "smash error: getfiletype: invalid arguments" << endl;
            this->success = false;
            return;
        }

        if (lstat(this->filepath.c_str(), &this->sb) == -1) {
            this->success = false;
            perror("smash error: lstat failed");
            return;
        }

        this->ftype = "invalid";
        this->size = 0;
    }

    void GetFileTypeCommand::execute()
    {
        switch (this->sb.st_mode & S_IFMT) {
        case S_IFREG:
            this->ftype = "regular file";
            break;
        case S_IFDIR:
            this->ftype = "directory";
            break;
        case S_IFCHR:
            this->ftype = "character device";
            break;
        case S_IFBLK:
            this->ftype = "block device";
            break;
        case S_IFIFO:
            this->ftype = "FIFO";
            break;
        case S_IFLNK:
            this->ftype = "symbolic link";
            break;
        case S_IFSOCK:
            this->ftype = "socket";
            break;
        default:
            this->ftype = "invalid";
            break;
        }

        if (this->ftype.compare("invalid") == 0) {
            cerr << "smash error: gettype: invalid arguments" << endl;
            return;
        }

        this->size = sb.st_size;
        cout << filepath << "'s type is \"" << this->ftype << "\" and takes up " << this->size << " bytes" << endl;

    }

    /* chmod */

    ChmodCommand::ChmodCommand(const char* cmd_line) :
        BuiltInCommand(cmd_line){
        //check input validity
        if (this->argc != 3) {
            std::cerr << "smash error: chmod: invalid arguments" << std::endl;
            this->success = false;
            return;
        }

        // Parse the new mode from the command line arguments
        char* endptr;
        this->new_mode = strtol(this->argv[1], &endptr, 8);

        // Check that the new mode was valid
        if (*endptr != '\0' || errno == ERANGE) {
            std::cerr << "smash error: chmod: invalid arguments" << std::endl;
            this->success = false;
            return;
        }

        struct stat file_stat;
        // Retrieve the current file mode of the file
        if (stat(this->argv[2], &file_stat) == -1) {
            this->success = false;
            perror("smash error: stat failed");
            return;
        }

        this->old_mode = file_stat.st_mode;

        // Update the file mode with the new mode
        this->new_file_mode = (old_mode & ~07777) | new_mode;
    }

    void ChmodCommand::execute()
    {
        // Set the updated file mode using chmod
        if (chmod(this->argv[2], this->new_file_mode) == -1) {
            perror("smash error: stat failed");
            return;
        }
    }

    /* timeout */

    TimeoutCommand::TimeoutCommand(const char* cmd_line) : 
        BuiltInCommand(cmd_line) {
        //check input validity
        if (this->argc < 3) {
            //std::cerr << "smash error: timeout: invalid arguments" << std::endl;
            this->success = false;
            return;
        }

        if (!isValidId(this->argv[1]))
        {
            //std::cerr << "smash error: timeout: invalid arguments" << std::endl;
            this->success = false;
            return;
        }

        this->starting_time = time(nullptr);
        this->duration = atoi(this->argv[1]);

        //Retrieve the command that needs to be executed
        //assert: number of arguments >=3
        std::stringstream ss;
        for (int i = 2; i < this->argc; i++) {
            ss << this->argv[i] << " ";
            }
        int size = ss.str().length() + 1;
        this->to_execute = new char[size];
        strcpy(this->to_execute, ss.str().c_str());
        _removeBackgroundSign(this->to_execute);
    }

    void TimeoutCommand::execute() {
        execlp("/bin/bash", "/bin/bash", "-c", this->to_execute, NULL);
    }

    time_t TimeoutCommand::getStartingTime() {
        return this->starting_time;
    }
    int TimeoutCommand::getDuration() {
        return this->duration;
    }

    TimeoutCommand::~TimeoutCommand() {
        if (this->to_execute) {
            delete[] this->to_execute;
        }
    }

SmallShell::SmallShell() {
// TODO: add your implementation
    this->jobs = new JobsList();
    this->isRunning = true;
    this->prompt = "smash> ";
    this->last_pwd = new char[COMMAND_ARGS_MAX_LENGTH];
    this->to_cmds = new std::list<TimeoutCommand*>();
    this->curr_running_fg = nullptr;
    this->min_TO_pid = -1;
    this->min_TO_cmd = "";
}

void SmallShell::setPrompt(const string new_prompt) {
    this->prompt = new_prompt + "> ";
}

string SmallShell::getPrompt(){
    return this->prompt;
}

void SmallShell::setLastWD(char* cwd){
    strcpy(this->last_pwd, cwd);
}

char* SmallShell::getLastWD(){
    return this->last_pwd;
}

void SmallShell::setRunning(bool is_running){
    this->isRunning = is_running;
}

bool SmallShell::getIsRunning(){
    return this->isRunning;
}

void SmallShell::setRunningFgCmd(Command* cmd) {
    this->curr_running_fg = cmd;
}

Command* SmallShell::getRunningFgCmd() {
    return this->curr_running_fg;
}

void SmallShell::insertJob(Command* cmd, bool isStopped) {
    this->jobs->addJob(cmd, isStopped);
}

void SmallShell::removeJob(pid_t pid, bool is_to, int jobid) {
    if (is_to) {
        for (auto it = this->to_cmds->begin(); it != to_cmds->end(); it++)
        {
            if ((*it)->getPID() == pid) {
                this->to_cmds->erase(it);
                break;
            }
        }
        this->setNextAlarmCmd();
        return;
    }
    if (jobid < 0) {
        this->jobs->removeJobByPId(pid);
    }
    else {
        this->jobs->removeJobById(jobid);
    }
}

pid_t SmallShell::getMinTimeoutPID() {
    return this->min_TO_pid;
}

string SmallShell::getMinTimeoutCMD(){
    return this->min_TO_cmd;
}

void SmallShell::setNextAlarmCmd() {
    if (this->to_cmds && this->to_cmds->size() > 0) {
        TimeoutCommand* to_alarm = nullptr;
        int starting_point = (*this->to_cmds->begin())->getDuration() - difftime(time(nullptr), (*this->to_cmds->begin())->getStartingTime());
        int min, current = starting_point;
        for (auto it = this->to_cmds->begin(); it != this->to_cmds->end(); it++) {
            //get how much time the current command has left to run 
            current = (*it)->getDuration() - difftime(time(nullptr), (*it)->getStartingTime());
            if (current < min && current >= 0) {
                min = current;
                to_alarm = *it;
            }
        }
        //set alarm to command who has the least time left to run
        alarm(min);
        //save command for signal handling
        this->min_TO_pid = to_alarm->getPID();
        this->min_TO_cmd = to_alarm->getCmd();
    }
}

SmallShell::~SmallShell() {
// TODO: add your implementation
    delete this->jobs;
    delete this->to_cmds;
    delete[] this->last_pwd;
    if (!this->curr_running_fg) {
        delete this->curr_running_fg;
    }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  this->jobs->removeFinishedJobs();

  //check if pipe command
  if (count(cmd_s.begin(), cmd_s.end(), '|') == 1 && cmd_s[0] != '|' && cmd_s[cmd_s.length() - 1] != '|') {
      PipeCommand* cmd = new PipeCommand(cmd_line);
      int index = cmd_s.find("|");
      cmd->setCommand1(_trim(cmd_s.substr(0, index)));
      if (cmd_s[index+1] == '&') {
          cmd->setCommand1Rd(fileno(stderr));
          cmd->setCommand2(_trim(cmd_s.substr(index + 2)));
      }
      else {
          cmd->setCommand1Rd(fileno(stdout));
          cmd->setCommand2(_trim(cmd_s.substr(index + 1)));
      }
      return cmd;
  }
  //check if redirection command
  if ((count(cmd_s.begin(), cmd_s.end(), '>') == 1 || count(cmd_s.begin(), cmd_s.end(), '>') == 2)
      && cmd_s[0] != '>' && cmd_s[cmd_s.length() - 1] != '>') {
      RedirectionCommand* rd_command = new RedirectionCommand(cmd_line);
      string::size_type index = cmd_s.find(">>");
      if (index != string::npos) {
          rd_command->setSyscallFlags((O_CREAT | O_WRONLY | O_APPEND));
          rd_command->setSrcPath(_trim(cmd_s.substr(0, index)));
          rd_command->setDestPath(_trim(cmd_s.substr(index + 2)));
      }
      else {
          index = cmd_s.find(">");
          if (index != string::npos) {
              rd_command->setSyscallFlags((O_CREAT | O_WRONLY | O_TRUNC));
              rd_command->setSrcPath(_trim(cmd_s.substr(0, index)));
              rd_command->setDestPath(_trim(cmd_s.substr(index + 1)));
          }
      }
      return rd_command;
  }

  if (firstWord.compare("chprompt") == 0) {
      char** argv = new char* [COMMAND_MAX_ARGS];
      // get number of passed arguments
      int argc = _parseCommandLine(cmd_line, argv);
      if (argc > 1) {
          setPrompt(argv[1]);      
      }
      else {
          setPrompt();
      }
      freeParsedCommandLine(argc, argv);
      delete[] argv;
      return nullptr;
  }

  //If any number of arguments were provided with this command then they will be ignored.
  else if (firstWord.compare("showpid") == 0) {
      return new ShowPidCommand(cmd_line);
  }

  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }

  else if (firstWord.compare("cd") == 0) {
      if (string(this->getLastWD()).length()>0) {
          char* cwd = this->getLastWD();
          return new ChangeDirCommand(cmd_line, &(cwd));
      }
      else {
          return new ChangeDirCommand(cmd_line, nullptr);
      }
  }
  else if (firstWord.compare("jobs") == 0) {
      return new JobsCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("fg") == 0) {
      return new ForegroundCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("bg") == 0) {
      return new BackgroundCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("quit") == 0) {
      //sets shell running attribute to false
      setRunning(false);
      return  new QuitCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("kill") == 0) {
      return new KillCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("setcore") == 0) {
      return new SetcoreCommand(cmd_line, this->jobs);
  }
  else if (firstWord.compare("getfiletype") == 0) {
      return new GetFileTypeCommand(cmd_line);
  }
  else if (firstWord.compare("chmod") == 0) {
      return new ChmodCommand(cmd_line);
  }
  else if (firstWord.compare("timeout") == 0) {
      return new TimeoutCommand(cmd_line);
  }
  else {
    return new ExternalCommand(cmd_line);
  }
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // Please note that you must fork smash process for some commands (e.g., external commands....)
    
  //create the new command
    Command* cmd = CreateCommand(cmd_line);
    //check validity
    if (cmd && cmd->getStatus()) {
        //get command type
        const std::type_info& type = typeid(*cmd);
        //check if timeout/external
        bool is_to = (type == typeid(TimeoutCommand));
        bool is_extern = (type == typeid(ExternalCommand));
        //if timeout or external - fork
        if (is_to || is_extern)
        {
            pid_t pid = fork();

            if (pid == -1) {
                perror("smash error: fork failed");
                return;
            }
            //child
            if (pid == 0) {
                setpgrp();
                cmd->execute();
            }//father
            else {
                //set cmd pid
                cmd->setPID(pid);
                //if timeout command, add to time out commands list and set an alarm
                if (is_to) {
                    this->to_cmds->push_back(dynamic_cast<TimeoutCommand*>(cmd));
                    setNextAlarmCmd();
                }
                //check if fg command
                if (cmd->get_fg()) {
                    this->curr_running_fg = cmd;
                    waitpid(pid, nullptr, WUNTRACED);
                }
                //bg commang
                else {
                    this->curr_running_fg = nullptr;
                    this->jobs->addJob(cmd, false);
                }
            }
        }
        //regular command, just execute
        else {
            cmd->execute();
        }
    }

}
