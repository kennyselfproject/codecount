#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define MAX_PATH    512
#define MAX_FILES   65535

typedef enum CodeStat{
    STAT_CODE_START,
    STAT_CODE_CONTEXT,
    STAT_SINGLE_START,
    STAT_SINGLE_CONTEXT,
    STAT_COMMENT_START,
    STAT_COMMENT_CONTEXT,
    STAT_MAX
}CodeStat;

typedef enum FileTyoe{
    UNKNOW_FILE,
    C_FILE,
    H_FILE,
    CPP_FILE,
    SQL_FILE,
    YML_FILE,
    JAVA_FILE,
    MAKE_FILE,
    SHELL_FILE,
    PYTHON_FILE,
    MAX_TYPE
}FileType;

typedef struct FileStat
{
    FileType fileType;
    char     filePath[MAX_PATH];
    long     totalLines;
    long     codeLines;
    long     commentLines;
    long     emptyLines;
    long     emptyCommentLines;
}FileStat;

typedef struct DirStat
{
    int       type;
    int       fileMax;
    int       fileKnow;
    int       fileTotal;
    FileStat  files[MAX_FILES];
    FileStat  languages[MAX_TYPE];
}DirStat;

void init_languages(DirStat *dirStat);
void print_files(DirStat *dirStat);
void search_files(DirStat *dirStat, char *dirPath);
void read_file_stat(FileStat *fileStat);
void init_file_stat(FileStat *fileStat, char *buffer, int fileSize);
char* get_suffix(char *fileName);
FileType get_file_type(char *fileName);
char* get_file_type_string(FileType fileType);

int main(int argc, char *argv[])
{
    char     *codeDir = (char*)"/work/code/tools/codecount/test";
    DirStat  *dirStat = (DirStat*)malloc(sizeof(DirStat));

    switch(argc)
    {
    case 2:
        codeDir = argv[1];
    case 1:
        printf("[INFO] the code root dirrectory [%s]\n", codeDir);
        break;

    default:
        printf("The command syntax as following:\n"
               "\t%s [code root dirrectory]",
               argv[0]);
        exit(1);
        break;
    }

    init_languages(dirStat);
    search_files(dirStat, codeDir);
    print_files(dirStat);

    return 0;
}

void print_file_array(long items, FileStat *filesStat){
    int i = 0;
    printf(" language | total lines |  code lines | cmmnt lines |"
           " empty lines | empty cmmnt lines |\n");
    printf("----------|-------------|-------------|-------------|"
           "-------------|-------------------|\n");
    for (i = 0; i < items; i++) {
        FileStat *fileStat = filesStat + i;
        printf("%9s |%12ld |%12ld |%12ld |%12ld |%19ld|\n", 
               get_file_type_string(fileStat->fileType), fileStat->totalLines, 
               fileStat->codeLines, fileStat->commentLines,
               fileStat->emptyLines, fileStat->emptyCommentLines);
    }
}

void print_files(DirStat *dirStat) {
    int i = 0;
    printf("[INFO] get languages files\n");
    print_file_array(MAX_TYPE, dirStat->languages);
    printf("\n[INFO] get files: %d\n", dirStat->fileTotal);
    print_file_array(dirStat->fileTotal, dirStat->files);
}

void init_languages(DirStat *dirStat) {
    int i;
    FileStat *fileStat = dirStat->languages;
    for (i = 0; i < MAX_TYPE; i++) {
        fileStat[i].fileType = (FileType)i;
    }
}

void search_files(DirStat *dirStat, char *dirPath) {
    char           path[MAX_PATH];
    struct dirent *fileName = NULL;
    DIR           *dir      = NULL;

    assert(dirPath != NULL);
    if(dirPath == NULL) {
        printf("open dir path is null!\n");
        exit(1);
    }
 
    dir = opendir(dirPath);
    if(dir == NULL) {
        printf("open dir %s error!\n", dirPath);
        exit(1);
    }
 
    while((fileName = readdir(dir)) != NULL) {
        if(!strcmp(fileName->d_name,".") || !strcmp(fileName->d_name,"..")
           || !strcmp(fileName->d_name,".git"))
            continue;
   
        sprintf(path,"%s/%s", dirPath, fileName->d_name);
 
        struct stat s;
        lstat(path, &s);
 
        if(S_ISDIR(s.st_mode)) {
            search_files(dirStat, path);
        } else {
            FileStat *fileStat = &dirStat->files[dirStat->fileTotal];
            FileStat *languagesStat = NULL;

            strcpy(fileStat->filePath, path);
            fileStat->fileType = get_file_type(fileName->d_name);
            read_file_stat(fileStat);

            languagesStat = &dirStat->languages[fileStat->fileType];
            languagesStat->totalLines        += fileStat->totalLines;
            languagesStat->codeLines         += fileStat->codeLines;
            languagesStat->commentLines      += fileStat->commentLines;
            languagesStat->emptyLines        += fileStat->emptyLines;
            languagesStat->emptyCommentLines += fileStat->emptyCommentLines;

            dirStat->fileTotal++;
            if (fileStat->fileType == UNKNOW_FILE)
                printf("%d. %s\n", dirStat->fileTotal, fileName->d_name);
        }
    }
    closedir(dir);
}

