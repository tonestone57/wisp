/*
 * Copyright 2008 François Revol <mmu_man@users.sourceforge.net>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define __STDBOOL_H__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "utils/log.h"
#include "utils/useragent.h"
#include "wisp/clipboard.h"
#include "curl/curlver.h"
#include "desktop/version.h"
#include "content/fetchers/about/atestament.h"
}
#include "beos/about.h"
#include "beos/scaffolding.h"
#include "beos/window.h"

#include <private/interface/AboutWindow.h>
#include <Application.h>
#include <Catalog.h>
#include <Invoker.h>
#include <String.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "About"

/**
 * Creates the about alert
 */
void nsbeos_about(struct gui_window *gui)
{
    BString text;
    text << B_TRANSLATE("Wisp  : ") << user_agent_string() << "\n";
    text << B_TRANSLATE("Version  : ") << wisp_version << "\n";
    text << B_TRANSLATE("Build ID : ") << WT_REVID << "\n";
    text << B_TRANSLATE("Date     : ") << WT_COMPILEDATE << "\n";
    text << B_TRANSLATE("cURL     : ") << LIBCURL_VERSION << "\n";

    BAboutWindow *alert = new BAboutWindow(B_TRANSLATE("About Wisp"), "application/x-vnd.Wisp");
    alert->AddExtraInfo(text);
    alert->Show();
}
