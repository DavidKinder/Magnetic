// epoc_console.cpp
//
// Code adapted from the NetHack for Psion code, written by Duncan Booth
//
// Code adapted by Simon Quinn, 14/12/00
// Port to ER6 with EPOC dialog improvements by Simon Quinn, 02/03/02
// Also added Picture displaying support for Magnetic
//
// Description: This is a wrapper for the Eikon environment to make
// use of the Eikon Console code, instead of the deficient E32 Console.
// This is run as an Active Object, separate from the main STDLIB code.
// Messages for printing, input etc are passed via the EPOC message passing
// mechanism.
// Code is written in a generic way so any STDLIB application can easily
// make use of it.
// To make use of this code just use the following statement:
//		iConsole = NewAdvConsole();		{Creates the console}
//
// Then just call as usual with iConsole->Write(), iConsole->Getch() etc.
//////////////////////////////////////////////////////////////////////////

#include "epoc_console.h"

enum TEikConClPanic
	{
	EEikConClPanicReadAlreadyOutstanding
	};

GLDEF_C void Panic(TEikConClPanic aPanic)
{
    User::Panic(_L("HGCONS"),aPanic);
}

enum
{
    EExit,
    ERead,
    EReadCancel,
    EWrite,
    ESetCursorPosAbs,
    ESetCursorPosRel,
    ESetCursorHeight,
    EClearScreen,
    EClearToEndOfLine,
	EClearChars,
	EScrollChars,
    ESetAttr,
    EKeyHit,
    ESetFont,
    EGetPointerPos,
//	EScreenZoom,
	ESetTitle,
	EOpenDialog,
	ESaveDialog,
	EPictureDialog,
	EPictureTitle,
	ERestoreScreen,
	EClearGraphicScreen
};

//
// class CAdvConsMessager
//
CAdvConsMessager::CAdvConsMessager(CEikConsoleScreen* aScreen,RThread aParentThread)
: CActive(EActivePriorityIpcEventsHigh),
iParentThread(aParentThread)
{
    iScreen=aScreen;
}

CAdvConsMessager::~CAdvConsMessager()
{
    iParentThread.Close();
    delete iKeyQ;
}

void CAdvConsMessager::ConstructL(CAdvConsoleClient* aClient)
{
    iKeyQ=new(ELeave) CCirBuf<TKeyEvent>;
    iKeyQ->SetLengthL(40);	// buffer length, too high? too low?
    iKeyEvent=(&aClient->iKeyEvent);
    aClient->iThreadStatus=(&iStatus);
    aClient->iMessage=(&iMessage);
    aClient->iParam=(&iParam);
    aClient->iReplyStatus=(&iReplyStatus);
    aClient->iScreen=iScreen;
    CActiveScheduler::Add(this);
    iStatus=KRequestPending;
    SetActive();
}

void CAdvConsMessager::DoCancel()
{
}