void read_file_stat(FileStat *fileStat) {
    FILE *stream = NULL;
    char *buffer = NULL;
    long  fileSize = 0;

    if (fileStat == NULL)
        return;

    fileStat->totalLines = 0;
    fileStat->codeLines = 0;
    fileStat->commentLines = 0;
    fileStat->emptyLines = 0;
    fileStat->emptyCommentLines = 0;

#ifdef _DEBUG
    printf("[INFO] open file [%s]\n", fileStat->filePath);
#endif

    if((stream = fopen(fileStat->filePath, "r")) == NULL) {
        printf("[ERROR] open file [%s] fail, errno:%d\n", 
               fileStat->filePath, errno);
        goto error;
    }

#ifdef _DEBUG
    printf("[INFO] get file [%s] size\n", fileStat->filePath);
#endif

    if (fseek(stream, 0, SEEK_END)) {
        printf("[ERROR] file [%s] fseek end with error [%d]\n", 
               fileStat->filePath, errno);
        goto error;
    }

    fileSize = ftell(stream);
    if (fseek(stream, 0, SEEK_SET)) {
        printf("[ERROR] file [%s] fseek head with error [%d]\n", 
               fileStat->filePath, errno);
        goto error;
    }

#ifdef _DEBUG
    printf("[INFO] the file [%s] size [%ld]\n", 
           fileStat->filePath, fileSize);
    printf("[INFO] malloc memory buffer size [%ld]\n", fileSize);
#endif

    buffer = (char*)malloc(fileSize + 10);

#ifdef _DEBUG
    printf("[INFO] the memory buffer [%p——%p]\n", 
           buffer, buffer + fileSize);

    printf("[INFO] read data from file [%s] size [%ld]\n", 
           fileStat->filePath, fileSize);
#endif

    if (fread(buffer, fileSize, 1, stream) != 1) {
        printf("[INFO] read file [%s] with error[%d]\n", 
               fileStat->filePath, errno);
        goto error;
    }

    if (stream != NULL) {
#ifdef _DEBUG
        printf("[INFO] read file [%s] is closed\n", fileStat->filePath);
#endif
        fclose(stream);
        stream = NULL;
    }

    init_file_stat(fileStat, buffer, fileSize);

error:
    if (buffer != NULL) {
        free(buffer);
        buffer = NULL;
    }

    if (stream != NULL) {
#ifdef _DEBUG
        printf("[INFO] read file [%s] is closed\n", fileStat->filePath);
#endif
        fclose(stream);
    }
}

