class value;
#include <CtrlInfoI.h>

typedef enum
{
   Invalid = 0,
   Numeric = 1,
   Enumerated = 2
} Type;

class ctrlinfo
{
public:
   ctrlinfo();
   ~ctrlinfo();
   
   Type getType() const;
   /* return type of control info:
      is either Invalid, Numeric or Enumerated. */
   
   long getPrecision() const;
   const char *getUnits() const;
   float getDisplayHigh() const;
   float getDisplayLow() const;
   float getHighAlarm() const;
   float getHighWarning() const;
   float getLowWarning() const;
   float getLowAlarm() const;
   /* Extract the fields of a Numeric control info */
   
   void setNumeric(long prec, const char *units,
		   float disp_low, float disp_high,
		   float low_alarm, float low_warn, 
		   float high_warn, float high_alarm);
   /* set a Numeric control info */

   void setEnumeratedString(int state, const char *str);
   void setEnumerated();
   /* set the enumeration strings of a new control info.
      The enumeration strings have to be set in ascending order starting at
      0 with no gaps. After the last enumeration strings has been set,
      <b>setEnumerated</b> has to be called.
   */

   int getNumStates() const;
   /* return the number of states in this enumeration */

   const char* getState(int state) const;
   /* return the enumeration string of state */

private:
#ifndef SWIG
   friend class value;
#endif

   void setValue( const ValueI* );

   size_t _enumCnt;
   char **_enumStr;

   CtrlInfoI *_ctrli;
};

