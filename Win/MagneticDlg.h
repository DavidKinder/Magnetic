/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticDlg.h: Base dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "Dialogs.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMagneticDlg : public BaseDialog
{
  DECLARE_DYNAMIC(CMagneticDlg)

public:
  CMagneticDlg(UINT templateId, CWnd* parent = NULL);

  virtual INT_PTR DoModal();

  DECLARE_MESSAGE_MAP()
};