void CAdvConsMessager::RunL()
{
    switch (iMessage)
    {
		case EExit:
			CBaActiveScheduler::Exit();
			break;
		case ERead:
			if (iReadStatus)
				Panic(EEikConClPanicReadAlreadyOutstanding);

			// Bring the cursor onto the visible part of the screen whenever we read some input.
			iScreen->DrawCursorInSight();
			iReadStatus=(TRequestStatus*)iParam;
			if (iKeyQ->Count()>0) // already a buffered event
				CompleteReadRequest();
			break;
		case EReadCancel:
			if (iReadStatus)
				iParentThread.RequestComplete(iReadStatus,KErrCancel);
			break;
		case EWrite:
			{
				//Before printing check if we are dealing with INVERSE characters.
				//INVERSE characters must be cleared before overwriting with other
				//INVERSE characters otherwise they become non-INVERSE!
				TInt qatt = iScreen->Att();
				if(qatt & ATT_INVERSE)
				{
					TInt x,y, templen;
					x = iScreen->WhereX();
					y = iScreen->WhereY();
					templen = (*(const TDesC*)iParam).Length();
          			iScreen->ClearChars(TRect(x, y, (x+templen), 1), ATT_INVERSE);
				}

				iScreen->Write(*(const TDesC*)iParam);
				break;
			}
		case ESetCursorPosAbs:
			iScreen->SetCursorPosAbs(*(const TPoint*)iParam);
			break;
		case ESetCursorPosRel:
			iScreen->SetCursorPosRel(*(const TPoint*)iParam);
			break;
		case ESetCursorHeight:
			iScreen->SetCursorHeight((TInt)iParam);
			break;
		case EClearScreen:
			iScreen->ClearScreen();
			break;
		case EClearToEndOfLine:
			iScreen->ClearToEndOfLine();
			break;
		case EClearChars:
			iScreen->ClearChars(*(const TRect*)iParam, 0);
			break;
		case EScrollChars:
		{
			TRect tr = *(const TRect*)iParam;
			iScreen->ScrollChars(tr, TPoint(0,-1));
			//Scrolling doesn't remove the bottom line, so clear the last line of the block
			iScreen->ClearChars(TRect(tr.iTl.iX, tr.iBr.iY - 1, tr.iBr.iX, tr.iBr.iY), 0);
			break;
		}
		case ESetAttr:
			iScreen->SetAtt((TInt)iParam);
			break;
		case EKeyHit:
		{
			// Returns the number of keys waiting in the buffer.
			TInt res = iKeyQ->Count();
			iParentThread.WriteL(iParam, TPckg<TInt>(res), 0);
			break;
		}
		case EGetPointerPos:
			iParentThread.WriteL(iParam, TPckgC<TPoint>(iPointer), 0);
			break;
//		case EScreenZoom:
//		{
//			iZoomFactor = (iZoomFactor ? 0 : 1);
//
//			iScreen->SetFontL(TFontSpec(_L("Terminal"), (iZoomFactor ? 12:10)));
//	
//			TBuf<64> buffer;
//			buffer.Format(_L("Screen size %dx%d"),
//					  iScreen->ConsoleControl()->Size().iWidth/iScreen->ConsoleControl()->CharSize().iWidth,
//					  iScreen->ConsoleControl()->Size().iHeight/iScreen->ConsoleControl()->CharSize().iHeight);
//			CEikonEnv::Static()->InfoMsg(buffer);
//			break;
//		}
		case EOpenDialog:
			{
				TFileName filename;
				filename.Copy(*(const TDes*)iParam);
				TParse parse;
				parse.Set(filename, NULL, NULL);
				filename.Copy(parse.DriveAndPath());

				CEikFileOpenDialog* dialog=new(ELeave) CEikFileOpenDialog(&filename);
				if (!(dialog->ExecuteLD(R_EIK_DIALOG_FILE_OPEN)))
					filename.Zero();

				iParentThread.WriteL(iParam, filename, 0);
				break;
			}
		case ESaveDialog:
			{
				TFileName filename;
				filename.Copy(*(const TDes*)iParam);

				CEikFileSaveAsDialog* dialog=new(ELeave) CEikFileSaveAsDialog(&filename);
				if (!(dialog->ExecuteLD(R_EIK_DIALOG_FILE_SAVEAS)))
					filename.Zero();
				iParentThread.WriteL(iParam, filename, 0);
				break;
			}
#ifdef __MAGNETIC__
		case EPictureDialog:
			{
				DisplayPicture(1);
				break; 
			}
		case ERestoreScreen:
			{
				//Save the current screen area to memory for later retrieval before drawing to screen
#ifndef __WINS__
				TUint8* savescreenAddress = iScreenAddress;
				TUint8* saveSaveScreen = SaveScreen;
				for(TInt j = 0; j < picheight; j++)
				{
					TUint8* screenBuf = savescreenAddress;
					for(TInt i = 0; i < (picwidth * (bitsperpixel/8)); i++) 
					{
						*screenBuf++ = *saveSaveScreen++;
					}
					savescreenAddress += iBytesPerScanLine;
				}
#endif
				User::Free(SaveScreen);
				break;
			}
		case EClearGraphicScreen:
			{
				//Clear complete screen, text clear can miss bits
				TPckgBuf<TScreenInfoV01> info;
				UserSvr::ScreenInfo(info);
				TSize screenSize = info().iScreenSize;
				TUint8* ibufScreen = iKeepScreenAddress;
				for (TInt i=0;i<(screenSize.iWidth * screenSize.iHeight * (bitsperpixel/8));i++)
				{
#ifdef __WINS__
					ibufScreen++;
#else
					*ibufScreen++ = 255;
#endif
				}
				break;
			}

		case EPictureTitle:
			{
				DisplayPicture(2);
				break;
			}
#endif __MAGNETIC__

	}

    iStatus=KRequestPending;
    SetActive();
    iParentThread.RequestComplete(iReplyStatus,0);
}

