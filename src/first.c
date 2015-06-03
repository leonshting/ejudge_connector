#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <openssl/hmac.h>
#include <ctype.h>

#include "cJSON.h"
#include "usefull_trash.h"
#include "c_example1.h"
#include "first.h"
#include "ezxml.h"



/*
I expect that kind of JSON post:
{
        "user_id" : int,
        "lesson_id" : int,
        "task_id" : string,
        "programming_language" : string,
        "file_name" : string,
        "filetype" : string, //extinct
        "file_content" : string,
        "response_url" : string,
        //"status" : string thats a modification for reply JSON post
}
some kind a change here .... changed succesfully
*/

/*Files where we save run data:
    num.txt number of run
    SID.txt session id
    return.txt CSV string about parcel
    return_report.txt file with detailed info about parcel
*/

/* in fact we need make custom filenames for each examplar to avoid conflicts
    maybe it doesnot appeal to the SID.txt
     but maybe appeals OR ican just make dir and change to it
*/

const char* ejudge_common[] = {"OK", "CE", "RT", "TL", "PE", "WA", "CF", "PT", "IG",
                               "DQ", "ML", "SE", "SV", "WT", "PR", "RJ", "SK", NULL
                              };

const char* ejudge_detailed[] = {"OK", "Compilation error", "Runtime error", "Time limit exceeded",
                                 "Presentation error", "Wrong answer", "Check Failed", "Partial Solution", "Ignored submit",
                                 "Disqualified", "Memeory limit exceeded", "Security violation", "Style violation", "WT", "PR", "RJ", "SK", NULL
                                };

const char* ejudge_error[] = {"CE", "PE", "CF", "IG",
                              "DQ", "ML", "SE", "SV", "WT", "PR", "RJ", "SK", NULL
                             };

