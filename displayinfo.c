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

int RefreshInterval = 5;
int AutoCloseSec = 20;

// --- cMenuSystemInfo ----------------------------------------------------
cMenuSystemInfo::cMenuSystemInfo(const char *Script)
:cOsdMenu(tr("System Information"))
{
   InfoLines = NULL;
   firstDisplay = true;
   autocloseTimer.Set(AutoCloseSec * 1000);

   if (access(Script, X_OK)) {
      Add(new cOsdItem(cString::sprintf(tr("Script '%s' not found or not executable"), Script), osUnknown, false));
   }
   else {
      Add(new cOsdItem(tr("please wait"), osUnknown, false));

      InfoLines = new cInfoLines(Script);
      InfoLines->StateChanged(infolinesState); // init state
   }
   SetHelp(NULL, NULL, NULL, NULL);
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
         case kNone:    if (InfoLines && InfoLines->StateChanged(infolinesState))
                           Set();
                        break;
         default:       break;
      }
      state = AutoCloseSec && autocloseTimer.TimedOut() ? osEnd : osContinue;
   }
   return state;
}


void cMenuSystemInfo::Set()
{
   if (firstDisplay)
   {
      Clear();
#if defined(APIVERSNUM) && APIVERSNUM >= 30013
      int t1 = 0;
      int t2 = 0;
      const cFont *Font = dynamic_cast<cSkinDisplayMenu *>(cSkinDisplay::Current())->GetTextAreaFont(false);
#endif
      cThreadLock InfoLinesLock(InfoLines);
      for (cInfoLine *line = InfoLines->First(); line; line = InfoLines->Next(line))
      {
         Add(line->GetOsdItem());
#if defined(APIVERSNUM) && APIVERSNUM >= 30013
         const char *text = line->GetStr();
         if (Font && !isempty(text)) {
            if (const char *Tab1 = strchr(text, '\t')) {
               int l = Font->Width(cString(text, Tab1));
               if (l > t1)
                  t1 = l;
               if (const char *Tab2 = strchr(Tab1 + 1, '\t')) {
                  int l = Font->Width(cString(Tab1 + 1, Tab2));
                  if (l > t2)
                     t2 = l;
               }
            }
         }
#endif
      }

#if defined(APIVERSNUM) && APIVERSNUM >= 30013
      if (Font) {
         if (t1 > 0) {
            t1 += Font->Width("  "); // to have some distance between name and value
            if (Font->Width("M") > 1)
               t1 = -t1;
         }
         if (t2 > 0) {
            t2 += Font->Width("  "); // to have some distance between name and value
            if (Font->Width("M") > 1)
               t2 = -t2;
         }
      }
      SetCols(t1, t2);
#else
      SetCols(14, 18);
#endif
      firstDisplay = false;
   }
   else
   {
      cThreadLock InfoLinesLock(InfoLines);
      for (cInfoLine *line = InfoLines->First(); line; line = InfoLines->Next(line))
         line->UpdateOsdText();
   }
   Display();
}


// --- cInfoLines ----------------------------------------------------
cInfoLines::cInfoLines(const char *Script)
:cThread("systeminfo", true)
{
   state = 0;
   scriptname = Script;
   Start();
}


cInfoLines::~cInfoLines()
{
   if (Running()) {
      Wait.Signal();
      Cancel(RefreshInterval);
   }
}


