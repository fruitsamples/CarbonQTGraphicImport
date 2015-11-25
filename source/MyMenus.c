/*	File:		MyMenus.c		Description:Functions for dealing with menus and the menu bar.	Author:		MC	Copyright: 	� Copyright 1999-2000 Apple Computer, Inc. All rights reserved.		Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.				("Apple") in consideration of your agreement to the following terms, and your				use, installation, modification or redistribution of this Apple software				constitutes acceptance of these terms.  If you do not agree with these terms,				please do not use, install, modify or redistribute this Apple software.				In consideration of your agreement to abide by the following terms, and subject				to these terms, Apple grants you a personal, non-exclusive license, under Apple�s				copyrights in this original Apple software (the "Apple Software"), to use,				reproduce, modify and redistribute the Apple Software, with or without				modifications, in source and/or binary forms; provided that if you redistribute				the Apple Software in its entirety and without modifications, you must retain				this notice and the following text and disclaimers in all such redistributions of				the Apple Software.  Neither the name, trademarks, service marks or logos of				Apple Computer, Inc. may be used to endorse or promote products derived from the				Apple Software without specific prior written permission from Apple.  Except as				expressly stated in this notice, no other rights or licenses, express or implied,				are granted by Apple herein, including but not limited to any patent rights that				may be infringed by your derivative works or by other works in which the Apple				Software may be incorporated.				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN				COMBINATION WITH YOUR PRODUCTS.				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.					Change History (most recent first):*/#define TARGET_API_MAC_CARBON 1#include <ToolUtils.h>#include "MyMenus.h"#include "NavServices.h"#include "Preferences.h"#include "Utilities.h"#include "WindowCode.h"#include "MyCarbonPrinting.h"#include "Defines.h"extern Boolean				gDone;OSErr MenuBarInit (void) {	Handle					menuBar;	MenuHandle				menu;	OSErr					err;	err = noErr;	menuBar = GetNewMBar (128);	if (menuBar != nil) {		SetMenuBar (menuBar);		menu = GetMenuHandle (128);		if (menu != nil) {			DrawMenuBar ();		} else {			err = memFullErr;		}	} else {		err = memFullErr;	}	return err;}static OSErr HandleAppleChoice (short item) {	OSErr						err;	err = noErr;	if (item == iAbout) {		DisplayAbout ();	}	return err;}static OSErr HandleFileOpen (void) {	OSErr						err;	err = NavGetFilePreview ();	return err;}static OSErr HandleFileChoice (short item) {	OSErr						err;	WindowPtr					theWindow;	err = noErr;	switch (item) {		case iNew:			err = MakeNewWindow (nil, true, (WindowPtr)-1L, &theWindow);			if (err != noErr) {				CloseThisWindow (theWindow);			}			break;		case iOpen:			err = HandleFileOpen ();			break;		case iClose:			if (OptionKeyDown ()) {				do {					CloseThisWindow (FrontWindow ());				} while (FrontWindow () != nil);			} else {				CloseThisWindow (FrontWindow ());						}			break;		case iPageSetup:			err = HandlePageSetupDialog (FrontWindow ());			break;		case iPrint:			err = HandlePrint (FrontWindow ());			break;		case iQuit:			gDone = true;			break;	}	return err;}static OSErr HandleEditChoice (short item) {	OSErr		err;	switch (item) {		case iUndo:					/* not supported */		case iCut:		case iCopy:		case iPaste:		case iClear:			err = noErr;			break;		case iPreferences:			err = DoPreferencesDialog ();			break;	}	return err;}OSErr DispatchMenuChoice (long menuChoice) {	short		menu;	short		item;	OSErr		err;	if (menuChoice != 0) {		menu = HiWord (menuChoice);		item = LoWord (menuChoice);	}	switch (menu) {		case mApple:			err = HandleAppleChoice (item);			break;		case mFile:			err = HandleFileChoice (item);			break;		case mEdit:			err = HandleEditChoice (item);			break;		default:			err = noErr;			break;	}	HiliteMenu (0);	return err;}