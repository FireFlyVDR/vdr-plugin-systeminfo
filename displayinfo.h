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
#include <atomic>

extern int RefreshInterval;
extern int AutoCloseSec;

class cInfoLine : public cListObject{
private:
   cString string;
   bool isstatic;
   cOsdItem *osdItem;
public:
   cInfoLine(cString String, bool IsStatic) { osdItem = new cOsdItem(*String); string = String; isstatic = IsStatic; } ;
   ~cInfoLine() {};
   void SetStr(const char *String) { string = String; };
   const char *GetStr(void) const { return *string; };
   bool isStatic(void) { return isstatic; }
   cOsdItem *GetOsdItem(void) { return osdItem; }
   void UpdateOsdText(void) { osdItem->SetText(*string); };
};


class cInfoLines : public cList<cInfoLine>, public cThread {
private:
   std::atomic<int> state;
   cString scriptname;
   cCondWait Wait;
   unsigned long long ticks[4], ticksold[4];

   void Action();
   float GetCpuPct();
   cString ExecShellCmd(const char *Cmd);
   cString PrepareInfoline(int Line, bool *IsStatic = NULL);
public:
   cInfoLines(const char *Script);
   ~cInfoLines();
   bool StateChanged(int &State);
};


class cMenuSystemInfo : public cOsdMenu {
private:
   cInfoLines *InfoLines;
   int infolinesState;
   bool firstDisplay;
   cTimeMs autocloseTimer;
   void Set(void);
public:
   cMenuSystemInfo(const char *Script);
   virtual ~cMenuSystemInfo();
   virtual eOSState ProcessKey(eKeys Key);
};
