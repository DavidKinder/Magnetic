/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// HintDialog.h: Hint dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "MagneticDlg.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Listbox for displaying hints
class CHintListBox : public DarkModeListBox
{
public:
  CHintListBox();
  virtual ~CHintListBox();

protected:
  DECLARE_MESSAGE_MAP()

public:
  virtual void MeasureItem(LPMEASUREITEMSTRUCT /*lpMeasureItemStruct*/);
  virtual void DrawItem(LPDRAWITEMSTRUCT /*lpDrawItemStruct*/);

public:
  void SetItemHeights(void);
};

// Dialog for showing hints
class CHintDialog : public CMagneticDlg
{
// Construction
public:
  CHintDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
  //{{AFX_DATA(CHintDialog)
  enum { IDD = IDD_HINTS };
  DarkModeButton m_doneButton;
  DarkModeButton m_topicButton;
  DarkModeButton m_hintButton;
  DarkModeButton m_prevButton;
  CHintListBox m_hintList;
  //}}AFX_DATA

// Overrides
  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CHintDialog)
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  virtual BOOL DestroyWindow();
  //}}AFX_VIRTUAL

// Implementation
protected:

  // Generated message map functions
  //{{AFX_MSG(CHintDialog)
  virtual BOOL OnInitDialog();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  afx_msg void OnPrevious();
  afx_msg void OnTopics();
  afx_msg void OnShowHint();
  afx_msg void OnChangeHints();
  afx_msg void OnDblClkHints();
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

public:
  void SetDarkMode(DarkMode* dark);
  void SetHints(struct ms_hint* hints);

protected:
  int LoadHintSet(int element);
  void UpdateHintList(void);
  void LayoutControls(void);

  struct ms_hint* m_allHints;
  int m_currHint;
  int m_visibleHints;

  CRect m_btnSize;
};
