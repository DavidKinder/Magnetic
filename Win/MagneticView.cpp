/////////////////////////////////////////////////////////////////////////////
//
// Magnetic 1.0
// Magnetic Scrolls Interpreter
//
// Visual C++ MFC Windows interface by David Kinder
//
// MagneticView.cpp: Implementation of the view class
//
/////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <stdio.h>
#include <string.h>

#include "Magnetic.h"
#include "MagneticDoc.h"
#include "MagneticView.h"
#include "MagneticTitle.h"

extern "C"
{
#include "defs.h"
extern type8 running;
extern type8 lastchar;
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

char GetInput(void);

/////////////////////////////////////////////////////////////////////////////
// Implementation of CMagneticView
/////////////////////////////////////////////////////////////////////////////

// Class constants
const int CMagneticView::SPECIAL_KEYS = 128;
const int CMagneticView::MAX_LINES = 64;

IMPLEMENT_DYNCREATE(CMagneticView, CView)

BEGIN_MESSAGE_MAP(CMagneticView, CView)
	//{{AFX_MSG_MAP(CMagneticView)
	ON_WM_CHAR()
	ON_WM_KEYDOWN()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_COMMAND(ID_FILE_RECORD, OnRecord)
	ON_COMMAND(ID_FILE_PLAYBACK, OnPlayback)
	ON_COMMAND(ID_FILE_SCRIPT, OnScript)
	ON_UPDATE_COMMAND_UI(ID_FILE_RECORD, OnUpdateRecord)
	ON_UPDATE_COMMAND_UI(ID_FILE_PLAYBACK, OnUpdatePlayback)
	ON_UPDATE_COMMAND_UI(ID_FILE_SCRIPT, OnUpdateScript)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPEN, OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FONT, OnUpdateViewFont)
	ON_COMMAND(ID_VIEW_SCROLLBACK, OnScrollback)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_VIEW_OPTIONS, OnUpdateViewOptions)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CMagneticView::CMagneticView() :
	m_strRecName("Magnetic.rec"),
	m_strScrName("Magnetic.scr")
{
	m_pOldFont = NULL;
	m_pTextFont = NULL;
	m_pTextDC = NULL;
	m_iMorePrompt = FALSE;
	m_pFileRecord = NULL;
	m_pFileScript = NULL;
}

CMagneticView::~CMagneticView()
{
}

BOOL CMagneticView::PreCreateWindow(CREATESTRUCT& cs)
{
	ClearAll();
	return CView::PreCreateWindow(cs);
}


int CMagneticView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	TextSetup();
	return 0;
}

void CMagneticView::OnDestroy() 
{
	TextClearup();
	CView::OnDestroy();
}

void CMagneticView::OnDraw(CDC* pDrawDC)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	// Don't draw anything if there isn't a loaded game
	if (pApp->m_iGameLoaded == 0)
		return;

	CDC* pDC = new CDC();
	pDC->CreateCompatibleDC(pDrawDC);

	CRect Client;
	GetClientRect(Client);

	CBitmap bmp;
	CSize size(Client.Width(),Client.Height());
	if (bmp.CreateCompatibleBitmap(pDrawDC,size.cx,size.cy) == FALSE)
	{
		delete pDC;
		return;
	}
	CBitmap* pbmpOld = pDC->SelectObject(&bmp);

	pDC->FillSolidRect(Client,pApp->GetBackColour());

	int y = 0;

	// Set up the font
	CFont* pOldFont = NULL;
	TEXTMETRIC FontInfo;

	pOldFont = pDC->SelectObject(m_pTextFont);
	pDC->GetTextMetrics(&FontInfo);
	int iFontHeight = (int)(FontInfo.tmHeight*1.1);

	// Display the status line
	LONG lStatSpace = (LONG)(Client.Width()*0.075);
	SolidRect(pDC,CRect(Client.left,0,Client.right,iFontHeight),pApp->GetForeColour());

	pDC->SetTextColor(pApp->GetBackColour());
	pDC->SetBkColor(pApp->GetForeColour());
	
	pDC->TextOut(lStatSpace,y,m_strStatLocation,
														m_strStatLocation.GetLength());
	pDC->TextOut(lStatSpace*10,y,m_strStatScore,
															 m_strStatScore.GetLength());

	// Repaginate if necessary
	TrimOutput();
	if (m_PageTable.GetSize() == 0)
	{
		Paginate(pDC,0,0);
		if (m_iMorePrompt)
			m_PageTable.RemoveAt(m_PageTable.GetSize()-1);
	}
	LPCSTR lpszOutput = m_strOutput;

	pDC->SetTextColor(pApp->GetForeColour());
	pDC->SetBkColor(pApp->GetBackColour());

	// Work out the number of lines of text to draw
	m_iMaxLines = (Client.Height()-(2*pApp->GetMargins().cy)) / iFontHeight;
	m_iMaxLines -=1;	// Account for the status line

	y += iFontHeight + pApp->GetMargins().cy;

	// Starting position in the text output buffer
	int i = m_PageTable.GetSize() - m_iMaxLines - 1;
	if (i < 0) i = 0;

	// Draw the text
	while (i < m_PageTable.GetSize()-1)
	{
		pDC->TextOut(pApp->GetMargins().cx,y,lpszOutput+m_PageTable[i],
								 m_PageTable[i+1]-m_PageTable[i]-1);
		y += iFontHeight;
		i++;
	}

	// Store information on the last line for updating later
	CRect LastLine(0,y-iFontHeight,Client.Width(),y);
	m_LastLineRect = LastLine;

	// Clear the end of the last line to allow input editing
	if (i > 0)
	{
		CSize TextLen = pDC->GetTextExtent(lpszOutput+m_PageTable[i-1],
																			 m_PageTable[i]-m_PageTable[i-1]-1);
		LastLine.left += TextLen.cx + pApp->GetMargins().cx;

		SolidRect(pDC,LastLine,pApp->GetBackColour());
	}

	// Remove the font
	pDC->SelectObject(pOldFont);

	pDrawDC->BitBlt(0,0,size.cx,size.cy,pDC,0,0,SRCCOPY);
	pDC->SelectObject(pbmpOld);
	delete pDC;
}

