/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 2
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// Magnetic.cpp: Implementation of application class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <string.h>
#include <stdlib.h>
#include <memory>

#include "Magnetic.h"
#include "MagneticDoc.h"
#include "MagneticView.h"
#include "MainFrm.h"
#include "OptionsDlg.h"
#include "Dialogs.h"

#include "png.h"

#pragma warning(disable : 4611)

namespace {
#include "2PassScale.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BOOL AFXAPI _AfxSetRegKey(LPCTSTR lpszKey, LPCTSTR lpszValue, LPCTSTR lpszValueName = NULL);
void AFXAPI AfxGetModuleShortFileName(HINSTANCE hInst, CString& strShortName);

/////////////////////////////////////////////////////////////////////////////
// Implementation of CMagneticApp
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CMagneticApp, CWinApp)
  //{{AFX_MSG_MAP(CMagneticApp)
  ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
  ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
  ON_COMMAND(ID_VIEW_FONT, OnViewFont)
  ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
  ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMagneticApp theApp;

CMagneticApp::CMagneticApp()
{
  EnableHtmlHelp();
}

BOOL CMagneticApp::InitInstance()
{
  ::CoInitialize(NULL);
  AfxEnableControlContainer();
  AfxInitRichEdit2();

  SetRegistryKey(_T("David Kinder"));
  LoadStdProfileSettings();

  NONCLIENTMETRICS ncm;
  ::ZeroMemory(&ncm,sizeof ncm);
  ncm.cbSize = sizeof ncm;
  ::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof ncm,&ncm,0);

  CRect screen = DPI::getMonitorRect(CWnd::GetDesktopWindow());
  int scalePics = 100;
  int scaleTitles = 100;
  if ((screen.Width() > 800) && (screen.Height() > 600))
  {
    scalePics = 200;
    scaleTitles = 150;
  }

  // Load Magnetic display settings
  int fontSize = GetProfileInt("Display","Font Size",10);
  if (fontSize < 0)
    m_iFontPoints = -MulDiv(fontSize,72,DPI::getSystemDPI());
  else
    m_iFontPoints = fontSize;
  m_LogFont.lfHeight = -MulDiv(m_iFontPoints,DPI::getSystemDPI(),72);
  m_LogFont.lfCharSet = ANSI_CHARSET;
  m_LogFont.lfOutPrecision = OUT_TT_PRECIS;
  m_LogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  m_LogFont.lfQuality = PROOF_QUALITY;
  m_LogFont.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
  strncpy(m_LogFont.lfFaceName,GetProfileString(
    "Display","Font Name",ncm.lfMessageFont.lfFaceName),LF_FACESIZE);

  SetRedrawStatus(Redraw::NoRedraw);

  m_WindowRect.left = GetProfileInt("Window","Left",0);
  m_WindowRect.top = GetProfileInt("Window","Top",0);
  m_WindowRect.right = GetProfileInt("Window","Right",0);
  m_WindowRect.bottom = GetProfileInt("Window","Bottom",0);
  m_iWindowMax = GetProfileInt("Window","Maximized",0);
  if (m_iWindowMax)
    m_nCmdShow = SW_SHOWMAXIMIZED;
  m_bToolBar = GetProfileInt("Window","Toolbar",1) ? TRUE : FALSE;
  m_bStatusBar = GetProfileInt("Window","Status Bar",1) ? TRUE : FALSE;

  m_PicTopLeft.x = GetProfileInt("Picture","Left",0);
  m_PicTopLeft.y = GetProfileInt("Picture","Top",0);

  m_ShowGfx = (ShowGraphics)GetProfileInt("Picture","Show",
    ShowGraphics::MainWindow);
  m_dScaleFactor = (double)GetProfileInt("Picture","Scale",scalePics)*0.01;
  m_dScaleTitles = (double)GetProfileInt("Titles","Scale",scaleTitles)*0.01;
  m_dGamma = (double)GetProfileInt("Picture","Gamma",100)*0.01;
  m_ForeColour = GetProfileInt("Display","Foreground",~0);
  m_BackColour = GetProfileInt("Display","Background",~0);
  m_GfxColour = GetProfileInt("Display","Graphics",~0);
  m_bHintWindow = GetProfileInt("Hints","Use hint window",1);
  m_bAnimWait = GetProfileInt("Debug","Wait for animations",0);
  m_bPredict = GetProfileInt("Debug","Predictable",0);
  m_iSeed = GetProfileInt("Debug","Seed",0);

  m_HintsRect.left = GetProfileInt("Hints","Left",0);
  m_HintsRect.top = GetProfileInt("Hints","Top",0);
  m_HintsRect.right = GetProfileInt("Hints","Right",0);
  m_HintsRect.bottom = GetProfileInt("Hints","Bottom",0);

  m_iGameLoaded = 0;

  CSingleDocTemplate* pDocTemplate;
  pDocTemplate = new CSingleDocTemplate(
    IDR_MAINFRAME,
    RUNTIME_CLASS(CMagneticDoc),
    RUNTIME_CLASS(CMainFrame),
    RUNTIME_CLASS(CMagneticView));
  AddDocTemplate(pDocTemplate);

  EnableShellOpen();
  RegisterShellFileTypes(FALSE);

  // Set up the icon for Magnetic game files
  CString path, type, key, value;
  AfxGetModuleShortFileName(AfxGetInstanceHandle(),path);
  pDocTemplate->GetDocString(type,CDocTemplate::regFileTypeId);
  key.Format("%s\\DefaultIcon",type);
  value.Format("%s,%d",path,0);
  _AfxSetRegKey(key,value);

  // Notify the shell that associations have changed
  ::SHChangeNotify(SHCNE_ASSOCCHANGED,SHCNF_IDLIST,0,0);

  // Create file dialog for loading games
  m_pNewGameDialog = new SimpleFileDialog(TRUE,NULL,
    GetProfileString("Settings","Last File",""),
    OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING,
    "Magnetic Files (*.mag)|*.mag|All Files (*.*)|*.*||",NULL);
  if (m_pNewGameDialog == NULL)
    return FALSE;
  m_pNewGameDialog->m_ofn.lpstrTitle = "Open a Magnetic Scrolls game";

  CCommandLineInfo cmdInfo;
  ParseCommandLine(cmdInfo);

  if (!ProcessShellCommand(cmdInfo))
    return FALSE;

  // Create font dialog
  m_pFontDialog = new DPI::FontDialog(&m_LogFont,CF_SCREENFONTS,m_pMainWnd);
  if (m_pFontDialog == NULL)
    return FALSE;

  m_pMainWnd->ShowWindow(SW_SHOW);
  m_pMainWnd->UpdateWindow();
  m_pMainWnd->DragAcceptFiles();
  return TRUE;
}

