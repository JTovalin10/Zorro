#pragma once
/* Minimal readline stub for test builds — no real readline required.
   Include <string.h> so callers get strdup/strcpy etc. just like the real
   readline.h does on most platforms. */
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef char** rl_completion_func_t(const char*, int, int);
typedef char*  rl_compentry_func_t(const char*, int);

extern rl_completion_func_t* rl_attempted_completion_function;
extern int                   rl_attempted_completion_over;
extern char                  rl_completion_append_character;

void  rl_ding(void);
void  rl_on_new_line(void);
void  rl_redisplay(void);
char* readline(const char* prompt);

#ifdef __cplusplus
}
#endif