void CMagneticView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	case ' ':
		// Use a special character for space to make sure that Repaginate()
		// doesn't split the input line.
		nChar = SPECIAL_KEYS + VK_SPACE;
		break;
	case 13:
		nChar = 10;
		break;
	}	

	// Add to the input buffer
	m_Input.Add(nChar);
	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CMagneticView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
	case VK_LEFT:		// Cursor left
	case VK_RIGHT:	// Cursor right
	case VK_UP:			// Cursor up
	case VK_DOWN:		// Cursor down
	case VK_HOME:		// Home
	case VK_END:		// End
	case VK_DELETE:	// Delete
		m_Input.Add(nChar + SPECIAL_KEYS);
		break;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CMagneticView::OnSize(UINT nType, int cx, int cy) 
{
	if (!m_iMorePrompt)
	{
		// Clear pagination
		m_PageTable.RemoveAll();
		CView::OnSize(nType, cx, cy);
	}
}

void CMagneticView::OnRecord() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CFileDialog RecordDlg(FALSE,NULL,m_strRecName,
		OFN_HIDEREADONLY,"All Files (*.*)|*.*||",this);
	RecordDlg.m_ofn.lpstrTitle = "Record Input File";

	switch (m_iRecording)
	{
	case 0:		// Off
		if (running)
			pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
		if (RecordDlg.DoModal() == IDOK)
		{
			m_strRecName = RecordDlg.GetPathName();
			if (m_pFileRecord = fopen(m_strRecName,"wt"))
				m_iRecording = 1;
		}
		break;
	case 1:		// Recording
		if (m_pFileRecord)
			fclose(m_pFileRecord);
		m_pFileRecord = NULL;
		m_iRecording = 0;
		break;
	case 2:		// Playback
		break;
	}
}

void CMagneticView::OnPlayback() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CFileDialog PlayDlg(TRUE,NULL,m_strRecName,
		OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,"All Files (*.*)|*.*||",this);
	PlayDlg.m_ofn.lpstrTitle = "Play Back a File";

	switch (m_iRecording)
	{
	case 0:		// Off
		if (running)
			pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
		if (PlayDlg.DoModal() == IDOK)
		{
			m_strRecName = PlayDlg.GetPathName();
			if (m_pFileRecord = fopen(m_strRecName,"rt"))
			{
				m_iRecording = 2;
				if (pApp->m_iGameLoaded && running)
					pApp->SetRedrawStatus(CMagneticApp::RedrawEndLine);
			}
		}
		break;
	case 1:		// Recording
		break;
	case 2:		// Playback
		if (m_pFileRecord)
			fclose(m_pFileRecord);
		m_pFileRecord = NULL;
		m_iRecording = 0;
		break;
	}
}

