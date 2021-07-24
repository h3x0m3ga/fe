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
} async_exec;

typedef struct
{
    GtkWidget *w_webkit_webview;
} app_widgets;

void on_quit()
{
    gtk_main_quit();
}

void *execute(void *aer)
{
    FILE *fp;
    char *rcvd, *output, tmp[BSIZE];
    int len = 0;
    fp = popen(((async_exec *)aer)->cmd, "r");
    if (!fp || fp < 0)
    {
        fprintf(stderr, "Error executing '%s' error no: %d\n", ((async_exec *)aer)->cmd, errno);
    }
    else
    {
        output = (char *)malloc(1);
        *output = 0;
        rcvd = fgets(tmp, BSIZE, fp);
        while (rcvd)
        {
            int rlen = strlen(rcvd);
            output = (char *)realloc(output, len + rlen + 1);
            output[len + rlen] = 0;
            if (output)
            {
                strcat(output, rcvd);
                len += rlen;
            }
            else
            {
                fprintf(stderr, "Memory Allocation Error...\n");
            }
            rcvd = fgets(tmp, BSIZE, fp);
        }
        pclose(fp);
        if (strlen(((async_exec *)aer)->cb) > 1)
        {
            gchar *script;
            script = g_strdup_printf("%s(`%s`)", ((async_exec *)aer)->cb, (output) ? output : "");
            webkit_web_view_run_javascript(((async_exec *)aer)->web_view, script, NULL, NULL, NULL);
            g_free(script);
        }
        free(output);
        free(((async_exec *)aer)->cb);
        free(((async_exec *)aer)->cmd);
        free(((async_exec *)aer));
    }
    return 0;
}

gboolean view_context_menu(WebKitWebView *web_view, WebKitContextMenu *context_menu, GdkEvent *event, WebKitHitTestResult *hit_test_result, gpointer user_data)
{
    return TRUE;
}

gboolean execute_mon(WebKitWebView *web_view, WebKitScriptDialog *dialog, gpointer user_data)
{
    const char *cb;
    char *cmd;
    cb = webkit_script_dialog_get_message(dialog);
    cmd = strchr(cb, ' ');
    *cmd = 0;
    cmd++;
    async_exec *req = (async_exec *)malloc(sizeof(async_exec));
    if (req == 0)
        return false;
    req->cb = (char *)malloc(strlen(cb) + 1);
    if (req->cb == 0)
        return false;
    strcpy(req->cb, cb);
    req->cmd = (char *)malloc(strlen(cmd) + 1);
    if (req->cmd == 0)
        return false;
    strcpy(req->cmd, cmd);
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
        if(!strcmp("fullscreen", param))
        {
            gtk_window_fullscreen (GTK_WINDOW(window));
            return true;
        }
        if(!strcmp("unfullscreen", param))
        {
            gtk_window_unfullscreen (GTK_WINDOW(window));
            return true;
        }
        if(!strcmp("resizable", param))
        {
            if(!strcmp("true", val)) {
                gtk_window_set_resizable(GTK_WINDOW(window),true);
                return true;
            } 
            gtk_window_set_resizable(GTK_WINDOW(window),false);
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
                    gtk_window_resize(GTK_WINDOW(window), width, height);
                }
            }
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
    case WEBKIT_SCRIPT_DIALOG_CONFIRM:
        return gtkreq_mon(web_view, dialog, user_data);
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
    FILE *fp;
    char *filename = 0;
    while ((opt = getopt(argc, argv, ":div?f:")) != -1)
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
            fprintf(stdout, "FrontEnd\tDeveloped by Michael Heeren 2021\n-d show debugger\n-i inhibit execution\n-v be verbose\n-f <filename>\n-? show help\n");
            exit(0);
            break;
        case 'f':
            filename = optarg;
            break;
        }
    }
    if (filename)
    {
        fp = fopen(filename, "r");
        if (!fp)
        {
            fprintf(stderr, "Error: File \"%s\" Does Not Exist!\n", filename);
            exit(1);
        }
    }
    else
    {
        fp = stdin;
    }
    char *rcvd, *html, tmp[BSIZE];
    int len = 0;
    html = (char *)malloc(1);
    *html = 0;
    rcvd = fgets(tmp, BSIZE, fp);
    while (rcvd)
    {
        int rlen = strlen(rcvd);
        html = (char *)realloc(html, len + rlen + 1);
        html[len + rlen] = 0;
        if (html)
        {
            strcat(html, rcvd);
            len += rlen;
        }
        else
        {
            fprintf(stderr, "Memory Allocation Error...\n");
            exit(1);
        }
        rcvd = fgets(tmp, BSIZE, fp);
    }
    fclose(fp);
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
    webkit_web_view_load_html(WEBKIT_WEB_VIEW(widgets->w_webkit_webview), html, "/");
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