int main(int argc, char *argv[])
{
    pid_t pid = getpid();
    int str_end,hash_beg,i = 0, sm, lo, rs, dr, hash_len;
    char dir[200],report[1000],mdString[40]= {0}, *parcel, tmp[100], *from_str, *from_str_tmp, *hash_et, *help_ptr, *arglist_sm[100], *arglist_lo[100], *arglist_rs[100], *arglist_dr[100];
    unsigned char *hash_cand = malloc(50*sizeof(char));
    t_elements parsed_elements;
    sprintf(dir,"%s%d_log.log",LOG_DIR,pid);
    FILE *log = fopen(dir,"w");
    if(log == NULL) exit(-4);
    from_str_tmp = malloc(sizeof(char)*100000);
    sprintf(tmp, "/tmp/%d/", pid);
    mkdir(tmp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    chdir(tmp);
    fprintf(log, "PID: %d, dir changed\nthats what we got:\n", pid);
    do
    {
        from_str_tmp[i] = getc(stdin);
        fputc(from_str_tmp[i],log);
        if(from_str_tmp[i] == '-' && from_str_tmp[i-1] == '-')
        {
            str_end=i-1;
            hash_beg=i+1;
            from_str_tmp[str_end]=0;
        }
    }
    while(from_str_tmp[i++] != EOF);
    hash_et = from_str_tmp + sizeof(char)*hash_beg;
    hash_et[40]=0;
    from_str_tmp += 8*sizeof(char);
    from_str = curl_decode(from_str_tmp);
    help_ptr = from_str;
    from_str_tmp = decode(from_str); /*base64 conversion*/
    fprintf(log,"\nAfter url_decode:%s",from_str);
    //\r\n
    from_str=from_str_tmp;
    fprintf(log,"\nhashed string:%s\n",help_ptr);
    HMAC(EVP_sha1(),SALT, strlen(SALT), (unsigned char*)help_ptr, strlen(help_ptr),hash_cand,(unsigned int*)&hash_len);
    for(i = 0; i < 20; i++)
        sprintf(&mdString[i*2], "%02x",hash_cand[i]);
    //FIXME leaks everywhere
    if(strcmp(mdString,hash_et)==0)
    {
        fprintf(log,"\nhash correct\n");
        fprintf(log,"decoded string: %s\n", from_str);
        printf("Content-Type: text/plain;charset=us-ascii\n\n");
    }
    else
    {
        fprintf(log, "%s\n%s\n hash incorrect\n",mdString,hash_et);
        printf("Content-Type: text/html\nStatus: 422 Bad Request\n\n");
        free(hash_cand);
        free(from_str);
        fclose(log);
        exit(-1);
    }
    /*we have now string to parse*/
    cJSON *root = cJSON_Parse(from_str);
    if (!root)
    {
        fprintf(log,"Error before: [%s]\n", cJSON_GetErrorPtr());
    }
    fprintf(log,"exec part\n");
    parse_into_elements(&parsed_elements, root, log);
    fflush(NULL);
    lo = make_arglist_for_login(arglist_lo, parsed_elements);
    fork_exec_with_inp_redir(arglist_lo, 1, "whatever");
    sm = make_arglist_for_submit_and_run(arglist_sm, parsed_elements, arglist_lo[4]);
    fork_exec_with_inp_redir(arglist_sm, 1, "num.txt");
    rs = make_arglist_for_run_status(arglist_rs, parsed_elements, arglist_lo[4]);
    int rep_num;
    if((rep_num=wait_for_report(5, arglist_rs, parsed_elements.status))==-1)
    {
        strcpy(parsed_elements.status,"error");
        strcpy(report,"ejudge marking error");
    }
    else
    {
        int flag=0,some;
        some = submit_checked(parsed_elements.status, ejudge_common);
        if(strcmp(parsed_elements.status,"OK")==0)
        {
            strcpy(parsed_elements.status,"ok");
            flag=1;
        }
        else if(submit_checked(parsed_elements.status, ejudge_error)!=-1)
        {
            strcpy(parsed_elements.status,"error");
            strcpy(report,ejudge_detailed[some]);
            if(some == 1) flag = 2; /*let report be when compilation error*/
        }
        else
        {
            strcpy(parsed_elements.status, "fail");
            flag=1;
        }
        if(flag)
        {
            dr = make_arglist_for_dump_report(arglist_dr, parsed_elements, arglist_lo[4]); //BACKTRACE part
            fork_exec_with_inp_redir(arglist_dr, 1, "return_report.xml");
            /*xml parse part*/
            if(flag==1){
                ezxml_t trace = ezxml_parse_file("return_report.xml");
                const char * points = ezxml_attr(trace, "score");
                const char * tp = ezxml_attr(trace, "tests-passed");
                const char * to = ezxml_attr(trace, "run-tests");
                sprintf(report, "%s. Your score is = %s, %s/%s tests passed",ejudge_detailed[some], points , tp, to);
                ezxml_free(trace);
                /* parse part end */
                /* BTW ezxml lib used so you have possibility to add smth to report */
            }
            else if(flag==2)
            {
                char *iftmp;
                iftmp = read_string("return_report.xml");
                strcat(report, iftmp);
                free(iftmp);
            }

        }
    }
    fprintf(log, "exec part end\n");
    cJSON_AddStringToObject(root, "trace", report);
    cJSON_AddStringToObject(root, "status", parsed_elements.status);
    cJSON_DeleteItemFromObject(root,"programming_language");
    cJSON_DeleteItemFromObject(root,"file_name");
    cJSON_DeleteItemFromObject(root,"file_content");
    parcel = cJSON_Print(root);
    fprintf(log,"output:%s\n",parcel);
    free(from_str);
    from_str=encode(parcel);
    fprintf(log,"encoded output:%s\n", from_str);
    parcel=curl_encode(from_str);
    fprintf(log,"curl_encoded output:%s\n", parcel);
    HMAC(EVP_sha1(),SALT, strlen(SALT), (unsigned char*)from_str, strlen(from_str),hash_cand,(unsigned int*)&hash_len);
    for(i = 0; i < 20; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)hash_cand[i]);
    fprintf(log,"hash HMAC:%s\n", mdString);
    sprintf(from_str,"session=%s--%s",parcel,mdString);
    fprintf(log,"final output:%s\n", from_str);
    /*ready to send backparcel*/
    sendJSON(parsed_elements.url, from_str);
    sendJSON("http://ejudge.100ege.ru/cgi-bin/a.out", from_str); //FIXME: for logging purposes
    //printf("%s",from_str);
    /*now clean part*/
    fprintf(log,"\n%s\n",from_str);
    str_vector_free(arglist_lo, lo);
    str_vector_free(arglist_rs, rs);
    str_vector_free(arglist_sm, sm);
    free(root);
    free(from_str);
    free(parcel);
    fclose(log);
    chdir("/tmp/");
    execl("/bin/rm", "/bin/rm", "-rf", tmp, NULL);
    return 0;
}