void CMagneticView::OnScript() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CFileDialog ScriptDlg(FALSE,NULL,m_strScrName,
		OFN_HIDEREADONLY,"All Files (*.*)|*.*||",this);
	ScriptDlg.m_ofn.lpstrTitle = "Scripting";

	switch (m_iScripting)
	{
	case 0:		// Off
		if (running)
			pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
		if (ScriptDlg.DoModal() == IDOK)
		{
			m_strScrName = ScriptDlg.GetPathName();
			if (m_pFileScript = fopen(m_strScrName,"wt"))
				m_iScripting = 1;
		}
		break;
	case 1:		// Scripting
		if (m_pFileScript)
			fclose(m_pFileScript);
		m_pFileScript = NULL;
		m_iScripting = 0;
		break;
	}
}

void CMagneticView::OnScrollback() 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
	GetWindowRect(m_Scrollback.m_DialogRect);
	m_Scrollback.DoModal();
}

void CMagneticView::OnEditPaste() 
{
	if (OpenClipboard())
	{
		// Get text from the clipboard
		LPCTSTR pszText = (LPCTSTR)GetClipboardData(CF_TEXT);
		if (pszText)
		{
			while (*pszText != '\0')
			{
				int c = (int)*(pszText++);
				switch (c)
				{
				case ' ':
					c = SPECIAL_KEYS + VK_SPACE;
					break;
				case 13:
				case 10:
					c = 0;
					break;
				}	
				// Add to the input buffer
				if (c)
					m_Input.Add(c);
			}
		}
		CloseClipboard();
	}
}

BOOL CMagneticView::OnEraseBkgnd(CDC* pDC) 
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CRect Background;
	GetClientRect(Background);

	if (pApp->m_iGameLoaded == 0)
		pDC->FillSolidRect(Background,pApp->GetBackColour());

	return 1;
}

/////////////////////////////////////////////////////////////////////////////
// Update handlers
/////////////////////////////////////////////////////////////////////////////

void CMagneticView::OnUpdateRecord(CCmdUI* pCmdUI) 
{
	switch (m_iRecording)
	{
	case 0:		// Off
		pCmdUI->Enable(!m_iMorePrompt);
		pCmdUI->SetCheck(0);
		break;
	case 1:		// Recording
		pCmdUI->Enable(!m_iMorePrompt);
		pCmdUI->SetCheck(1);
		break;
	case 2:		// Playback
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(0);
		break;
	}
}

void CMagneticView::OnUpdatePlayback(CCmdUI* pCmdUI) 
{
	switch (m_iRecording)
	{
	case 0:		// Off
		pCmdUI->Enable(!m_iMorePrompt);
		pCmdUI->SetCheck(0);
		break;
	case 1:		// Recording
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(0);
		break;
	case 2:		// Playback
		pCmdUI->Enable(!m_iMorePrompt);
		pCmdUI->SetCheck(1);
		break;
	}
}

void CMagneticView::OnUpdateScript(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_iMorePrompt);
	switch (m_iScripting)
	{
	case 0:		// Off
		pCmdUI->SetCheck(0);
		break;
	case 1:		// Scripting
		pCmdUI->SetCheck(1);
		break;
	}
}

void CMagneticView::OnUpdateFileOpen(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_iMorePrompt);
}

void CMagneticView::OnUpdateViewFont(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_iMorePrompt);
}

void CMagneticView::OnUpdateViewOptions(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(!m_iMorePrompt);
}

#ifdef _DEBUG
void CMagneticView::AssertValid() const
{
	CView::AssertValid();
}

void CMagneticView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CMagneticDoc* CMagneticView::GetDocument()
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CMagneticDoc)));
	return (CMagneticDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMagneticView support functions for the Magnetic interpreter
/////////////////////////////////////////////////////////////////////////////

CMagneticView* CMagneticView::GetView(void)
{
	CFrameWnd* pFrame = (CFrameWnd*)(AfxGetApp()->m_pMainWnd);
	if (pFrame == NULL)
		return NULL;
  CView* pView = pFrame->GetActiveView();
  if (pView == NULL)
    return NULL;

  // Fail if view is of wrong kind
  if (!pView->IsKindOf(RUNTIME_CLASS(CMagneticView)))
		return NULL;

	return (CMagneticView*)pView;
}

void CMagneticView::SolidRect(CDC* pDC, LPCRECT lpRect, COLORREF Colour)
{
	CPen* pOldPen = NULL;
	CPen* pNewPen = new CPen(PS_SOLID,1,Colour);
	pOldPen = pDC->SelectObject(pNewPen);

	CBrush* pOldBrush = NULL;
	CBrush* pNewBrush = new CBrush(Colour);
	pOldBrush = pDC->SelectObject(pNewBrush);

	pDC->Rectangle(lpRect);

	pDC->SelectObject(pOldPen);
	delete pNewPen;
	pDC->SelectObject(pOldBrush);
	delete pNewBrush;
}

void CMagneticView::ClearAll(void)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	m_strOutput.Empty();
	m_strStatLocation.Empty();
	m_strStatScore.Empty();
	m_strStatCurrent.Empty();
	m_PageTable.RemoveAll();
	m_Input.RemoveAll();
	m_iStatMode = 0;
	m_iLines = 0;
	m_iMaxLines = 0;

	pApp->m_iGameLoaded = 0;

	m_iRecording = 0;
	if (m_pFileRecord)
		fclose(m_pFileRecord);
	m_pFileRecord = NULL;
	m_iScripting = 0;
	if (m_pFileScript)
		fclose(m_pFileScript);
	m_pFileScript = NULL;
	m_strScript.Empty();

	if (m_Picture.m_hWnd)
		m_Picture.SendMessage(WM_CLOSE,0,0);
}

