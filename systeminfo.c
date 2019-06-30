/*
 * systeminfo.c: A VDR plugin to display various system informations in the OSD
 *
 * Copyright (C) 2008 Christoph Haubrich
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 * Or, point your browser to http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 * See the README file how to reach the author.
 *
 * $Id$
 */

#include <getopt.h>
#include <vdr/plugin.h>
#include "displayinfo.h"

static const char *VERSION        = "0.1.5";
static const char *DESCRIPTION    = trNOOP("Display various system informations");
static const char *MAINMENUENTRY  = trNOOP("System Information");


// --- cMenuSetupSysteminfo ----------------------------------------------------

class cMenuSetupSysteminfo : public cMenuSetupPage {
private:
   int newRefreshIntervall;
   int newAutoClose;
protected:
   virtual void Store(void);
public:
   cMenuSetupSysteminfo(void);
};

cMenuSetupSysteminfo::cMenuSetupSysteminfo(void)
{
   newRefreshIntervall = RefreshIntervall;
   newAutoClose = AutoClose;
   Add(new cMenuEditIntItem(tr("Refresh intervall (s)"), &newRefreshIntervall, 1, 20));
   Add(new cMenuEditBoolItem(tr("Close display after 2 min."), &newAutoClose));
}

void cMenuSetupSysteminfo::Store(void)
{
   SetupStore("RefreshIntervall",   RefreshIntervall = newRefreshIntervall);
   SetupStore("AutoClose",          AutoClose = newAutoClose);
}

// --- cPluginSysteminfo -------------------------------------------------------

class cPluginSysteminfo : public cPlugin {
private:
   // Add any member variables or functions you may need here.
   char scriptname[MaxFileName];
public:
   cPluginSysteminfo(void);
   virtual ~cPluginSysteminfo();
   virtual const char *Version(void) { return VERSION; }
   virtual const char *Description(void) { return tr(DESCRIPTION); }
   virtual const char *CommandLineHelp(void);
   virtual bool ProcessArgs(int argc, char *argv[]);
   virtual bool Initialize(void);
   virtual bool Start(void);
   //virtual void Stop(void);
   //virtual void Housekeeping(void);
   //virtual void MainThreadHook(void);
   virtual cString Active(void);
   virtual time_t WakeupTime(void);
   virtual const char *MainMenuEntry(void) { return tr(MAINMENUENTRY); }
   virtual cOsdObject *MainMenuAction(void);
   virtual cMenuSetupPage *SetupMenu(void);
   virtual bool SetupParse(const char *Name, const char *Value);
   virtual bool Service(const char *Id, void *Data = NULL);
   virtual const char **SVDRPHelpPages(void);
   virtual cString SVDRPCommand(const char *Command, const char *Option, int &ReplyCode);
};

cPluginSysteminfo::cPluginSysteminfo(void)
{
   // Initialize any member variables here.
   // DON'T DO ANYTHING ELSE THAT MAY HAVE SIDE EFFECTS, REQUIRE GLOBAL
   // VDR OBJECTS TO EXIST OR PRODUCE ANY OUTPUT!
   strncpy(scriptname, "/usr/local/bin/systeminfo.sh", sizeof(scriptname));
}

cPluginSysteminfo::~cPluginSysteminfo()
{
   // Clean up after yourself!
}

const char *cPluginSysteminfo::CommandLineHelp(void)
{
   // Return a string that describes all known command line options.
   return "  -s SCRIPT, --script=SCRIPT   optional name and path of system info script\n"
          "                               (default: '/usr/local/bin/systeminfo.sh')\n";
}

bool cPluginSysteminfo::ProcessArgs(int argc, char *argv[])
{
   // Implement command line argument processing here if applicable.
   static struct option long_options[] = {
       { "script", required_argument, NULL, 's' },
       { NULL }
   };
   int c, option_index = 0;
   while ((c = getopt_long(argc, argv, "s:", long_options, &option_index)) != -1) {
      switch (c) {
        case 's': strncpy(scriptname, optarg, sizeof(scriptname));
                  isyslog("systeminfo: using systeminfo script: '%s'", scriptname);
                  break;
        default:
                  isyslog("systeminfo: unknown command-line argument: '%s'", optarg);
                  break;
      }
   }
   return true;
}

bool cPluginSysteminfo::Initialize(void)
{
   // Initialize any background activities the plugin shall perform.
   return true;
}

bool cPluginSysteminfo::Start(void)
{
   // Start any background activities the plugin shall perform.
#if APIVERSNUM < 10507
   RegisterI18n(Phrases);
#endif

   return true;
}

/*
void cPluginSysteminfo::Stop(void)
{
   // Stop any background activities the plugin is performing.
}

void cPluginSysteminfo::Housekeeping(void)
{
   // Perform any cleanup or other regular tasks.
}

void cPluginSysteminfo::MainThreadHook(void)
{
   // Perform actions in the context of the main program thread.
   // WARNING: Use with great care - see PLUGINS.html!
}
*/

cString cPluginSysteminfo::Active(void)
{
   // Return a message string if shutdown should be postponed
   return NULL;
}

time_t cPluginSysteminfo::WakeupTime(void)
{
   // Return custom wakeup time for shutdown script
   return 0;
}

cOsdObject *cPluginSysteminfo::MainMenuAction(void)
{
   // Perform the action when selected from the main VDR menu.
   return new cMenuSystemInfo(scriptname);
}

cMenuSetupPage *cPluginSysteminfo::SetupMenu(void)
{
   // Return a setup menu in case the plugin supports one.
   return new cMenuSetupSysteminfo;
}

bool cPluginSysteminfo::SetupParse(const char *Name, const char *Value)
{
   // Parse your own setup parameters and store their values.
   if      (!strcasecmp(Name, "RefreshIntervall"))   RefreshIntervall = atoi(Value);
   else if (!strcasecmp(Name, "AutoClose"))          AutoClose = atoi(Value);
   else
      return false;
   return true;

}

bool cPluginSysteminfo::Service(const char *Id, void *Data)
{
   // Handle custom service requests from other plugins
   return false;
}

const char **cPluginSysteminfo::SVDRPHelpPages(void)
{
   // Return help text for SVDRP commands this plugin implements
   return NULL;
}

cString cPluginSysteminfo::SVDRPCommand(const char *Command, const char *Option, int &ReplyCode)
{
   // Process SVDRP commands this plugin implements
   return NULL;
}

VDRPLUGINCREATOR(cPluginSysteminfo); // Don't touch this!
