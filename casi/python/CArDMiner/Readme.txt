IMPORTANT: The destination archive cannot be written in the path of
the source archive!  This is not guarded against, and will cause bad
things to happen to your family.

ALSO IMPORTANT: Archives already existing will not be overwritten but
simply appended.  If you want a clean archive in the place of an old
one, you must delete the old one.

LAST IMPORTANT: For Batch Processing to work as expected there are a
few guidlines to follow:

	1) All imports go in the #$GLBL section.
	2) When writing a #$POST section, you can only reference variables
created in the #$GLBL section, otherwise you risk a NameError.