void CMagneticView::TrimOutput(void)
{
	// Remove old output
	int iTrim = m_PageTable.GetSize() - MAX_LINES;
	if (iTrim > 0)
	{
		while (iTrim > 0)
		{
			m_PageTable.RemoveAt(0);
			iTrim--;
		}
		m_strOutput =
			m_strOutput.Right(m_strOutput.GetLength() - m_PageTable[0]);
		m_PageTable.RemoveAll();
	}
}

int CMagneticView::Paginate(CDC* pDC, int p1, int p2)
{
	int iNewLines = 0;

	// Clear previous pagination
	if (p1+p2 == 0)
		m_PageTable.RemoveAll();

	LPCSTR lpszOutput = m_strOutput;
	int iOutSize = m_strOutput.GetLength();
	char c;

	while (p1+p2 < iOutSize)
	{
		c = lpszOutput[p1+p2];

		// Break line where possible
		switch (c)
		{
		case 10:
			if (LineFull(pDC,lpszOutput+p1,p2))
				p2 = FindPreviousSpace(lpszOutput+p1,p2-1);
			p1 += p2+1;
			p2 = 0;
			m_PageTable.Add(p1);
			iNewLines++;
			break;
		case ' ':
			if (LineFull(pDC,lpszOutput+p1,p2))
			{
				p2 = FindPreviousSpace(lpszOutput+p1,p2-1);
				p1 += p2+1;
				p2 = 0;
				m_PageTable.Add(p1);
				iNewLines++;
			}
			else
				p2++;
			break;
		default:
			p2++;
			break;
		}
	}

	// Add the last line
	if (p1+p2 > iOutSize)
		m_PageTable.Add(iOutSize+1);
	else
		m_PageTable.Add(p1+p2+1);

	return iNewLines;
}

int CMagneticView::FindPreviousSpace(LPCSTR lpszText, int iPos)
{
	if (iPos < 0)
		return 0;

	int iNewPos = iPos;

	// Find previous space character, if there is one
	while (lpszText[iNewPos] != ' ')
	{
		if (iNewPos > 0)
			iNewPos--;
		else
		{
			iNewPos = iPos;
			break;
		}
	}
	return iNewPos;
}

BOOL CMagneticView::LineFull(CDC* pDC, LPCSTR lpszText, int iLength)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CSize TextLen = pDC->GetTextExtent(lpszText,iLength);
	CRect Client;
  GetClientRect(Client);

	return (TextLen.cx >= (Client.Width()-(2*pApp->GetMargins().cx))) ? 1 : 0;
}

