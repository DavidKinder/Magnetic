/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// OptionsDlg.h: Options dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "ColorBtn.h"

#if !defined(AFX_OPTIONSDLG_H__05C883D8_CCBC_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_OPTIONSDLG_H__05C883D8_CCBC_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class COptionsDlg : public CDialog
{
// Construction
public:
	COptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COptionsDlg)
	enum { IDD = IDD_OPTIONS };
	BOOL	m_bShowPics;
	double	m_dScaleFactor;
	double	m_dScaleTitles;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COptionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COptionsDlg)
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CSpinButtonCtrl m_Spin, m_SpinTitles;
	CColorBtn m_FColour, m_BColour;
	CButton m_FDefault, m_BDefault;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OPTIONSDLG_H__05C883D8_CCBC_11D1_ACE4_8081A5F82D24__INCLUDED_)
