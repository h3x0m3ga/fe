#include "main.h"

static pthread_mutex_t aerlist_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t js_mtx = PTHREAD_MUTEX_INITIALIZER;

GtkWidget *window;
bool verbose = false;
bool debug = false;
int NUM_OF_THREADS = 10;
size_t BSIZE = 2048;
AER *rootObject;
pthread_t *wrkrthrd;
WebKitWebView *web_view;
WebKitWebInspector *dev;

char *read_file_until_end(FILE *fp)
{
    char *output;
    size_t len = 0, rlen = 0;
    output = (char *)malloc(BSIZE);
    if (!output)
    {
        fprintf(stderr, "\nMemory Allocation Error: read_file_until_end\n");
        return NULL;
    }
    else
    {
        rlen = fread(output + len, 1, BSIZE, fp);
        while (rlen == BSIZE)
        {
            len += BSIZE;
            if (verbose)
            {
                fprintf(stderr, "REALLOC(len: %lu, rlen: %lu)\n", len, BSIZE);
            }
            output = (char *)realloc(output, len + BSIZE);
            if (!output)
            {
                fprintf(stderr, "\nMemory Allocation Error in Thread: %ld\n", pthread_self());
                return NULL;
            }
            rlen = fread(output + len, 1, BSIZE, fp);
        }
        *(output + len + rlen) = 0;
    }
    return output;
}

gboolean save_to_file_mon(WebKitWebView *web_view, WebKitScriptDialog *dialog, gpointer user_data)
{
    const char *uddta;
    char *ptr, *fn, *dta;
    int fnlen, dtalen;
    uddta = webkit_script_dialog_get_message(dialog);
    ptr = strchr(uddta, ' ');
    if (!ptr)
    {
        return 1;
    }
    ptr++;
    if (sscanf(uddta, "%d,%d", &fnlen, &dtalen) != 2)
    {
        fprintf(stderr, "Parsing Error: incorrect arguments: %s", uddta);
        return false;
    }
    fn = (char *)malloc(fnlen + 1);
    dta = (char *)malloc(dtalen + 1);
    if (fn && dta)
    {
        strncpy(fn, ptr, fnlen);
        strncpy(dta, ptr + fnlen, dtalen);
        fn[fnlen] = 0;
        dta[dtalen] = 0;
        FILE *fp = fopen(fn, "w+");
        fputs(dta, fp);
        fclose(fp);
    }
    return TRUE;
}

void on_quit()
{
    pthread_mutex_destroy(&aerlist_mtx);
    pthread_mutex_destroy(&js_mtx);
    gtk_main_quit();
}

void *execute()
{
    while (1)
    {
        pthread_mutex_lock(&aerlist_mtx);
        AER *aer1 = rootObject->next;
        if (aer1)
        {
            rootObject->next = aer1->next;
        }
        pthread_mutex_unlock(&aerlist_mtx);
        if (aer1)
        {
            if (verbose)
            {
                printf("THREAD: %ld\nCOMMAND: %s\nCALLBACK: %s\n", pthread_self(), aer1->cmd, aer1->cb);
            }
        }
        else
        {
            continue;
        }
        FILE *fp;
        char *output;
        int return_value;
        fp = popen(aer1->cmd, "r");
        if (!fp || fp < 0)
        {
            fprintf(stderr, "Error executing '%s' error no: %d\n", aer1->cmd, errno);
            return_value = -1;
            output = NULL;
        }
        else
        {
            output = read_file_until_end(fp);
            return_value = pclose(fp);
        }
        if (strlen(aer1->cb))
        {
            gchar *script;
            script = g_strdup_printf("%s(%d, `%s`);\ndelete %s;", aer1->cb, return_value, output, aer1->cb);
            if (aer1->cb)
            {
                pthread_mutex_lock(&js_mtx);
                if (verbose)
                {
                    fprintf(stdout, "SCRIPT:\n%s\n", script);
                }
                webkit_web_view_run_javascript(web_view, script, NULL, NULL, NULL);
                pthread_mutex_unlock(&js_mtx);
            }
            g_free(script);
        }
        free(output);
        free(aer1->cmd);
        free(aer1->cb);
        free(aer1);
    }
    return 0;
}

