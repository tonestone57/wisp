Common Wisp user options
---------------------------

This document outlines the common configuration options supported by
  the Wisp core.

Overview
========

The users configurations are generally stored in a "Choices" file and
  are loaded at browser startup. Most user interfaces provide a way
  to configure these parameters in a manner consistant with the
  toolkit in use.

The user choices are stored as a simple key:value list.

Each entry has a type, one of: boolean, integer, unsigned integer,
  hexadecimal colour value or string.

General Options
===============

 Option Key           | Type   | Default   | Description                      
 -------------------- | ------ | --------- | -------------------------------- 
 http_proxy           | bool   | false     | An HTTP proxy should be used.    
 http_proxy_host      | string | NULL      | Hostname of proxy.               
 http_proxy_port      | int    | 8080      | Proxy port.                      
 http_proxy_auth      | int    | 0         | Proxy authentication method.     
 http_proxy_auth_user | string | NULL      | Proxy authentication user name   
 http_proxy_auth_pass | string | NULL      | Proxy authentication password    
 http_proxy_noproxy   | string | localhost | Proxy omission list              
 font_size            | int    | 128       | Default font size / 0.1pt.       
 font_min_size        | int    | 85        | Minimum font size.               
 font_sans            | string | NULL      | Default sans serif font          
 font_serif           | string | NULL      | Default serif font               
 font_mono            | string |  NULL     | Default monospace font           
 font_cursive         | string |  NULL     | Default cursive font             
 font_fantasy         | string |  NULL     | Default fantasy font             
 accept_language      | string |  NULL     | Accept-Language header.          
 accept_charset       | string |  NULL     | Accept-Charset header.           
 memory_cache_size    | int    | 12MiB     | Preferred maximum size of memory cache in bytes. 
 disc_cache_size      | uint   | 1GiB      | Preferred expiry size of disc cache in bytes. 
 disc_cache_age       | int    | 28        | Preferred expiry age of disc cache in days. 
 disc_cache_path      | string |  NULL     | Path to disc cache, NULL means to use system path |
 block_advertisements | bool   | false     | Whether to block advertisements  
 disable_popups       | bool   | false     | Whether to block popups
 do_not_track         | bool   | false     | Disable website tracking [1]     
 send_referer         | bool   | true      | Whether to send the referer HTTP header.
 foreground_images    | bool   | true      | Whether to fetch foreground images 
 background_images    | bool   | true      | Whether to fetch background images 
 animate_images       | bool   | true      | Whether to animate images        
 enable_javascript    | bool   | true      | Whether to execute javascript
 script_timeout       | int    | 10        | Maximum time to wait for a script to run in seconds 
 expire_url           | int    | 28        | How many days to retain URL data for. 
 font_default         | int    | 0         | Default font family              
 ca_bundle            | string | NULL      | ca-bundle location               
 ca_path              | string | NULL      | ca-path location                 
 cookie_file          | string | NULL      | Cookie file location             
 cookie_jar           | string | NULL      | Cookie jar location              
 homepage_url         | string | NULL      | Home page location               
 search_url_bar       | bool   | true      | search web from url bar
 search_web_provider  | string | DuckDuckGo| default web search provider
 url_suggestion       | bool   | true      | URL completion in url bar        
 window_x             | int    | 0         | default x position of new windows 
 window_y             | int    | 0         | default y position of new windows 
 window_width         | int    | 0         | default width of new windows     
 window_height        | int    | 0         | default height of new windows    
 toolbar_status_size  | int    | 6667      | default size of status bar vs. h scroll bar 
 scale                | int    | 100       | default window scale             
 min_reflow_period    | uint   | 25        | Minimum time (in cs) between HTML reflows while objects are fetching 
 core_select_menu     | bool   | false     | Use core selection menu          
 display_decoded_idn  | bool   | false     | Display decoded IDN hostnames
 prefer_dark_mode     | bool   | false     | Prefer dark mode system colors
 log_filter           | string | (WISP_BUILTIN_LOG_FILTER) | Filter for non-verbose logging
 verbose_filter       | string | (WISP_BUILTIN_VERBOSE_FILTER) | Filter for verbose logging

[1] http://www.w3.org/Submission/2011/SUBM-web-tracking-protection-20110224/#dnt-uas

Fetcher options
===============

 Option Key               | Type | Default | Description                         
 ------------------------ | -----| ------- | ----------------------------------- 
 max_fetchers             | int  | 256     | Maximum simultaneous active fetchers
 max_fetchers_per_host    | int  | 64      | Maximum simultaneous active fetchers per host. (<=option_max_fetchers else it makes no sense) [2]
 max_cached_fetch_handles | int  |  6      | Maximum number of inactive fetchers cached. The total number of handles wisp will therefore have open is this plus option_max_fetchers. 
 suppress_curl_debug      | bool | true    | Suppress debug output from cURL.    
 target_blank             | bool | true    | Whether to allow target="_blank"    
 button_2_tab             | bool | true    | Whether second mouse button opens in new tab. 

[2] Note that rfc2616 section 8.1.4 says that there should be no more
     than two keepalive connections per host. None of the main browsers
     follow this as it slows page fetches down considerably.
     See https://bugzilla.mozilla.org/show_bug.cgi?id=423377#c4


PDF / Print options
===================

 Option Key             | Type | Deflt | Description                         
 ---------------------- | ---- | ----- | ------------------------------------
 margin_top             | int  | 10    | top margin of exported page          
 margin_bottom          | int  | 10    | bottom margin of exported page       
 margin_left            | int  | 10    | left margin of exported page         
 margin_right           | int  | 10    | right margin of exported page        
 export_scale           | int  | 70    | scale of exported content            
 suppress_images        | bool | false | suppressing images in printed content
 remove_backgrounds     | bool | false | turning off all backgrounds for printed content 
 enable_loosening       | bool | true  | turning on content loosening for printed content 
 enable_PDF_compression | bool | true  | compression of PDF documents         
 enable_PDF_password    | bool | false | setting a password and encoding PDF documents 

System colours
==============

These are the css system colours which the browser also uses to style
generated output.

 Option Key                     | Type   | Default   
 ------------------------------ | ------ | ----------
 sys_colour_AccentColor         | colour | 0x00666666
 sys_colour_AccentColorText     | colour | 0x00ffffff
 sys_colour_ActiveText          | colour | 0x000000ee
 sys_colour_ButtonBorder        | colour | 0x004e4e4e
 sys_colour_ButtonFace          | colour | 0x00f9f9f9
 sys_colour_ButtonText          | colour | 0x004c4c4c
 sys_colour_Canvas              | colour | 0x00f1f1f1
 sys_colour_CanvasText          | colour | 0x00000000
 sys_colour_Field               | colour | 0x00f1f1f1
 sys_colour_FieldText           | colour | 0x00000000
 sys_colour_GrayText            | colour | 0x00a6a6a6
 sys_colour_Highlight           | colour | 0x00c00800
 sys_colour_HighlightText       | colour | 0x00ffffff
 sys_colour_LinkText            | colour | 0x00ee0000
 sys_colour_Mark                | colour | 0x0000ffff
 sys_colour_MarkText            | colour | 0x00000000
 sys_colour_SelectedItem        | colour | 0x00e48435
 sys_colour_SelectedItemText    | colour | 0x00ffffff
 sys_colour_VisitedText         | colour | 0x008b1a55
