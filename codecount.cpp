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
    TEXT_FILE,
    PIC_FILE,
    PDF_FILE,
    EXE_FILE,
    EMPTY_FILE,
    MAX_TYPE
}FileType;

typedef struct FileStat
{
    FileType fileType;
    char     fileName[MAX_PATH];
    long     files;
    long     totalSize;
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
void init_c_file_stat(FileStat *fileStat, char *buffer, int fileSize);
void init_shell_file_stat(FileStat *fileStat, char *buffer, int fileSize);
void init_sql_file_stat(FileStat *fileStat, char *buffer, int fileSize);
void init_text_file_stat(FileStat *fileStat, char *buffer, int fileSize);
void init_yml_file_stat(FileStat *fileStat, char *buffer, int fileSize);
void init_file_stat(FileStat *fileStat, char *buffer, int fileSize);
char* get_suffix(char *fileName);
FileType get_file_type(char *fileName);
char* get_file_type_string(FileType fileType);

void end_file_stat(FileStat *fileStat, char end, CodeStat stat);

int main(int argc, char *argv[])
{
    char     *codeDir = (char*)"/work/code/tools/codecount/test";
    DirStat  *dirStat = NULL;

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

    dirStat = (DirStat*)malloc(sizeof(DirStat));
    init_languages(dirStat);
    search_files(dirStat, codeDir);
    print_files(dirStat);

    return 0;
}

void print_file_array(long items, FileStat *filesStat){
    int i = 0;
    printf("                              file                               |"
           " total files |  size   | total lines |  code lines | cmmnt lines |"
           " empty lines | empty cmmnt lines |\n");
    printf("-----------------------------------------------------------------|"
           "-------------|---------|-------------|-------------|-------------|"
           "-------------|-------------------|\n");
    for (i = 0; i < items; i++) {
        FileStat *fileStat = filesStat + i;
        printf("%-64s |%12ld |%8ld |%12ld |%12ld |%12ld |%12ld |%19ld|\n", 
               fileStat->fileName, fileStat->files, fileStat->totalSize, 
               fileStat->totalLines, fileStat->codeLines, fileStat->commentLines,
               fileStat->emptyLines, fileStat->emptyCommentLines);
    }
}

void print_files(DirStat *dirStat) {
    int i = 0;
    printf("[INFO] get languages files\n");
    print_file_array(MAX_TYPE, dirStat->languages);
    printf("\n[INFO] get files: %d\n", dirStat->fileTotal);
    // print_file_array(dirStat->fileTotal, dirStat->files);
}

void init_languages(DirStat *dirStat) {
    int i;
    FileStat *fileStat = dirStat->languages;
    for (i = 0; i < MAX_TYPE; i++) {
        fileStat[i].fileType = (FileType)i;
        sprintf(fileStat[i].fileName, "%s", get_file_type_string((FileType)i));
    }
}

void search_files(DirStat *dirStat, char *dirPath) {
    FileStat      *languagesStat = NULL;
    struct dirent *fileName = NULL;
    char           path[MAX_PATH];
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

            strcpy(fileStat->fileName, path);
            fileStat->fileType = get_file_type(fileName->d_name);
            fileStat->files++;
            read_file_stat(fileStat);

            languagesStat = &dirStat->languages[fileStat->fileType];
            languagesStat->files++;
            languagesStat->totalSize         += fileStat->totalSize;
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
    long  fileSize = 0;

    if (fileStat == NULL)
        return;

#ifdef _DEBUG
    printf("[INFO] open file [%s]\n", fileStat->fileName);
#endif

    if((stream = fopen(fileStat->fileName, "r")) == NULL) {
        printf("[ERROR] open file [%s] fail, errno:%d\n", 
               fileStat->fileName, errno);
        goto error;
    }

#ifdef _DEBUG
    printf("[INFO] get file [%s] size\n", fileStat->fileName);
#endif

    if (fseek(stream, 0, SEEK_END)) {
        printf("[ERROR] file [%s] fseek end with error [%d]\n", 
               fileStat->fileName, errno);
        goto error;
    }

    fileSize = ftell(stream);
    if (fseek(stream, 0, SEEK_SET)) {
        printf("[ERROR] file [%s] fseek head with error [%d]\n", 
               fileStat->fileName, errno);
        goto error;
    }

#ifdef _DEBUG
    printf("[INFO] the file [%s] size [%ld]\n", 
           fileStat->fileName, fileSize);
    printf("[INFO] malloc memory buffer size [%ld]\n", fileSize);
#endif

    fileStat->totalSize = fileSize;
    if (fileSize == 0) {
        fileStat->fileType = EMPTY_FILE;
        goto error;
    }

    if (fileStat->fileType != PDF_FILE && fileStat->fileType != PIC_FILE) {
        char *buffer = (char*)malloc(fileSize + 10);

#ifdef _DEBUG
        printf("[INFO] the memory buffer [%p——%p]\n", 
               buffer, buffer + fileSize);

        printf("[INFO] read data from file [%s] size [%ld]\n", 
               fileStat->fileName, fileSize);
#endif

        if (fread(buffer, fileSize, 1, stream) != 1) {
            printf("[INFO] read file [%s] with error[%d]\n", 
                   fileStat->fileName, errno);
            free(buffer);
            goto error;
        }
        init_file_stat(fileStat, buffer, fileSize);

        free(buffer);
    }

error:
    if (stream != NULL) {
#ifdef _DEBUG
        printf("[INFO] read file [%s] is closed\n", fileStat->fileName);
#endif
        fclose(stream);
    }
}

void init_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    if (buffer == NULL)
        return;

    switch (fileStat->fileType) {
    case C_FILE:
    case H_FILE:
    case CPP_FILE:
    case JAVA_FILE:
        init_c_file_stat(fileStat, buffer, fileSize);
        break;

    case MAKE_FILE:
    case SHELL_FILE:
        init_shell_file_stat(fileStat, buffer, fileSize);
        break;

    case SQL_FILE:
        init_sql_file_stat(fileStat, buffer, fileSize);
        break;

    case YML_FILE:
        init_yml_file_stat(fileStat, buffer, fileSize);
        break;

    default:
        init_text_file_stat(fileStat, buffer, fileSize);
        break;
    } // switch (fileType)
}