gboolean view_context_menu(WebKitWebView *web_view, WebKitContextMenu *context_menu, GdkEvent *event, WebKitHitTestResult *hit_test_result, gpointer user_data)
{
    return TRUE;
}

gboolean execute_mon(WebKitWebView *web_view, WebKitScriptDialog *dialog, gpointer user_data)
{
    const char *uddta;
    char *ptr, *cmd, *cb;
    int cblen, cmdlen;
    uddta = webkit_script_dialog_get_message(dialog);
    ptr = strchr(uddta, ' ');
    if (!ptr)
    {
        return 1;
    }
    ptr++;
    if (sscanf(uddta, "%d,%d", &cmdlen, &cblen) != 2)
    {
        fprintf(stderr, "Parsing Error: incorrect arguments: %s", uddta);
        return false;
    }
    cmd = (char *)malloc(cmdlen + 1);
    cb = (char *)malloc(cblen + 1);
    if (cmd && cb)
    {
        strncpy(cmd, ptr, cmdlen);
        strncpy(cb, ptr + cmdlen, cblen);
        cmd[cmdlen] = 0;
        cb[cblen] = 0;
        AER *req = (AER *)malloc(sizeof(AER));
        req->cmd = cmd;
        req->cb = cb;
        req->next = 0;
        pthread_mutex_lock(&aerlist_mtx);
        AER *tmp = rootObject;
        while (tmp && tmp->next)
        {
            tmp = tmp->next;
        }
        tmp->next = req;
        pthread_mutex_unlock(&aerlist_mtx);
    }
    return true;
}

gboolean gtkreq_mon(WebKitWebView *web_view, WebKitScriptDialog *dialog, gpointer user_data)
{
    char *val;
    const char *param;
    param = webkit_script_dialog_get_message(dialog);
    val = strchr(param, '=');
    *val = 0;
    val++;
    if (!(strlen(val) == 0 || strlen(param) == 0))
    {
        if (verbose)
        {
            fprintf(stdout, "Setting GTK parameter: \"%s\" to %s\n", param, val);
        }
        if(!strcmp("debugger",param)) {
            webkit_web_inspector_show(dev);
            return true;
        }
        if (!strcmp("title", param))
        {
            gtk_window_set_title(GTK_WINDOW(window), val);
            return true;
        }
        if (!strcmp("icon", param))
        {
            if (!gtk_window_set_icon_from_file(GTK_WINDOW(window), val, 0))
            {
                fprintf(stderr, "Error setting icon: %s\n", val);
                return true;
            }
            return true;
        }
        if (!strcmp("maximize", param))
        {
            gtk_window_maximize(GTK_WINDOW(window));
            return true;
        }
        if (!strcmp("unmaximize", param))
        {
            gtk_window_unmaximize(GTK_WINDOW(window));
            return true;
        }
        if (!strcmp("fullscreen", param))
        {
            gtk_window_fullscreen(GTK_WINDOW(window));
            return true;
        }
        if (!strcmp("unfullscreen", param))
        {
            gtk_window_unfullscreen(GTK_WINDOW(window));
            return true;
        }
        if (!strcmp("resizable", param))
        {
            if (!strcmp("true", val))
            {
                gtk_window_set_resizable(GTK_WINDOW(window), true);
                return true;
            }
            gtk_window_set_resizable(GTK_WINDOW(window), false);
            return true;
        }
        if (!strcmp("resize", param))
        {
            char *swidth, *sheight;
            swidth = val;
            sheight = strchr(swidth, '=');
            *sheight = 0;
            sheight++;
            if (strlen(swidth) && strlen(sheight))
            {
                int width, height;
                width = atoi(swidth);
                height = atoi(sheight);
                if (width && height)
                {
                    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
                    gtk_window_resize(GTK_WINDOW(window), width, height);
                }
                else if (verbose)
                {
                    fprintf(stderr, "GTKWindow Resize: bad length values\n");
                }
            }
            else if (verbose)
            {
                fprintf(stderr, "GTKWindow Resize: bad string length\n");
            }
            return true;
        }
    }
    return true;
}

