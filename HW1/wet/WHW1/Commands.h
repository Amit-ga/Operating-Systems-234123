#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <string.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <list>
#include <sys/stat.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;

class Command {
// TODO: Add your data members
protected:
    int argc;
    //SHOULD BE NULL TERMINATED FOR EXECV AND SHOULD NOT CONTAIN &
    char** argv;
    bool success;
    bool is_fg;
    char* cmd;
    //char* bg_cmd;
    pid_t pid;   

 public:
  Command(const char* cmd_line);
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
  // Set Methods
  void setPID(pid_t pid);
  //Get Methods
  char* getCmd();
  pid_t getPID();
  bool getStatus();
  bool get_fg();
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
    bool is_special;
    char* non_special_trimmed_cmd;
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {
      if (this->non_special_trimmed_cmd) {
          delete[] this->non_special_trimmed_cmd;
      }
  }
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
    string cmd1;
    string cmd2;
    int redirection_channel;
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
  void setCommand1(string cmd);
  void setCommand2(string cmd2);
  void setCommand1Rd(int ch);
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
    string src_path;
    int dest_fd;
    string dest_path;
    int stdout_fd;
    int syscall_flags;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
  void setSrcPath(string src_path);
  void setDestPath(string dest_path);
  void setSyscallFlags(int syscall_flags);
};


class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
    char* cmd_line;
    char* plastPwd;
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
    JobsList* jobs;
public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
   // TODO: Add your data members
      Command* cmd;
      bool isStopped;
      int id;
      int pid;
      time_t insert_time;
      time_t diff_time;
  public:
      JobEntry(Command* cmd, bool isStopped, int id, time_t insert_time);
      Command* getJobCommand();
      bool getIsJobStopped();
      int geJobtId();
      time_t getJobInsertTime();
      void setIsJobStopped(bool is_stopped);
  };
 // TODO: Add your data members
  list<JobEntry*>* jobs;
  int max_job_id;
  int size;
 public:
  JobsList();
  ~JobsList() = default;
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
  void removeJobByPId(pid_t pid);
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList* jobs;
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
    JobsList* jobs;
    int fg_id;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
    JobsList* jobs;
    int bg_id;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
    time_t starting_time;
    int duration;
    char* to_execute;
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand();
  void execute() override;
  time_t getStartingTime();
  int getDuration();
};

class ChmodCommand : public BuiltInCommand {
  // TODO: Add your data members
    int new_mode;
    mode_t old_mode;
    mode_t new_file_mode;
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class GetFileTypeCommand : public BuiltInCommand {
  // TODO: Add your data members
    string filepath;
    struct stat sb;
    string ftype;
    long int size;
 public:
  GetFileTypeCommand(const char* cmd_line);
  virtual ~GetFileTypeCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  // TODO: Add your data members
    JobsList* jobs;
    JobsList::JobEntry* to_change_core;
    int core_num;
 public:
  SetcoreCommand(const char* cmd_linem, JobsList* jobs);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
    JobsList* jobs;
    JobsList::JobEntry* to_sig;
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  // TODO: Add your data members
     JobsList* jobs;
     bool isRunning;
     string prompt;
     Command* curr_running_fg;
     char* last_pwd;
     std::list<TimeoutCommand*>* to_cmds;
     pid_t min_TO_pid;
     string min_TO_cmd;
     SmallShell();
 public:
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
  void setPrompt(const string new_prompt = "smash");
  string getPrompt();
  void setLastWD(char* cwd);
  char* getLastWD();
  void setRunning(bool is_running);
  bool getIsRunning();
  void setRunningFgCmd(Command*);
  Command* getRunningFgCmd();
  void insertJob(Command* cmd, bool isStopped);
  void removeJob(pid_t pid, bool is_to, int jobid = -1);
  pid_t getMinTimeoutPID();
  string getMinTimeoutCMD();
  //void insertTO(TimeoutCommand* timout_cmd);
  void setNextAlarmCmd();
  JobsList* getAllJobs() {
      return this->jobs;
  }
};

#endif //SMASH_COMMAND_H_
