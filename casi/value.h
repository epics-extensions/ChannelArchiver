
class value
{
public:
	value();
	~value();

	/* Is this value valid?
	   The following calls are only allowed if the value is valid!
	 */
	bool valid(); 

	/* Some values are not numeric values but purely informational. 
		In that case the status method returns 

		Archive_Off,
		Disconnected,
		Disabled,
		... maybe more special values

	   To check for such "info" values, this command can be used. 
	 */
	bool isInfo();

	/* Get native type of this value as string */
	const char *type();

	/* Get array size of this value (0: invalid, 1: scalar) */
	int count();

	/* Get current value as number, same as getidx(0) */
	double get();

	/* Get current value as number,
	   accessing the specified array element
	 */
	double getidx(int index); 

	/* Get current value as text */
	const char *text();

	/* Get current time stamp */
	const char *time();

    /* Return current time stamp as double */
    double getDoubleTime();

	/* Get current status string */
	const char *status();

	/* Position on next value. Result: succesful? */
	bool next();

	/* Position on prev value. Result: succesful? */
	bool prev();

    /* Count values of matching type/control information
     * up to given time stamp */
    int determineChunk(const char *until);

private:
	friend class channel;

	void setIter(ValueIteratorI *iter);
	ValueIteratorI *_iter;
};
