
// SkyCaptureDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include <vector>
#include <gdiplus.h>
using namespace Gdiplus;


struct sckeyinfo_t
{
	byte Char;
	byte ScanCode;
	bool Shift;
	int	 VirtualKey;
};


struct scshotinfo_t
{
	int	  ShotIndex;
	int   CellX;
	int   CellY;

	scshotinfo_t(const int i, const int x, const int y)  :
		ShotIndex(i), CellX(x), CellY(y)
	{
	}

};

typedef std::vector<scshotinfo_t> CScShotInfoArray;


struct scloc_t {
	int x;
	int y;
};

typedef std::vector<scloc_t> CScLocationArray;

#define SKYCAP_HOTKEY_ACTIVATE 0x1234
#define SKYRIM_CELLSIZE 4096
#define SKYCAP_STARTDELAY 5000
#define SKYCAP_KEYDELAY 50
#define SKYCAP_CMDDELAY 400
#define SKYCAP_CMDPOSDELAY 500
#define SKYCAP_LOADCELLDELAY 10000
#define SKYCAP_SCREENSHOTDELAY 1000
#define SKYCAP_HOTKEYDELAY 2000
#define SKYCAP_ENABLEINGAMEHOTKEY false

#define SKYCAP_AFTERLOADDELAY 30000

	// 1 = 5x3 cells at 256x256
	// 2 = 5x1 cells at 512x512
#define SKYCAP_CELLSIZE_DIFFTYPE 2

#define SKYCAP_CKIMAGE_OUTPUTPATH "d:\\skyrimmaps\\finalpng\\"
#define SKYCAP_CKIMAGE_RAWPATH     "raw"
#define SKYCAP_CKIMAGE_MAPPATH     "maps"
#define SKYCAP_CKIMAGE_NOWATERPATH "nowater"


class CSkyCKCaptureInfo
{
public:
	int		CellOffsetX;
	int		CellOffsetY;
	int		CellCountX;
	int		CellCountY;
	int		CellSizeX;
	int		CellSizeY;
	int		CellX;
	int		CellY;
	int		CellRangeX1;
	int		CellRangeX2;
	int		CellRangeY1;
	int		CellRangeY2;
	int		CellOutputOffsetX;
	int		CellOutputOffsetY;
	CString OutputPath;


};

class CSkyCaptureDlg : public CDialogEx
{
public:
	CWinThread*	m_pWorkerThread;
	CWinThread*	m_pWorkerCKThread;
	HANDLE		m_RunEvent;
	HANDLE		m_WaitCKEvent;
	HANDLE		m_RunCKEvent;
	bool		m_IsRunning;
	HWND		m_hRenderView;
	HWND		m_hCellView;
	HWND		m_hXCellWnd;
	HWND		m_hYCellWnd;
	HWND		m_hGoCellWnd;
	HWND		m_hListCellWnd;
	int			m_CellChildCount;
	bool		m_IsAutoCKRunning;
	bool		m_AutoOnlyDoMissing;

	int		m_NextScreenShotIndex;
	float	m_PlayerPosX;
	float	m_PlayerPosY;
	bool	m_IsInConsole;
	bool	m_ToggleBorders;
	int		m_CaptureCellCount;
	int		m_CaptureCellResize;
	bool	m_SaveasPNG;

	int		m_StartCellX;
	int		m_EndCellX;
	int		m_StartCellY;
	int		m_EndCellY;

	CScShotInfoArray	m_ShotInfos;

	CScLocationArray	m_MissingShots;

public:
	CSkyCaptureDlg(CWnd* pParent = NULL);
	virtual ~CSkyCaptureDlg();

	static sckeyinfo_t s_KeyInfos[256];
	static bool s_InitKeyInfos;
	void InitKeyInfos (void);
	void ResetInGameScreenshots (void);
	void DeleteInGameScreenshots (void);
	void BackupInGameScreenshots (void);
	void CopyInGameScreenshots (const char* pDestPath);
	void SetupCameraPos (void);
	void SendConsoleCmd (const char* pString, ...);
	void EnterConsoleMode (void);
	void LeaveConsoleMode (void);
	void TakeInGameScreenshots (void);
	void TakeInGameScreenshot (const int CellX, const int CellY);
	void OnActivateHotKey (void);

	bool CheckIfCKCellLoaded (const int CellX, const int CellY);

	void ProcessCKImage(Bitmap& RawBitmap, Bitmap& NoWaterBmp);
	void SplitCKImage(Bitmap& RawBitmap, CSkyCKCaptureInfo& CaptureInfo, const char* pSubPath);

	CSkyCKCaptureInfo GetCurrentCKData (void);

	bool CKMoveToCell (const int CellX, const int CellY);

	HWND FindCKRenderWindow (void);
	HWND FindCKCellViewWindow (void);
	void ResetCKRenderWindow (void);

	UINT DoWorkerThread (void);
	UINT DoCKWorkerThread (void);

	int FindLastInGameScreenshotIndex (void);
	void CopyMapInGameScreenshot (const scshotinfo_t ShotInfo);

	enum { IDD = IDD_SKYCAPTURE_DIALOG };

	virtual void DoDataExchange(CDataExchange* pDX);

	void PrintLog (const char* pString, ...);

	void SimulateKeyPress (const BYTE KeyCode);
	void SimulateKeyPress (const char* pString);
	void SimulateKeyPrintScreen (void);
	void SimulateKeyPress1 (const BYTE InputCode);
	void SimulateKeyWindowsAltPrintScreen (void);
	void SimulateShiftKeyPress (const TCHAR KeyChar);

	void SimulateWindowsKeyPress (const BYTE KeyCode, HWND hWnd);

	HWND FindSkyrimWindow (void);

	bool UnregisterHotKeys (void);
	bool RegisterHotKeys (void);

	bool PrintWindowsError (const char* pString, ...);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

public:

	CRichEditCtrl m_LogText;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedFindRenderview();
	afx_msg void OnBnClickedTestSendmouseevent();
	afx_msg void OnBnClickedCellgobutton();
	CEdit m_XCellText;
	CEdit m_YCellText;
	afx_msg void OnBnClickedResetRenderview();
	afx_msg void OnBnClickedCaptureRenderview();
	CEdit m_OutputPathText;
	CEdit m_OutputCellSizeXText;
	CEdit m_OutputOffsetXText;
	CEdit m_OutputOffsetYText;
	CEdit m_OutputCountXText;
	CEdit m_OutputCountYText;
	CEdit m_OutputCellRangeY1;
	CEdit m_OutputCellRangeY2;
	CEdit m_OutputCellRangeX1;
	CEdit m_OutputCellRangeX2;
	afx_msg void OnBnClickedTestNowater();
	CEdit m_OutputCellSizeYText;
	afx_msg void OnBnClickedCellstartbutton();
	afx_msg void OnBnClickedCellnextbutton();
	afx_msg void OnBnClickedTestCheckfiles();
	afx_msg void OnBnClickedStartautocapture();
	CButton m_ToggleAutoButton;
	afx_msg void OnBnClickedAutoskipwait();
	afx_msg void OnBnClickedTestChecklist();
	CButton m_OnlyMissingCheck;
	afx_msg void OnBnClickedTestLoadcheck();
};
