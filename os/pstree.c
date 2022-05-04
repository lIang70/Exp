#include <sys/stat.h>

#include <errno.h>
#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#define DEBUG 5
#define DEBUG_CALL(level, format, ...) if(level > DEBUG) { printf(format, __VA_ARGS__); fflush(stdout); }

#define LEVEL0  ""
#define LEVEL1  " "
#define LEVEL2  "  "
#define LEVEL3  "   "
#define LEVEL4  "    "
#define LEVEL5  "     "
#define LEVEL6  "      "
#define LEVEL7  "       "
#define LEVEL8  "        "
#define LEVEL9  "         "
#define LEVEL10 "          "
#define LEVEL11 "           "
#define LEVEL12 "            "
#define PRINTNODE(level, proc) {\
    switch (level) { \
    case 0:  printf(LEVEL0);  break; \
    case 1:  printf(LEVEL1);  break; \
    case 2:  printf(LEVEL2);  break; \
    case 3:  printf(LEVEL3);  break; \
    case 4:  printf(LEVEL4);  break; \
    case 5:  printf(LEVEL5);  break; \
    case 6:  printf(LEVEL6);  break; \
    case 7:  printf(LEVEL7);  break; \
    case 8:  printf(LEVEL8);  break; \
    case 9:  printf(LEVEL9);  break; \
    case 10: printf(LEVEL10); break; \
    case 11: printf(LEVEL11); break; \
    case 12: printf(LEVEL12); break; \
    };\
    printf("|- %s[%d] %c\n", proc->name, proc->pid, proc->status);\
    fflush(stdout);\
}

/**
 * @brief Custom Type
 */
typedef int BOOL;
typedef long LONG;
typedef unsigned long ULONG;

typedef char CHAR;
typedef CHAR FILEPATH[256];
typedef CHAR NAME[64];

/**
 * @brief The structure of the process snapshot.
 */
typedef struct ProcSnapshot {
    NAME    name;
    CHAR    status;
    ULONG   pid;
    ULONG   ppid;
    struct ProcSnapshot* parent;

    /*> child of proc*/
    struct ProcSnapshot** childs;
    LONG    childIndex;
    LONG    childCap;
} ProcSnapshot;

/**
 * @brief Check if the string is a valid number.
 * 
 * @param str 
 * @return BOOL 
 */
BOOL isValidNumber(CHAR str[]) {
    LONG i = 0;
    while (str[i++] != '\0') {
        if (str[i-1] > '9' || 
            str[i-1] < '0')
            return (0);
    }
    return (1);
}

/**
 * @brief Determine whether the array space needs to be expanded.
 * 
 * @param index 
 * @param cap 
 * @return ProcSnapshot** 
 */
LONG isNeedExpand(ProcSnapshot ***array, LONG index, LONG *cap) {

    ProcSnapshot **_array;  // A temporary array to open up space

    if (index <= *cap && *cap != 0)
        return (0);
    
    if (*cap == 0) {
        *cap = 8;
    } else {
        *cap *= 2;
    }

    _array = (ProcSnapshot**)realloc(*array, sizeof(ProcSnapshot*) * (*cap));
    
    if (_array != NULL) 
        *array = _array;

    return (1);
}

/**
 * @brief Get a linked list of process snapshots.
 * 
 * @return ProcSnapshot* 
 */
ProcSnapshot *getSnapshot() {

    DIR *procDir;               // open proc dir 
    struct dirent *entry;       // all dirs of proc
    ProcSnapshot *proc = NULL;  // the proc info pointer
    ProcSnapshot **array = NULL;// the array of proc info pointer
    LONG j;                     // the index of loop
    LONG index = -1, cap = 0;   // index & capacity of proc array
    ULONG pid = -1, ppid = -1;  // proc id & parent proc id
    CHAR status;                // status of proc
    NAME name;                  // name of proc

    procDir = opendir("/proc");
    if (procDir == NULL) {
        printf("Open dir /proc error, detail: \n    %s\n", strerror(errno));
        return (proc);
    }

    while ((entry = readdir(procDir))) {
        if (!(isValidNumber(entry->d_name))) {
            continue;
        }
        FILEPATH filepath;
        sprintf(filepath, "/proc/%s/stat", entry->d_name);
        DEBUG_CALL(1, "%s\n", filepath)

        FILE *fp = fopen(filepath, "r");
        if (fp == NULL) {
            printf("Open %s error, detail: \n    %s\n", filepath, strerror(errno));
            continue;
        }

        fscanf(fp, "%lu %s %c %lu", &pid, &name, &status, &ppid);
        DEBUG_CALL(2, "%lu %s %c %lu\n", pid, name, status, ppid)
        
        index++;
        if (isNeedExpand(&array, index, &cap)) {
            DEBUG_CALL(2, "%s\n", "Arrays open up new space.")
        }

        array[index] = (ProcSnapshot*)malloc(sizeof(ProcSnapshot));
        array[index]->pid = pid;
        array[index]->ppid = ppid;
        array[index]->status = status;
        array[index]->childIndex = -1;
        memcpy(array[index]->name, name, strlen(name) * sizeof(char));

        if (ppid != 0) {
            for (j = index - 1; j >= 0; j--) {
                if (array[j]->pid == ppid) {
                    array[index]->parent = array[j];
                    isNeedExpand(&array[j]->childs, ++array[j]->childIndex, &array[j]->childCap);
                    array[j]->childs[array[j]->childIndex] = array[index];
                    break;
                }
            }
        }

        fclose(fp);
    }

    proc = array[0];

    return (proc);
}

/**
 * @brief print the tree of proc
 * 
 * @param proc 
 */
void printPTree(LONG level, ProcSnapshot *proc) {
    LONG i;  // the index of loop

    PRINTNODE(level, proc)
    
    for (i = 0; i <= proc->childIndex; i++) {
        if (proc->childs[i])
            printPTree(level+1, proc->childs[i]);
    }
}

/**
 * @brief main test of pstree
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[]) {

    ProcSnapshot *procSnapshot; // The start of proc snapshot.
    
    // Obtain system permissions.
    umask(000);  

    procSnapshot = getSnapshot();
    printPTree(0, procSnapshot);

    return (0);
}