void CMagneticView::AddOutChar(char c)
{
	static const int SCRIPT_WIDTH = 70;

	if (c == '\b') // Is this a delete character?
	{
		int len;

		len = m_strOutput.GetLength();
		if (len > 0)
			m_strOutput = m_strOutput.Left(len-1);

		len = m_Scrollback.m_strScrollback.GetLength();
		if (len > 0)
		{
			m_Scrollback.m_strScrollback =
				m_Scrollback.m_strScrollback.Left(len-1);
		}

		if (m_iScripting)
		{
			len = m_strScript.GetLength();
			if (len > 0)
				m_strScript = m_strScript.Left(len-1);
		}
	}
	else
	{
		m_strOutput += c;

		// Update pagination information
		LPCTSTR lpszOutput = m_strOutput;
		if (m_PageTable.GetSize() > 1)
		{
			int p1, p2;

			p1 = m_PageTable[m_PageTable.GetSize()-2];
			p2 = m_PageTable[m_PageTable.GetSize()-1]-p1-1;
			m_PageTable.RemoveAt(m_PageTable.GetSize()-1);
			m_iLines += Paginate(m_pTextDC,p1,p2);
		}
		else
			m_iLines += Paginate(m_pTextDC,0,0);

		if ((m_iLines > m_iMaxLines) && (m_iRecording != 2))
		{
			CFrameWnd* pFrame;
			
			pFrame = (CFrameWnd*)(AfxGetApp()->m_pMainWnd);
			if (pFrame)
				pFrame->SetMessageText("Press a key for more...");
			m_iMorePrompt = TRUE;
			GetInput();
			m_iMorePrompt = FALSE;
			pFrame = (CFrameWnd*)(AfxGetApp()->m_pMainWnd);
			if (pFrame)
				pFrame->SetMessageText(AFX_IDS_IDLEMESSAGE);
			else
				return;	// Program ending
		}

		if (c == '\n')
			m_Scrollback.m_strScrollback += '\r';
		m_Scrollback.m_strScrollback += c;

		if (m_iScripting)
		{
			switch (c)
			{
			case 10:
				if (m_pFileScript)
					fprintf(m_pFileScript,"%s\n",m_strScript);
				m_strScript.Empty();
				break;
			case ' ':
				if (m_strScript.GetLength() > SCRIPT_WIDTH)
				{
					if (m_pFileScript)
						fprintf(m_pFileScript,"%s\n",m_strScript);
					m_strScript.Empty();
				}
				else
					m_strScript += c;
				break;
			default:
				m_strScript += c;
				break;
			}
		}
	}
}

void CMagneticView::AddStatChar(char c)
{
	switch (c)
	{
	case 9:
		m_iStatMode = 1;
		m_strStatLocation = m_strStatCurrent;
		m_strStatCurrent.Empty();
		break;
	case 10:
		m_iStatMode = 0;
		m_strStatScore = m_strStatCurrent;
		m_strStatCurrent.Empty();
		break;
	default:
		m_strStatCurrent += c;
		break;
	}
}

void CMagneticView::SetCursorPos(CDC* pDC, int iRight)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	int i = m_PageTable.GetSize()-2;	// Start of the last line

	if (i >= 0)
	{
		CSize TextLen = pDC->GetTextExtent((LPCTSTR)m_strOutput+m_PageTable[i],
											m_PageTable[i+1]-m_PageTable[i]-1-iRight);

		// Set the caret position
		SetCaretPos(CPoint(TextLen.cx+pApp->GetMargins().cx,m_LastLineRect.top));
	}
}

void CMagneticView::InsertChar(CString& strInsert, char cChar, int iPos, int iIsOut)
{
	// Get access to the buffer with space for one more character
	int iLength = strInsert.GetLength();
	LPSTR lpszBuffer = strInsert.GetBuffer(iLength+1);

	// Shift the right hand characters up one position
	for (int i = iLength+1; i > iLength-iPos; i--)
		lpszBuffer[i] = lpszBuffer[i-1];

	// Add the new character
	lpszBuffer[i] = cChar;

	// Back to a CString
	strInsert.ReleaseBuffer();

	if (iIsOut && m_PageTable.GetSize() > 0)
		m_PageTable[m_PageTable.GetSize()-1] = m_PageTable[m_PageTable.GetSize()-1] + 1;
}

void CMagneticView::RemoveChar(CString& strRemove, int iPos, int iIsOut)
{
	// Get access to the buffer
	int iLength = strRemove.GetLength();
	LPSTR lpszBuffer = strRemove.GetBuffer(iLength);

	// Shift the characters down one position
	for (int i = iLength-iPos; i < iLength; i++)
		lpszBuffer[i] = lpszBuffer[i+1];

	// Back to a CString
	strRemove.ReleaseBuffer();

	if (iIsOut && m_PageTable.GetSize() > 0)
		m_PageTable[m_PageTable.GetSize()-1] = m_PageTable[m_PageTable.GetSize()-1] - 1;

}

void CMagneticView::TextSetup(void)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	if (m_pTextDC)
		return;

	// Set up a device context for font information
	m_pTextDC = new CPaintDC(this);

	// Create the font
	m_pTextFont = new CFont();
	m_pTextFont->CreateFontIndirect(pApp->GetLogFont());
	m_pOldFont = m_pTextDC->SelectObject(m_pTextFont);
}

void CMagneticView::TextClearup(void)
{
	m_PageTable.RemoveAll();

	if (m_pTextDC && m_pOldFont)
		m_pTextDC->SelectObject(m_pOldFont);
	if (m_pTextFont)
		delete m_pTextFont;
	if (m_pTextDC)
		delete m_pTextDC;

	m_pOldFont = NULL;
	m_pTextFont = NULL;
	m_pTextDC = NULL;
}