void parse_into_elements(t_elements *elements, cJSON *root, FILE *log)
{
    char str[100000]= {0};
    int i = 0;
    sprintf(elements->task_id, "%d", cJSON_GetObjectItem(root, "task_id")->valueint);
    elements->lesson_id = cJSON_GetObjectItem(root, "lesson_id")->valueint;
    elements->user_id = cJSON_GetObjectItem(root, "user_id")->valueint;
    strcpy(elements->full_filename, cJSON_GetObjectItem(root, "file_name")->valuestring);
    strcpy(str, cJSON_GetObjectItem(root, "file_content")->valuestring);
    strcpy(elements->prog_language,cJSON_GetObjectItem(root,"programming_language")->valuestring);
    strcpy(elements->url,cJSON_GetObjectItem(root,"response_url")->valuestring);
    //reponse is temp change
    fprintf(log,"\nsourcecode:\n%s",str);
    FILE *source = fopen(elements->full_filename, "w");
    do
    {
        fputc(str[i], source);
    }
    while(str[++i] != 0);

    fclose(source);
}

void choose_lang(char *langid, char *filetype)
{
    if(!strcmp(filetype, "C")) strcpy(langid, "gcc");
    else if(!strcmp(filetype, "C++")) strcpy(langid, "g++");
    else if(!strcmp(filetype, "Python 3")) strcpy(langid, "python3");
    else if(!strcmp(filetype, "pas")) strcpy(langid, "fpc");
}

int make_arglist_for_submit_and_run(char **outp, t_elements element, char *session_id)
{
    int i = 0;
    char tmp_string[100];
    outp[i++] = strdup("/home/leonshting/bin/ejudge-contests-cmd");
    outp[i] = strdup(outp[i-1]);
    i++;
    sprintf(tmp_string, "%d", element.lesson_id);
    outp[i++] = strdup(tmp_string);
    outp[i++] = strdup("submit-run");
    outp[i++] = strdup(session_id);
    outp[i++] = strdup(element.task_id);
    choose_lang(tmp_string, element.prog_language);
    outp[i++] = strdup(tmp_string);
    outp[i++] = strdup(element.full_filename);
    outp[i] = NULL;
    return i;
}

int make_arglist_for_login(char **outp, t_elements element)
{
    int i = 0;
    char tmp_string[100];
    outp[i++] = strdup("/home/leonshting/bin/ejudge-contests-cmd");
    outp[i] = strdup(outp[i-1]);
    i++;
    sprintf(tmp_string, "%d", element.lesson_id);
    outp[i++] = strdup(tmp_string);
    outp[i++] = strdup("login");
    outp[i++] = strdup("SID.txt");
    outp[i++] = strdup(LOGIN); //login input is here
    outp[i++] = strdup(PASSWD); //password input is here
    outp[i] = NULL;
    return i;
}

