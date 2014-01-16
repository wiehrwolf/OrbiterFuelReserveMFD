#ifndef __FUELMFD_H
#define __FUELMFD_H

class FuelMFD: public MFD {
public:
	FuelMFD (DWORD w, DWORD h, VESSEL *vessel);
	~FuelMFD ();
	bool ConsumeKeyBuffered (DWORD key);
	bool ConsumeButton (int bt, int event);
	char *ButtonLabel (int bt);
	int  ButtonMenu (const MFDBUTTONMENU **menu) const;
	void Update (HDC hDC);
	void FuelBar (HDC hDC, int tank_nr, int cur_line);
	static int MsgProc (UINT msg, UINT mfd, WPARAM wparam, LPARAM lparam);
	void StoreStatus (void) const;
	void RecallStatus (void);
	char PotToSuffix (int i);

private:
	void InitReferences (void);
	OBJHANDLE ref;
	int page;
	int maxpage;
	int LINE; // Number of pixels to jump to write the next line
	int MAX_LINES; // Maximal # of lines
	int CurrentLine;
	VESSEL *v;
	int disp_mode;

	static struct g_Data { // Save this!
		int disp_mode;
		int page;
	} g_data;
};

#endif //!__FUELMFD_H