#ifdef __MAGNETIC__
/////////////////////////////////////////////////////////////////////////////
// Magnetic Scrolls specific display picture code
// Call with 1 for normal picture
// Call with 2 for title picture
// Code is messy but it works well, does conversions and direct drawing in this procedure. 
// Gets around the problem with using a console and not being able to draw to the screen.
// Unfortunately Psion 5mx/Revo put screen into 4 bit mode, console mode, which is no good for displaying pictures.
// Haven't found a solution for this bit problem.
// Pictures aren't displayed on screen due to direct drawing used and not able to draw through 
// the window server to the screen - at least solution not found yet.
/////////////////////////////////////////////////////////////////////////////
void CAdvConsMessager::DisplayPicture(TInt pictype)
{
	TInt i,j;

	//This section is specific to Magnetic Scrolls
	//Draws the picture directly to the screen!
	typedef struct
	{
	   unsigned char red, green, blue ;
	} GPalette;

	GPalette palette[16];
	GPalette colour;

	ECBITMAP rawbitmap;
	rawbitmap = *(const ECBITMAP*)iParam;

	//Our EPOC bitmap we will draw to
	CFbsBitmap* abitmap = new (ELeave) CFbsBitmap();	
	CleanupStack::PushL(abitmap);

#ifdef __ER6__
	drawcolour = EColor4K;
	scaleup = 1;
	bitsperpixel = 16;
#else
	if(iScreen->ConsoleControl()->Size().iHeight < 400)
	{
		drawcolour = EGray16;
		bitsperpixel = 4;
		scaleup = 1;
	}
	else
	{
		drawcolour = EColor256;
		bitsperpixel = 8;
		scaleup = 2;		//Scale up to double size on Psion 7
	}
#endif

	if(pictype == 1)
	{
		abitmap->Create(rawbitmap.aSize,drawcolour); 
	}
	else
	{
#ifndef __ER6__
		abitmap->Load(*(const TDes*)iParam,0); 
		rawbitmap.aSize = abitmap->SizeInPixels();
#else
		//Conversion required for Nokia which runs in 12 bit mode
		//The title screen loaded is 8 bit
		CFbsBitmap* atempbitmap = new (ELeave) CFbsBitmap();
		CleanupStack::PushL(atempbitmap);
		atempbitmap->Load(*(const TDes*)iParam,0); 
		rawbitmap.aSize = atempbitmap->SizeInPixels();
		abitmap->Create(rawbitmap.aSize,drawcolour); 
		
		//Convert 8bit to 12bit display
		TRgb theTRgb;
		TBitmapUtil theBitmap1Util(atempbitmap);    
		TBitmapUtil theBitmap2Util(abitmap);    
		theBitmap1Util.Begin(TPoint(0,0));
		theBitmap2Util.Begin(TPoint(0,0), theBitmap1Util);

		for (TInt y=0; y<rawbitmap.aSize.iHeight; y++)
		{

			theBitmap1Util.SetPos(TPoint(0,y));
			theBitmap2Util.SetPos(TPoint(0,y));
			for (TInt x=0; x<rawbitmap.aSize.iWidth; x++)
			{
				theTRgb=TRgb::Color256(theBitmap1Util.GetPixel());
				theBitmap2Util.SetPixel(theTRgb.Color4K());
				theBitmap1Util.IncXPos();
				theBitmap2Util.IncXPos();
			} 
		}
		theBitmap1Util.End();
		theBitmap2Util.End();

		CleanupStack::PopAndDestroy(); //Pop and destroy atempbitmap bitmap.

#endif
	}

	if(pictype == 1)	
	{
		//Create our palette for bitmap conversion, from the one passed from Magnetic
		for(i=0 ; i< 16; i++ )
		{
		  palette[i].red=( rawbitmap.palette[i] & 0x0f00 ) >> 3;
		  palette[i].green=( rawbitmap.palette[i] & 0x00f0 ) << 1;
		  palette[i].blue=( rawbitmap.palette[i] & 0x000f ) << 5;
		}

		//Do actual conversion of bitmap. Bitmap from Magnetic has colours according
		//to the 16 colour palette, we need to convert these into displayable colours
		TRgb theTRgb;
		TBitmapUtil theBitmapUtil(abitmap);    //myData_p : CFbsBitmap object
		theBitmapUtil.Begin(TPoint(0,0));
		for (TInt y=0; y<rawbitmap.aSize.iHeight; y++)
		{

			theBitmapUtil.SetPos(TPoint(0,y));
			for (TInt x=0; x<rawbitmap.aSize.iWidth; x++)
			{
				colour = palette[ rawbitmap.picdata[ (y * rawbitmap.aSize.iWidth) + x] ];
				theTRgb = TRgb(colour.red, colour.green, colour.blue);
#ifdef __ER6__
				theBitmapUtil.SetPixel( theTRgb.Color4K() );
#else
				if(drawcolour == EGray16)
					theBitmapUtil.SetPixel( theTRgb.Gray16() );
				else
					theBitmapUtil.SetPixel( theTRgb.Color256() );
#endif
				theBitmapUtil.IncXPos();
			}
		}
		theBitmapUtil.End();
	}

	TSize screenSize;

    TPckgBuf<TScreenInfoV01> info;
	UserSvr::ScreenInfo(info);
	screenSize = info().iScreenSize;

	iScreenAddress = (TUint8*)(info().iScreenAddress);

	//Move to start of actual screen, skipping the palette memory
	if(drawcolour == EColor256)
		iScreenAddress += 512;
	else
		if(drawcolour == EColor4K)
			iScreenAddress += 16 * 2;

	iKeepScreenAddress = iScreenAddress;
	iKeepScreenEndAddress = iScreenAddress + (screenSize.iWidth * screenSize.iHeight * (bitsperpixel/8));

	TUint8* myBufferAddress;
	TUint8* screenAddress;
	TInt jumpInPixelsInBuffer;

	picheight = rawbitmap.aSize.iHeight * scaleup;
	picwidth = rawbitmap.aSize.iWidth * scaleup;

	myBufferAddress = (TUint8*)abitmap->DataAddress();
	screenAddress = iScreenAddress;
	jumpInPixelsInBuffer = 	abitmap->ScanLineLength(rawbitmap.aSize.iWidth, drawcolour) - (rawbitmap.aSize.iWidth * (bitsperpixel/8));
	iBytesPerScanLine	= (screenSize.iWidth * bitsperpixel) / 8;

	//Move start position to centre picture horizontally, picwidth calc is done to force to an even address in 16bit graphics
	screenAddress += (iBytesPerScanLine/2) - ((((picwidth / 2)*2)* (bitsperpixel/8))/ 2);
	//Move start position to centre picture vertically
	screenAddress += ((screenSize.iHeight / 2) - (picheight / 2)) * (screenSize.iWidth * (bitsperpixel/8));

	if(pictype == 1)	//Title screen doesn't need saving
	{
		//Save the current screen area to memory for later retrieval before drawing to screen
		TInt buffsize = rawbitmap.aSize.iHeight * screenSize.iWidth * scaleup * (bitsperpixel/8);
		iScreenAddress = screenAddress;		//used in restore!!
		SaveScreen = (unsigned char *) User::Alloc(buffsize) ;
#ifndef __WINS__
		TUint8* savescreenAddress = screenAddress;
		TUint8* saveSaveScreen = SaveScreen;
		for(j = 0; j < picheight; j++)
		{
			TUint8* screenBuf = savescreenAddress;
			for(i = 0; i < (picwidth * (bitsperpixel/8)); i++) 
			{
				*saveSaveScreen++ = *screenBuf++;
			}
			savescreenAddress += iBytesPerScanLine;
		}
#endif
	}

	//Now draw the bitmap directly to the screen, scaling if appropriate. Bitmap will already
	//be in the correct bitdepth for the current screen.
	TInt passflag=0;
	TUint8 *savebuffer=0, *screenBuf=0;
	for(j = 0; j < (rawbitmap.aSize.iHeight * scaleup); j++)
	{
		//This bit of code duplicates lines vertically when we are scaling up
		if(passflag && scaleup == 2)
			myBufferAddress = savebuffer;

		savebuffer = myBufferAddress;

		screenBuf = screenAddress;					//Set buffer ptr to next scanline

		for(i = 0; i < rawbitmap.aSize.iWidth; i++) 
		{
			if(screenBuf < iKeepScreenAddress || screenBuf > iKeepScreenEndAddress)
			{
				if(drawcolour == EColor4K)			//16 bits per pixel so write the extra byte
					myBufferAddress+=2;
				else
					myBufferAddress++;
			}
			else
			{
#ifndef __WINS__
#ifdef __ER6__
				if(drawcolour == EColor4K)			//16 bits per pixel so write the extra byte
				{
					*screenBuf++ = *myBufferAddress++;
					*screenBuf++ = *myBufferAddress++;
				}
#else
				if(scaleup == 2)					//scaleup only in 8 bit pixel mode	
				{
					*screenBuf++ = *myBufferAddress;
					*screenBuf++ = *myBufferAddress++;
				}
				else
					*screenBuf++ = *myBufferAddress++;
#endif
#endif
			}
		}
		screenAddress += iBytesPerScanLine;
		myBufferAddress += jumpInPixelsInBuffer;
		passflag = (passflag ? 0 : 1);
	}

	CleanupStack::PopAndDestroy(); //Pop and destroy the bitmap.
}
#endif __MAGNETIC__

