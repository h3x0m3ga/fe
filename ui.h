const char gladeui[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
     <interface>\
     <requires lib =\"gtk+\" version=\"3.8\"/>\
     <requires lib=\"webkit2gtk\" version=\"2.12\"/>\
     <object class=\"WebKitSettings\" id=\"settings1\">\
    <property name=\"enable_offline_web_application_cache\">False</property>\
    <property name=\"enable_html5_local_storage\">False</property>\
    <property name=\"enable_html5_database\">False</property>\
    <property name=\"enable_xss_auditor\">False</property>\
    <property name=\"enable_plugins\">False</property>\
    <property name=\"enable_java\">False</property>\
    <property name=\"javascript_can_open_windows_automatically\">True</property>\
    <property name=\"enable_hyperlink_auditing\">False</property>\
    <property name=\"enable_developer_extras\">True</property>\
    <property name=\"javascript_can_access_clipboard\">True</property>\
    <property name=\"media_playback_allows_inline\">False</property>\
    <property name=\"enable_page_cache\">False</property>\
    <property name=\"user_agent\">Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0 Safari/605.1.15</property>\
    <property name=\"enable_smooth_scrolling\">True</property>\
    <property name=\"enable_accelerated_2d_canvas\">True</property>\
    <property name=\"enable_mediasource\">False</property>\
    <property name=\"allow_file_access_from_file_urls\">True</property>\
    <property name=\"allow_universal_access_from_file_urls\">True</property>\
  </object>\
  <object class=\"GtkApplicationWindow\" id=\"main_window\">\
    <property name=\"can_focus\">False</property>\
    <property name=\"title\" translatable=\"yes\"></property>\
    <property name=\"window_position\">center</property>\
    <property name=\"default_width\">850</property>\
    <property name=\"default_height\">550</property>\
    <property name=\"has_resize_grip\">True</property>\
    <property name=\"show_menubar\">False</property>\
    <signal name=\"destroy\" handler=\"on_quit\" swapped=\"no\"/>\
    <child>\
      <placeholder/>\
    </child>\
    <child>\
      <object class=\"WebKitWebView\" id=\"webkit_webview\">\
        <property name=\"name\">webview</property>\
        <property name=\"visible\">True</property>\
        <property name=\"can_focus\">True</property>\
        <property name=\"settings\">settings1</property>\
        <property name=\"is_ephemeral\">True</property>\
        <signal name=\"close\" handler=\"on_quit\" swapped=\"no\"/>\
        <signal name=\"context-menu\" handler=\"view_context_menu\" swapped=\"no\"/>\
        <signal name=\"script-dialog\" handler=\"dialog_mon\" swapped=\"no\"/>\
        <child>\
          <placeholder/>\
        </child>\
      </object>\
    </child>\
  </object>\
</interface>";
