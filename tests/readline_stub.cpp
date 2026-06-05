/* Stub implementations of readline and history symbols used by the shell.
   Allows test executables to be built without libreadline installed. */
#include "readline/readline.h"
#include "readline/history.h"

rl_completion_func_t* rl_attempted_completion_function = nullptr;
int                   rl_attempted_completion_over      = 0;
char                  rl_completion_append_character    = ' ';
int                   history_length                    = 0;

void  rl_ding()        {}
void  rl_on_new_line() {}
void  rl_redisplay()   {}
char* readline(const char*) { return nullptr; }

void         using_history()                              {}
void         stifle_history(int)                          {}
int          read_history(const char*)                    { return 0; }
int          write_history(const char*)                   { return 0; }
int          history_truncate_file(const char*, int)      { return 0; }
int          append_history(int, const char*)             { return 0; }
HIST_ENTRY*  remove_history(int)                          { return nullptr; }
void         clear_history()                              {}
void         add_history(const char*)                     {}
HIST_ENTRY** history_list()                               { return nullptr; }
