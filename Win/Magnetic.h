/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// Magnetic.h: Declaration of the application class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAGNETIC_H__459160E6_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_MAGNETIC_H__459160E6_887D_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// Declaration of CMagneticApp
/////////////////////////////////////////////////////////////////////////////

#define CHAR_BUFFER_SIZE 256

class CMagneticApp : public CWinApp
{
public:
	CMagneticApp() {};

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagneticApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL OnIdle(LONG lCount);
	//}}AFX_VIRTUAL

// Implementation
public:

	//{{AFX_MSG(CMagneticApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	afx_msg void OnViewFont();
	afx_msg void OnUpdateRecentFileMenu(CCmdUI* pCmdUI);
	afx_msg void OnViewOptions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

// Attributes
public:
	LOGFONT* GetLogFont(void);
	CSize& GetMargins(void);
	COLORREF GetForeColour(void);
	COLORREF GetBackColour(void);

	void SetRedrawStatus(int iNewStatus);
	int GetRedrawStatus(void);

	CRect& GetWindowRect(void);
	int& GetWindowMax(void);
	CPoint& GetPicTopLeft(void);

public:
	enum Redraw
	{
		NoActionNeeded,
		RedrawThisLine,
		RedrawEndLine,
	};

protected:
	// File dialog data
	CFileDialog* m_pNewGameDialog;

	// Font dialog data
	LOGFONT m_LogFont;
	CFontDialog* m_pFontDialog;

	// Text window data
	int m_iRedrawStatus;
	CSize m_Margins;
	CRect m_WindowRect;
	int m_iWindowMax;

	// Picture window data
	CPoint m_PicTopLeft;

public:
	// Options dialog
	BOOL m_bShowPics;
	double m_dScaleFactor;
	double m_dScaleTitles;
	COLORREF m_ForeColour;
	COLORREF m_BackColour;

	int m_iGameLoaded;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGNETIC_H__459160E6_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