void CMagneticView::UseHistory(CString& strNewInput, int iOldLength)
{
	m_strOutput = m_strOutput.Left(m_strOutput.GetLength()-iOldLength);
	m_strOutput += strNewInput;

	if (m_PageTable.GetSize() > 0)
	{
		m_PageTable[m_PageTable.GetSize()-1] =
			m_PageTable[m_PageTable.GetSize()-1] +
			strNewInput.GetLength() - iOldLength;
	}
}

/////////////////////////////////////////////////////////////////////////////
// Interpreter support functions
/////////////////////////////////////////////////////////////////////////////

void CaretOn(void)
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return;

	TEXTMETRIC FontInfo;
	pView->m_pTextDC->GetTextMetrics(&FontInfo);
	int iFontHeight = (int)(FontInfo.tmHeight*1.1);

	// Turn the caret on
	pView->CreateSolidCaret(2,iFontHeight);
	pView->ShowCaret();
}

void CaretOff(void)
{
	CMagneticView* pView = CMagneticView::GetView();

	if (pView)
		pView->HideCaret();
	DestroyCaret();
}

BOOL OpenGame(LPCTSTR lpszPathName)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CMagneticView* pView = CMagneticView::GetView();

	if (pApp->m_iGameLoaded && running)
		pApp->SetRedrawStatus(CMagneticApp::RedrawEndLine);
	lastchar = 0;

	if (pView)
		pView->ClearAll();

	LPCTSTR lpszExt = ".gfx";
	CString strGfxName = lpszPathName;
	LPSTR lpszConvName = strGfxName.GetBuffer(strGfxName.GetLength()+
												 strlen(lpszExt)+1);

	char* pExtPos = strrchr(lpszConvName,'.');
	char* pDirPos = strrchr(lpszConvName,'/');
	if (pExtPos > pDirPos)
		strcpy(pExtPos,lpszExt);
	else
		strcat(lpszConvName,lpszExt);

	strGfxName.ReleaseBuffer();
	LPCTSTR lpszGfxName = strGfxName;

	// Free previous game
	ms_freemem();

	// Initialize new game
	pApp->m_iGameLoaded = ms_init((type8*)lpszPathName,(type8*)lpszGfxName);

	// Check status of loaded game
	if (pApp->m_iGameLoaded == 0)
	{
		CString strMessage;

		strMessage.Format("Failed to load game \"%s\"",lpszPathName);
		AfxMessageBox(strMessage,MB_ICONEXCLAMATION);
	}
	else
	{
		// Show the title picture, if possible
		CMagneticTitleDlg Title;
		Title.ShowTitle(lpszPathName);
	}

	if (pView)
		pView->Invalidate();
	return (pApp->m_iGameLoaded != 0) ? TRUE : FALSE;
}