gboolean dialog_mon(WebKitWebView *web_view, WebKitScriptDialog *dialog, gpointer user_data)
{
    switch (webkit_script_dialog_get_dialog_type(dialog))
    {
    case WEBKIT_SCRIPT_DIALOG_ALERT:
        return execute_mon(web_view, dialog, user_data);
        break;
    case WEBKIT_SCRIPT_DIALOG_CONFIRM:
        return gtkreq_mon(web_view, dialog, user_data);
        break;
    case WEBKIT_SCRIPT_DIALOG_PROMPT:
        return save_to_file_mon(web_view, dialog, user_data);
        break;
    case WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM:
        break;
    }
    return true;
}

int main(int argc, char *argv[], char *env[])
{
    int opt;
    while ((opt = getopt(argc, argv, ":dit:v?")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debug = true;
            break;
        case 'i':
            NUM_OF_THREADS = 0;
            break;
        case 't':
            NUM_OF_THREADS = atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            fprintf(stdout, "fe\tDeveloped by Michael Heeren 2021\n-d show debugger\n-i inhibit execution\n-t <count> number of threads\n-v be verbose\n-? show help\n");
            exit(0);
            break;
        }
    }
    wrkrthrd = (pthread_t *)calloc(NUM_OF_THREADS, sizeof(pthread_t));
    rootObject = (AER *)malloc(sizeof(AER));
    memset(rootObject, 0, sizeof(AER));
    char *html = read_file_until_end(stdin);
    GtkBuilder *builder;
    GtkWidget *w_webkit_webview;
    WebKitUserContentManager *manager;
    WebKitUserScript *script;
    gchar *scriptsrc = g_strdup_printf("%s", spark_js);
    script = webkit_user_script_new(
        scriptsrc,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL,
        NULL);
    gtk_init(&argc, &argv);
    webkit_web_view_get_type();
    webkit_settings_get_type();
    builder = gtk_builder_new_from_string((gchar *)gladeui, strlen(gladeui));
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    w_webkit_webview = GTK_WIDGET(gtk_builder_get_object(builder, "webkit_webview"));
    web_view = WEBKIT_WEB_VIEW(w_webkit_webview);
    manager = webkit_web_view_get_user_content_manager(web_view);
    if (!manager)
    {
        exit(1);
    }
    webkit_user_content_manager_add_script(manager, script);
    gtk_builder_connect_signals(builder, w_webkit_webview);
    webkit_web_view_load_html(web_view, html, "file:///");
    dev = webkit_web_view_get_inspector(web_view);
    if (debug)
    {
        webkit_web_inspector_show(dev);
    }
    for (int i = 0; i < NUM_OF_THREADS; i++)
    {

        switch (pthread_create(&wrkrthrd[i], NULL, execute, NULL))
        {
        case EAGAIN:
            fprintf(stderr, "Insufficient resources to create another thread.\n");
            break;
        case EINVAL:
            fprintf(stderr, "Invalid settings in thread attribute.\n");
            break;
        case EPERM:
            fprintf(stderr, "No permission to set the scheduling policy and parameters specified in thread attribute.\n");
            break;
        default:
            if (verbose)
            {
                fprintf(stderr, "Thread Created\n");
            }
            break;
        }
    }
    gtk_widget_show_all(window);
    gtk_main();
    g_object_unref(builder);
    webkit_user_script_unref(script);
    return 0;
}
