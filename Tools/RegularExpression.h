#if !defined(_REGULAREXPRESSION_H_)
#define _REGULAREXPRESSION_H_

#include <ToolsConfig.h>

// RegularExpression:
// Wrapper for Unix/GNU regex library.
//
// Detail:
// Handles reference count since the compiled pattern
// cannot be copied with library calls and
// it would be to costly to recompile the pattern
// when copying RegularExpressions.
class RegularExpression  
{
public:
	// Create RegularExpression with pattern for further matches.
	// Returns 0 when pattern cannot be compiled
	static RegularExpression *reference(const char *pattern,
                                        bool case_sensitive=true);

    // Create a regular expression for a "glob" pattern:
    // question mark - any character
    // star          - many characters
    // case insensitive
	static stdString fromGlobPattern(const stdString &glob);

	RegularExpression *reference()
	{	++_refs; return this; }

	void release()
	{	if (--_refs <= 0)  delete this; }

	// Test if 'input' matches current pattern.
	//
	// Currently uses
	// * EXTENDED regular expression,
	// * case sensitive
	// * input must be anchored for full-string match,
	//   otherwise substrings will match:
	//     abc        matches        b
	//     abc     does not match    $b^
	//
	// When no pattern was supplied, anything matches!
	bool doesMatch(const char *input);

private:
	friend class ToRemoveGNUCompilerWarning;
	// Use create/reference/unreference instead of new/delete:
	RegularExpression(const RegularExpression &); // intentionally not implemented
	RegularExpression & operator = (const RegularExpression &); // intentionally not implemented
	
	RegularExpression();
	~RegularExpression();

	void	*_compiled_pattern;
	int		_refs;
};

#endif // !defined(_REGULAREXPRESSION_H_)