void CAdvConsMessager::CompleteReadRequest()
{
    if (iReadStatus)
    { 
        iKeyQ->Remove(iKeyEvent);;
        iParentThread.RequestComplete(iReadStatus,0);
    }
}

void CAdvConsMessager::HandleKeyEvent(const TKeyEvent& aKeyEvent)
{
    TInt ret=iKeyQ->Add(&aKeyEvent);
    if (ret==0)
        CEikonEnv::Beep();
    if (iKeyQ->Count()==1) // client may be waiting on this key event
        CompleteReadRequest();
}

//
// class CAdvConsAppUi
//

CAdvConsAppUi::~CAdvConsAppUi()
{
    delete(iScreen);
    delete(iMessager);
}

void CAdvConsAppUi::ConstructL(const SCommandLine* aComLine)
{
    CEikAppUi::BaseConstructL(ENoAppResourceFile);

    iScreen=new(ELeave) CEikConsoleScreen;
    iScreen->ConstructL(*(aComLine->iTitle), 0);

//sqdebug
	TZoomFactor iZoomFactor;
	iZoomFactor.SetGraphicsDeviceMap(iCoeEnv->ScreenDevice());
	iZoomFactor.SetZoomFactor(TZoomFactor::EZoomOneToOne);

//sqdebug
#ifdef __ER6__
	iScreen->SetFontL(TFontSpec(_L("Terminal"), 167));
#else
	if(iScreen->ConsoleControl()->Size().iHeight < 400)
		iScreen->SetFontL(TFontSpec(_L("Courier"), 140));
	else
		iScreen->SetFontL(TFontSpec(_L("Courier"), 160));
#endif


	TBuf<64> buffer;
	buffer.Format(_L("Screen size %dx%d"),
			  iScreen->ConsoleControl()->Size().iWidth/iScreen->ConsoleControl()->CharSize().iWidth - 2,
			  iScreen->ConsoleControl()->Size().iHeight/iScreen->ConsoleControl()->CharSize().iHeight);
	CEikonEnv::Static()->InfoMsg(buffer);

    iScreen->ConstructL(*(aComLine->iTitle), 0);

    iScreen->SetKeepCursorInSight(EFalse);

#ifdef __ER6__
	//Fix ER6, has problems showing all the console without a scrollbar
	iScreen->SetScrollBarVisibilityL(CEikScrollBarFrame::EOff, CEikScrollBarFrame::EOn);
#endif

    iControl=iScreen->ConsoleControl();
    iControl->SetFocus(ETrue,EDrawNow);
	iMessager=new(ELeave) CAdvConsMessager(iScreen,aComLine->iParentThread);
    iMessager->ConstructL(aComLine->iClient);
    RThread().SetPriority(EPriorityMore);
}

