/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// ScrollBackDlg.h: Scrollback dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "MagneticDlg.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CScrollBackDlg : public CMagneticDlg
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
  void ResizeRichEdit(void);

  // Generated message map functions
  //{{AFX_MSG(CScrollBackDlg)
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnCopy();
  //}}AFX_MSG
  afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
  afx_msg LRESULT OnSameSizeAsMain(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

public:
  void SetDarkMode(DarkMode* dark);
  CString& GetScrollback(void);

protected:
  CString m_strScrollback;
  DarkModeRichEditCtrl m_RichEdit;
  DarkModeButton m_CopyButton;
  DarkModeButton m_CloseButton;
};
