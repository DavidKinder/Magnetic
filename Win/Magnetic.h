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
	enum Redraw
	{
		NoRedraw = 0,
		ThisLine,
		EndLine,
		EndPlayback,
		EndOpcode,
	};
	enum ShowGraphics
	{
		NoGraphics = 0,
		SeparateWindow,
		MainWindow,
	};

	LOGFONT* GetLogFont(void);
	CSize& GetMargins(void);
	COLORREF GetForeColour(void);
	COLORREF GetBackColour(void);
	COLORREF GetGfxColour(void);

	void SetRedrawStatus(Redraw Status);
	Redraw GetRedrawStatus(void);

	CRect& GetWindowRect(void);
	int& GetWindowMax(void);
	CPoint& GetPicTopLeft(void);

	ShowGraphics GetShowGraphics(void);
	void SetShowGraphics(ShowGraphics Show);
	double GetScaleFactor(void);
	void SetScaleFactor(double dFactor);
	double GetScaleTitles(void);
	void SetScaleTitles(double dTitles);
	void SetForeColour(COLORREF Colour);
	void SetBackColour(COLORREF Colour);
	void SetGfxColour(COLORREF Colour);
	BOOL GetAnimWait(void);
	void SetAnimWait(BOOL bWait);
	BOOL GetPredictable(void);
	void SetPredictable(BOOL bPredict);
	int GetRandomSeed(void);
	void SetRandomSeed(int iSeed);
	double GetGamma(void);
	void SetGamma(double dGamma);

	int GetGameLoaded(void);
	void SetGameLoaded(int iLoaded);

protected:
	// File dialog data
	CFileDialog* m_pNewGameDialog;

	// Font dialog data
	LOGFONT m_LogFont;
	CFontDialog* m_pFontDialog;

	// Text window data
	Redraw m_RedrawStatus;
	CSize m_Margins;
	CRect m_WindowRect;
	int m_iWindowMax;

	// Picture window data
	CPoint m_PicTopLeft;

	// Options dialog
	ShowGraphics m_ShowGfx;
	double m_dScaleFactor;
	double m_dScaleTitles;
	COLORREF m_ForeColour;
	COLORREF m_BackColour;
	COLORREF m_GfxColour;
	BOOL m_bAnimWait;
	BOOL m_bPredict;
	int m_iSeed;
	double m_dGamma;

	int m_iGameLoaded;
};