void CAdvConsAppUi::HandleForegroundEventL(TBool aForeground)
{
    if (aForeground)
        RThread().SetPriority(EPriorityMore);
}

void CAdvConsAppUi::SetAndDrawFocus(TBool aFocus)
{
    if (iControl)
        iControl->SetFocus(aFocus,EDrawNow);
}

#ifdef __ER6__
TKeyResponse CAdvConsAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
#else
void CAdvConsAppUi::HandleKeyEventL(const TKeyEvent& aKeyEvent,TEventCode aType)
#endif
{
	if (aType==EEventKey) 
	switch(aKeyEvent.iCode) {
		case NULL:
			break;
//		case EEikSidebarZoomInKey:
//			ChangeZoomL();
//			break;
//		case EEikSidebarZoomOutKey:
//			ChangeZoomL();
//			break;
		default:
			iMessager->HandleKeyEvent(aKeyEvent);
	}
//sqdebug
#ifdef __ER6__
	return EKeyWasConsumed;
#endif
}

void CConsEikonEnv::ConstructConsoleEnvironmentL(const SCommandLine* aComLine)
{
    ConstructL();
    CAdvConsAppUi* appUi=new(ELeave) CAdvConsAppUi;
    appUi->ConstructL(aComLine);
    CApaWindowGroupName* wgName=CApaWindowGroupName::NewLC(iWsSession);
    TPtrC caption=*(aComLine->iTitle);
    wgName->SetCaptionL(caption);
    wgName->SetRespondsToShutdownEvent(EFalse);
    wgName->SetRespondsToSwitchFilesEvent(EFalse);
    wgName->SetWindowGroupName(iRootWin);
    CleanupStack::PopAndDestroy(); // wgName
#if defined(__EPOC32__)
    RProcess().Rename(caption);
#endif
    RThread().Rename(caption);
}

