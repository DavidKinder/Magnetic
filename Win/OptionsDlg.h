/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// OptionsDlg.h: Options dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "ColourButton.h"
#include "MagneticDlg.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class COptionsDlg : public CMagneticDlg
{
// Construction
public:
  COptionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(COptionsDlg)
  enum { IDD = IDD_OPTIONS };
  CButton m_Predict;
  double  m_dScaleFactor;
  BOOL    m_bPredict;
  int     m_iSeed;
  int     m_iShowPics;
  double  m_dGamma;
  BOOL    m_bAnimWait;
  BOOL    m_bHintWindow;
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
  afx_msg void OnChangePredict();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

public:
  void SetDarkMode(DarkMode* dark);

  COLORREF GetForeColour(void);
  COLORREF GetBackColour(void);
  COLORREF GetGfxColour(void);

protected:
  DarkModeButton m_OK, m_Cancel;
  DarkModeGroupBox m_GrahicsGroup, m_ColoursGroup, m_OtherGroup;
  DarkModeComboBox m_ShowPicsCombo;
  DarkModeEdit m_Scale, m_Gamma;
  DarkModeSpinButtonCtrl m_SpinScale, m_SpinGamma;
  ColourButton m_FColour, m_BColour, m_GColour;
  DarkModeCheckButton m_HintCheck, m_AnimWaitCheck, m_PredictCheck;
  DarkModeEdit m_Seed;

  COLORREF m_DefaultFColour, m_DefaultBColour, m_DefaultGColour;
};