char GetInput(void)
{
	static const int MAX_HISTORY = 20;

	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return 0;

	int cInput = 0;				// Input character
  int iPosition = 0;		// Current cursor position
	int iHistory = -1;		// Current history position
	static CString strFullLine;

	// Input line already obtained?
	if (strFullLine.GetLength() > 0)
	{
		cInput = strFullLine[0];
		strFullLine = strFullLine.Right(strFullLine.GetLength()-1);
		if (cInput == (signed char)(CMagneticView::SPECIAL_KEYS + VK_SPACE))
			cInput = ' ';
		return cInput;
	}

	if (pView->m_iMorePrompt)
		pView->m_PageTable.RemoveAt(pView->m_PageTable.GetSize()-1);
	else
		pView->m_PageTable.RemoveAll();

	// Refresh the view
	pView->Invalidate();
	CaretOn();

	while (cInput != 10)
	{
		pView = CMagneticView::GetView();
		if (pView == NULL)
			break;

		// Wait for a character
		CArray<int, int>& Input = pView->m_Input;
		if (Input.GetSize() == 0)
		{
    	if (pView)
		    pView->ShowCaret();
			pApp->PumpMessage();
			pApp->CWinApp::OnIdle(0);	// Call base class OnIdle();
			pView = CMagneticView::GetView();
			if (pView)
			{
		    pView->HideCaret();
				switch (pApp->GetRedrawStatus())
				{
				case CMagneticApp::RedrawEndLine:
					Input.RemoveAll();
					strFullLine.Empty();
					cInput = 10;	// intentional fall-through
				case CMagneticApp::RedrawThisLine:
					CaretOff();
					CaretOn();
					pView->Invalidate();
					break;
				}
				pView->SetCursorPos(pView->m_pTextDC,strFullLine.GetLength()-iPosition);
			}
		}
		else
		{
			cInput = (pView->m_iMorePrompt) ? 10 : Input[0];
			Input.RemoveAt(0);
			
			int iInsertPos, iRemovePos;
			switch (cInput)
			{
			case 10:																			// Return
				strFullLine += cInput;
				break;
			case CMagneticView::SPECIAL_KEYS + VK_LEFT:		// Cursor left
				if (iPosition > 0)
					iPosition--;
				break;
			case CMagneticView::SPECIAL_KEYS + VK_RIGHT:	// Cursor right
				if (iPosition < strFullLine.GetLength())
					iPosition++;
				break;
			case CMagneticView::SPECIAL_KEYS + VK_HOME:		// Home
				iPosition = 0;
				break;
			case CMagneticView::SPECIAL_KEYS + VK_END:		// End
				iPosition = strFullLine.GetLength();
				break;
			case CMagneticView::SPECIAL_KEYS + VK_DELETE:	// Delete
				if (iPosition < strFullLine.GetLength())
				{
					iRemovePos = strFullLine.GetLength() - iPosition;
					pView->RemoveChar(strFullLine,iRemovePos);
					pView->RemoveChar(pView->m_strOutput,iRemovePos,TRUE);
				}
				break;
			case 8:																				// Backspace
				if (iPosition > 0)
				{
					iRemovePos = strFullLine.GetLength() - iPosition + 1;
					pView->RemoveChar(strFullLine,iRemovePos);
					pView->RemoveChar(pView->m_strOutput,iRemovePos,TRUE);
					iPosition--;
				}
				break;
			case CMagneticView::SPECIAL_KEYS + VK_UP:			// Cursor up
				if (iHistory < pView->m_History.GetSize()-1)
					iHistory++;
				if ((iHistory >= 0) && (pView->m_History.GetSize() > 0))
				{
					int iOldLength = strFullLine.GetLength();
					strFullLine = pView->m_History[iHistory];
					pView->UseHistory(strFullLine,iOldLength);
					iPosition = strFullLine.GetLength();
				}
				break;
			case CMagneticView::SPECIAL_KEYS + VK_DOWN:		// Cursor down
				if (iHistory > 0)
					iHistory--;
				if ((iHistory >= 0) && (pView->m_History.GetSize() > 0))
				{
					int iOldLength = strFullLine.GetLength();
					strFullLine = pView->m_History[iHistory];
					pView->UseHistory(strFullLine,iOldLength);
					iPosition = strFullLine.GetLength();
				}
				break;
			case CMagneticView::SPECIAL_KEYS + VK_SPACE:	// Space
				iInsertPos = strFullLine.GetLength() - iPosition;
				pView->InsertChar(pView->m_strOutput,cInput,iInsertPos,TRUE);
				pView->InsertChar(strFullLine,cInput,iInsertPos);
				iPosition++;
				break;
			default:
				if (isprint(cInput) && (cInput < CMagneticView::SPECIAL_KEYS))
				{
					// Insert the character into the input string
					iInsertPos = strFullLine.GetLength() - iPosition;
					pView->InsertChar(pView->m_strOutput,cInput,iInsertPos,TRUE);
					pView->InsertChar(strFullLine,cInput,iInsertPos);
					iPosition++;
				}
				break;
			}

			// Update the input line
			pView->InvalidateRect(pView->m_LastLineRect,FALSE);
		}
	}

	if (pView && (strFullLine.GetLength() > 0))
	{
		if (!pView->m_iMorePrompt)
		{
			// Store in input history
			CString strHistory = strFullLine.Left(strFullLine.GetLength()-1);
			if (strHistory.GetLength() > 0)
			{
				pView->m_History.InsertAt(0,strHistory);
				if (pView->m_History.GetSize() > MAX_HISTORY)
					pView->m_History.RemoveAt(pView->m_History.GetSize()-1);
			}

			int i;
			while ((i = strHistory.Find((char)(CMagneticView::SPECIAL_KEYS + VK_SPACE))) >= 0)
				strHistory.SetAt(i,' ');

			// Input recording
			if ((pView->m_iRecording == 1) && (pView->m_pFileRecord))
					fprintf(pView->m_pFileRecord,"%s\n",strHistory);

			// Scrollback buffer
			pView->m_Scrollback.m_strScrollback += strHistory;

			// Scripting
			if (pView->m_iScripting)
				pView->m_strScript += strHistory;
		}

		if (strFullLine.CompareNoCase("#undo\n") == 0)
		{
			cInput = 0;
			strFullLine.Empty();
		}
		else
		{
			cInput = strFullLine[0];
			strFullLine = strFullLine.Right(strFullLine.GetLength()-1);
		}
	}

	if (pView)
		pView->m_iLines = 0;
	CaretOff();

	if (cInput == (signed char)(CMagneticView::SPECIAL_KEYS + VK_SPACE))
		cInput = ' ';

	return cInput;
}

