/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticView.h: Declaration of the view class
//
/////////////////////////////////////////////////////////////////////////////

#include "ScrollBackDlg.h"

#if !defined(AFX_MAGNETICVIEW_H__459160EE_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_MAGNETICVIEW_H__459160EE_887D_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "PictureWnd.h"

class CMagneticView : public CView
{
protected: // create from serialization only
	CMagneticView();
	DECLARE_DYNCREATE(CMagneticView)

// Attributes
public:
	CMagneticDoc* GetDocument();

// Operations
public:
	static CMagneticView* GetView(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMagneticView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMagneticView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

public:
	void TextSetup(void);
	void TextClearup(void);
	void UseHistory(CString& strNewInput, int iOldLength);
	
	void AddOutChar(char c);
	void AddStatChar(char c);
	void SetCursorPos(CDC* pDC, int iRight);
	void InsertChar(CString& strInsert, char cChar, int iPos, int iIsOut = FALSE);
	void RemoveChar(CString& strRemove, int iPos, int iIsOut = FALSE);
	void ClearAll(void);
	void TrimOutput(void);

protected:
	void SolidRect(CDC* pDC, LPCRECT lpRect, COLORREF Colour);
	int Paginate(CDC* pDC, int p1, int p2);
	BOOL LineFull(CDC* pDC, LPCSTR lpszText, int iLength);
	int FindPreviousSpace(LPCSTR lpszText, int iPos);

// Generated message map functions
protected:
	//{{AFX_MSG(CMagneticView)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnRecord();
	afx_msg void OnPlayback();
	afx_msg void OnScript();
	afx_msg void OnUpdateRecord(CCmdUI* pCmdUI);
	afx_msg void OnUpdatePlayback(CCmdUI* pCmdUI);
	afx_msg void OnUpdateScript(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpen(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewFont(CCmdUI* pCmdUI);
	afx_msg void OnScrollback();
	afx_msg void OnEditPaste();
	afx_msg void OnUpdateViewOptions(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	static const int SPECIAL_KEYS;
	static const int MAX_LINES;

	CArray<int, int> m_PageTable;		// Keeps track of current pagination

	int m_iLines;
	int m_iMaxLines;
	int m_iMorePrompt;

	CString m_strOutput;
	CArray<int, int> m_Input;
	CRect m_LastLineRect;
	CArray<CString, CString&> m_History;

	CString m_strStatLocation;
	CString m_strStatScore;
	CString m_strStatCurrent;
	int m_iStatMode;

	CFont* m_pTextFont;			// Current output text fount
	CFont* m_pOldFont;			// Previous DC font
	CDC* m_pTextDC;					// Device context for text attributes

	CPictureWnd m_Picture;	// Picture window

	CString m_strFileName;	// File name for load and save dialog
	CString m_strRecName;		// File name for recording
	CString m_strScrName;		// File name for scripting

	int m_iRecording;				// File recording status
	FILE* m_pFileRecord;
	int m_iScripting;				// Scripting status
	FILE* m_pFileScript;
	CString m_strScript;

	CScrollBackDlg m_Scrollback;	// Scrollback dialog
};

#ifndef _DEBUG  // debug version in MagneticView.cpp
inline CMagneticDoc* CMagneticView::GetDocument()
   { return (CMagneticDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAGNETICVIEW_H__459160EE_887D_11D1_ACE4_8081A5F82D24__INCLUDED_)
