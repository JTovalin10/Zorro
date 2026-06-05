#pragma once
/* Minimal readline/history stub for test builds. */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _hist_entry {
    char* line;
    char* timestamp;
    void* data;
} HIST_ENTRY;

extern int history_length;

void        using_history(void);
void        stifle_history(int max);
int         read_history(const char* filename);
int         write_history(const char* filename);
int         history_truncate_file(const char* filename, int nlines);
int         append_history(int nelements, const char* filename);
HIST_ENTRY* remove_history(int which);
void        clear_history(void);
void        add_history(const char* string);
HIST_ENTRY** history_list(void);

#ifdef __cplusplus
}
#endif