void init_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    if (buffer == NULL)
        return;

    for(; cur < end; cur++) {
        switch (fileStat->fileType) {
        case C_FILE:
        case H_FILE:
        case CPP_FILE:
        case JAVA_FILE:
            switch(*cur) {
            case '/':
                if (*(cur+1) == '/' || *(cur+1) == '*') {
                    cur++;
                    switch(stat) {
                    case STAT_CODE_CONTEXT:
                        // code line, comments after the code
                        fileStat->codeLines++;
                    case STAT_CODE_START:
                        if (*cur == '/')
                            stat = STAT_SINGLE_START;
                        else
                            stat = STAT_COMMENT_START;
                        break;

                    case STAT_SINGLE_START:
                    case STAT_SINGLE_CONTEXT:
                    case STAT_COMMENT_START:
                    case STAT_COMMENT_CONTEXT:
                        break;

                    default:
                        assert(false);
                        break;
                    }
                }
                break;

            case '*':
                if (*(cur+1) == '/') {
                    cur++;
                    switch(stat) {
                    case STAT_COMMENT_START:
                        fileStat->emptyCommentLines++;
                        stat = STAT_CODE_START;
                        while(*(cur+1) == ' ')
                            cur++;
                        if (*(cur+1) == '\r')
                            cur++;
                        if (*(cur+1) == '\n')
                            cur++;
                        fileStat->totalLines++;
                        break;

                    case STAT_COMMENT_CONTEXT:
                        fileStat->commentLines++;
                        stat = STAT_CODE_START;
                        while(*(cur+1) == ' ')
                            cur++;
                        if (*(cur+1) == '\r')
                            cur++;
                        if (*(cur+1) == '\n')
                            cur++;
                        fileStat->totalLines++;
                        break;

                    default:
                        break;
                    }
                }
                break;

            case '\r':
                if (*(cur+1) == '\n')
                    cur++;
            case '\n':
                switch(stat) {
                case STAT_SINGLE_START:
                    stat = STAT_CODE_START;
                case STAT_COMMENT_START:
                    // empty comment line
                    fileStat->emptyCommentLines++;
                    break;

                case STAT_SINGLE_CONTEXT:
                    stat = STAT_CODE_START;
                    // comment line
                    fileStat->commentLines++;
                    break;

                case STAT_COMMENT_CONTEXT:
                    stat = STAT_COMMENT_START;
                    // comment line
                    fileStat->commentLines++;
                    break;

                case STAT_CODE_START:
                    // empty code line
                    fileStat->emptyLines++;
                    break;

                case STAT_CODE_CONTEXT:
                    // code line
                    fileStat->codeLines++;
                    stat = STAT_CODE_START;
                    break;

                default:
                    break;
                }
                fileStat->totalLines++;
                break;

            case ' ':
                break;

            default:
                switch (stat) {
                case STAT_CODE_START:
                    stat = STAT_CODE_CONTEXT;
                    break;

                case STAT_SINGLE_START:
                    stat = STAT_SINGLE_CONTEXT;
                    break;

                case STAT_COMMENT_START:
                    stat = STAT_COMMENT_CONTEXT;
                    break;

                default:
                    break;
                }
            }
            break;

        case MAKE_FILE:
        case SHELL_FILE:
            switch(*cur) {
            case '#':
                switch(stat) {
                case STAT_CODE_CONTEXT:
                    // code line, comments after the code
                    fileStat->codeLines++;
                    stat = STAT_SINGLE_START;
                    break;

                case STAT_SINGLE_CONTEXT:
                    break;

                default:
                    stat = STAT_SINGLE_START;
                    break;
                }
                break;

            case '\r':
                if (*(cur+1) == '\n')
                    cur++;
            case '\n':
                switch(stat) {
                case STAT_SINGLE_START:
                    // empty comment line
                    fileStat->emptyCommentLines++;
                    break;

                case STAT_SINGLE_CONTEXT:
                    // comment line
                    fileStat->commentLines++;
                    break;

                case STAT_CODE_START:
                    // empty code line
                    fileStat->emptyLines++;
                    break;

                case STAT_CODE_CONTEXT:
                    // code line
                    fileStat->codeLines++;
                    break;

                default:
                    break;
                }
                stat = STAT_CODE_START;
                fileStat->totalLines++;
                break;

            case ' ':
                break;

            default:
                switch (stat) {
                case STAT_CODE_START:
                    stat = STAT_CODE_CONTEXT;
                    break;

                case STAT_SINGLE_START:
                    stat = STAT_SINGLE_CONTEXT;
                    break;

                default:
                    break;
                }
            }
            break;

        case SQL_FILE:
            switch(*cur) {
            case '-':
                if (*(cur+1) == '-') {
                    switch(stat) {
                    case STAT_CODE_CONTEXT:
                        // code line, comments after the code
                        fileStat->codeLines++;
                        stat = STAT_SINGLE_START;
                        break;

                    case STAT_SINGLE_CONTEXT:
                        break;

                    default:
                        stat = STAT_SINGLE_START;
                        break;
                    }
                }
                break;

            case '\r':
                if (*(cur+1) == '\n')
                    cur++;
            case '\n':
                switch(stat) {
                case STAT_SINGLE_START:
                    // empty comment line
                    fileStat->emptyCommentLines++;
                    break;

                case STAT_SINGLE_CONTEXT:
                    // comment line
                    fileStat->commentLines++;
                    break;

                case STAT_CODE_START:
                    // empty code line
                    fileStat->emptyLines++;
                    break;

                case STAT_CODE_CONTEXT:
                    // code line
                    fileStat->codeLines++;
                    break;

                default:
                    break;
                }
                stat = STAT_CODE_START;
                fileStat->totalLines++;
                break;

            case ' ':
                break;

            default:
                switch (stat) {
                case STAT_CODE_START:
                    stat = STAT_CODE_CONTEXT;
                    break;

                case STAT_SINGLE_START:
                    stat = STAT_SINGLE_CONTEXT;
                    break;

                default:
                    break;
                }
            }
            break;

        case YML_FILE:
            switch(*cur) {
            case '-':
                switch(stat) {
                case STAT_CODE_CONTEXT:
                    // code line, comments after the code
                    fileStat->codeLines++;
                    stat = STAT_SINGLE_START;
                    break;

                case STAT_SINGLE_CONTEXT:
                    break;

                default:
                    stat = STAT_SINGLE_START;
                    break;
                }
                break;

            case '\r':
                if (*(cur+1) == '\n')
                    cur++;
            case '\n':
                switch(stat) {
                case STAT_SINGLE_START:
                    // empty comment line
                    fileStat->emptyCommentLines++;
                    break;

                case STAT_SINGLE_CONTEXT:
                    // comment line
                    fileStat->commentLines++;
                    break;

                case STAT_CODE_START:
                    // empty code line
                    fileStat->emptyLines++;
                    break;

                case STAT_CODE_CONTEXT:
                    // code line
                    fileStat->codeLines++;
                    break;

                default:
                    break;
                }
                stat = STAT_CODE_START;
                fileStat->totalLines++;
                break;

            case ' ':
                break;

            default:
                switch (stat) {
                case STAT_CODE_START:
                    stat = STAT_CODE_CONTEXT;
                    break;

                case STAT_SINGLE_START:
                    stat = STAT_SINGLE_CONTEXT;
                    break;

                default:
                    break;
                }
            }
            break;

        default:
            switch(*cur) {
            case '\r':
                if (*(cur+1) == '\n')
                    cur++;
            case '\n':
                fileStat->totalLines++;
                fileStat->codeLines++;
                break;

            default:
                break;
            }
            break;
        } // switch (fileType)
    } // while

    if (*(cur-1) != '\n') {
        switch(stat) {
        case STAT_SINGLE_START:
        case STAT_COMMENT_START:
            // empty comment line
            fileStat->emptyCommentLines++;
            break;

        case STAT_SINGLE_CONTEXT:
        case STAT_COMMENT_CONTEXT:
            // comment line
            fileStat->commentLines++;
            break;

        case STAT_CODE_START:
            // empty code line
            fileStat->emptyLines++;
            break;

        case STAT_CODE_CONTEXT:
            // code line
            fileStat->codeLines++;
            break;

        default:
            break;
        }
    }
}

