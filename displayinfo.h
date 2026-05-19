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

extern int RefreshIntervall;
extern int AutoClose;

class cInfoLine : public cListObject{
private:
   cString string;
   bool isstatic;
   cOsdItem *osditem;
public:
   cInfoLine(cString String, bool IsStatic) { SetStr(String); isstatic = IsStatic; } ;
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
   cString scriptname;
   cCondWait Wait;
   bool firstDisplay;
   bool OsdInitialized;
   unsigned long long ticks[4], ticksold[4];

   void Action();
   float GetCpuPct();
   char *ExecShellCmd(const char *Cmd);
   cString PrepareInfoline(int Line, bool *IsStatic);
public:
   cInfoLines(const char *Script);
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
   char *ExecShellCmd(const char *Cmd);
   void Set(void);
public:
   cMenuSystemInfo(const char *Script);
   virtual ~cMenuSystemInfo();
   virtual eOSState ProcessKey(eKeys Key);
};
