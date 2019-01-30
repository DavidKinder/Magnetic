/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticTitle.cpp: Title picture dialog class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "MagneticTitle.h"
#include "DpiFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Implementation of the CMagneticTitleDlg dialog
/////////////////////////////////////////////////////////////////////////////

CMagneticTitleDlg::CMagneticTitleDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CMagneticTitleDlg::IDD, pParent)
{
  m_dpi = 96;
}

void CMagneticTitleDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CMagneticTitleDlg)
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMagneticTitleDlg, CDialog)
  //{{AFX_MSG_MAP(CMagneticTitleDlg)
  ON_WM_KEYDOWN()
  ON_WM_LBUTTONDOWN()
  ON_WM_PAINT()
  //}}AFX_MSG_MAP
  ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMagneticTitleDlg message handlers
/////////////////////////////////////////////////////////////////////////////

BOOL CMagneticTitleDlg::OnInitDialog() 
{
  CDialog::OnInitDialog();

  m_dpi = DPI::getWindowDPI(this);
  SizeTitle();
  return TRUE;
}

void CMagneticTitleDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
  CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
  OnOK();
}

void CMagneticTitleDlg::OnLButtonDown(UINT nFlags, CPoint point) 
{
  CDialog::OnLButtonDown(nFlags, point);
  OnOK();
}

void CMagneticTitleDlg::OnPaint() 
{
  CPaintDC dc(this);

  CRect clientR;
  GetClientRect(clientR);

  if (!m_ScaledPicture.Pixels() || (m_ScaledPicture.Size() != clientR.Size()))
    m_ScaledPicture.Scale(m_Picture,clientR.Size());
  m_ScaledPicture.Draw(&dc,CPoint(0,0));
}

LRESULT CMagneticTitleDlg::OnDpiChanged(WPARAM wparam, LPARAM)
{
  int newDpi = (int)HIWORD(wparam);
  if (m_dpi != newDpi)
  {
    m_dpi = newDpi;
    SizeTitle();
    Invalidate();
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// Call to show the dialog
/////////////////////////////////////////////////////////////////////////////

void CMagneticTitleDlg::ShowTitle(LPCTSTR pszGamePath)
{
  CString strGamePath(pszGamePath);
  if (strGamePath.GetLength() > 4)
  {
    CString strTitlePic = strGamePath.Left(strGamePath.GetLength()-4);
    strTitlePic += ".png";

    if (m_Picture.LoadFile(strTitlePic))
    {
      StartMusic(pszGamePath);
      DoModal();
      StopMusic();
    }
  }
}

void CMagneticTitleDlg::SizeTitle(void)
{
  CRect screen = DPI::getMonitorRect(GetParent());
  CSize picSize = m_Picture.Size();

  double scaleW = (screen.Width() * m_Picture.AspectRatio()) / picSize.cx;
  double scaleH = (double)screen.Height() / picSize.cy;
  double scale = (scaleW < scaleH) ? scaleW : scaleH;
  int width = (int)(picSize.cx * scale * m_Picture.AspectRatio() * 0.6);
  int height = (int)(picSize.cy * scale * 0.6);

  // Adjust for the borders
  CRect wndR, clientR;
  GetWindowRect(wndR);
  GetClientRect(clientR);
  width += (wndR.Width() - clientR.Width());
  height += (wndR.Height() - clientR.Height());

  MoveWindow(
    screen.left + ((screen.Width() - width) / 2),
    screen.top + ((screen.Height() - height) / 2),
    width,height);
}

/////////////////////////////////////////////////////////////////////////////
// Start and stop playing the title MP3 audio file
/////////////////////////////////////////////////////////////////////////////

void CMagneticTitleDlg::StartMusic(LPCTSTR pszGamePath)
{
  CString strGamePath(pszGamePath);

  if (strGamePath.GetLength() > 4)
  {
    CString strMusicPath;
    strMusicPath = strGamePath.Left(strGamePath.GetLength()-4);
    strMusicPath += ".mp3";

    if (::GetFileAttributes(strMusicPath) != INVALID_FILE_ATTRIBUTES)
    {
      CString cmd;
      cmd.Format("open \"%s\" type mpegvideo alias title",(LPCSTR)strMusicPath);
      MCIERROR err = mciSendString(cmd,NULL,0,0);
      if (err == 0)
        mciSendString("play title",NULL,0,0);
    }
  }
}

void CMagneticTitleDlg::StopMusic()
{
  mciSendString("stop title",NULL,0,0);
  mciSendString("close title",NULL,0,0);
}