int CMagneticApp::ExitInstance() 
{
  // Write out settings
  WriteProfileString("Settings","Last File",m_pNewGameDialog->GetPathName());

  WriteProfileString("Display","Font Name",CString(m_LogFont.lfFaceName));
  WriteProfileInt("Display","Font Size",m_iFontPoints);
  WriteProfileInt("Display","Foreground",m_ForeColour);
  WriteProfileInt("Display","Background",m_BackColour);
  WriteProfileInt("Display","Graphics",m_GfxColour);

  WriteProfileInt("Hints","Use hint window",m_bHintWindow);

  WriteProfileInt("Debug","Wait for animations",m_bAnimWait);
  WriteProfileInt("Debug","Predictable",m_bPredict);
  WriteProfileInt("Debug","Seed",m_iSeed);

  WriteProfileInt("Window","Left",m_WindowRect.left);
  WriteProfileInt("Window","Top",m_WindowRect.top);
  WriteProfileInt("Window","Right",m_WindowRect.right);
  WriteProfileInt("Window","Bottom",m_WindowRect.bottom);
  WriteProfileInt("Window","Maximized",m_iWindowMax);
  WriteProfileInt("Window","Toolbar",m_bToolBar ? 1 : 0);
  WriteProfileInt("Window","Status Bar",m_bStatusBar ? 1 : 0);

  WriteProfileInt("Picture","Left",m_PicTopLeft.x);
  WriteProfileInt("Picture","Top",m_PicTopLeft.y);
  WriteProfileInt("Picture","Show",m_ShowGfx);
  WriteProfileInt("Picture","Scale",(int)(m_dScaleFactor*100));
  WriteProfileInt("Titles","Scale",(int)(m_dScaleTitles*100));
  WriteProfileInt("Picture","Gamma",(int)(m_dGamma*100));

  WriteProfileInt("Hints","Left",m_HintsRect.left);
  WriteProfileInt("Hints","Top",m_HintsRect.top);
  WriteProfileInt("Hints","Right",m_HintsRect.right);
  WriteProfileInt("Hints","Bottom",m_HintsRect.bottom);

  // Free memory
  delete m_pNewGameDialog;
  delete m_pFontDialog;
  ms_freemem();

  return CWinApp::ExitInstance();
}