TInt ConsoleClientStartFunction(TAny* aParam)
{
    const SCommandLine* comLine=(const SCommandLine*)aParam;
    TInt err=KErrNoMemory;
    CConsEikonEnv* coe=new CConsEikonEnv;
    if (coe)
        TRAP(err,coe->ConstructConsoleEnvironmentL(comLine));
    TRequestStatus* pS=(comLine->iStatus);
    comLine->iParentThread.RequestComplete(pS,err);
    if (!err)
        coe->ExecuteD();
    return(0);
}

//
// class CAdvConsoleClient
//

CAdvConsoleClient::~CAdvConsoleClient()
{
    if (iLogonStatus.Int()==KRequestPending && iReplyStatus)
        SendReceive(EExit,NULL);
    iThread.Close();
}

CAdvConsoleClient::CAdvConsoleClient()
{
}

const TInt KMaxHeapSize=0x1000*254; // chunks are a megabyte anyway

TInt CAdvConsoleClient::Create(const TDesC& aTitle,TSize aSize)
{ 
	TInt err;
    TRequestStatus status=KRequestPending;
    SCommandLine comLine;
    comLine.iStatus=(&status);
    comLine.iClient=this;
    comLine.iSize=aSize;
    comLine.iTitle=&aTitle;
    comLine.iFaceName = &iFaceName;
    comLine.iTwipSize = iTwipSize;
    TBuf<20> threadName;
    TInt num=0;

    do
    {
        threadName.Format(_L("UI%02d"),num++); // !! review the title
        err=iThread.Create(threadName,ConsoleClientStartFunction,KDefaultStackSize,KMinHeapSize,KMaxHeapSize,&comLine,EOwnerThread);
    } while(err==KErrAlreadyExists);
    if (!err)
    {
        iThread.Logon(iLogonStatus);
        comLine.iParentThread.Duplicate(iThread);
        iThread.Resume();
        User::WaitForRequest(status,iLogonStatus);
        err=status.Int();
    }
    return(err);
}

void CAdvConsoleClient::SendReceive(TInt aMessage,const TAny* aParam)
{
    if (iLogonStatus.Int()!=KRequestPending)
        User::Exit(KErrCancel);
    *iMessage=aMessage;
    *iParam=aParam;
    TRequestStatus replyStatus=KRequestPending;
    *iReplyStatus=(&replyStatus);
    TRequestStatus* pS=iThreadStatus;
    iThread.RequestComplete(pS,0);
    User::WaitForRequest(replyStatus,iLogonStatus);
}

void CAdvConsoleClient::Read(TRequestStatus& aStatus)
{
    aStatus=KRequestPending;
    SendReceive(ERead,&aStatus);
}