void init_c_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    for(; cur < end; cur++) {
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
    }

    end_file_stat(fileStat, *(cur+1), stat);
}

void init_shell_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    for(; cur < end; cur++) {
        switch(*cur) {
        case '!':
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
    }

    end_file_stat(fileStat, *(cur+1), stat);
}

void init_sql_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    for(; cur < end; cur++) {
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
    }

    end_file_stat(fileStat, *(cur+1), stat);
}

void init_yml_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    for(; cur < end; cur++) {
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
    }

    end_file_stat(fileStat, *(cur+1), stat);
}

void init_text_file_stat(FileStat *fileStat, char *buffer, int fileSize) {
    CodeStat stat = STAT_CODE_START;
    char    *cur  = buffer;
    char    *end  = buffer + fileSize;

    for(; cur < end; cur++) {
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
    }

    end_file_stat(fileStat, *(cur+1), stat);
}

void end_file_stat(FileStat *fileStat, char end, CodeStat stat) {
    if (end != '\n') {
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
    while (end >= fileName) {
        if (*end == '.') {
            return end + 1;
        }
        end--;
    }

    return end + 1;
}

FileType get_file_type(char *fileName) {
    char *suffix = get_suffix(fileName);

    if (!strcasecmp(suffix, "C")
        || !strcasecmp(suffix, "Y")
        || !strcasecmp(suffix, "LL")
        || !strcasecmp(suffix, "IN")
        || !strcasecmp(suffix, "IDL")
        || !strcasecmp(suffix, "INL")) {
        return C_FILE;
    } else if (!strcasecmp(suffix, "H")
               || !strcasecmp(suffix, "HH")) {
        return H_FILE;
    } else if (!strcasecmp(suffix, "CPP")
               || !strcasecmp(suffix, "CXX")
               || !strcasecmp(suffix, "CC")) {
        return CPP_FILE;
    } else if (!strcasecmp(suffix, "SQL")
               || !strcasecmp(suffix, "SB")
               || !strcasecmp(suffix, "OS") 
               || !strcasecmp(suffix, "EXP")
               || !strcasecmp(suffix, "AWK")
               || !strcasecmp(suffix, "DDL")
               || !strcasecmp(suffix, "DML")
               || !strcasecmp(suffix, "ETE")
               || !strcasecmp(suffix, "OPT")
               || !strcasecmp(suffix, "DATA")
               || !strcasecmp(suffix, "NTEST") 
               || !strcasecmp(suffix, "KNOWN") 
               || !strcasecmp(suffix, "LINUX") 
               || !strncasecmp(suffix, "QAT", 3)
               || !strncasecmp(suffix, "EQAT", 4)
               || !strncasecmp(suffix, "TEST", 4)
               || !strncasecmp(suffix, "DIFF", 4)
               || !strncasecmp(suffix, "FILTER", 6)
               || !strncasecmp(suffix, "EXPECTED", 8)) {
        return SQL_FILE;
    } else if (!strcasecmp(suffix, "YML")) {
        return YML_FILE;
    } else if (!strcasecmp(suffix, "PB")
               || !strcasecmp(suffix, "JAVA")
               || !strcasecmp(suffix, "JAVA-TMPL")
               || !strcasecmp(suffix, "TMPL")) {
        return JAVA_FILE;
    } else if (!strcasecmp(suffix, "SH") 
               || !strcasecmp(suffix, "SUB")
               || !strcasecmp(suffix, "CMD")
               || !strcasecmp(suffix, "KSH")
               || !strcasecmp(suffix, "BAT")
               || !strncasecmp(suffix, "GO", 2)) {
        return SHELL_FILE;
    } else if (!strcasecmp(suffix, "MK")
               || !strcasecmp(suffix, "GMK")
               || !strcasecmp(suffix, "MAK")
               || !strcasecmp(suffix, "NMAK")
               || !strcasecmp(suffix, "CMAKE")
               || !strcasecmp(suffix, "Makefile") ) {
        return MAKE_FILE;
    } else if (!strcasecmp(suffix, "PY")) {
        return PYTHON_FILE;
    } else if (!strcasecmp(suffix, "JPG")
               || !strcasecmp(suffix, "BMP")
               || !strcasecmp(suffix, "PNG")
               || !strcasecmp(suffix, "ICO")
               || !strcasecmp(suffix, "GIF")) {
        return PIC_FILE;
    } else if (!strcasecmp(suffix, "PDF")) {
        return PDF_FILE;
    } else if (!strcasecmp(suffix, "JAR")
               || !strcasecmp(suffix, "CLASS")) {
        return EXE_FILE;
    } else if (!strcasecmp(suffix, "MD") 
               || !strcasecmp(suffix, "JS")
               || !strcasecmp(suffix, "J2")
               || !strcasecmp(suffix, "PL")
               || !strcasecmp(suffix, "PM")
               || !strcasecmp(suffix, "RB")
               || !strcasecmp(suffix, "UI")
               || !strcasecmp(suffix, "CRC")
               || !strcasecmp(suffix, "DAT")
               || !strcasecmp(suffix, "ENV")
               || !strcasecmp(suffix, "RST")
               || !strcasecmp(suffix, "CSS")
               || !strcasecmp(suffix, "DSP")
               || !strcasecmp(suffix, "DSW")
               || !strcasecmp(suffix, "SDL")
               || !strcasecmp(suffix, "MDL")
               || !strcasecmp(suffix, "INI")
               || !strcasecmp(suffix, "TXT")
               || !strcasecmp(suffix, "XML")
               || !strcasecmp(suffix, "XSL")
               || !strcasecmp(suffix, "JSP")
               || !strcasecmp(suffix, "FMT")
               || !strcasecmp(suffix, "NOPB")
               || !strcasecmp(suffix, "HTML") 
               || !strcasecmp(suffix, "JSON") 
               || !strcasecmp(suffix, "ADOC") 
               || !strcasecmp(suffix, "CONF") 
               || !strcasecmp(suffix, "JAMON") 
               || !strcasecmp(suffix, "PREFS")
               || !strcasecmp(suffix, "PROTO") 
               || !strcasecmp(suffix, "README") 
               || !strcasecmp(suffix, "PARCEL")
               || !strcasecmp(suffix, "CONFIG") 
               || !strcasecmp(suffix, "GITIGNORE")
               || !strcasecmp(suffix, "PROPERTIES")
               || !strncasecmp(suffix, "FILE", 4)
               || !strncasecmp(suffix, "LIC", 3)) {
        return TEXT_FILE;
    }

    if(!strncasecmp(fileName, "POM", 3) 
       || !strncasecmp(fileName, "TEST", 4)
       || !strncasecmp(fileName, "HIVE", 4)
       || !strncasecmp(fileName, "DIFF", 4)) {
        return TEXT_FILE;
    } else if(!strncasecmp(fileName, "SQ", 2)
              || !strncasecmp(fileName, "RUN", 3)
              || !strncasecmp(fileName, "TRAF", 4)) {
        return SHELL_FILE;
    } else if(!strncasecmp(fileName, "CMAKE", 4)
              || !strncasecmp(fileName, "MAKE", 4)){
        return MAKE_FILE;
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

    case TEXT_FILE:
        return (char*)"TEXT";

    case PIC_FILE:
        return (char*)"PICTURE";

    case PDF_FILE:
        return (char*)"PDF";

    case EXE_FILE:
        return (char*)"EXE";

    case EMPTY_FILE:
        return (char*)"EMPTY";

    default:
        break;
    }

    return (char*)"UNKNOWN";
}