BOOL CMagneticApp::OnIdle(LONG lCount)
{
  BOOL bIdle = CWinApp::OnIdle(lCount);

  // Run interpeter opcodes. If the interpreter is running,
  // take more idle time.
  if (ms_rungame())
    bIdle = TRUE;
  else
  {
    if (m_iGameLoaded)
    {
      OnFileNew();
      CMagneticView* pView = CMagneticView::GetView();
      if (pView)
        pView->ClearAll();
    }
  }

  return bIdle;
}

void CMagneticApp::OnFileOpen() 
{
  if (ms_is_running())
    SetRedrawStatus(Redraw::ThisLine);

  if (m_pNewGameDialog->DoModal() == IDOK)
  {
    if (ms_is_running())
      SetRedrawStatus(Redraw::EndLine);
    OpenDocumentFile(m_pNewGameDialog->GetPathName());
  }
}

void CMagneticApp::OnViewFont() 
{
  SetRedrawStatus(Redraw::ThisLine);

  // Change the display font
  if (m_pFontDialog->DoModal() == IDOK)
  {
    m_iFontPoints = m_pFontDialog->m_cf.iPointSize / 10;

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
    {
      pView->TextClearup();
      pView->TextSetup();
      pView->Invalidate();
    }
  }
}

void CMagneticApp::OnViewOptions() 
{
  SetRedrawStatus(Redraw::ThisLine);

  COptionsDlg Options;

  // Set up initial values in the dialog
  Options.m_iShowPics = m_ShowGfx;
  Options.m_dScaleFactor = m_dScaleFactor;
  Options.m_dScaleTitles = m_dScaleTitles;
  Options.m_dGamma = m_dGamma;
  Options.m_bHintWindow = m_bHintWindow;
  Options.m_bAnimWait = m_bAnimWait;
  Options.m_bPredict = m_bPredict;
  Options.m_iSeed = m_iSeed;

  if (Options.DoModal() == IDOK)
  {
    m_ShowGfx = (ShowGraphics)Options.m_iShowPics;
    m_dScaleFactor = Options.m_dScaleFactor;
    m_dScaleTitles = Options.m_dScaleTitles;
    m_dGamma = Options.m_dGamma;
    m_bHintWindow = Options.m_bHintWindow;
    m_bAnimWait = Options.m_bAnimWait;
    m_bPredict = Options.m_bPredict;
    m_iSeed = Options.m_iSeed;

    m_ForeColour = Options.GetForeColour();
    if (m_ForeColour == GetSysColor(COLOR_WINDOWTEXT))
      m_ForeColour = (COLORREF)~0;
    m_BackColour = Options.GetBackColour();
    if (m_BackColour == GetSysColor(COLOR_WINDOW))
      m_BackColour = (COLORREF)~0;
    m_GfxColour = Options.GetGfxColour();
    if (m_GfxColour == GetSysColor(COLOR_APPWORKSPACE))
      m_GfxColour = (COLORREF)~0;

    CMagneticView* pView = CMagneticView::GetView();
    if (pView)
      pView->SetPictureWindowState();
  }
}

