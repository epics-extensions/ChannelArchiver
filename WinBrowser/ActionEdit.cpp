// ActionEdit.cpp : implementation file
//

#include "stdafx.h"
#include "ActionEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BEGIN_MESSAGE_MAP(CActionEdit, CEdit)
	//{{AFX_MSG_MAP(CActionEdit)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


CActionEdit::CActionEdit ()
{
	m_cb_func = NULL;
	m_cb_arg = NULL;
}

void CActionEdit::SetCallback (Callback func, void *arg)
{
	m_cb_func = func;
	m_cb_arg  = arg;
}

const CString &CActionEdit::GetText ()
{
	GetWindowText (m_txt);

	return m_txt;
}

int CActionEdit::GetTextAsInt ()
{
	return atoi (GetText ());
}

double CActionEdit::GetTextAsDouble ()
{
	return atof (GetText ());
}

void CActionEdit::SetText (const CString &txt)
{
	m_txt = txt;
	SetWindowText (m_txt);
}

void CActionEdit::SetText (long number)
{
	m_txt.Format ("%ld", number);
	SetWindowText (m_txt);
}

void CActionEdit::SetText (double number)
{
	m_txt.Format ("%f", number);
	SetWindowText (m_txt);
}

//	Trigger Callback function on <Return>
void CActionEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == 0x0D)
	{
		GetWindowText (m_txt);
		TRACE ("CActionEdit: Calling callback with '%s'\n",
			m_txt);
		if (m_cb_func != NULL)
		{
			m_cb_func (m_txt, m_cb_arg);
		}
	}
	else
		CEdit::OnKeyDown (nChar, nRepCnt, nFlags);
}

