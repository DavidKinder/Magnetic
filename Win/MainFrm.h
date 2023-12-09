/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MainFrm.h: Declaration of the frame class
//
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "MenuBar.h"

#define DARKMODE_REGISTRY "Software\\David Kinder\\Magnetic"

class CMainFrame : public MenuBarFrameWnd
{
protected: // create from serialization only
  CMainFrame();
  DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:
  void SetModalDialog(CWnd* dialog);

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CMainFrame)
  public:
  virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
  virtual BOOL DestroyWindow();
  //}}AFX_VIRTUAL

// Implementation
public:
  virtual ~CMainFrame();
#ifdef _DEBUG
  virtual void AssertValid() const;
  virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
  DarkModeStatusBar m_statusBar;

// Generated message map functions
  //{{AFX_MSG(CMainFrame)
  afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
  afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
  afx_msg BOOL OnQueryNewPalette();
  afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
  //}}AFX_MSG
  afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);

  DECLARE_MESSAGE_MAP()

  int m_dpi;
  CWnd* m_modalDialog;
};
