/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// ScrollBackDlg.h: Scrollback dialog class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCROLLBACKDLG_H__45480CA2_CBF9_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_SCROLLBACKDLG_H__45480CA2_CBF9_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CScrollBackDlg : public CDialog
{
// Construction
public:
	CScrollBackDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CScrollBackDlg)
	enum { IDD = IDD_SCROLLBACK };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScrollBackDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScrollBackDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnCopy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CRect m_DialogRect;
	CString m_strScrollback;

protected:
	CRichEditCtrl m_RichEdit;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCROLLBACKDLG_H__45480CA2_CBF9_11D1_ACE4_8081A5F82D24__INCLUDED_)
