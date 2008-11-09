/*
 * displayinfo.h: display handling
 *
 * See the main source file 'systeminfo.c' for copyright information and
 * how to reach the author.
 *
 * $Id$
 */

#include <vdr/config.h>
#include <vdr/osdbase.h>

#if APIVERSNUM < 10507
#include "i18n.h"
#define trNOOP(s) (s)
#endif

#if APIVERSNUM < 10509
#define trVDR(s) tr(s)
#endif


extern int RefreshIntervall;
extern int AutoClose;

class cInfoLine : public cListObject{
private:
   cString string;
   bool isstatic;
   cOsdItem *osditem;
public:
   cInfoLine(cString String, bool isStatic) { SetStr(String); isstatic = isStatic; } ;
   ~cInfoLine() {};
   void SetStr(cString String) { string = String; };
   cString GetStr(void) { return string; };
   void SetOsdItem(cOsdItem *OsdItem) { osditem = OsdItem; }
   cOsdItem *GetOsdItem(void) { return osditem; }
   bool isStatic(void) { return isstatic; }
};


class cInfoLines : public cList<cInfoLine>, public cThread {
private:
   int state;
   char scriptname[MaxFileName];
   cCondWait Wait;
   bool firstDisplay;
   bool OsdInitialized;
   unsigned long long ticks[4], ticksold[4];

   void Action();
   float GetCpuPct();
   char* ExecShellCmd(const char*);
   cString PrepareInfoline(int, bool*);
public:
   cInfoLines(const char *path);
   ~cInfoLines();
   bool StateChanged(int &State);
   bool FirstDisplay(void) { return firstDisplay; };
   void SetOsdInitialized() { OsdInitialized = true; };
};


class cMenuSystemInfo : public cOsdMenu {
private:
   cInfoLines *InfoLines;
   int infolinesState;
   char scriptpath[MaxFileName];
   char *ExecShellCmd(const char *cmd);
   void Set(void);
public:
   cMenuSystemInfo(const char *path);
   virtual ~cMenuSystemInfo();
   virtual eOSState ProcessKey(eKeys Key);
};
