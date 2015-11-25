/*	File:		WindowCode.c		Description:Code for dealing with windows and the memory associated with each window.	Author:		MC	Copyright: 	� Copyright 1999-2000 Apple Computer, Inc. All rights reserved.		Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.					Change History (most recent first):*/#define TARGET_API_MAC_CARBON 1#include <LowMem.h>#include <TextUtils.h>#include <TextServices.h>#include "WindowCode.h"#include "MovieCode.h"#include "GraphicImportDrawCode.h"extern	long		gSleepTime;extern	Boolean		gInForeground;void	DisplayAbout (void) {	Alert (500, nil);}OSErr DispatchWindowUpdate (WindowPtr theWindow) {	OSErr						err;	WindowInfoHandle			winInfo;	err = noErr;	if (theWindow != nil) {		winInfo = (WindowInfoHandle)GetWRefCon (theWindow);	}	if (winInfo != nil && (**winInfo).sig == kMySig) {		BeginUpdate (theWindow);		if ((**winInfo).winType == kGraphPicWinType) {			err = UpdateGraphWindow (theWindow, nil);		} else if ((**winInfo).winType == kMovieWinType) {			err = UpdateMovieWindow (theWindow);		}		EndUpdate (theWindow);	}	return err;}void DoActivate (EventRecord *event, Boolean kindOfActivate) {	WindowPtr					theWindow;	WindowInfoHandle			winInfo;	short						activateWindow,								activateProgram;	Boolean						moviePlaying;	OSErr						err;	SInt8						hState;							if (kindOfActivate == true) {		// It is a Activate/Deactivate event		activateWindow = (short)(event->modifiers & activeFlag);		theWindow = (WindowPtr)event->message;		if (theWindow != nil) {			winInfo = (WindowInfoHandle)GetWRefCon (theWindow);		}		if (winInfo != nil && (**winInfo).sig == kMySig) {			hState = HGetState ((Handle)winInfo);			HLock ((Handle)winInfo);			if (activateWindow) {				// Make this window the active window				if ((**winInfo).winType == kMovieWinType) {					if ((**winInfo).winMovieController == nil) {						// Movie has been disposed, get it back						err = DrawMovie (&((**winInfo).winSpec), theWindow, (**winInfo).winMovieTime);					}					MCActivate ((**winInfo).winMovieController, theWindow, true);				}			} else {				// Make this window no longer the active window				// To save memory we close the component when the window goes into the background.				if ((**winInfo).winMovieController != nil) {					MCActivate ((**winInfo).winMovieController, theWindow, false);					DisposeMovieController ((**winInfo).winMovieController);					(**winInfo).winMovieController = nil;				}				if ((**winInfo).winMovie != nil) {					DisposeMovie ((**winInfo).winMovie);					(**winInfo).winMovie = nil;				}				if ((**winInfo).winGraphicImporter != nil) {					CloseComponent ((**winInfo).winGraphicImporter);					(**winInfo).winGraphicImporter = nil;				}			}			HSetState ((Handle)winInfo, hState);		}	} else {		// It is a Suspend/Resume event		activateProgram = (short)(event->message & suspendResumeMessage);		// Turn off any menu hilite to correct a bug with AMO		HiliteMenu (0);		theWindow = FrontWindow ();		if (activateProgram) {			gInForeground = true;			gSleepTime = kForegroundIdleSleepTime;			while (theWindow != nil) {				winInfo = (WindowInfoHandle)GetWRefCon (theWindow);				if (winInfo != nil && (**winInfo).sig == kMySig) {					hState = HGetState ((Handle)winInfo);					HLock ((Handle)winInfo);					if ((**winInfo).winMovieController != nil) {						if (theWindow == FrontWindow ()) {							MCActivate ((**winInfo).winMovieController, theWindow, true);						}					}					HSetState ((Handle)winInfo, hState);				}				theWindow = GetNextWindow (theWindow);			}		} else {			gInForeground = false;			while (theWindow != nil) {				winInfo = (WindowInfoHandle)GetWRefCon (theWindow);				if (winInfo != nil && (**winInfo).sig == kMySig) {					hState = HGetState ((Handle)winInfo);					HLock ((Handle)winInfo);					if ((**winInfo).winMovieController != nil) {						if (GetMovieRate ((**winInfo).winMovie)) {							moviePlaying = true;						}						MCActivate ((**winInfo).winMovieController, theWindow, false);					}					HSetState ((Handle)winInfo, hState);				}				theWindow = GetNextWindow (theWindow);			}			if (!moviePlaying) {				gSleepTime = kBackgroundIdleSleepTime;			} else {				gSleepTime = kBackgroundBusySleepTime;	// Keep the movies playing well			}		}	}	return;}void	ReleaseMemory (WindowInfoHandle winInfo, Boolean fullDispose) {	SInt8						hState;							if (winInfo != nil && (**winInfo).sig == kMySig && ((**winInfo).winType == kGraphPicWinType || (**winInfo).winType == kMovieWinType)) {		hState = HGetState ((Handle)winInfo);		HLock ((Handle)winInfo);		if ((**winInfo).winGraphicImporter != nil) {			(void)CloseComponent ((**winInfo).winGraphicImporter);			(**winInfo).winGraphicImporter = nil;		}		if ((**winInfo).winMovieController != nil) {			DisposeMovieController ((**winInfo).winMovieController);			(**winInfo).winMovieController = nil;		}		if ((**winInfo).winMovie != nil) {			DisposeMovie ((**winInfo).winMovie);			(**winInfo).winMovie = nil;		}		if ((**winInfo).winGWorld != nil) {			DisposeGWorld ((**winInfo).winGWorld);			(**winInfo).winGWorld = nil;		}		if ((**winInfo).winDataHandle != nil) {			DisposeHandle ((**winInfo).winDataHandle);			(**winInfo).winDataHandle = nil;		}		if ((**winInfo).flatPageFormatHandle != nil) {			DisposeHandle ((**winInfo).flatPageFormatHandle);			(**winInfo).flatPageFormatHandle = nil;		}		if ((**winInfo).flatPrintSettingsHandle != nil) {			DisposeHandle ((**winInfo).flatPrintSettingsHandle);			(**winInfo).flatPrintSettingsHandle = nil;		}		if (fullDispose == true) {			DisposeHandle ((Handle)winInfo);			winInfo = nil;		}		HSetState ((Handle)winInfo, hState);	}}void	CloseThisWindow (WindowPtr theWindow) {	RgnHandle			grayRgn;	WindowInfoHandle	winInfo;	Boolean				hasPalette,						redrawMenuBar;	if (theWindow != nil) {		winInfo = (WindowInfoHandle)GetWRefCon (theWindow);		if (winInfo != nil && (**winInfo).sig == kMySig) {			hasPalette = false;		}		ReleaseMemory (winInfo, true);		SetPalette (theWindow, nil, false);		DisposeWindow (theWindow);	}	if (FrontWindow () == nil) {		MenuHandle	theFileMenu;		ShowCursor ();		theFileMenu = GetMenuHandle (mFile);		if (theFileMenu != nil) {			DisableMenuItem (theFileMenu, iClose);			DisableMenuItem (theFileMenu, iPageSetup);			DisableMenuItem (theFileMenu, iPrint);		}		if (hasPalette == true) {			// Cause all background windows to redraw with the default palette			grayRgn = GetGrayRgn ();			PaintBehind (nil, grayRgn);			redrawMenuBar = true;		}	}	if (redrawMenuBar == true) {		// Redraw the menu bar		DrawMenuBar ();	}}OSErr	MakeNewWindow (Str255 name, Boolean visible, WindowPtr behindWindow, WindowPtr *theWindow) {	OSErr						err;	WindowInfoHandle			winInfo;	WindowPtr					newWindow;	Str255						title;	GetIndString (title, 130, 1);	if (name != nil) {		BlockMoveData (name, title, name[0]+1);	}	winInfo = (WindowInfoHandle)NewHandleClear (sizeof (WindowInfoRec));	if (winInfo != nil) {		Rect	boundsRect = {0, 7, 250, 210};		(**winInfo).sig = kMySig;		(**winInfo).winType = kGraphPicWinType;		(**winInfo).winScale = 0.0;		boundsRect.top = (short)(GetMBarHeight () + 22);		newWindow = NewCWindow (nil, &boundsRect, title, visible, documentProc, behindWindow, true, (long)winInfo);	}	if (newWindow != nil) {		MenuHandle	theFileMenu;		theFileMenu = GetMenuHandle (mFile);		if (theFileMenu != nil) {			EnableMenuItem (theFileMenu, iClose);			EnableMenuItem (theFileMenu, iPageSetup);			EnableMenuItem (theFileMenu, iPrint);		}		*theWindow = newWindow;		err = noErr;	} else {		err = memFullErr;	}	return err;}static OSErr DeleteFile (FSSpec * file) {	return FSpDelete (file);}OSErr MoveFrontWindowFileToTrash (void) {	OSErr						err;	WindowPtr					theWindow;	WindowInfoHandle			winInfo;	FSSpec						fileToDelete;	theWindow = FrontWindow ();	if (theWindow != nil) {		winInfo = (WindowInfoHandle)GetWRefCon (theWindow);	}	if (winInfo != nil && (**winInfo).sig == kMySig) {		BlockMoveData (&(**winInfo).winSpec, &fileToDelete, sizeof (FSSpec));		CloseThisWindow (theWindow);		err = DeleteFile (&fileToDelete);	} else {		err = paramErr;	}	return err;}