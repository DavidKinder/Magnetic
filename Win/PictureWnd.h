/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// PictureWnd.h: Declaration of the picture window class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PICTUREWND_H__74AE6F17_94E7_11D1_ACE4_8081A5F82D24__INCLUDED_)
#define AFX_PICTUREWND_H__74AE6F17_94E7_11D1_ACE4_8081A5F82D24__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CPictureWnd window
/////////////////////////////////////////////////////////////////////////////

class CPictureWnd : public CWnd
{
// Construction
public:
	CPictureWnd();

// Attributes
public:

// Operations
public:
	BOOL CreatePicWnd(CWnd* pParent, const RECT& rect);
	void NewPicture(int iWidth, int iHeight, unsigned char* pBuffer, unsigned short* pPalette);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPictureWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPictureWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CPictureWnd)
	afx_msg void OnPaint();
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnDestroy();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	// Main input window
	CWnd* m_pMagneticWnd;

	// Window borders
	CSize m_Borders;

	// Bitmap data
	BITMAPINFO* m_pBitmapInfo;
	char* m_pBitmap;
	CSize m_PictureSize;
	CPalette* m_pPalette;
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PICTUREWND_H__74AE6F17_94E7_11D1_ACE4_8081A5F82D24__INCLUDED_)
