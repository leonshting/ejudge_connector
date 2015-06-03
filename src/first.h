#ifndef FIRST_H_INCLUDED
#define FIRST_H_INCLUDED

#define PASSWD "curtis"
#define LOGIN "ejudge"
#define SALT "A61<7?8NnrhsE2V?KO,yhPKJc|33R"
#define LOG_DIR "/tmp/ejudge_100ege_checker/"

typedef struct
{
    char status[100];
    char task_id[10];
    int lesson_id;
    int user_id;
    char full_filename[110];
    char prog_language[20];
    char url[1000];
} t_elements;

void parse_into_elements(t_elements *elements, cJSON *root, FILE *log);
int make_arglist_for_submit_and_run(char **outp, t_elements element, char *session_id_file);
int make_arglist_for_login(char **outp, t_elements element);
int make_arglist_for_run_status(char **outp, t_elements element, char *session_id);
int make_arglist_for_dump_report(char **outp, t_elements element, char *session_id);
void str_vector_free(char ** str, int num_of);
void choose_lang(char *langid, char *filetype);
int wait_for_report(int a, char **arglist, char * status);
int get_number(const char *filename);
int read_CSV(const char * filename, char ** element);
int submit_checked(const char * status, const char **);
int sendJSON(const char * url, const char * parcel);
char* curl_decode(const char* inp);
char* curl_encode(const char* inp);
char * strcat_ext(char **outp, int num);
char * read_string(const char *);
#endif // FIRST_H_INCLUDED