int make_arglist_for_run_status(char **outp, t_elements element, char *session_id)
{
    int i = 0, g = 0;
    char tmp_string[100];
    outp[i++] = strdup("/home/leonshting/bin/ejudge-contests-cmd");
    outp[i] = strdup(outp[i-1]);
    i++;
    sprintf(tmp_string, "%d", element.lesson_id);
    outp[i++] = strdup(tmp_string);
    outp[i++] = strdup("run-status");
    outp[i++] = strdup(session_id);
    g = get_number("num.txt");
    sprintf(tmp_string, "%d", g);
    outp[i++] = strdup(tmp_string);
    outp[i] = NULL;
    return i;
}

int make_arglist_for_dump_report(char **outp, t_elements element, char *session_id)
{
    int i = 0, g = 0;
    char tmp_string[100];
    outp[i++] = strdup("/home/leonshting/bin/ejudge-contests-cmd");
    outp[i]=strdup(outp[i-1]);
    i++;
    sprintf(tmp_string, "%d", element.lesson_id);
    outp[i++] = strdup(tmp_string);
    outp[i++] = strdup("dump-report");
    outp[i++] = strdup(session_id);
    g = get_number("num.txt");
    sprintf(tmp_string, "%d", g);
    outp[i++] = strdup(tmp_string);
    outp[i] = NULL;
    return i;
}

char * strcat_ext(char **outp, int num)
{
    int i=0;
    char * retv = malloc(10000*sizeof(char));
    while(outp[i]!=NULL)
    {
        strcat(retv,outp[i++]);
        strcat(retv, " ");
    }
    return retv;
}
int get_number(const char *filename)
{
    int retval;
    FILE *f = fopen(filename, "r");
    fscanf(f, "%d", &retval);
    fclose(f);
    return retval;
}

void str_vector_free(char ** str, int num_of)
{
    int i;
    for(i = 0; i<num_of; i++) free(str[i]);
}

int read_CSV(const char * filename, char ** element)
{
    FILE *f = fopen(filename, "r");
    char c, tmp_string[100];
    int i = 0, j = 0;
    while(1)
    {
        while((c = getc(f)) != ';' && c != EOF)
        {
            tmp_string[i++] = c;
        }
        tmp_string[i] = 0;
        i = 0;
        if(c == EOF) break;
        element[j++] = strdup(tmp_string);
    }
    fclose(f);
    return j;
}

char * read_string(const char * filename)
{
    FILE *f=fopen(filename, "r");
    struct stat st;
    stat(filename, &st);
    char *tmp_string = malloc((100+st.st_size)*sizeof(char));
    int i=0;
    do
    {
        tmp_string[i]=fgetc(f);
    }
    while(tmp_string[i++]!=EOF);
    tmp_string[i-1]=0;
    fclose(f);
    return tmp_string;
}
int wait_for_report(int a, char **arglist, char * status)
{
    int i, elmn = 0, timeout=1;
    char *tmp_string[100], *needed;
    for(i = 0; i<a; i++)
    {
        sleep(timeout++);
        fork_exec_with_inp_redir(arglist, 1, "return.txt");
        elmn = read_CSV("return.txt", tmp_string);
        needed = strdup(tmp_string[6]);
        if(submit_checked(needed,ejudge_common)!= -1)
        {
            strcpy(status, needed);
            str_vector_free(tmp_string, elmn);
            return i;
        }
        str_vector_free(tmp_string, elmn);
    }
    str_vector_free(tmp_string, elmn);
    return -1;
}


int submit_checked(const char * status, const char **ejudge_status)
{
    int i;
    for(i = 0; ejudge_status[i] != NULL; i++)
    {
        if (strcmp(status, ejudge_status[i]) == 0)
            return i;
    }
    return -1;
}

int sendJSON(const char * url, const char * parcel)
{
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POST, 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, parcel);
        res = curl_easy_perform(curl);
        /* Check for errors */
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}

char* curl_decode(const char* inp)
{
    /*this func allocates memory by itself*/
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    return curl_easy_unescape(curl, inp, 0, 0);
}

char* curl_encode(const char* inp)
{
    /*this func allocates memory by itself*/
    CURL *curl;
    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    return curl_easy_escape(curl, inp, 0);
}