void CMagneticApp::OnUpdateRecentFileMenu(CCmdUI* pCmdUI) 
{
  int nID = pCmdUI->m_nID;
  CWinApp::OnUpdateRecentFileMenu(pCmdUI);

  if (m_pRecentFileList == NULL)
    return;
  if (pCmdUI->m_pMenu == NULL)
    return;

  CMagneticView* pView = CMagneticView::GetView();
  if (pView)
  {
    if (pView->GetMorePrompt())
    {
      for (int iMRU = 0; iMRU < m_pRecentFileList->m_nSize; iMRU++)
        pCmdUI->m_pMenu->EnableMenuItem(nID + iMRU, MF_DISABLED|MF_GRAYED);
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
// Get and set program settings
/////////////////////////////////////////////////////////////////////////////

LOGFONT* CMagneticApp::GetLogFont(void)
{
  return &m_LogFont;
}

int CMagneticApp::GetFontPoints(void)
{
  return m_iFontPoints;
}

void CMagneticApp::SetRedrawStatus(Redraw Status)
{
  m_RedrawStatus = Status;
}

CMagneticApp::Redraw CMagneticApp::GetRedrawStatus(void)
{
  Redraw Return = m_RedrawStatus;
  m_RedrawStatus = Redraw::NoRedraw;
  return Return;
}

COLORREF CMagneticApp::GetForeColour(void)
{
  COLORREF Colour;

  if (m_ForeColour == ~0)
    Colour = GetSysColor(COLOR_WINDOWTEXT);
  else
    Colour = m_ForeColour;

  return Colour;
}

COLORREF CMagneticApp::GetBackColour(void)
{
  COLORREF Colour;

  if (m_BackColour == ~0)
    Colour = GetSysColor(COLOR_WINDOW);
  else
    Colour = m_BackColour;

  return Colour;
}

COLORREF CMagneticApp::GetGfxColour(void)
{
  COLORREF Colour;

  if (m_GfxColour == ~0)
    Colour = GetSysColor(COLOR_APPWORKSPACE);
  else
    Colour = m_GfxColour;

  return Colour;
}

CRect& CMagneticApp::GetWindowRect(void)
{
  return m_WindowRect;
}

int& CMagneticApp::GetWindowMax(void)
{
  return m_iWindowMax;
}

CPoint& CMagneticApp::GetPicTopLeft(void)
{
  return m_PicTopLeft;
}

void CMagneticApp::GetControlBars(BOOL& bToolBar, BOOL& bStatusBar)
{
  bToolBar = m_bToolBar;
  bStatusBar = m_bStatusBar;
}

void CMagneticApp::SetControlBars(BOOL bToolBar, BOOL bStatusBar)
{
  m_bToolBar = bToolBar;
  m_bStatusBar = bStatusBar;
}

CRect& CMagneticApp::GetHintsRect(void)
{
  return m_HintsRect;
}

CMagneticApp::ShowGraphics CMagneticApp::GetShowGraphics(void)
{
  return m_ShowGfx;
}

void CMagneticApp::SetShowGraphics(ShowGraphics Show)
{
  m_ShowGfx = Show;
}

double CMagneticApp::GetScaleFactor(void)
{
  return m_dScaleFactor;
}

double CMagneticApp::GetScaleTitles(void)
{
  return m_dScaleTitles;
}

BOOL CMagneticApp::GetUseHintWindow(void)
{
  return m_bHintWindow;
}

BOOL CMagneticApp::GetAnimWait(void)
{
  return m_bAnimWait;
}

BOOL CMagneticApp::GetPredictable(void)
{
  return m_bPredict;
}

int CMagneticApp::GetRandomSeed(void)
{
  return m_iSeed;
}

double CMagneticApp::GetGamma(void)
{
  return m_dGamma;
}

int CMagneticApp::GetGameLoaded(void)
{
  return m_iGameLoaded;
}

void CMagneticApp::SetGameLoaded(int iLoaded)
{
  m_iGameLoaded = iLoaded;
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog class
/////////////////////////////////////////////////////////////////////////////

class CLogoStatic : public CStatic
{
public:
  CLogoStatic();
  ~CLogoStatic();

  virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

private:
  void GetBitmap(const CSize& size);

  BYTE* m_logoPixels;
  CSize m_logoSize;
  BYTE* m_scaledPixels;
  CSize m_scaledSize;
};

CLogoStatic::CLogoStatic()
{
  m_logoPixels = NULL;
  m_scaledPixels = NULL;
}

CLogoStatic::~CLogoStatic()
{
  delete[] m_logoPixels;
  delete[] m_scaledPixels;
}

void CLogoStatic::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
  CDC* dc = CDC::FromHandle(lpDrawItemStruct->hDC);
  CRect r(lpDrawItemStruct->rcItem);

  GetBitmap(r.Size());

  BITMAPINFOHEADER bitmapInfo = { 0 };
  bitmapInfo.biSize = sizeof bitmapInfo;
  bitmapInfo.biWidth = m_scaledSize.cx;
  bitmapInfo.biHeight = m_scaledSize.cy*-1;
  bitmapInfo.biPlanes = 1;
  bitmapInfo.biBitCount = 32;
  bitmapInfo.biCompression = BI_RGB;
  ::StretchDIBits(dc->GetSafeHdc(),r.left,r.top,m_scaledSize.cx,m_scaledSize.cy,
    0,0,m_scaledSize.cx,m_scaledSize.cy,
    m_scaledPixels,(LPBITMAPINFO)&bitmapInfo,DIB_RGB_COLORS,SRCCOPY);
}

struct PngDataIO
{
  BYTE* data;
  ULONG offset;

  static void Read(png_structp png_ptr, png_bytep data, png_size_t length)
  {
    PngDataIO* dataIO = (PngDataIO*)png_get_io_ptr(png_ptr);
    memcpy(data,dataIO->data+dataIO->offset,length);
    dataIO->offset += length;
  }
};

void CLogoStatic::GetBitmap(const CSize& size)
{
  if (m_logoPixels == NULL)
  {
    // Get the raw PNG logo data
    HRSRC res = ::FindResource(NULL,MAKEINTRESOURCE(IDR_LOGO),"PICTURE");
    if (!res)
      return;
    HGLOBAL resData = ::LoadResource(NULL,res);
    if (!resData)
      return;
    BYTE* logoData = (BYTE*)::LockResource(resData);
    if (!logoData)
      return;

    if (!png_check_sig(logoData,8))
      return;
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if (!png_ptr)
      return;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
    {
      png_destroy_read_struct(&png_ptr,NULL,NULL);
      return;
    }
    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info)
    {
      png_destroy_read_struct(&png_ptr,&info_ptr,(png_infopp)NULL);
      return;
    }
    png_bytep* pixelRows = NULL;
    if (setjmp(png_jmpbuf(png_ptr)))
    {
      png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
      delete[] pixelRows;
      return;
    }

    // Read the logo image as an array of RGBA pixels
    PngDataIO data;
    data.data = logoData;
    data.offset = 8;
    png_set_read_fn(png_ptr,&data,PngDataIO::Read);
    png_set_sig_bytes(png_ptr,8);
    png_read_info(png_ptr,info_ptr);
    m_logoSize.cx = png_get_image_width(png_ptr,info_ptr);
    m_logoSize.cy = png_get_image_height(png_ptr,info_ptr);
    int bit_depth = png_get_bit_depth(png_ptr,info_ptr);
    int color_type = png_get_color_type(png_ptr,info_ptr);

    if (color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
      png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (bit_depth == 16)
      png_set_strip_16(png_ptr);
    if (bit_depth < 8)
      png_set_packing(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png_ptr);

    png_color_16 white = { 0,255,255,255,0 };
    png_set_background(png_ptr,&white,PNG_BACKGROUND_GAMMA_SCREEN,0,1.0);
    png_set_bgr(png_ptr);
    png_set_filler(png_ptr,0,PNG_FILLER_AFTER);

    m_logoPixels = new BYTE[m_logoSize.cx*m_logoSize.cy*4];
    pixelRows = new png_bytep[m_logoSize.cy];
    for (int i = 0; i < (int)m_logoSize.cy; i++)
      pixelRows[i] = m_logoPixels+(m_logoSize.cx*i*4);
    png_read_image(png_ptr,pixelRows);
    png_read_end(png_ptr,end_info);
    png_destroy_read_struct(&png_ptr,&info_ptr,&end_info);
    delete[] pixelRows;
  }

  if ((m_scaledPixels == NULL) || (m_scaledSize != size))
  {
    delete[] m_scaledPixels;
    m_scaledPixels = new BYTE[size.cx*size.cy*4];
    m_scaledSize = size;

    // Scale the bitmap
    TwoPassScale<BilinearFilter> scaler;
    scaler.Scale((COLORREF*)m_logoPixels,m_logoSize.cx,m_logoSize.cy,
      (COLORREF*)m_scaledPixels,m_scaledSize.cx,m_scaledSize.cy);
  }
}

class CAboutDlg : public BaseDialog
{
public:
  CAboutDlg();

// Dialog Data
  //{{AFX_DATA(CAboutDlg)
  enum { IDD = IDD_ABOUTBOX };
  //}}AFX_DATA

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CAboutDlg)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  // Generated message map functions
  //{{AFX_MSG(CAboutDlg)
  virtual BOOL OnInitDialog();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

  CLogoStatic m_logo;
};

CAboutDlg::CAboutDlg() : BaseDialog(CAboutDlg::IDD)
{
  //{{AFX_DATA_INIT(CAboutDlg)
  //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
  BaseDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CAboutDlg)
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, BaseDialog)
  //{{AFX_MSG_MAP(CAboutDlg)
    // No message handlers
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog() 
{
  BaseDialog::OnInitDialog();

  // Subclass the static logo bitmap control
  if (m_logo.SubclassDlgItem(IDC_LOGO,this) == FALSE)
    return FALSE;
  CRect logoRect;
  m_logo.GetWindowRect(logoRect);
  ScreenToClient(logoRect);
  double aspect = ((double)logoRect.Width())/logoRect.Height();

  // Get the credits group control
  CRect creditsRect;
  CWnd* creditsWnd = GetDlgItem(IDC_CREDITS);
  creditsWnd->GetWindowRect(creditsRect);
  ScreenToClient(creditsRect);

  // Resize the logo
  logoRect.right = creditsRect.left-logoRect.left;
  logoRect.bottom = logoRect.top+(int)(logoRect.Width()/aspect);
  m_logo.MoveWindow(logoRect);
  return TRUE;
}

// App command to run the dialog
void CMagneticApp::OnAppAbout()
{
  CAboutDlg AboutDlg;
  AboutDlg.DoModal();
}
