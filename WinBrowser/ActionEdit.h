//	CActionEdit
//
//	CEdit-derived class for EDIT controls
//	which triggers a callback routine when
//	<Return> is pressed.
//
//	EDIT must be configured with these options:
//	* Multiline
//	* Want Return
//
//	but the size should be only one line in hight
//	so that it's not really 'multilined'.
//	Windows will beep on each <Return> because it
//	cannot insert a new line in the control.
//	It's not a bug: audible feedback ;-)  !
//
#if !defined(ACTIONEDIT_H)
#define ACTIONEDIT_H

#if _MSC_VER >= 1000
#pragma once
#endif

#define ActionEditCallback(name)	\
	void name (const CString &txt, void *arg)

class CActionEdit : public CEdit
{
public:
	CActionEdit ();

	//	Callback will receive current text and user arg:
	typedef void (* Callback) (const CString &txt, void *arg);

	void SetCallback (Callback func, void *arg);
	const CString &GetText ();
	int GetTextAsInt ();
	double GetTextAsDouble ();

	void SetText (const CString &txt);
	void SetText (long number);
	void SetText (int number)
	{
		SetText ((long) number);
	}
	void SetText (double number);

	//{{AFX_VIRTUAL(CActionEdit)
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CActionEdit)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	Callback	m_cb_func;
	void		*m_cb_arg;
	CString		m_txt;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}

#endif // !defined(ACTIONEDIT_H)
