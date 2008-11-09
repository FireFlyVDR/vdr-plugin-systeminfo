/*
 * displayinfo.c: display handling
 *
 * See the main source file 'systeminfo.c' for copyright information and
 * how to reach the author.
 *
 * $Id$
 */

#include <vdr/osd.h>
#include <vdr/tools.h>
#include "displayinfo.h"

#define MAX_LINES 50

int RefreshIntervall = 5;
int AutoClose = 1;

// --- cMenuSystemInfo ----------------------------------------------------

cMenuSystemInfo::cMenuSystemInfo(const char *script)
:cOsdMenu(tr("System Information"), 14, 18)
{
   Add(new cOsdItem(tr("please wait"), osUnknown, false));
   SetHelp(NULL, NULL, NULL, NULL);

   InfoLines = new cInfoLines(script);
   InfoLines->StateChanged(infolinesState);
}


cMenuSystemInfo::~cMenuSystemInfo()
{
   delete InfoLines;
}


eOSState cMenuSystemInfo::ProcessKey(eKeys Key)
{
   eOSState state = cOsdMenu::ProcessKey(Key);

   if (state == osUnknown) {
      switch (Key) {
         case kBack:    return osEnd;
         case kNone:    if (InfoLines->StateChanged(infolinesState))
                           Set();
                        break;
         default:       break;
      }
      if (!AutoClose)
         state = osContinue;
   }
   return state;
}


void cMenuSystemInfo::Set()
{
   int current = Current();
   cThreadLock InfoLinesLock(InfoLines);

   if (InfoLines->FirstDisplay())
   {
      Clear();
      for (cInfoLine *line = InfoLines->First(); line; line = InfoLines->Next(line))
      {
         line->SetOsdItem(new cOsdItem((const char *)line->GetStr(), osUnknown, true));
         Add(line->GetOsdItem());
      }
      SetCurrent(Get(current));
      InfoLines->SetOsdInitialized();
      Display();
   }
   else
   {
      for (cInfoLine *line = InfoLines->First(); line; line = InfoLines->Next(line))
      {
         line->GetOsdItem()->SetText((const char *)line->GetStr());
      }
      Display();
   }
}


// --- cMenuSetupSysteminfo ----------------------------------------------------

cInfoLines::cInfoLines(const char *script)
:cThread("systeminfo")
{
   state = 0;
   firstDisplay = true;
   OsdInitialized = false;
   strncpy(scriptname, script, sizeof(scriptname));
   Start();
}


cInfoLines::~cInfoLines()
{
   if (Running()) {
      Wait.Signal();
      Cancel(RefreshIntervall);
   }
}


bool cInfoLines::StateChanged(int &State)
{
   int NewState = state;
   bool Result = State != NewState;
   State = state;

   return Result;
}


float cInfoLines::GetCpuPct(void)
{
   float CpuPct = 0.0;
   FILE *statfile;
   int found = 0;

   statfile = fopen("/proc/stat", "r");
   if(!statfile) {
      isyslog("systeminfo: error opening /proc/stat: %s", strerror(errno));
   }
   else {
      rewind(statfile);
      fflush(statfile);
      found = fscanf(statfile, "cpu %lld %lld %lld %lld", &ticks[0], &ticks[1], &ticks[2], &ticks[3]);
      fclose(statfile);

      if ( found == 4) {
         unsigned long long luser, lnice, lsystem, lidle;
         luser   = ticks[0]-ticksold[0];
         lnice   = ticks[1]-ticksold[1];
         lsystem = ticks[2]-ticksold[2];
         lidle   = ticks[3]-ticksold[3];
         CpuPct = 100.0*(luser+lsystem+lnice)/(luser+lsystem+lnice+lidle);
         ticksold[0] = ticks[0];
         ticksold[1] = ticks[1];
         ticksold[2] = ticks[2];
         ticksold[3] = ticks[3];
      }
   }
   return CpuPct;
}


