#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <gtk/gtk.h>
#include <webkit2/webkit2.h>
#include "sparkjs.h"
#include "ui.h"


GtkWidget *window;
bool inhibited = false;
bool verbose = false;
bool debug = false;
size_t BSIZE = 2048;

typedef struct
{
    char *cmd;
    char *cb;
    WebKitWebView *web_view;
} asynchronous_execution_request;

typedef struct
{
    GtkWidget *w_webkit_webview;
} app_widgets;


char *read_file_until_end(FILE *fp)
{
    char *rcvd, *output, tmp[BSIZE];
    size_t len = 1, rlen = 0;
    output = (char *)malloc(1);
    *output = 0;
    if (!output)
    {
        if (verbose)
        {
            fprintf(stderr, "\nMemory Allocation Error: read_until_end\n");
        }
        exit(1);
    }
    rcvd = fgets(tmp, BSIZE, fp);
    while (rcvd)
    {
        rlen = strlen(rcvd) + 1;
        if(verbose) {
            fprintf(stderr, "\nREALLOC:\nlen: %lu, rlen: %lu\n", len, rlen);
        }
        output = (char *)realloc(output, len + rlen);
        if (!output)
        {
            fprintf(stderr, "\nMemory Allocation Error: read_until_end\n");
            exit(1);
        }
        strcat(output, rcvd);
        len += rlen;
        rcvd = fgets(tmp, BSIZE, fp);
    }
    return output;
}

void on_quit()
{
    gtk_main_quit();
}

void *execute(void *asynch_exec_req)
{
    FILE *fp;
    char *output;
    unsigned int return_value;
    fp = popen(((asynchronous_execution_request *)asynch_exec_req)->cmd, "r");
    if (!fp || fp < 0)
    {
        fprintf(stderr, "Error executing '%s' error no: %d\n", ((asynchronous_execution_request *)asynch_exec_req)->cmd, errno);
    }
    else
    {
        output = read_file_until_end(fp);
        return_value = pclose(fp);
        if (strlen(((asynchronous_execution_request *)asynch_exec_req)->cb) > 1)
        {
            gchar *script;
            script = g_strdup_printf("%s(%d, `%s`);\ndelete %s;", ((asynchronous_execution_request *)asynch_exec_req)->cb, return_value, output, ((asynchronous_execution_request *)asynch_exec_req)->cb);
            if (verbose)
            {
                fprintf(stderr,"SCRIPT:\n%s\n", script);
            }
            webkit_web_view_run_javascript(((asynchronous_execution_request *)asynch_exec_req)->web_view, script, NULL, NULL, NULL);
            g_free(script);
        }
        free(output);
        free(((asynchronous_execution_request *)asynch_exec_req)->cb);
        free(((asynchronous_execution_request *)asynch_exec_req)->cmd);
        free(((asynchronous_execution_request *)asynch_exec_req));
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
    char *ptr;
    int cblen, cmdlen;
    uddta = webkit_script_dialog_get_message(dialog);
    ptr = strchr(uddta,' ');
    if(!ptr) {
        return 1;
    }
    ptr++;
    if(sscanf(uddta, "%u,%u", &cmdlen, &cblen) != 2) {
        if(verbose) {
            fprintf(stderr, "Parsing Error: incorrect arguments: %s", uddta);
        }
        return true;
    }
    asynchronous_execution_request *req = (asynchronous_execution_request *)malloc(sizeof(asynchronous_execution_request));
    if (req == 0)
        return true;
    req->cmd = (char *)malloc(cmdlen + 1);
    if (req->cmd == 0)
    {
        return true;
    }
    req->cmd[cmdlen] = 0;
    strncpy(req->cmd, ptr, cmdlen);

    req->cb = (char *)malloc(cblen + 1);
    if (req->cb == 0)
    {
        return true;
    }
    req->cb[cblen] = 0;
    strncpy(req->cb, ptr + cmdlen, cblen);
    req->web_view = web_view;
    if (verbose)
    {
        fprintf(stdout, "COMMAND: %s\nCALLBACK: %s\nTHREAD ID: %ld\n", req->cmd, req->cb, pthread_self());
    }
    if (!inhibited)
    {
        pthread_t thread;
        pthread_create(&thread, NULL, execute, (void *)req);
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
        break;
    case WEBKIT_SCRIPT_DIALOG_BEFORE_UNLOAD_CONFIRM:
        break;
    }
    return true;
}

int main(int argc, char *argv[], char *env[])
{

    int opt;
    while ((opt = getopt(argc, argv, ":div?")) != -1)
    {
        switch (opt)
        {
        case 'd':
            debug = true;
            break;
        case 'i':
            inhibited = true;
            break;
        case 'v':
            verbose = true;
            break;
        case '?':
            fprintf(stdout, "FrontEnd\tDeveloped by Michael Heeren 2021\n-d show debugger\n-i inhibit execution\n-v be verbose\n-? show help\n");
            exit(0);
            break;
        }
    }
    char *html = read_file_until_end(stdin);
    GtkBuilder *builder;
    WebKitUserContentManager *manager;
    WebKitWebInspector *dev;
    WebKitUserScript *script;
    gchar *scriptsrc = g_strdup_printf("%s", spark_js);
    script = webkit_user_script_new(
        scriptsrc,
        WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
        WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
        NULL,
        NULL);
    app_widgets *widgets = g_slice_new(app_widgets);
    gtk_init(&argc, &argv);
    webkit_web_view_get_type();
    webkit_settings_get_type();
    builder = gtk_builder_new_from_string((gchar *)gladeui, strlen(gladeui));
    window = GTK_WIDGET(gtk_builder_get_object(builder, "main_window"));
    widgets->w_webkit_webview = GTK_WIDGET(gtk_builder_get_object(builder, "webkit_webview"));
    manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(widgets->w_webkit_webview));
    if (!manager)
    {
        exit(1);
    }
    webkit_user_content_manager_add_script(manager, script);
    gtk_builder_connect_signals(builder, widgets);
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(widgets->w_webkit_webview), html, "file:///");
    if (debug)
    {
        dev = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(widgets->w_webkit_webview));
        webkit_web_inspector_show(dev);
    }
    gtk_widget_show_all(window);
    gtk_main();
    g_object_unref(builder);
    webkit_user_script_unref(script);
    g_slice_free(app_widgets, widgets);
    return 0;
}
