const char spark_js[] = "(function () {\
     let execstr = alert;\
     let gtksetstr = confirm;\
     alert = prompt = confirm = undefined;\
     window.bcblist = {};\
     window.execute = function execute(cmd, cb) {\
      if(typeof cb == \"function\")\
      {\
         let uid = Date.now().toString(36) + Math.random().toString(36).substr(2);\
         let bvars = [].slice.call(arguments, 2, arguments.length);\
         let vname = `bcblist[\"${uid}\"]`;\
         bcblist[uid] = cb.bind(null, ...bvars);\
         execstr(`${cmd.length},${vname.length} ${cmd}${vname}`);\
      }\
      return true;\
     };\
     window.gtk_window_set_title=function set_title(value) {\
        return gtksetstr(`title=${value}`);\
     };\
     window.gtk_window_set_size=function set_size(width, height) {\
        return gtksetstr(`resize=${width}=${height}`);\
     };\
     window.gtk_window_maximize=function maximize() {\
        return gtksetstr(`maximize=true`);\
     };\
     window.gtk_window_fullscreen=function fullscreen() {\
        return gtksetstr(`fullscreen=true`);\
     };\
     window.gtk_window_unfullscreen=function unfullscreen() {\
        return gtksetstr(`unfullscreen=true`);\
     };\
     window.gtk_window_set_resizable=function set_resizable(bool) {\
        return gtksetstr(`resizable=${bool}`);\
     };\
     window.gtk_window_unmaximize=function unmaximize() {\
        return gtksetstr(`unmaximize=true`);\
     };\
     window.gtk_window_set_icon_from_file=function set_Icon_From_File(value) {\
        return gtksetstr(`icon=${value}`);\
     };\
     })();";
     
