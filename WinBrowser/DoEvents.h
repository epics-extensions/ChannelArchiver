#ifndef __DOEVENTS_H__
#define __DOEVENTS_H__

// Call periodically to keep the Windows message pump going
// while running sidetracks
inline bool DoEvents ()
{
	MSG wmsg;
	while ( ::PeekMessage( &wmsg, NULL, 0, 0, PM_NOREMOVE ) ) 
	{
		if (! AfxGetThread()->PumpMessage())
        { 
            ::PostQuitMessage(0); 
            return false;
        } 
	}
    return true;
}

#endif //__DOEVENTS_H__