cString cInfoLines::PrepareInfoline(int line, bool *isStatic)
{
   #define BARLEN 30
   char progressbar[BARLEN+3] = "";
   progressbar[0] = '[';
   progressbar[BARLEN+1] = ']';
   progressbar[BARLEN+2] = 0;

   char *systeminfo = NULL;
   cString osdline = NULL;

   systeminfo = ExecShellCmd((const char *)cString::sprintf("%s %d", scriptname, line));
   //isyslog("systeminfo: %2d, %s", line, systeminfo);
   if (NULL != (const char *)systeminfo) {
      float fval1, fval2 = 0;
      char *pname = NULL;
      *isStatic = !strncasecmp(systeminfo, "s\t", 2);
      osdline = *isStatic ? systeminfo+2 : systeminfo;

      // check for unformatted disk usage (in kB without units)
      if (3 == sscanf(systeminfo, "%a[a-zA-Z,/0-9 ]: %f %f", &pname, &fval1, &fval2)) {
         fval1 /= 1024.0*1024.0;
         fval2 /= 1024.0*1024.0;
         if (fval1 == 0.0)
            osdline = cString::sprintf("%s:\t%.1f GB / %.1f GB", pname, fval1, fval2);
         else {
            int frac = min(BARLEN,max(0, int((fval1-fval2)*BARLEN/fval1)));
            memset(progressbar + 1,'|',frac);
            memset(progressbar + 1 + frac ,' ', BARLEN - frac);

            osdline = cString::sprintf("%s:\t%.1f GB / %.1f GB\t%s", pname, fval1, fval2, progressbar);
         }

         free(pname);
      }

      // check for values in kB
      else if (3 == sscanf(systeminfo, "%a[a-zA-Z,/0-9 ]: %fkB %fkB", &pname, &fval1, &fval2)) {
         if (fval1 == 0.0)
            osdline = cString::sprintf("%s:\t%.1f kB / %.1f kB", pname, fval1, fval2);
         else {
            int frac = min(BARLEN,max(0, int((fval1-fval2)*BARLEN/fval1)));
            memset(progressbar + 1,'|',frac);
            memset(progressbar + 1 + frac ,' ', BARLEN - frac);

            if (fval1 > 1024.0*1024.0 && fval2 > 1024.0*1024.0) {
               fval1 /=1024.0*1024.0;
               fval2 /=1024.0*1024.0;
               osdline = cString::sprintf("%s:\t%.1f GB / %.1f GB\t%s", pname, fval1, fval2, progressbar);
            } else if (fval1 > 1024.0 && fval2 > 1024.0) {
               fval1 /=1024.0;
               fval2 /=1024.0;
               osdline = cString::sprintf("%s:\t%.1f MB / %.1f MB\t%s", pname, fval1, fval2, progressbar);
            } else 
               osdline = cString::sprintf("%s:\t%.1f kB / %.1f kB\t%s", pname, fval1, fval2, progressbar);
         }
         free(pname);
      }

      // check for CPU%
      else if (1 == sscanf(systeminfo, "%a[a-zA-Z,/0-9 ]: CPU%%", &pname) && NULL != strstr(systeminfo, "CPU%")) {
         fval2 = GetCpuPct();
         int frac = min(BARLEN,max(0, int(fval2*BARLEN/100.0)));
         memset(progressbar + 1,'|',frac);
         memset(progressbar + 1 + frac ,' ', BARLEN - frac);

         osdline = cString::sprintf("%s:\t%.1f %%\t%s", pname, fval2, progressbar);
         free(pname);
      }
   }
   return osdline;
}


void cInfoLines::Action()
{
   int line = 0;
   Lock();
   Clear();
   Unlock();
   cString osdline = NULL;

   GetCpuPct(); // init cpu usage

   do {
      bool isStatic = false;

      osdline = PrepareInfoline(++line, &isStatic);
      if ((const char*)osdline) {
         Lock();
         Add(new cInfoLine(osdline, isStatic));
         Unlock();
      }
   }
   while (Running() && NULL != (const char*)osdline && line <= MAX_LINES);

   if (!First()) {
      Lock();
      osdline = tr("Error getting system information");
      Add(new cInfoLine(osdline, true));
      state++;
      Unlock();
   }
      else
   {
      Lock();
      state++;
      Unlock();
      if (Running())
         Wait.Wait(RefreshIntervall*1000);

      while (Running()) {
         cInfoLine * currentline = NULL;
         Lock();
         if (OsdInitialized)
            firstDisplay = false;
         currentline = First();
         Unlock();

         line = 0;
         do {
            cString osdline = NULL;
            bool isStatic = false;
            line++;
            if (!currentline || !currentline->isStatic()) {
               osdline = PrepareInfoline(line, &isStatic);
               if ((const char*)osdline) {
                  Lock();
                  currentline->SetStr(osdline);
                  Unlock();
               }
            }
            currentline = Next(currentline);
         }
         while (Running() && NULL != currentline && line <= MAX_LINES);

         Lock();
         state++;
         Unlock();

         if (Running()) {
            Wait.Wait(RefreshIntervall*1000);
         }
      } // while Running()
   }
   isyslog("systeminfo: Action end");
}


char *cInfoLines::ExecShellCmd(const char *cmd)
{  // taken from vdr config.c
   char *result = NULL;
   //isyslog("executing command '%s'", cmd);
   cPipe p;
   if (!p.Open(cmd, "r"))
      esyslog("ERROR: can't open pipe for command '%s'", cmd);
   else {
      int l = 0;
      int c;
      while ((c = fgetc(p)) != EOF) {
         if (!l && c == ' ') continue; // skip leading spaces

         if (l % 20 == 0)
            result = (char *)realloc(result, l + 21);
         result[l++] = c;
      }
      p.Close();

#ifdef DEBUG
      isyslog("result: '%s'", result);
      cString hex;
      for (int i = 0; i<= l; i++) {
         hex = cString::sprintf("%s %02X", (const char *)hex, result[i]);
      }
      isyslog("result: %s", (const char *)hex);
#endif

      if (result) {
         if (result[l-1] == 0x0A)
            result[l-1] = 0;
         else
            result[l] = 0;
      }
      //isyslog("result: '%s'", result);
   }

  return result;
}
