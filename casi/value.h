class ctrlinfo;

class value
{
public:
   value();
   ~value();

   void clone(value&);
   /* "Clone" an existing value by copying all value-, time-, status- and
      control-info
   */

   bool valid() const; 
   /* Is this value valid?
      The following calls are only allowed if the value is valid!
   */
   
   bool isInfo() const;
   /* Some values are not numeric values but purely informational. 
      In that case the status method returns 
      
      Archive_Off,
      Disconnected,
      Disabled,
      ... maybe more special values

      To check for such "info" values, this command can be used. 
   */

   const char *type() const;
   /* Get native type of this value as string */

   int count() const;
   /* Get array size of this value (0: invalid, 1: scalar) */

   double get() const { return getidx(0); };
   /* Get current value as number, same as getidx(0) */

   double getidx(int index) const; 
   /* Get current value as number,
    * accessing the specified array element
    */

   void set(double v) { setidx(v, 0); };
   /* set the current value as double (requires write-accessible value!) */

   void setidx(double v, int index);
   /* set the value with index index as double (requires write-accessible
    * value!) */

   void parse( const char* );
   /* extract a new value from the string passed */

   const char *text() const;
   /* Get current value as text */

   const char *time() const;
   /* Get current time stamp */

   bool isRepeat() const;
   /* true if the value is a repeat-value */

   void setTime(const char *);
   /* set the timestamp of the current value (requires write-accessible
    * value!) */

   double getDoubleTime() const;
   /* Return current time stamp as double
    */

   const char *status() const;
   /* Get status of current value as string
    */

   int stat() const;
   /* Get status of current value as integer */

   int sevr() const;
   /* Get severity of current value as integer */

   void setStat(int stat, int sevr);
   void setStatus( const char* );
   /* Set status and severity of the current value
    * (requires write-accessible value!) */

   bool next();
   /* Position on next value. Result: succesful? */

   bool prev();
   /* Position on prev value. Result: succesful? */

   int determineChunk(const char *until);
   /* Count values of matching type/control information
    * up to given time stamp */

   void getCtrlInfo(ctrlinfo&) const;
   void setCtrlInfo(ctrlinfo&);
   /* get/set the control info of the current value
    * (setting requires write-accessible value!) */

private:
   friend class channel;
   friend class archive;

   const ValueI *getVal() const;

   void setIter(ValueIteratorI *iter);
   ValueIteratorI *_iter;
   ValueI *_valI;
};

