/*
 * displayinfo.h: display handling
 *
 * See the main source file 'systeminfo.c' for copyright information and
 * how to reach the author.
 *
 * $Id$
 */

#include <vdr/osdbase.h>

extern int RefreshIntervall;

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

   void Action();
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
