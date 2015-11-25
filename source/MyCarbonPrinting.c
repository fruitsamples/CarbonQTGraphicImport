/*	File:		MyCarbonPrinting.c		Description:Code that prints pictures via the new Carbon Print Manager.	Author:		MC	Copyright: 	� Copyright 1999-2000 Apple Computer, Inc. All rights reserved.		Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.					Change History (most recent first):*/#define TARGET_API_MAC_CARBON 1#include <QuickTimeComponents.h>#include <Windows.h>#include "MyCarbonPrinting.h"#include "Defines.h"#include "Structs.h"#include "GraphicImportDrawCode.h"#include "LinkedList.h"#include "DisplayErrorAlert.h"// Globalsextern PreferencesHandle			gPreferences;static void PMRectToRect (PMRect * pRect, Rect * rRect) {	rRect->top = (short)pRect->top;	rRect->bottom = (short)pRect->bottom;	rRect->left = (short)pRect->left;	rRect->right = (short)pRect->right;}static UInt32 CountRectsInRect (Rect * pageRect, Rect * imageRect) {	UInt32						count;	Rect						tempPageRect,								tempImageRect;	UInt32						pagesAcross,								pagesDown;	tempPageRect = *pageRect;	tempImageRect = *imageRect;	OffsetRect (&tempPageRect, (short)-tempPageRect.left, (short)-tempPageRect.top);	OffsetRect (&tempImageRect, (short)-tempImageRect.left, (short)-tempImageRect.top);	pagesAcross = (UInt32)((tempImageRect.right + (tempPageRect.right - 1)) / tempPageRect.right);	pagesDown = (UInt32)((tempImageRect.bottom + (tempPageRect.bottom - 1)) / tempPageRect.bottom);	count = pagesAcross * pagesDown;	return count;}static OSStatus MakeDefaultPageFormatObject (PMPageFormat * pageFormat) {	OSStatus				err;	//	Set up a valid PageFormat object.	if (*pageFormat == kPMNoPageFormat) {		err = PMNewPageFormat (pageFormat);		if ((err == noErr) && (*pageFormat != kPMNoPageFormat))			err = PMDefaultPageFormat (*pageFormat);	} else {		err = PMValidatePageFormat (*pageFormat, kPMDontWantBoolean);	}	return err;}static OSErr TileRectsInRect (Rect * pageRect, Rect * imageRect, myListPtr * overlapList) {	Rect						tempImageRect,								tempPageRect,								overlappingRect;	Rect *						rectPtr;	Boolean						overlap;	OSErr						err;	tempImageRect = *imageRect;	tempPageRect = *pageRect;	do {		do {			overlap = SectRect (&tempImageRect, &tempPageRect, &overlappingRect);			if (overlap) {				rectPtr = (Rect *)NewPtr (sizeof(Rect));				err = MemError ();				if (err == noErr) {					*rectPtr = overlappingRect;					err = AppendToList (rectPtr, overlapList);					OffsetRect (&tempPageRect, tempPageRect.right, 0);				}			}		} while (overlap == true);		tempPageRect.right = pageRect->right;		tempPageRect.left = pageRect->left;		OffsetRect (&tempPageRect, 0, tempPageRect.bottom);		overlap = SectRect (&tempImageRect, &tempPageRect, &overlappingRect);	} while (overlap == true && err == noErr);	return err;}/*------------------------------------------------------------------------------	Function:	DetermineNumberOfPagesInDoc	Parameters:		pageRect	- size of the document, from PMGetAdjustedPageRect	Description:		Compares the size of the document with the number of pages in the		PageFormat object to calculate the number of pages required to print		the document.------------------------------------------------------------------------------*/static UInt32 DetermineNumberOfPagesInDoc (PMRect pageRect, WindowPtr theWindow, myListPtr * rectsList) {#pragma unused (pageRect)	WindowInfoHandle			winInfo;	SInt8						hState;	UInt32						pageCount;	Rect						pRect;	OSErr						err;	pageCount = 0;	if (theWindow != nil) {		winInfo = (WindowInfoHandle)GetWRefCon (theWindow);	}	if (winInfo != nil && (**winInfo).sig == kMySig) {		hState = HGetState ((Handle)winInfo);		HLock ((Handle)winInfo);		PMRectToRect (&pageRect, &pRect);		pageCount = CountRectsInRect (&pRect, &(**winInfo).winNatGraphicBoundsRect);		if (pageCount >= 1) {			err = TileRectsInRect (&pRect, &(**winInfo).winNatGraphicBoundsRect, rectsList);		}		HSetState ((Handle)winInfo, hState);	}	return pageCount;}static OSStatus DrawIntoPrintPort (WindowInfoHandle winInfo, Rect * sourceRect) {	OSStatus					err;	GraphicsImportComponent		graphicImporter;	Rect						tempRect;	if ((**winInfo).winDataHandle != nil) {		graphicImporter = OpenDefaultComponent (GraphicsImporterComponentType, (**winInfo).winGraphicType);		if (graphicImporter != nil) {			err = GraphicsImportSetDataHandle (graphicImporter, (**winInfo).winDataHandle);		}	} else {		err = GetGraphicsImporterForFile (&(**winInfo).winSpec, &graphicImporter);	}	if (err == noErr) {		err = GraphicsImportGetSourceRect (graphicImporter, &tempRect);	}	if (err == noErr) {		err = GraphicsImportSetSourceRect (graphicImporter, sourceRect);	}	if (err == noErr) {		OffsetRect (&tempRect, (short)-sourceRect->left, (short)-sourceRect->top);		err = GraphicsImportSetBoundsRect (graphicImporter, &tempRect);	}	if (err == noErr) {		(void)GraphicsImportSetQuality ((**winInfo).winGraphicImporter, (**gPreferences).quality);	}	if (err == noErr) {		err = GraphicsImportDraw (graphicImporter);	}	if (graphicImporter != nil) {		(void)CloseComponent (graphicImporter);	}	return err;}#define PREVIEWPRINTING 0static OSErr PrintPage (WindowPtr theWindow, PMRect pageRect, PMPrintContext printingPort, UInt32 pageNumber, Rect * printRect) {#pragma unused (pageNumber)	OSStatus					err;	WindowInfoHandle			winInfo;	SInt8						hState;	Rect						pRect;	PixMapHandle				pixMap;#if PREVIEWPRINTING	WindowPtr					testWindow;	GrafPtr						oldPort;	Rect						winBounds;#pragma unused (printingPort)#else	GrafPtr						pmPort;#endif	if (theWindow != nil) {		winInfo = (WindowInfoHandle)GetWRefCon (theWindow);	}	if (winInfo != nil && (**winInfo).sig == kMySig) {		hState = HGetState ((Handle)winInfo);		HLock ((Handle)winInfo);		PMRectToRect (&pageRect, &pRect);		pRect.right = (short)(printRect->right - printRect->left);		pRect.bottom = (short)(printRect->bottom - printRect->top);		if ((**winInfo).winGWorld != nil) {			pixMap = GetGWorldPixMap ((**winInfo).winGWorld);			if (LockPixels (pixMap)) {#if PREVIEWPRINTING				GetPort (&oldPort);				testWindow = NewWindow(nil, &pRect, "\ptest", true, 0, (WindowPtr)-1L, true, 0);				SetPort (GetWindowPort(testWindow));				GetWindowPortBounds (testWindow, &winBounds);				CopyBits (GetPortBitMapForCopyBits ((**winInfo).winGWorld),							GetPortBitMapForCopyBits (GetWindowPort(testWindow)),							printRect, &pRect, srcCopy, nil);				while (!Button ()) {}				DisposeWindow (testWindow);				SetPort (oldPort);#else				PMGetGrafPtr (printingPort, &pmPort);				CopyBits ((BitMap*)*GetPortPixMap ((**winInfo).winGWorld),							(BitMap*)*GetPortPixMap (pmPort),							printRect, &pRect, srcCopy, nil);#endif				UnlockPixels (pixMap);			} else {				DisposeGWorld ((**winInfo).winGWorld);				(**winInfo).winGWorld = nil;				err = DrawIntoPrintPort (winInfo, printRect);			}		} else {#if PREVIEWPRINTING			GetPort (&oldPort);			testWindow = NewWindow(nil, &pRect, "\ptest", true, 0, (WindowPtr)-1L, true, 0);			SetPort (GetWindowPort(testWindow));			GetWindowPortBounds (testWindow, &winBounds);			err = DrawIntoPrintPort (winInfo, printRect);			while (!Button ()) {}			DisposeWindow (testWindow);			SetPort (oldPort);#else			err = DrawIntoPrintPort (winInfo, printRect );#endif		}		HSetState ((Handle)winInfo, hState);	}	return (OSErr)err;}static OSErr PrintWindow (WindowPtr theWindow, PMPageFormat * pageFormat, PMPrintSettings * printSettings) {	OSStatus					err,								printError;	PMRect						pageRect;	PMPrintContext				thePrintingPort;	UInt32						realNumberOfPagesinDoc,								pageNumber,								firstPage,								lastPage;	myListPtr					rectsList;	Rect *						printRect;	thePrintingPort = kPMNoReference;	//	PMGetAdjustedPageRect returns the page size taking into account rotation,	//	resolution and scaling settings.  DetermineNumberOfPagesInDoc returns the	//	number of pages required to print the document.	err = PMGetAdjustedPageRect (*pageFormat, &pageRect);	if (err == noErr) {		rectsList = nil;		realNumberOfPagesinDoc = DetermineNumberOfPagesInDoc (pageRect, theWindow, &rectsList);	}	//	Get the user's selection for first and last pages	if (err == noErr) {		err = PMGetFirstPage (*printSettings, &firstPage);		if (err == noErr) {			err = PMGetLastPage (*printSettings, &lastPage);		}	}	//	Check that the selected page range does not go beyond the actual	//	number of pages in the document.	if (err == noErr) {		if (realNumberOfPagesinDoc < lastPage) {			lastPage = realNumberOfPagesinDoc;		}	}	//	NOTE:  We don't have to worry about the number of copies.  The Printing	//	Manager handles this.  So we just iterate through the document from the	//	first page to be printed, to the last.	if (err == noErr) {		//	Establish a printing port for drawing.		err = PMBeginDocument (*printSettings, *pageFormat, &thePrintingPort);		if ((err == noErr) && (thePrintingPort != kPMNoReference)) {			//	Print the selected range of pages in the document.			pageNumber = firstPage;			while ((pageNumber <= lastPage) && (err == noErr) && (PMError () == noErr)) {				//	NOTE:  We don't have to deal with the old Printing Manager's				//	128-page boundary limit anymore.				//	Establish a printing page.				err = PMBeginPage (thePrintingPort, nil);				if (err == noErr) {					//	Draw the page.					printRect = (Rect *)GetItemNumFromList (pageNumber - 1, rectsList);					err = PrintPage (theWindow, pageRect, thePrintingPort, pageNumber, printRect);				}				//	Close the page.  Have to call PMEndPage if we called PMBeginPage				err = PMEndPage (thePrintingPort);				if (err == noErr) {					//	And loop.					pageNumber++;				}			}			// Close the printing port			(void)PMEndDocument (thePrintingPort);		}	}	//	Only report a printing error once we have completed the print loop. This	//	ensures that every PMBeginXXX call is followed by a matching PMEndXXX	//	call, so the Printing Manager can release all temporary memory and close	//	properly.	printError = PMError ();	if (printError != noErr) {		DisplayErrorAlert ((OSErr)printError);	}	do {		printRect = (Rect *)GetItemNumFromList (0, rectsList);		DeleteHeadFromList (&rectsList);	} while (printRect != nil);	return (OSErr)err;}/*------------------------------------------------------------------------------	Function:		FlattenAndSavePageFormat	Parameters:		pageFormat	-	a PageFormat object	Description:		Flattens a PageFormat object so it can be saved with the document.		Assumes caller passes a validated PageFormat object.------------------------------------------------------------------------------*/static OSStatus FlattenAndSavePageFormat (PMPageFormat pageFormat, Handle * flatFormatHandle) {	OSStatus				err;	//	Flatten the PageFormat object to memory.	err = PMFlattenPageFormat (pageFormat, flatFormatHandle);	return err;}/*------------------------------------------------------------------------------	Function:	LoadAndUnflattenPageFormat	Parameters:		pageFormat	- PageFormat object read from document file	Description:		Gets flattened PageFormat data from the document and returns a PageFormat		object.------------------------------------------------------------------------------*/static OSStatus LoadAndUnflattenPageFormat (PMPageFormat * pageFormat, Handle flatFormatHandle) {	OSStatus				err;	//	Convert the PageFormat flattened data into a PageFormat object.	err = PMUnflattenPageFormat (flatFormatHandle, pageFormat);		return err;}static OSStatus FlattenAndSavePrintSettings (PMPrintSettings printSettings, Handle * flatFormatHandle) {	OSStatus				err;	//	Flatten the PrintSettings object to memory.	err = PMFlattenPrintSettings (printSettings, flatFormatHandle);	return err;}static OSStatus LoadAndUnflattenPrintSettings (PMPrintSettings * printSettings, Handle flatFormatHandle) {	OSStatus				err;	//	Convert the PrintSettings flattened data into a PrintSettings object.	err = PMUnflattenPrintSettings (flatFormatHandle, printSettings);		return err;}/*------------------------------------------------------------------------------	Function:	DoPageSetupDialog	Parameters:		pageFormat	-	a PageFormat object addr	Description:		If the caller passes in an empty PageFormat object, create a new one,		otherwise validate the one provided by the caller.		Invokes the Page Setup dialog and checks for Cancel.		Flattens the PageFormat object so it can be saved with the document.		Note that the PageFormat object is modified by this function.------------------------------------------------------------------------------*/static OSErr DoPageSetupDialog (PMPageFormat * pageFormat, Handle * flatFormatHandle) {	OSStatus				err;	Boolean					accepted;	err = MakeDefaultPageFormatObject (pageFormat);	//	Display the Page Setup dialog.	if (err == noErr) {		err = PMPageSetupDialog (*pageFormat, &accepted);		if (!accepted) {			err = kPMCancel;		// user clicked Cancel button		}	}	//	If the user did not cancel, flatten and save the PageFormat object	//	with our document.	if (err == noErr) {		err = FlattenAndSavePageFormat (*pageFormat, flatFormatHandle);	}	return (OSErr)err;}/*------------------------------------------------------------------------------	Function:	DoPrintDialog	Parameters:		pageFormat		-	a PageFormat object addr		printSettings	-	a PrintSettings object addr	Description:		If the caller passes an empty PrintSettings object, create a new one,		otherwise validate the one provided by the caller.		Invokes the Print dialog and checks for Cancel.		Note that the PrintSettings object is modified by this function.------------------------------------------------------------------------------*/static OSErr DoPrintDialog (PMPageFormat * pageFormat, PMPrintSettings * printSettings) {	OSStatus		err;	Boolean			accepted;		//	In this sample code the caller provides a valid PageFormat reference but in	//	your application you may want to load and unflatten the PageFormat object	//	that was saved at PageSetup time.  See LoadAndUnflattenPageFormat below.		//	Set up a valid PrintSettings object.	if (*printSettings == kPMNoPrintSettings) {		err = PMNewPrintSettings (printSettings);				if ((err == noErr) && (*printSettings != kPMNoPrintSettings))			err = PMDefaultPrintSettings (*printSettings);	} else {		err = PMValidatePrintSettings (*printSettings, kPMDontWantBoolean);	}		//	Display the Print dialog.	if (err == noErr) {		err = PMPrintDialog (*printSettings, *pageFormat, &accepted);		if (!accepted) {			err = kPMCancel;		// user clicked Cancel button		}	}			return (OSErr)err;}OSErr HandlePageSetupDialog (WindowPtr theWindow) {	OSStatus					err;	WindowInfoHandle			winInfo;	SInt8						hState;	PMPageFormat				pageFormat;	err = PMBegin ();	if (err == noErr) {		if (theWindow != nil) {			winInfo = (WindowInfoHandle)GetWRefCon (theWindow);		}		if (winInfo != nil && (**winInfo).sig == kMySig) {			hState = HGetState ((Handle)winInfo);			HLock ((Handle)winInfo);			if ((**winInfo).flatPageFormatHandle == nil) {				pageFormat = kPMNoPageFormat;			} else {				err = LoadAndUnflattenPageFormat (&pageFormat, (**winInfo).flatPageFormatHandle);			}			err = DoPageSetupDialog (&pageFormat, &(**winInfo).flatPageFormatHandle);			HSetState ((Handle)winInfo, hState);		}	}	(void)PMEnd ();	return (OSErr)err;}OSErr HandlePrint (WindowPtr theWindow) {	OSStatus					err;	WindowInfoHandle			winInfo;	SInt8						hState;	PMPageFormat				pageFormat;	PMPrintSettings				printSettings;	err = (OSErr)PMBegin ();	if (err == noErr) {		if (theWindow != nil) {			winInfo = (WindowInfoHandle)GetWRefCon (theWindow);		}		if (winInfo != nil && (**winInfo).sig == kMySig) {			hState = HGetState ((Handle)winInfo);			HLock ((Handle)winInfo);			if ((**winInfo).flatPageFormatHandle == nil) {				// Just use the default Page Setup options				pageFormat = kPMNoPageFormat;				err = (OSErr)MakeDefaultPageFormatObject (&pageFormat);				if (err == noErr) {					err = (OSErr)FlattenAndSavePageFormat (pageFormat, &(**winInfo).flatPageFormatHandle);				}			} else {				err = (OSErr)LoadAndUnflattenPageFormat (&pageFormat, (**winInfo).flatPageFormatHandle);			}			if ((**winInfo).flatPrintSettingsHandle != nil) {				err = (OSErr)LoadAndUnflattenPrintSettings (&printSettings, (**winInfo).flatPrintSettingsHandle);			} else {				printSettings = kPMNoPrintSettings;			}			err = DoPrintDialog (&pageFormat, &printSettings);			//	If the user did not cancel, flatten and save the PrintSettings object			//	with our document.			if (err == noErr) {				err = (OSErr)FlattenAndSavePrintSettings (printSettings, &(**winInfo).flatPrintSettingsHandle);			}			if (err == noErr) {				err = PrintWindow (theWindow, &pageFormat, &printSettings);			}			HSetState ((Handle)winInfo, hState);		}	}	(void)PMEnd ();	return (OSErr)err;}