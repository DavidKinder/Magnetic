/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// PictureWnd.cpp: Implementation of the picture window
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#include "Magnetic.h"
#include "PictureWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPictureWnd
/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CPictureWnd, CWnd)
	//{{AFX_MSG_MAP(CPictureWnd)
	ON_WM_PAINT()
	ON_WM_CHAR()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	ON_WM_PALETTECHANGED()
	ON_WM_QUERYNEWPALETTE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CPictureWnd::CPictureWnd() : m_Borders(0,0)
{
	m_pBitmapInfo = NULL;
	m_pBitmap = NULL;
	m_pPalette = NULL;
	m_pMagneticWnd = NULL;
}

CPictureWnd::~CPictureWnd()
{
	if (m_pBitmapInfo)
		delete m_pBitmapInfo;
	if (m_pBitmap)
		delete m_pBitmap;
	if (m_pPalette)
		delete m_pPalette;
}

BOOL CPictureWnd::CreatePicWnd(CWnd* pParent, const RECT& rect)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	m_pMagneticWnd = pParent;
	BOOL bCreate = CreateEx(0,AfxRegisterWndClass(0),"Magnetic",
													WS_CAPTION|WS_VISIBLE|WS_SYSMENU,
													rect,pParent,0,NULL);

	SetIcon(pApp->LoadIcon(IDR_MAINFRAME),TRUE);

	// Get the horizontal and vertical border sizes
	CRect r1, r2;
	GetWindowRect(r1);
	GetClientRect(r2);
	m_Borders.cx = r1.Width() - r2.Width();
	m_Borders.cy = r1.Height() - r2.Height();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPictureWnd message handlers
/////////////////////////////////////////////////////////////////////////////

void CPictureWnd::OnPaint() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CPaintDC dcWnd(this); // device context for painting

	if (m_pPalette)
	{
		dcWnd.SelectPalette(m_pPalette,FALSE);
		dcWnd.RealizePalette();
	}

	if (m_pBitmap && m_pBitmapInfo)
	{
		CRect rClient;
		GetClientRect(rClient);

		::StretchDIBits(dcWnd.m_hDC,
		  0,0,
		  rClient.Width(),
		  rClient.Height(),
		  0,0,
		  m_PictureSize.cx,
		  m_PictureSize.cy,
		  (LPVOID)m_pBitmap,m_pBitmapInfo,
		  DIB_RGB_COLORS,SRCCOPY);
	}
}

void CPictureWnd::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// Send the key to the Magnetic window
	if (m_pMagneticWnd)
		m_pMagneticWnd->SendMessage(WM_CHAR,nChar,nRepCnt|(nFlags<<16));
	
	CWnd::OnChar(nChar, nRepCnt, nFlags);
}

void CPictureWnd::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// Send the key to the Magnetic window
	if (m_pMagneticWnd)
		m_pMagneticWnd->SendMessage(WM_KEYDOWN,nChar,nRepCnt|(nFlags<<16));
	
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CPictureWnd::OnDestroy() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	// Store the position of the top-left corner of the window.
	CRect rWnd;
	CPoint& TopLeft = pApp->GetPicTopLeft();
	GetWindowRect(rWnd);

	TopLeft = rWnd.TopLeft();

	CWnd::OnDestroy();
}

void CPictureWnd::OnPaletteChanged(CWnd* pFocusWnd) 
{
	CDC* pDC = GetDC();

	if (m_pPalette)
	{
		pDC->SelectPalette(m_pPalette,FALSE);
		if (pDC->RealizePalette())
			Invalidate(FALSE);
	}
	ReleaseDC(pDC);
}

BOOL CPictureWnd::OnQueryNewPalette() 
{
	CDC* pDC = GetDC();
	int iColours = 0;

	if (m_pPalette)
	{
		pDC->SelectPalette(m_pPalette,FALSE);
		if (iColours = pDC->RealizePalette())
			Invalidate(FALSE);
	}
	ReleaseDC(pDC);
	return iColours;
}