/////////////////////////////////////////////////////////////////////////////
// Interface to the Magnetic interpreter
/////////////////////////////////////////////////////////////////////////////

type8 ms_load_file(type8 *name, type8 *ptr, type16 size)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CString strLoadName;
	FILE *fh;

	if (name == NULL)
	{
		CMagneticView* pView = CMagneticView::GetView();
		if (pView == NULL)
			return 0;

		CFileDialog LoadDlg(TRUE,NULL,pView->m_strFileName,
			OFN_FILEMUSTEXIST|OFN_HIDEREADONLY,"All Files (*.*)|*.*||",pView);
		if (running)
			pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
		if (LoadDlg.DoModal() == IDOK)
		{
			strLoadName = LoadDlg.GetPathName();
			pView->m_strFileName = strLoadName;
		}
		else
			return 0;
	}
	else
		strLoadName = name;

	if (!(fh = fopen(strLoadName,"rb")))
		return 1;
	if (fread(ptr,1,size,fh) != size)
		return 1;
	fclose(fh);
	return 0;
}

type8 ms_save_file(type8 *name, type8 *ptr, type16 size)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CString strLoadName;
	FILE *fh;

	if (name == NULL)
	{
		CMagneticView* pView = CMagneticView::GetView();
		if (pView == NULL)
			return 0;

		CFileDialog SaveDlg(FALSE,NULL,pView->m_strFileName,
			OFN_HIDEREADONLY,"All Files (*.*)|*.*||",pView);
		if (running)
			pApp->SetRedrawStatus(CMagneticApp::RedrawThisLine);
		if (SaveDlg.DoModal() == IDOK)
		{
			strLoadName = SaveDlg.GetPathName();
			pView->m_strFileName = strLoadName;
		}
		else
			return 0;
	}
	else
		strLoadName = name;

	if (!(fh = fopen(strLoadName,"wb")))
		return 1;
	if (fwrite(ptr,1,size,fh) != size)
		return 1;
	fclose(fh);
	return 0;
}

void ms_statuschar(type8 c)
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView != NULL)
		pView->AddStatChar(c);
}

void ms_putchar(type8 c)
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView != NULL)
		pView->AddOutChar(c);
}

void ms_flush(void)
{
}

type8 ms_getchar(void)
{
	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return 0;

	if (pView->m_iRecording == 2)		// Playback
	{
		if (pView->m_pFileRecord)
		{
			char cInput = fgetc(pView->m_pFileRecord);
			if (feof(pView->m_pFileRecord) != 0)
			{
				pView->m_iRecording = 0;
				fclose(pView->m_pFileRecord);
				pView->m_pFileRecord = NULL;
			}
			else
			{
				if (cInput == '\n')
				{
					cInput = 10;
					pView->TrimOutput();
					pView->m_PageTable.RemoveAll();
					pView->Invalidate();
				}
				else
					pView->AddOutChar(cInput);
				return cInput;
			}
		}
	}

	return GetInput();
}

void ms_showpic(type8 c,type8 mode)
{
	CMagneticApp* pApp = (CMagneticApp*)AfxGetApp();
	ASSERT_VALID(pApp);

	CMagneticView* pView = CMagneticView::GetView();
	if (pView == NULL)
		return;

	static type8 *pPictureData = NULL;
	static type16 Width, Height;
	static type16 Palette[16];

  switch (mode)
  {
    case 0:	// Graphics off
			if (pView->m_Picture.m_hWnd)
				pView->m_Picture.SendMessage(WM_CLOSE,0,0);
      break;
    case 1:	// Graphics on (thumbnails)
    case 2:	// Graphics on (normal)
			if (pApp->m_bShowPics)
				pPictureData = ms_extract(c,&Width,&Height,Palette);
			else
				pPictureData = NULL;
    case 3:	// Use last picture (not sent by emulator code)
      if (pPictureData && pApp->m_bShowPics)
			{
				if (pView->m_Picture.m_hWnd == NULL)
				{
					CRect rWnd;
					CPoint& PicTopLeft = pApp->GetPicTopLeft();
					rWnd.left = PicTopLeft.x;
					rWnd.top = PicTopLeft.y;
					rWnd.right = rWnd.left + Width;
					rWnd.bottom = rWnd.top + Height;

					if (pView->m_Picture.CreatePicWnd(pView,rWnd) == FALSE)
							return;
					pView->SetFocus();
				}
				pView->m_Picture.NewPicture(Width,Height,pPictureData,Palette);
			}
      break;
  }
}
