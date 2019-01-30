/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticTitle.h: Title picture dialog class
//
/////////////////////////////////////////////////////////////////////////////

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "ImagePNG.h"
#include "Resource.h"

class CMagneticTitleDlg : public CDialog
{
// Construction
public:
  CMagneticTitleDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(CMagneticTitleDlg)
  enum { IDD = IDD_TITLE };
  //}}AFX_DATA

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CMagneticTitleDlg)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(CMagneticTitleDlg)
  virtual BOOL OnInitDialog();
  afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
  afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
  afx_msg void OnPaint();
  //}}AFX_MSG
  afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

public:
  void ShowTitle(LPCTSTR pszGamePath);

protected:
  void SizeTitle(void);
  void StartMusic(LPCTSTR pszGamePath);
  void StopMusic();

  ImagePNG m_Picture;
  ImagePNG m_ScaledPicture;
  int m_dpi;
};
