// EIKCONCL.H
//
// Copyright (c) 1997-1999 Symbian Ltd.  All rights reserved.
//

#include <apgwgnam.h>
#include <fbs.h>
#include <coeutils.h>
#include <eikappui.h>
#include <eikapp.h>
#include <eikenv.h>
#include <eikdoc.h>
#include <eikmenup.h>
#include <eikdialg.h>
#include <eiksbfrm.h>
#include <eikfutil.h>
#include <eikcfdlg.h>
#include <basched.h>

#ifdef __ER6__
#include <ckndgopn.h>
#include <ckndgsve.h>
#endif

//sqdebug
#include <coemain.h>
#include <w32std.h>

#ifdef __ER6__
#include <ckninfo.h>
#endif
//#include <apgicnfl.h>

#include <coeccntx.h>

#include <eikon.rsg>

#ifdef __ER6__
#include <eikon.hrh>
#endif

#if !defined(__EIKCONCL_H__)
#define __EIKCONCL_H__

#if !defined(__E32CONS_H__)
#include <e32cons.h>
#endif

#if !defined(__W32STD_H__)
#include <w32std.h>
#endif
#if !defined(__EIKCONSO_H__)
#include <eikconso.h>
#endif

class CEikConsoleScreen;
class CAdvConsoleClient;

CAdvConsoleClient *NewAdvConsole();

typedef const TAny* CATP;
typedef unsigned char  type8;
typedef unsigned short type16;

struct ECBITMAP {
	type8* picdata;
	TSize aSize;
	type16* palette;
};

class CAdvConsoleClient : public CConsoleBase
	{
public:
	~CAdvConsoleClient();
public: // from CConsoleBase
	virtual void SetAttr(int attrib);
	void Read(TRequestStatus& aStatus);
	void ReadCancel();
	void Write(const TDesC& aDes);
	TPoint CursorPos() const;
	void SetCursorPosAbs(const TPoint& aPoint);
	void SetCursorPosRel(const TPoint& aPoint);
	void SetCursorHeight(TInt aPercentage);
	void SetTitle(const TDesC& aTitle);
	void ClearScreen();
	void ClearToEndOfLine();
	void ClearChars(const TRect &aRect, TUint aCharacterAttributes);
	void ScrollChars(const TRect &anArea,const TPoint &aVector);	
	TSize ScreenSize() const;
	TSize ScreenSizePixels() const;
	void ScreenZoom();
	TKeyCode KeyCode() const;
	TUint KeyModifiers() const;
	virtual int KeyHit();
	void SetFont(TDesC *aFaceName, TInt aPointSize);
	TPoint PointerPos();	  // Last known pointer position.
	void OpenDialog(TDes& aDes);
	void SaveDialog(TDes& aDes);
	void PictureDialog(type8* picdata, TSize aSize, type16* apalette);
	void PictureTitle(TDes& aDes);
	void RestoreScreen();
	void ClearGraphicScreen();


public:
	friend CAdvConsoleClient* NewAdvConsole();
	CAdvConsoleClient();
private: // from CConsoleBase
	TInt Create(const TDesC& aTitle,TSize aSize);
private: // internal use only
	void SendReceive(TInt aMessage,const TAny* aParam);
	inline void SendReceive(TInt aMessage,TInt aParam) { SendReceive(aMessage,(const TAny*)aParam); }
private:
	friend class CAdvConsMessager;
	RThread iThread;
	TRequestStatus iLogonStatus;
	TRequestStatus* iThreadStatus;
	TInt* iMessage;
	CATP* iParam;
	TRequestStatus** iReplyStatus;
	TKeyEvent iKeyEvent;
	CEikConsoleScreen* iScreen;
	TBuf<20> iFaceName;
	TInt iTwipSize;
};

//
// class CAdvConsMessager
//
class CAdvConsMessager : public CActive
{
public:
	CAdvConsMessager(CEikConsoleScreen* aScreen,RThread aParentThread);
	~CAdvConsMessager();
	void ConstructL(CAdvConsoleClient* aClient);
	void HandleKeyEvent(const TKeyEvent& aKeyEvent);
//		struct SQDUDE {
//			TBuf<256> sqstr;
//			TInt	sqint;
//		};
private: // overridden
	void RunL();
	void DoCancel();
private: // internal
	void CompleteReadRequest();
private:
	CEikConsoleScreen* iScreen;
	RThread iParentThread;
	TRequestStatus* iReadStatus;
	TKeyEvent* iKeyEvent;
	TInt iMessage;
	TPoint iPointer;
	const TAny* iParam;
	TRequestStatus* iReplyStatus;
	CCirBuf<TKeyEvent>* iKeyQ;
	TInt iZoomFactor;
	TUint8 *SaveScreen;			//Save screen behind picture
	TUint8 *iScreenAddress;		//Actual memory of the screen - low level
	TUint8 *iKeepScreenAddress, *iKeepScreenEndAddress;

	TInt picheight, picwidth;
	TInt scaleup;
	TInt bitsperpixel;
	TInt iBytesPerScanLine;
	TDisplayMode drawcolour;
	void DisplayPicture(TInt pictype);
};


struct SCommandLine
{
	RThread iParentThread;
	TRequestStatus* iStatus;
	CAdvConsoleClient* iClient;
	TSize iSize;
	const TDesC* iTitle;
	const TDesC* iFaceName;
	TInt iTwipSize;
};

class CAdvConsAppUi : public CEikAppUi
{
public:
	void ConstructL(const SCommandLine* aComLine);
	~CAdvConsAppUi();
private: // overridden
#ifdef __ER6__
	TKeyResponse HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
#else
	void HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType);
#endif
	void HandleForegroundEventL(TBool aForeground);
	void SetAndDrawFocus(TBool aFocus);
	TBool FindFile(TDes& aFileName, const TDesC *aDefault = NULL);
private:
	CEikConsoleScreen* iScreen;
	CEikConsoleControl* iControl;
	CAdvConsMessager* iMessager;
};

//
// class CConsEikonEnv
//
class CConsEikonEnv : public CEikonEnv
{
public:
    void ConstructConsoleEnvironmentL(const SCommandLine* aComLine);
};

#endif
