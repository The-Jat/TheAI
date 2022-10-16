/*
 * Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk.
 * Distributed under the terms of the MIT License.
 */
#ifndef NNWINDOW_H
#define NNWINDOW_H


#include <Bitmap.h>
#include <Window.h>


enum {
	kActionKillTeam,
	kActionDebugTeam,
	kActionWriteCoreFile,
	kActionSaveReportTeam,
	kActionPromptUser
};


//#define HANDOVER_USE_GDB 1
#define HANDOVER_USE_DEBUGGER 1

#define USE_GUI true
	// define to false if the debug server shouldn't use GUI (i.e. an alert)

class nnWindow : public BWindow {
public:
					nnWindow(const char* appName);
					~nnWindow();

	void			MessageReceived(BMessage* message);
	int32			Go();

private:
	static	BRect	IconSize();

private:
	BBitmap			fBitmap;
	BButton*		fOKButton;
	sem_id			fSemaphore;
	int32			fAction;
};


#endif /* !NNWINDOW_H */