/////////////////////////////////////////////////////////////////////////////
// CPictureWnd interpreter interface
/////////////////////////////////////////////////////////////////////////////

void CPictureWnd::NewPicture(int iWidth, int iHeight, unsigned char* pBuffer, unsigned short* pPalette)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	if (m_pBitmapInfo)
	{
		delete m_pBitmapInfo;
		m_pBitmapInfo = NULL;
	}

	m_pBitmapInfo = (BITMAPINFO*)
		(new char[sizeof(BITMAPINFOHEADER)+(16*sizeof(RGBQUAD))]);
	if (m_pBitmapInfo == NULL)
		return;

	// The bitmap data must be long-word aligned
	int iModulo = iWidth % 4;
	int iDibWidth = iWidth + ((iModulo > 0) ? (4-iModulo) : iModulo);

	m_pBitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_pBitmapInfo->bmiHeader.biWidth = iDibWidth;
	m_pBitmapInfo->bmiHeader.biHeight = iHeight * -1;	// top-down bitmap
	m_pBitmapInfo->bmiHeader.biPlanes = 1;
	m_pBitmapInfo->bmiHeader.biBitCount = 8;
	m_pBitmapInfo->bmiHeader.biCompression = BI_RGB;
	m_pBitmapInfo->bmiHeader.biSizeImage = 0;
	m_pBitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	m_pBitmapInfo->bmiHeader.biClrUsed = 0;
	m_pBitmapInfo->bmiHeader.biClrImportant = 0;

	LOGPALETTE* pLogPalette = (LPLOGPALETTE)
		(new char[sizeof(LOGPALETTE)+(16*sizeof(PALETTEENTRY))]);

	pLogPalette->palVersion = 0x300;
	pLogPalette->palNumEntries = 16;

	for (int i = 0; i < 16; i++)
	{
		m_pBitmapInfo->bmiColors[i].rgbRed   = (pPalette[i]&0x0F00)>>3;
		m_pBitmapInfo->bmiColors[i].rgbGreen = (pPalette[i]&0x00F0)<<1;
		m_pBitmapInfo->bmiColors[i].rgbBlue  = (pPalette[i]&0x000F)<<5;
		m_pBitmapInfo->bmiColors[i].rgbReserved = 0;
		pLogPalette->palPalEntry[i].peRed = m_pBitmapInfo->bmiColors[i].rgbRed;
		pLogPalette->palPalEntry[i].peGreen = m_pBitmapInfo->bmiColors[i].rgbGreen;
		pLogPalette->palPalEntry[i].peBlue = m_pBitmapInfo->bmiColors[i].rgbBlue;
		pLogPalette->palPalEntry[i].peFlags = 0;
	}

	if (m_pBitmap)
	{
		delete m_pBitmap;
		m_pBitmap = NULL;
	}

	m_pBitmap = new char[iDibWidth*iHeight*8];
	if (m_pBitmap == NULL)
		return;

	for (i = 0; i < iHeight; i++)
	{
		for (int j = 0; j < iDibWidth; j++)
		{
			if (j < iWidth)
				m_pBitmap[(i*iDibWidth)+j] = pBuffer[(i*iWidth)+j];
			else
				m_pBitmap[(i*iDibWidth)+j] = 0;
		}
	}

	if (m_pPalette)
	{
		delete m_pPalette;
		m_pPalette = NULL;
	}

	m_pPalette = new CPalette;
	if (m_pPalette == NULL)
		return;

	m_pPalette->CreatePalette(pLogPalette);
	delete pLogPalette;

	// Store the bitmap size
	m_PictureSize = CSize(iWidth,iHeight);

	// Resize the window to fit the picture and then repaint
	double dScale = pApp->m_dScaleFactor;
	CRect rWnd;
	GetWindowRect(rWnd);
	rWnd.right = rWnd.left + (int)(iWidth * dScale) + m_Borders.cx;
	rWnd.bottom = rWnd.top + (int)(iHeight * dScale) + m_Borders.cy;
	MoveWindow(rWnd,TRUE);
	Invalidate();
}
