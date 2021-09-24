#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "sparkjs.h"
#include "ui.h"

typedef struct asynchronous_execution_request
{
    char *cmd;
    char *cb;
    struct asynchronous_execution_request *next;
} AER;