bool cInfoLines::StateChanged(int &State)
{
   int NewState = state;
   bool changed = State != NewState;
   State = NewState;

   return changed;
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


cString cInfoLines::PrepareInfoline(int Line, bool *IsStatic)
{
   #define BARLEN 30
   char progressbar[BARLEN+3];
   progressbar[0] = '[';
   progressbar[BARLEN+1] = ']';
   progressbar[BARLEN+2] = 0;
   cString osdline;

   cString systeminfo = ExecShellCmd(*cString::sprintf("%s %d", *scriptname, Line));
   //isyslog("systeminfo:  %2d, %s", Line, *systeminfo);
   if (!isempty(*systeminfo)) {
      float fval1, fval2 = 0;
      char *pname = NULL;
      if (IsStatic) {
         *IsStatic = !strncasecmp(*systeminfo, "s\t", 2);
         systeminfo = *IsStatic ? cString(strdup((*systeminfo) + 2), true) : systeminfo;
      }

      // check for two values (total and free) with our without 'kB' like e.g. disk usage
      if (3 == sscanf(*systeminfo, "%m[a-zA-Z,/0-9 ]: %f %f", &pname, &fval1, &fval2) ||
         (3 == sscanf(*systeminfo, "%m[a-zA-Z,/0-9 ]: %f kB %f kB", &pname, &fval1, &fval2)))
      {
         compactspace(pname);
         if (fval1 == 0.0)
            osdline = cString::sprintf("%s:\t%.1f kB / %.1f kB", pname, fval1, fval2);
         else {
            int frac = min(BARLEN, max(0, int((fval1 - fval2) * BARLEN / fval1)));
            memset(progressbar + 1,'|',frac);
            memset(progressbar + 1 + frac ,' ', BARLEN - frac);

            cString unit = "kB";
            if (fval1 > 1024.0 && fval2 > 1024.0) {
               fval1 /= 1024.0;
               fval2 /= 1024.0;
               unit = "MB";
               if (fval1 > 1024.0 && fval2 > 1024.0) {
                  fval1 /= 1024.0;
                  fval2 /= 1024.0;
                  unit = "GB";
               }
            }
            osdline = cString::sprintf("%s:\t%.1f %s / %.1f %s\t%s", pname, fval1, *unit, fval2, *unit, progressbar);
            free(pname);
         }
      }

      // check for CPU%
      else if (strstr(systeminfo, "CPU%") && (1 == sscanf(*systeminfo, "%m[a-zA-Z,/0-9 ]: CPU%%", &pname)))
      {
         compactspace(pname);
         fval2 = GetCpuPct();
         int frac = min(BARLEN,max(0, int(fval2*BARLEN/100.0)));
         memset(progressbar + 1,'|',frac);
         memset(progressbar + 1 + frac ,' ', BARLEN - frac);

         osdline = cString::sprintf("%s:\t%.1f %%\t%s", pname, fval2, progressbar);
         free(pname);
      }
      else
         osdline = systeminfo;
   }
   return osdline;
}


void cInfoLines::Action()
{
   int line = 0;
   cString osdline;
   GetCpuPct(); // init CPU usage

   do {
      bool isStatic = false;

      osdline = PrepareInfoline(++line, &isStatic);
      if (!isempty(*osdline)) {
         Add(new cInfoLine(osdline, isStatic));
      }
   }
   while (Running() && !isempty(*osdline) && line <= MAX_LINES);

   if (First() == NULL) {
      Add(new cInfoLine(tr("Error getting system information"), true));
      state++;
   }
   else
   {
      state++;

      while (Running()) {
         Wait.Wait(RefreshInterval*1000);
         cInfoLine *currentline = First();
         line = 0;
         while (Running() && currentline && ++line <= MAX_LINES) {
            if (!currentline->isStatic()) {
               osdline = PrepareInfoline(line);
               if (!isempty(*osdline)) {
                  cThread::Lock();
                  currentline->SetStr(*osdline);
                  cThread::Unlock();
               }
            }
            currentline = Next(currentline);
         }

         state++;
      } // while Running()
   }
}


cString cInfoLines::ExecShellCmd(const char *Cmd)
{  // taken from vdr menu.c with modifications
   char *result = NULL;
   //isyslog("executing command '%s'", Cmd);
   cPipe p;
   if (!p.Open(Cmd, "r"))
      esyslog("ERROR: can't open pipe for command '%s'", Cmd);
   else {
      int l = 0;
      int c;
      while ((c = fgetc(p)) != EOF) {
         if ((!l || result[l-1]== '\t') && c == 0x20) continue; // skip leading spaces
         if ( c == 0x0A || c == 0x0D) continue;

         if (l % 50 == 0) {
            if (char *NewBuffer = (char *)realloc(result, l + 51))
               result = NewBuffer;
            else {
               esyslog("ERROR: out of memory");
               break;
            }
         }
         result[l++] = c;
      }
      p.Close();
      if (result) {
         result[l] = 0;

#ifdef DEBUG
         isyslog("result: '%s'", result);
         cString hex = "";
         for (int i = 0; i <= l; i++)
            hex = cString::sprintf("%s %02X", *hex, result[i]);
         isyslog("resultHex: %s", *hex);
#endif
      }
   }

  return cString(result, true);
}