char* get_suffix(char *fileName) {
    char *end = fileName + strlen(fileName) - 1;
    while (end > fileName) {
        if (*end == '.') {
            return end + 1;
        }
        end--;
    }

    return end;
}

FileType get_file_type(char *fileName) {
    char *suffix = get_suffix(fileName);
  
    if (!strcasecmp(suffix, "C")) {
        return C_FILE;
    } else if (!strcasecmp(suffix, "H")) {
        return H_FILE;
    } else if (!strcasecmp(suffix, "CPP") || !strcasecmp(suffix, "CXX")
                || !strcasecmp(suffix, "CC")) {
        return CPP_FILE;
    } else if (!strcasecmp(suffix, "SQL")) {
        return SQL_FILE;
    } else if (!strcasecmp(suffix, "YML")) {
        return YML_FILE;
    } else if (!strcasecmp(suffix, "JAVA")) {
        return JAVA_FILE;
    } else if (!strcasecmp(suffix, "SH") || !strcasecmp(suffix, "KSH")) {
        return SHELL_FILE;
    } else if (!strcasecmp(suffix, "Makefile")) {
        return MAKE_FILE;
    } else if (!strcasecmp(suffix, "PY")) {
        return PYTHON_FILE;
    }

    return UNKNOW_FILE;
}

char* get_file_type_string(FileType fileType) {
    switch(fileType) {
    case C_FILE:
        return (char*)"C";

    case H_FILE:
        return (char*)"HEADER";

    case CPP_FILE:
        return (char*)"CPP";

    case SQL_FILE:
        return (char*)"SQL";

    case YML_FILE:
        return (char*)"YML";

    case JAVA_FILE:
        return (char*)"JAVA";

    case SHELL_FILE:
        return (char*)"SHELL";

    case MAKE_FILE:
        return (char*)"MAKEFILE";

    case PYTHON_FILE:
        return (char*)"PYTHON";

    default:
        break;
    }

    return (char*)"UNKNOWN";
}