void CAdvConsoleClient::ReadCancel()
{
    SendReceive(EReadCancel,NULL);
}

void CAdvConsoleClient::Write(const TDesC& aDes)
{
    SendReceive(EWrite,&aDes);
}

TPoint CAdvConsoleClient::CursorPos() const
{
    return(iScreen->CursorPos());
}

TPoint CAdvConsoleClient::PointerPos()
{
    TPoint position;
    TPckg<TPoint> p(position);
    SendReceive(EGetPointerPos, &p);
    return position;
}

void CAdvConsoleClient::SetCursorPosAbs(const TPoint& aPosition)
{
    SendReceive(ESetCursorPosAbs,&aPosition);
}

void CAdvConsoleClient::SetCursorPosRel(const TPoint &aVector)
{
    SendReceive(ESetCursorPosRel,&aVector);
}

void CAdvConsoleClient::SetCursorHeight(TInt aPercentage)
{
    SendReceive(ESetCursorHeight,aPercentage);
}

void CAdvConsoleClient::SetTitle(const TDesC& aTitle)
{
    SendReceive(ESetTitle,&aTitle);
}

void CAdvConsoleClient::ClearScreen()
{
    SendReceive(EClearScreen,NULL);
}

void CAdvConsoleClient::ClearToEndOfLine()
{
    SendReceive(EClearToEndOfLine,NULL);
}

void CAdvConsoleClient::ClearChars(const TRect &aRect , TUint /*aCharacterAttributes*/ )	
{
	SendReceive(EClearChars, &aRect);
}

void CAdvConsoleClient::ScrollChars(const TRect &anArea,const TPoint &/*aVector */)
{
	SendReceive(EScrollChars, &anArea);
}

TSize CAdvConsoleClient::ScreenSize() const
{
	TSize tempsize;

	//Do it this way as to allow dynamic screen size changing
	tempsize.iWidth = iScreen->ConsoleControl()->Size().iWidth/iScreen->ConsoleControl()->CharSize().iWidth - 2;
	tempsize.iHeight = iScreen->ConsoleControl()->Size().iHeight/iScreen->ConsoleControl()->CharSize().iHeight;
  
	return(tempsize);
}

TSize CAdvConsoleClient::ScreenSizePixels() const
{
	return(iScreen->ConsoleControl()->Size());
}


//void CAdvConsoleClient::ScreenZoom()
//{
//	SendReceive(EScreenZoom, NULL);
//}

TKeyCode CAdvConsoleClient::KeyCode() const
{
    return((TKeyCode)iKeyEvent.iCode);
}

TUint CAdvConsoleClient::KeyModifiers() const
{
    return(iKeyEvent.iModifiers);
}

TInt CAdvConsoleClient::KeyHit() 
{
    TInt Status = 0;
    TPckg<TInt> p(Status);
    SendReceive(EKeyHit, &p);
    return Status;
}

void CAdvConsoleClient::SetAttr(int attrib)
{
    SendReceive(ESetAttr,attrib);
}

void CAdvConsoleClient::OpenDialog(TDes& aDes)
{
    SendReceive(EOpenDialog,&aDes);
}

void CAdvConsoleClient::SaveDialog(TDes& aDes)
{
    SendReceive(ESaveDialog,&aDes);
}

void CAdvConsoleClient::PictureDialog(type8* picdata, TSize aSize, type16* apalette)
{
	ECBITMAP passdata;
	passdata.picdata = picdata;
	passdata.aSize = aSize;
	passdata.palette = apalette;

    SendReceive(EPictureDialog,&passdata); 
}

void CAdvConsoleClient::RestoreScreen()
{
    SendReceive(ERestoreScreen,NULL); 
}

void CAdvConsoleClient::ClearGraphicScreen()
{
    SendReceive(EClearGraphicScreen,NULL); 
}

void CAdvConsoleClient::PictureTitle(TDes& aDes)
{
    SendReceive(EPictureTitle,&aDes);
}


CAdvConsoleClient* NewAdvConsole()
{
#if defined(__WINS__)
    // return null if the graphical window server thread is not running
    TFindThread findT(_L("Wserv"));
    TFullName name;
    if (findT.Next(name)!=KErrNone)
        return(NULL);
#endif
    return(new CAdvConsoleClient);
}
