// SkyCaptureDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SkyCapture.h"
#include "SkyCaptureDlg.h"
#include "afxdialogex.h"


struct blockinfo_t
{
	byte Type;
	byte Flags;
	byte Extra1;
	byte Extra2;
};

struct octreenode_t
{
	octreenode_t	*pNodes[8];
	int				Size;
	blockinfo_t*	pBlockInfo;
};


//Note: Doesn't play well with GDI+ classes
//#ifdef _DEBUG
//	#define new DEBUG_NEW
//#endif


#define SKYRIM_SCREENSHOT_PATH "D:\\Steam\\steamapps\\common\\skyrim\\"
#define SKYRIM_SCREENSHOT_BACKUPPATH "D:\\Steam\\steamapps\\common\\skyrim\\screenshots\\"
#define SKYRIM_SCREENSHOT_MAPPATH "D:\\Steam\\steamapps\\common\\skyrim\\maps\\"


sckeyinfo_t CSkyCaptureDlg::s_KeyInfos[256] = { 0 };
bool CSkyCaptureDlg::s_InitKeyInfos = false;


class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()




CSkyCaptureDlg::CSkyCaptureDlg(CWnd* pParent)
	: CDialogEx(CSkyCaptureDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	if (!s_InitKeyInfos) InitKeyInfos();

	m_IsRunning = false;
	m_pWorkerThread = NULL;
	m_pWorkerCKThread = NULL;
	m_NextScreenShotIndex = 1;
	m_PlayerPosX = 0;
	m_PlayerPosY = 0;
	m_IsInConsole = false;
	m_StartCellX = 0;
	m_StartCellY = 0;
	m_EndCellX = 4;
	m_EndCellY = 0;
	m_IsAutoCKRunning = false;
	m_AutoOnlyDoMissing = false;
	m_SaveasPNG = true;

	m_ToggleBorders = false;
	m_CaptureCellCount = 4;
	m_CaptureCellResize = 512;

	m_hRenderView  = NULL;
	m_hCellView    = NULL;
	m_hXCellWnd    = NULL;
	m_hYCellWnd    = NULL;
	m_hGoCellWnd   = NULL;
	m_hListCellWnd = NULL;

	m_WaitCKEvent = CreateEvent(NULL, TRUE, FALSE, "SkyCaptureWaitCKEvent");
	m_RunCKEvent  = CreateEvent(NULL, TRUE, FALSE, "SkyCaptureRunCKEvent");
	m_RunEvent    = CreateEvent(NULL, TRUE, FALSE, "SkyCaptureRunEvent");
}


CSkyCaptureDlg::~CSkyCaptureDlg()
{
	if (m_pWorkerThread)
	{
		m_pWorkerThread->ExitInstance();
		m_pWorkerThread = NULL;
	}

	if (m_pWorkerCKThread)
	{
		m_pWorkerCKThread->ExitInstance();
		m_pWorkerCKThread = NULL;
	}

	CloseHandle(m_RunEvent);
	CloseHandle(m_RunCKEvent);
	CloseHandle(m_WaitCKEvent);
}


void CSkyCaptureDlg::InitKeyInfos (void)
{
	memset(s_KeyInfos, 0, sizeof(s_KeyInfos));

#define SETKEYINFO(ch, sc, shift, vk) s_KeyInfos[ch].Char = ch; s_KeyInfos[ch].ScanCode = sc; s_KeyInfos[ch].Shift = shift; s_KeyInfos[ch].VirtualKey = vk; 
	SETKEYINFO('1', 2, false, '1')
	SETKEYINFO('!', 2, true, '1')
	SETKEYINFO('2', 3, false, '2')
	SETKEYINFO('@', 3, true, '2')
	SETKEYINFO('3', 4, false, '3')
	SETKEYINFO('#', 4, true, '3')
	SETKEYINFO('4', 5, false, '4')
	SETKEYINFO('$', 5, true, '4')
	SETKEYINFO('5', 6, false, '5')
	SETKEYINFO('%', 6, true, '5')
	SETKEYINFO('6', 7, false, '6')
	SETKEYINFO('^', 7, true, '6')
	SETKEYINFO('7', 8, false, '7')
	SETKEYINFO('&', 8, true, '7')
	SETKEYINFO('8', 9, false, '8')
	SETKEYINFO('*', 9, true, '8')
	SETKEYINFO('9', 10, false, '9')
	SETKEYINFO('(', 10, true, '9')
	SETKEYINFO('0', 11, false, '0')
	SETKEYINFO(')', 11, true, '0')
	SETKEYINFO('-', 12, false, VK_SUBTRACT)
	SETKEYINFO('_', 12, true, VK_OEM_MINUS)
	SETKEYINFO('=', 13, false, VK_OEM_PLUS)
	SETKEYINFO('+', 13, true, VK_OEM_PLUS)
	SETKEYINFO('\b', 14, false, VK_BACK)
	SETKEYINFO('\t', 15, false, VK_TAB)
	SETKEYINFO('q', 0x10, false, 'Q')
	SETKEYINFO('Q', 0x10, true, 'Q')
	SETKEYINFO('w', 0x11, false, 'W')
	SETKEYINFO('W', 0x11, true, 'W')
	SETKEYINFO('e', 0x12, false, 'E')	
	SETKEYINFO('E', 0x12, true, 'E')
	SETKEYINFO('r', 0x13, false, 'R')
	SETKEYINFO('R', 0x13, true, 'R')
	SETKEYINFO('t', 0x14, false, 'T')
	SETKEYINFO('T', 0x14, true, 'T')
	SETKEYINFO('y', 0x15, false, 'Y')
	SETKEYINFO('Y', 0x15, true, 'Y')
	SETKEYINFO('u', 0x16, false, 'U')
	SETKEYINFO('U', 0x16, true, 'U')
	SETKEYINFO('i', 0x17, false, 'I')
	SETKEYINFO('I', 0x17, true, 'I')
	SETKEYINFO('o', 0x18, false, 'O')
	SETKEYINFO('O', 0x18, true, 'O')
	SETKEYINFO('p', 0x19, false, 'P')
	SETKEYINFO('P', 0x19, true, 'P')
	SETKEYINFO('[', 0x1a, false, VK_OEM_4)
	SETKEYINFO('{', 0x1a, true, VK_OEM_4)
	SETKEYINFO(']', 0x1b, false, VK_OEM_6)
	SETKEYINFO('}', 0x1b, true, VK_OEM_6)
	SETKEYINFO('\n', 0x1c, false, VK_RETURN)
	SETKEYINFO('a', 0x1e, false, 'A')
	SETKEYINFO('A', 0x1e, true, 'A')
	SETKEYINFO('s', 0x1f, false, 'S')
	SETKEYINFO('S', 0x1f, true, 'S')
	SETKEYINFO('d', 0x20, false, 'D')
	SETKEYINFO('D', 0x20, true, 'D')
	SETKEYINFO('f', 0x21, false, 'F')
	SETKEYINFO('F', 0x21, true, 'F')
	SETKEYINFO('g', 0x22, false, 'G')
	SETKEYINFO('G', 0x22, true, 'G')
	SETKEYINFO('h', 0x23, false, 'H')
	SETKEYINFO('H', 0x23, true, 'H')
	SETKEYINFO('j', 0x24, false, 'J')
	SETKEYINFO('J', 0x24, true, 'J')
	SETKEYINFO('k', 0x25, false, 'K')
	SETKEYINFO('K', 0x25, true, 'K')
	SETKEYINFO('l', 0x26, false, 'L')
	SETKEYINFO('L', 0x26, true, 'L')
	SETKEYINFO(';', 0x27, false, VK_OEM_1)
	SETKEYINFO(':', 0x27, true, VK_OEM_1)
	SETKEYINFO('\'', 0x28, false, VK_OEM_7)
	SETKEYINFO('"', 0x28, true, VK_OEM_7)
	SETKEYINFO('`', 0x29, false, VK_OEM_3)
	SETKEYINFO('~', 0x29, true, VK_OEM_3)
	SETKEYINFO('\\', 0x2b, false, VK_OEM_5)
	SETKEYINFO('|', 0x2b, true, VK_OEM_5)
	SETKEYINFO('z', 0x2c, false, 'Z')
	SETKEYINFO('Z', 0x2c, true, 'Z')
	SETKEYINFO('x', 0x2d, false, 'X')
	SETKEYINFO('X', 0x2d, true, 'X')
	SETKEYINFO('c', 0x2e, false, 'C')
	SETKEYINFO('C', 0x2e, true, 'C')
	SETKEYINFO('v', 0x2f, false, 'V')
	SETKEYINFO('V', 0x2f, true, 'V')
	SETKEYINFO('b', 0x30, false, 'B')
	SETKEYINFO('B', 0x30, true, 'B')
	SETKEYINFO('n', 0x31, false, 'N')
	SETKEYINFO('N', 0x31, true, 'N')
	SETKEYINFO('m', 0x32, false, 'M')
	SETKEYINFO('M', 0x32, true, 'M')
	SETKEYINFO(',', 0x33, false, VK_OEM_COMMA)
	SETKEYINFO('<', 0x33, true, VK_OEM_COMMA)
	SETKEYINFO('.', 0x34, false, VK_OEM_PERIOD)
	SETKEYINFO('>', 0x34, true, VK_OEM_PERIOD)
	SETKEYINFO('/', 0x35, false, VK_OEM_2)
	SETKEYINFO('?', 0x35, true, VK_OEM_2)
	SETKEYINFO(' ', 0x39, false, VK_SPACE)
#undef SETKEYINFO

	s_InitKeyInfos = true;
}


void CSkyCaptureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LOGTEXT, m_LogText);
	DDX_Control(pDX, IDC_CELLXEDIT, m_XCellText);
	DDX_Control(pDX, IDC_CELLYEDIT, m_YCellText);
	DDX_Control(pDX, IDC_OUTPUTPATH, m_OutputPathText);
	DDX_Control(pDX, IDC_OUTPUTCELLOFFSETX, m_OutputOffsetXText);
	DDX_Control(pDX, IDC_OUTPUTCELLOFFSETY, m_OutputOffsetYText);
	DDX_Control(pDX, IDC_OUTPUTCELLCOUNTX, m_OutputCountXText);
	DDX_Control(pDX, IDC_OUTPUTCELLCOUNTY, m_OutputCountYText);
	DDX_Control(pDX, IDC_CELLRANGEY1, m_OutputCellRangeY1);
	DDX_Control(pDX, IDC_CELLRANGEY2, m_OutputCellRangeY2);
	DDX_Control(pDX, IDC_CELLRANGEX1, m_OutputCellRangeX1);
	DDX_Control(pDX, IDC_CELLRANGEX2, m_OutputCellRangeX2);
	DDX_Control(pDX, IDC_OUTPUTCELLSIZEX, m_OutputCellSizeXText);
	DDX_Control(pDX, IDC_OUTPUTCELLSIZEY, m_OutputCellSizeYText);
	DDX_Control(pDX, IDC_STARTAUTOCAPTURE, m_ToggleAutoButton);
	DDX_Control(pDX, IDC_ONLY_MISSING_CHECK, m_OnlyMissingCheck);
}

BEGIN_MESSAGE_MAP(CSkyCaptureDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_FIND_RENDERVIEW, &CSkyCaptureDlg::OnBnClickedFindRenderview)
	ON_BN_CLICKED(IDC_TEST_SENDMOUSEEVENT, &CSkyCaptureDlg::OnBnClickedTestSendmouseevent)
	ON_BN_CLICKED(IDC_CELLGOBUTTON, &CSkyCaptureDlg::OnBnClickedCellgobutton)
	ON_BN_CLICKED(IDC_RESET_RENDERVIEW, &CSkyCaptureDlg::OnBnClickedResetRenderview)
	ON_BN_CLICKED(IDC_CAPTURE_RENDERVIEW, &CSkyCaptureDlg::OnBnClickedCaptureRenderview)
	ON_BN_CLICKED(IDC_TEST_NOWATER, &CSkyCaptureDlg::OnBnClickedTestNowater)
	ON_BN_CLICKED(IDC_CELLSTARTBUTTON, &CSkyCaptureDlg::OnBnClickedCellstartbutton)
	ON_BN_CLICKED(IDC_CELLNEXTBUTTON, &CSkyCaptureDlg::OnBnClickedCellnextbutton)
	ON_BN_CLICKED(IDC_TEST_CHECKFILES, &CSkyCaptureDlg::OnBnClickedTestCheckfiles)
	ON_BN_CLICKED(IDC_STARTAUTOCAPTURE, &CSkyCaptureDlg::OnBnClickedStartautocapture)
	ON_BN_CLICKED(IDC_AUTOSKIPWAIT, &CSkyCaptureDlg::OnBnClickedAutoskipwait)
	ON_BN_CLICKED(IDC_TEST_CHECKLIST, &CSkyCaptureDlg::OnBnClickedTestChecklist)
	ON_BN_CLICKED(IDC_TEST_LOADCHECK, &CSkyCaptureDlg::OnBnClickedTestLoadcheck)
END_MESSAGE_MAP()


UINT __cdecl l_WorkerThreadProc (LPVOID pParam)
{
	CSkyCaptureDlg* pThis = (CSkyCaptureDlg *) pParam;
	if (pThis) return pThis->DoWorkerThread();
	return 0;
}


UINT __cdecl l_WorkerCKThreadProc (LPVOID pParam)
{
	CSkyCaptureDlg* pThis = (CSkyCaptureDlg *) pParam;
	if (pThis) return pThis->DoCKWorkerThread();
	return 0;
}


UINT CSkyCaptureDlg::DoWorkerThread (void)
{
	DWORD WaitResult;

	while (1)
	{

		WaitResult = WaitForSingleObject(m_RunEvent, INFINITE);

		if (WaitResult == WAIT_OBJECT_0)
		{
			TakeInGameScreenshots();
		}
		else 
		{
			break;
		}	

	}

	return 0;
}


UINT CSkyCaptureDlg::DoCKWorkerThread (void)
{
	DWORD WaitResult;

	while (1)
	{

		WaitResult = WaitForSingleObject(m_RunCKEvent, INFINITE);

		if (WaitResult == WAIT_OBJECT_0)
		{
			PrintLog("AUTO: Resetting current cell view...");
			ResetCKRenderWindow();

			Sleep(2000);

			PrintLog("AUTO: Capturing current cell view...");
			OnBnClickedCaptureRenderview();

			//Sleep(1000);
			//OnBnClickedCaptureRenderview();

			if (m_AutoOnlyDoMissing)
			{
				if (m_MissingShots.empty())
				{
					PrintLog("AUTO: No missing shots to capture...stopping auto capture!");
					ResetEvent(m_RunCKEvent);
					continue;
				}

				scloc_t Loc = m_MissingShots.front();
				CString Buffer;

				m_MissingShots.erase(m_MissingShots.begin());
				PrintLog("AUTO: Moving to missing cell (%d, %d). %d missing shots remain!", Loc.x, Loc.y, m_MissingShots.size());

				Buffer.Format("%d", Loc.x);
				m_XCellText.SetWindowTextA(Buffer);

				Buffer.Format("%d", Loc.y);
				m_YCellText.SetWindowTextA(Buffer);

				CKMoveToCell(Loc.x, Loc.y);
			}
			else
			{
				PrintLog("AUTO: Moving to next cell...");
				OnBnClickedCellnextbutton();
			}

			PrintLog("AUTO: Waiting for cell load...");
			Sleep(4000);

			CWnd* pWnd;

			do {
				Sleep(1000);
				pWnd = FindWindow(NULL, "Loading Cell");
			} while (pWnd != nullptr);				

			PrintLog("AUTO: Waiting %d secs after load...", SKYCAP_AFTERLOADDELAY/1000);
			ResetEvent(m_WaitCKEvent);
			WaitForSingleObject(m_WaitCKEvent, SKYCAP_AFTERLOADDELAY);
		}
		else 
		{
			break;
		}	

	}

	return 0;
}


BOOL CSkyCaptureDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	m_pWorkerThread   = AfxBeginThread(l_WorkerThreadProc, this);
	m_pWorkerCKThread = AfxBeginThread(l_WorkerCKThreadProc, this);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
		
	PrintLog("Welcome to SkyCapture...");
	RegisterHotKeys();

	HWND hWnd = FindSkyrimWindow();

	if (hWnd == NULL)
		PrintLog("Failed to find Skyrim's window!");
	else
		PrintLog("Found Skyrim's window handle 0x%08X!", hWnd);

	m_NextScreenShotIndex = FindLastInGameScreenshotIndex() + 1;
	PrintLog("Next In-Game Screenshot Index is %d", m_NextScreenShotIndex);

	SHCreateDirectoryEx(NULL, SKYRIM_SCREENSHOT_MAPPATH, NULL);

	m_XCellText.SetWindowTextA("0");
	m_YCellText.SetWindowTextA("0");
	m_OutputPathText.SetWindowTextA(SKYCAP_CKIMAGE_OUTPUTPATH);

	m_OutputCellSizeXText.SetWindowTextA("304");
	m_OutputCellSizeYText.SetWindowTextA("297");
	m_OutputOffsetXText.SetWindowTextA("29");
	m_OutputOffsetYText.SetWindowTextA("30");
	m_OutputCountXText.SetWindowTextA("5");
	m_OutputCountYText.SetWindowTextA("3");
	m_OutputCellRangeX1.SetWindowTextA("-57");
	m_OutputCellRangeX2.SetWindowTextA("62");
	m_OutputCellRangeY1.SetWindowTextA("-43");
	m_OutputCellRangeY2.SetWindowTextA("47");
		
	m_OutputPathText.SetWindowTextA("d:\\skyrimmaps\\test\\");
	
	m_OutputCellSizeXText.SetWindowTextA("607");
	m_OutputCellSizeYText.SetWindowTextA("582");
	m_OutputCountXText.SetWindowTextA("5");
	m_OutputCountYText.SetWindowTextA("1");
	m_OutputOffsetXText.SetWindowTextA("5");
	m_OutputOffsetYText.SetWindowTextA("30");


	m_OutputPathText.SetWindowTextA("d:\\skyrimmaps\\beyond\\");

	m_OutputCellSizeXText.SetWindowTextA("764");
	m_OutputCellSizeYText.SetWindowTextA("731");
	m_OutputCountXText.SetWindowTextA("5");
	m_OutputCountYText.SetWindowTextA("1");
	m_OutputOffsetXText.SetWindowTextA("15");
	m_OutputOffsetYText.SetWindowTextA("49");

	m_OutputCellRangeX1.SetWindowTextA("0");
	m_OutputCellRangeX2.SetWindowTextA("55");
	m_OutputCellRangeY1.SetWindowTextA("35");
	m_OutputCellRangeY2.SetWindowTextA("72");
	
	FindCKRenderWindow();
	FindCKCellViewWindow();

	return TRUE;
}


void CSkyCaptureDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}


void CSkyCaptureDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CSkyCaptureDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


bool CSkyCaptureDlg::RegisterHotKeys (void)
{
	BOOL Result;

	if (SKYCAP_ENABLEINGAMEHOTKEY) 
	{
		Result = RegisterHotKey(m_hWnd, SKYCAP_HOTKEY_ACTIVATE, MOD_CONTROL | MOD_ALT, 'Z');
		if (!Result) return PrintWindowsError("Failed to register the hotkey!");
	}
	else 
	{	
		PrintLog("Skipping the in-game hot key registration...");
	}

	PrintLog("Registered the global Windows hot keys!");
	return true;
}


bool CSkyCaptureDlg::UnregisterHotKeys (void)
{
	if (SKYCAP_ENABLEINGAMEHOTKEY) UnregisterHotKey(m_hWnd, SKYCAP_HOTKEY_ACTIVATE);
	return true;
}


bool CSkyCaptureDlg::PrintWindowsError (const char* pString, ...)
{
	DWORD ErrorCode = GetLastError();
	va_list Args;
	CString Buffer;
	char    ErrorString[256];

	va_start(Args, pString);
	Buffer.FormatV(pString, Args);
	va_end(Args);

	PrintLog(Buffer);

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, ErrorCode, LANG_USER_DEFAULT, ErrorString, 250, NULL);
	PrintLog(ErrorString);

	return false;
}


void CSkyCaptureDlg::PrintLog (const char* pString, ...)
{
	va_list Args;
	CString Buffer;

	va_start(Args, pString);
	Buffer.FormatV(pString, Args);
	va_end(Args);

	Buffer += "\r\n";
	m_LogText.SetSel(m_LogText.GetWindowTextLengthA(), m_LogText.GetWindowTextLengthA());
	m_LogText.ReplaceSel(Buffer, FALSE);
}


void CSkyCaptureDlg::OnActivateHotKey (void)
{

	if (m_IsRunning)
	{
		PrintLog("Stopping screenshot activation!");
		m_IsRunning = false;
		ResetEvent(m_RunEvent);
	}
	else
	{
		PrintLog("Received hot key activation!");
		Sleep(SKYCAP_STARTDELAY);
		m_IsRunning = true;
		SetEvent(m_RunEvent);
	}
	
}



LRESULT CSkyCaptureDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{

	switch (wParam)
	{
	case SKYCAP_HOTKEY_ACTIVATE:
		OnActivateHotKey();
		break;
	}

	return 0;
}


void CSkyCaptureDlg::OnDestroy()
{
	UnregisterHotKeys();
	CDialogEx::OnDestroy();
}


HWND CSkyCaptureDlg::FindSkyrimWindow (void)
{
	CWnd* pWnd = FindWindow(NULL, "Skyrim");
	if (pWnd == NULL) return NULL;

	return pWnd->m_hWnd;
}


void CSkyCaptureDlg::SimulateKeyPress (const char* pString)
{
	if (pString == NULL) return;

	while (*pString)
	{
		SimulateKeyPress1(*pString);
		++pString;
		Sleep(SKYCAP_KEYDELAY);
	}
}


void CSkyCaptureDlg::SimulateKeyPress (const BYTE InputCode)
{	 
	BYTE KeyCode = toupper(InputCode);
	INPUT Inputs[2];

	DWORD BaseFlag = 0;	
	if (KeyCode >= 0x70) BaseFlag = KEYEVENTF_EXTENDEDKEY;

	Inputs[0].type = INPUT_KEYBOARD;
	Inputs[0].ki.wVk = toupper(KeyCode);
	Inputs[0].ki.dwFlags = BaseFlag;
	Inputs[0].ki.wScan = MapVirtualKeyEx(KeyCode, MAPVK_VK_TO_VSC, NULL);
	//Inputs[0].ki.wScan = 0;
	Inputs[0].ki.time = 0;
	Inputs[0].ki.dwExtraInfo = 0;

	Inputs[1].type = INPUT_KEYBOARD;
	Inputs[1].ki.wVk = KeyCode;
	Inputs[1].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
	Inputs[1].ki.wScan = MapVirtualKeyEx(KeyCode, MAPVK_VK_TO_VSC, NULL);
	//Inputs[1].ki.wScan = 0;
	Inputs[1].ki.time = 0;
	Inputs[1].ki.dwExtraInfo = 0;

	UINT Result = ::SendInput(2, &Inputs[0], sizeof(INPUT));

	if (Result != 2)
		PrintWindowsError("SendInput() failed!");
	else
		PrintLog("SendInput() returned %d", Result);

	//keybd_event( KeyCode, 0x14, BaseFlag | 0, 0 );
	//Sleep(1);
    //keybd_event( KeyCode, 0x14, BaseFlag | KEYEVENTF_KEYUP, 0);
}


void CSkyCaptureDlg::SimulateKeyPress1 (const BYTE InputCode)
{	 
	BYTE KeyCode = toupper(InputCode);
	INPUT Inputs[4];
	int i = 0;

	DWORD BaseFlag = 0;	
	//if (KeyCode >= 0x70) BaseFlag = KEYEVENTF_EXTENDEDKEY;

	if (s_KeyInfos[InputCode].Char == 0)
	{
		PrintLog("SendInput(): Don't know how to output '%c' (0x%02X), ignoring!", InputCode, (int)InputCode);
		return;
	}

	if (s_KeyInfos[InputCode].Shift)
	{
		Inputs[i].type = INPUT_KEYBOARD;
		Inputs[i].ki.wVk = VK_LSHIFT;
		Inputs[i].ki.dwFlags = BaseFlag;
		Inputs[i].ki.wScan = 0x2A;
		Inputs[i].ki.time = 0;
		Inputs[i].ki.dwExtraInfo = 0;
		++i;
	}

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = s_KeyInfos[InputCode].VirtualKey;
	Inputs[i].ki.dwFlags = BaseFlag;
	Inputs[i].ki.wScan = s_KeyInfos[InputCode].ScanCode;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = s_KeyInfos[InputCode].VirtualKey;
	Inputs[i].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = s_KeyInfos[InputCode].ScanCode;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	if (s_KeyInfos[InputCode].Shift)
	{
		Inputs[i].type = INPUT_KEYBOARD;
		Inputs[i].ki.wVk = VK_LSHIFT;
		Inputs[i].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
		Inputs[i].ki.wScan = 0x2A;
		Inputs[i].ki.time = 0;
		Inputs[i].ki.dwExtraInfo = 0;
		++i;
	}

	UINT Result = ::SendInput(i, &Inputs[0], sizeof(INPUT));
	if (Result != i) PrintWindowsError("SendInput() failed!");	
}


void CSkyCaptureDlg::SimulateKeyPrintScreen (void)
{
	INPUT Inputs[4];
	int i = 0;
	DWORD BaseFlag = 0;	

	BaseFlag = KEYEVENTF_EXTENDEDKEY;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SNAPSHOT;
	Inputs[i].ki.dwFlags = BaseFlag;
	Inputs[i].ki.wScan = 0x37;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SNAPSHOT;
	Inputs[i].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = 0x37;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	UINT Result = ::SendInput(i, &Inputs[0], sizeof(INPUT));
	if (Result != i) PrintWindowsError("SendInput() failed!");	
	Sleep(SKYCAP_KEYDELAY);
}


void CSkyCaptureDlg::SimulateKeyWindowsAltPrintScreen (void)
{
	INPUT Inputs[6];
	int i = 0;
	DWORD BaseFlag = 0;	

	BaseFlag = KEYEVENTF_EXTENDEDKEY;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_MENU;
	Inputs[i].ki.dwFlags = BaseFlag;
	Inputs[i].ki.wScan = 0x38;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SNAPSHOT;
	Inputs[i].ki.dwFlags = BaseFlag;
	Inputs[i].ki.wScan = 0x37;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SNAPSHOT;
	Inputs[i].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = 0x37;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_MENU;
	Inputs[i].ki.dwFlags = BaseFlag | KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = 0x38;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	++i;

	UINT Result = ::SendInput(i, &Inputs[0], sizeof(INPUT));
	if (Result != i) PrintWindowsError("SendInput() failed!");	
	Sleep(SKYCAP_KEYDELAY);
}


void CSkyCaptureDlg::BackupInGameScreenshots (void)
{
	CopyInGameScreenshots(SKYRIM_SCREENSHOT_BACKUPPATH);
}


void CSkyCaptureDlg::DeleteInGameScreenshots (void)
{	
	CStringArray Files;
	CString Buffer(SKYRIM_SCREENSHOT_PATH);
	CString Buffer1;
	CString Buffer2;
	WIN32_FIND_DATA FindData;
	HANDLE  hFind;
	BOOL    Result;

	Buffer += "ScreenShot*.bmp";
	hFind = FindFirstFileA(Buffer, &FindData);
	Result = hFind != INVALID_HANDLE_VALUE;

	while (Result)
	{
		Files.Add(FindData.cFileName);
		Result = FindNextFileA(hFind, &FindData);
	}

	FindClose(hFind);

	for (int i = 0; i < Files.GetCount(); ++i)
	{
		DeleteFileA(Files.GetAt(i));
	}

}


void CSkyCaptureDlg::CopyInGameScreenshots (const char* pDestPath)
{
	CString Buffer(SKYRIM_SCREENSHOT_PATH);
	CString Buffer1;
	CString Buffer2;
	WIN32_FIND_DATA FindData;
	HANDLE  hFind;
	BOOL    Result;

	SHCreateDirectoryEx(NULL, pDestPath, NULL);

	Buffer += "ScreenShot*.bmp";
	hFind = FindFirstFileA(Buffer, &FindData);
	Result = hFind != INVALID_HANDLE_VALUE;

	while (Result)
	{
		Buffer1 = SKYRIM_SCREENSHOT_PATH;
		Buffer1 += FindData.cFileName;
		Buffer2 = pDestPath;
		Buffer2 += FindData.cFileName;

		CopyFileA(Buffer1, Buffer2, FALSE);
		Result = FindNextFileA(hFind, &FindData);
	}

	FindClose(hFind);
}


void CSkyCaptureDlg::ResetInGameScreenshots (void)
{
	m_ShotInfos.clear();
	DeleteInGameScreenshots();
	m_NextScreenShotIndex = 1;
}


void CSkyCaptureDlg::EnterConsoleMode (void)
{
	if (m_IsInConsole) return;

	SimulateKeyPress("~");
	Sleep(SKYCAP_CMDDELAY);

	m_IsInConsole = true;
}


void CSkyCaptureDlg::LeaveConsoleMode (void)
{
	if (!m_IsInConsole) return;

	SimulateKeyPress("~");
	Sleep(SKYCAP_CMDDELAY);

	m_IsInConsole = false;
}


void CSkyCaptureDlg::SendConsoleCmd (const char* pString, ...)
{
	CString Buffer;
	va_list Args;

	va_start(Args, pString);
	Buffer.FormatV(pString, Args);
	va_end(Args);

	PrintLog("Sending Console Command '%s'", Buffer); 
	
	EnterConsoleMode();	
	SimulateKeyPress(Buffer);
	Sleep(SKYCAP_KEYDELAY);
	SimulateKeyPress("\n");
}


void CSkyCaptureDlg::SetupCameraPos (void)
{
	SendConsoleCmd("tgm 1");
	SendConsoleCmd("tcl");
	SendConsoleCmd("set timescale to 0");
	SendConsoleCmd("set gamehour to 12");
	SendConsoleCmd("setfog 10000000 100000000");
	SendConsoleCmd("tb");
	//SendConsoleCmd("tfc 1");
	//SendConsoleCmd("tm");

	//SendConsoleCmd("cow tamriel 0 0");
	//m_IsInConsole = false;
	//Sleep(4000);
	
	LeaveConsoleMode();
}


void CSkyCaptureDlg::TakeInGameScreenshots (void)
{
	SetupCameraPos();
		
	for (int y = m_StartCellY; y <= m_EndCellY; ++y)
	{
		for (int x = m_StartCellX; x <= m_EndCellX; ++x)
		{
			if (!m_IsRunning) return;
			TakeInGameScreenshot(x, y);			
		}
	}

}


void CSkyCaptureDlg::TakeInGameScreenshot (const int CellX, const int CellY)
{
	PrintLog("Setting up for cell (%d, %d)...", CellX, CellY);

	SendConsoleCmd("cow tamriel %d %d", CellX, CellY);
	//m_IsInConsole = false;
	LeaveConsoleMode();
	Sleep(SKYCAP_LOADCELLDELAY);

	SendConsoleCmd("player.setpos x %d", CellX*SKYRIM_CELLSIZE + SKYRIM_CELLSIZE/2);
	SendConsoleCmd("player.setpos y %d", CellY*SKYRIM_CELLSIZE + SKYRIM_CELLSIZE/2);
	SendConsoleCmd("player.setpos z 10000");
		
	SendConsoleCmd("player.setangle y 0");
	SendConsoleCmd("player.setangle z 0");
	SendConsoleCmd("player.setangle x 90");

	LeaveConsoleMode();
	Sleep(SKYCAP_CMDPOSDELAY);

	PrintLog("Taking screenshot #%d for cell (%d, %d)...", m_NextScreenShotIndex, CellX, CellY);
	m_ShotInfos.push_back(scshotinfo_t(m_NextScreenShotIndex, CellX, CellY));
	++m_NextScreenShotIndex;
	SimulateKeyPrintScreen();
	Sleep(SKYCAP_SCREENSHOTDELAY);

	CopyMapInGameScreenshot(m_ShotInfos.back());
}


void CSkyCaptureDlg::CopyMapInGameScreenshot (const scshotinfo_t ShotInfo)
{
	CString SourceFile(SKYRIM_SCREENSHOT_PATH);
	CString DestFile(SKYRIM_SCREENSHOT_MAPPATH);
	CString Buffer;

	Buffer.Format("ScreenShot%d.bmp", ShotInfo.ShotIndex);
	SourceFile += Buffer;

	Buffer.Format("skyrim_%d_%d.bmp", ShotInfo.CellX, ShotInfo.CellY);
	DestFile += Buffer;

	BOOL Result = CopyFileA(SourceFile, DestFile, FALSE);

	if (!Result) this->PrintWindowsError("Failed to copy screenshot #%d for cell (%d, %d)!", ShotInfo.ShotIndex, ShotInfo.CellX, ShotInfo.CellY);
}


int CSkyCaptureDlg::FindLastInGameScreenshotIndex (void)
{
	CString Filename(SKYRIM_SCREENSHOT_PATH);
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	BOOL FindResult;
	int MaxIndex = 0;
		
	Filename += "ScreenShot*.bmp";

	hFind = FindFirstFileA(Filename, &FindData);
	if (hFind == INVALID_HANDLE_VALUE ) return 0;

	do
	{
		int Index = atoi(FindData.cFileName + 10);
		if (Index > MaxIndex) MaxIndex = Index;

		FindResult = FindNextFileA(hFind, &FindData);
	} while (FindResult);

	FindClose(hFind);

	return MaxIndex;
}


HWND CSkyCaptureDlg::FindCKRenderWindow (void)
{
	m_hRenderView = ::FindWindow("MonitorClass", NULL);

	if (m_hRenderView == NULL) 
	{
		PrintLog("Failed to find the CK render view window!");
	}
	else
	{
		PrintLog("Found the CK render view window (0x%08X).", m_hRenderView);
	}

	return m_hRenderView;
}


BOOL CALLBACK l_CellViewEnumProc(HWND hWnd, LPARAM lParam)
{
	CSkyCaptureDlg* pDlg = (CSkyCaptureDlg *) lParam;

	if (pDlg->m_CellChildCount == 7)
	{
		pDlg->m_hXCellWnd = hWnd;
	}
	else if (pDlg->m_CellChildCount == 8)
	{
		pDlg->m_hYCellWnd = hWnd;
	}
	else if (pDlg->m_CellChildCount == 9)
	{
		pDlg->m_hGoCellWnd = hWnd;
	}
	else if (pDlg->m_CellChildCount == 5) 
	{
		pDlg->m_hListCellWnd = hWnd;
	}

	//pDlg->PrintLog("CK child window #%d (0x%08X).", pDlg->m_CellChildCount+1, hWnd);

	++pDlg->m_CellChildCount;
	return TRUE;
}


HWND CSkyCaptureDlg::FindCKCellViewWindow (void)
{
	m_hCellView = ::FindWindow(NULL, "Cell View");

	if (m_hCellView == NULL)
	{
		m_hXCellWnd  = NULL;
		m_hYCellWnd  = NULL;
		m_hGoCellWnd = NULL;
		PrintLog("Failed to find the CK cell view window!");
		return NULL;
	}
		
	m_CellChildCount = 0;
	::EnumChildWindows(m_hCellView, l_CellViewEnumProc, (LPARAM) this);
		
	PrintLog("Found the CK cell view window (0x%08X).", m_hCellView);
	PrintLog("Found the CK cell X window (0x%08X).", m_hXCellWnd);
	PrintLog("Found the CK cell Y window (0x%08X).", m_hYCellWnd);
	PrintLog("Found the CK cell Go button (0x%08X).", m_hGoCellWnd);
	PrintLog("Found the CK cell list (0x%08X).", m_hListCellWnd);
    
	return m_hCellView;
}



void CSkyCaptureDlg::OnBnClickedFindRenderview()
{
	FindCKRenderWindow();
	FindCKCellViewWindow();
}


int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
   UINT  num = 0;          // number of image encoders
   UINT  size = 0;         // size of the image encoder array in bytes

   ImageCodecInfo* pImageCodecInfo = NULL;

   GetImageEncodersSize(&num, &size);
   if(size == 0)
      return -1;  // Failure

   pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
   if(pImageCodecInfo == NULL)
      return -1;  // Failure

   GetImageEncoders(num, size, pImageCodecInfo);

   for(UINT j = 0; j < num; ++j)
   {
      if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
      {
         *pClsid = pImageCodecInfo[j].Clsid;
         free(pImageCodecInfo);
         return j;  // Success
      }    
   }

   free(pImageCodecInfo);
   return -1;  // Failure
}


void CSkyCaptureDlg::ResetCKRenderWindow (void)
{
	WPARAM wParam;
	LPARAM lParam;

	if (m_hRenderView == NULL) return;

	SimulateWindowsKeyPress('0', m_hRenderView);

	wParam = MAKEWPARAM(0, -12000);
	lParam = 0;
	::SendMessage(m_hRenderView, WM_MOUSEWHEEL, wParam, lParam);
	Sleep(SKYCAP_KEYDELAY);

	SimulateWindowsKeyPress('0', m_hRenderView);
	SimulateWindowsKeyPress('M', m_hRenderView);
	SimulateWindowsKeyPress('M', m_hRenderView);

	::SetForegroundWindow(m_hRenderView);
	::SetActiveWindow(m_hRenderView);
	::SetFocus(m_hRenderView);

	Sleep(SKYCAP_CMDDELAY);

	SetForegroundWindow();
	SetActiveWindow();
	SetFocus();

	Sleep(SKYCAP_CMDDELAY);
}


void CSkyCaptureDlg::OnBnClickedTestSendmouseevent()
{
	WPARAM wParam;
	LPARAM lParam;

	if (m_hRenderView == NULL) 
	{
		FindCKRenderWindow();
		if (m_hRenderView == NULL) return;
	}

	wParam = 0x30;
	lParam = 0;
	::SendMessage(m_hRenderView, WM_KEYDOWN, wParam, lParam);

	Sleep(100);

	wParam = MAKEWPARAM(0, -12000);
	lParam = 0;
	::SendMessage(m_hRenderView, WM_MOUSEWHEEL, wParam, lParam);

	Sleep(100);

	wParam = 0x30; //0
	lParam = 0;
	::SendMessage(m_hRenderView, WM_KEYDOWN, wParam, lParam);

	Sleep(100);

	wParam = 0x4D; //M
	lParam = 0;
	::SendMessage(m_hRenderView, WM_KEYDOWN, wParam, lParam);

	Sleep(100);

	wParam = 0x4D; //M
	lParam = 0;
	::SendMessage(m_hRenderView, WM_KEYDOWN, wParam, lParam);

	/*
	CRect RenderRect;
	::GetWindowRect(m_hRenderView, &RenderRect);
	PrintLog("Render window size = %d x %d", RenderRect.Width(), RenderRect.Height());

	HDC hDCWnd = ::GetWindowDC(m_hRenderView);
    HDC hDCMem = ::CreateCompatibleDC(hDCWnd);
	HBITMAP hBmp = CreateCompatibleBitmap(hDCWnd, RenderRect.Width(), RenderRect.Height());

    ::ReleaseDC(m_hRenderView, hDCWnd);
    HBITMAP hBmpOld = (HBITMAP) ::SelectObject(hDCMem, hBmp);
    ::SetBkMode(hDCMem, TRANSPARENT);
    ::RedrawWindow(m_hRenderView, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    Sleep (1000);

    BOOL lRet = ::PrintWindow(m_hRenderView, hDCMem, 0);

    if (lRet == 0) 
	{
		PrintLog("Failed to capture the render view window!");
		return;
	}
	
	hBmp = (HBITMAP) ::SelectObject(hDCMem, hBmpOld);
	
	PBITMAPINFO pBmpInfo = CreateBitmapInfoStruct(NULL, hBmp);

	if (pBmpInfo != NULL) 
	{
		CreateBMPFile(NULL, "d:\\test.bmp", pBmpInfo, hBmp, hDCMem);
		LocalFree(pBmpInfo);
	}
	
	Bitmap TestBmp(hBmp, NULL);
	CLSID pngClsid;
	GetEncoderClsid(L"image/png", &pngClsid);
	Status Result = TestBmp.Save(L"d:\\test.png", &pngClsid, NULL);

	//BITMAPINFO BmpInfo;
	//::GetDIBits(hDCMem, hBmp, 0, 0, &Bytes, &BmpInfo, 0);
	       
    ::DeleteObject(hBmpOld);
    ::DeleteObject(hBmp);
    ::DeleteDC(hDCMem); //*/

	::SetForegroundWindow(m_hRenderView);
	::SetActiveWindow(m_hRenderView);
	::SetFocus(m_hRenderView);

	Sleep(200);

	SimulateKeyWindowsAltPrintScreen();

	if (!IsClipboardFormatAvailable(CF_DIB)) return;

    if ( OpenClipboard() )
    {
        HBITMAP hbitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
        Bitmap bitmap(hbitmap, NULL);
 
        CloseClipboard();
		CLSID pngClsid;
		GetEncoderClsid(L"image/png", &pngClsid);
		Status Result = bitmap.Save(L"d:\\test1.png", &pngClsid, NULL);
    }

	Sleep(200);

	SetForegroundWindow();
	SetActiveWindow();
	SetFocus();

}


bool CSkyCaptureDlg::CKMoveToCell (const int CellX, const int CellY)
{
	CString Buffer;

	if (m_hXCellWnd == NULL || m_hYCellWnd == NULL || m_hGoCellWnd == NULL) 
	{
		PrintLog("Cannot move to cell: Invalid cell view child window handles!");
		return false;
	}

	Sleep(100);

	//::SetForegroundWindow(m_hRenderView);
	//::SetActiveWindow(m_hRenderView);
	//::SetFocus(m_hRenderView);

	Sleep(100);

	Buffer.Format("%d", CellX);
	::SendMessageA(m_hXCellWnd, WM_SETTEXT, 0, (LPARAM) (const char *)Buffer);

	Buffer.Format("%d\r\n", CellY);
	::SendMessageA(m_hYCellWnd, WM_SETTEXT, 0, (LPARAM) (const char *)Buffer);

	Sleep(50);
		
	//::SetCursorPos(1000, 500);	
	//Sleep(100);

	::SetForegroundWindow(m_hCellView);
	::SetActiveWindow(m_hCellView);
	::SetFocus(m_hCellView);
	Sleep(400);
	
	INPUT Inputs[2];
	Inputs[0].type = INPUT_MOUSE;
	Inputs[0].mi.dx = -2000;
	Inputs[0].mi.dy = 0;
	Inputs[0].mi.time = 0;
	Inputs[0].mi.dwExtraInfo = 0;
	Inputs[0].mi.dwFlags = MOUSEEVENTF_MOVE;
	Inputs[0].mi.mouseData = 0;
	SendInput(1, &Inputs[0], sizeof(INPUT));
	Sleep(100);	

	WPARAM wParam = BN_CLICKED;
	wParam = MAKEWPARAM(0xe61, BN_CLICKED );
	LPARAM lParam = (LPARAM) m_hGoCellWnd;
	::SendMessageA(m_hCellView, WM_COMMAND, wParam, lParam);
	Sleep(1000);

	//LPARAM lParam = MAKELPARAM(5, 5);
	//::SendMessageA(m_hGoCellWnd, WM_LBUTTONDOWN, 0, lParam);
	//Sleep(400);
	//::SendMessageA(m_hGoCellWnd, WM_LBUTTONUP, 0, lParam);
	//Sleep(1000);
	
	Inputs[0].mi.dx = 2000;
	SendInput(1, &Inputs[0], sizeof(INPUT));
	Sleep(100); //*/
	
	//lParam = MAKELPARAM(500, 501);
	//::SendMessageA(m_hRenderView, WM_MOUSEMOVE, 0, lParam);

	//::SetForegroundWindow(m_hRenderView);
	//::SetActiveWindow(m_hRenderView);
	//::SetFocus(m_hRenderView);
	//::RedrawWindow(m_hRenderView, NULL, NULL, 0);

	ResetCKRenderWindow();

	//SetForegroundWindow();
	//SetActiveWindow();
	//SetFocus();

	return true;
}


void CSkyCaptureDlg::OnBnClickedCellgobutton()
{
	CString Buffer;
	int CellX;
	int CellY;

	m_XCellText.GetWindowText(Buffer);
	CellX = atoi(Buffer);

	m_YCellText.GetWindowText(Buffer);
	CellY = atoi(Buffer);
	
	PrintLog("Attempting to move to cell (%d, %d)...", CellX, CellY);

	CKMoveToCell(CellX, CellY);
}


void CSkyCaptureDlg::OnBnClickedResetRenderview()
{
	ResetCKRenderWindow();
}



CSkyCKCaptureInfo CSkyCaptureDlg::GetCurrentCKData (void)
{
	CSkyCKCaptureInfo	CaptureInfo;
	CString				Buffer;

	m_XCellText.GetWindowTextA(Buffer);
	CaptureInfo.CellX = atoi(Buffer);

	m_YCellText.GetWindowTextA(Buffer);
	CaptureInfo.CellY = atoi(Buffer);

	m_OutputCountXText.GetWindowTextA(Buffer);
	CaptureInfo.CellCountX = atoi(Buffer);

	m_OutputCountYText.GetWindowTextA(Buffer);
	CaptureInfo.CellCountY = atoi(Buffer);

	m_OutputOffsetXText.GetWindowTextA(Buffer);
	CaptureInfo.CellOffsetX = atoi(Buffer);

	m_OutputOffsetYText.GetWindowTextA(Buffer);
	CaptureInfo.CellOffsetY = atoi(Buffer);

	m_OutputCellSizeXText.GetWindowTextA(Buffer);
	CaptureInfo.CellSizeX = atoi(Buffer);

	m_OutputCellSizeYText.GetWindowTextA(Buffer);
	CaptureInfo.CellSizeY = atoi(Buffer);

	m_OutputCellRangeX1.GetWindowTextA(Buffer);
	CaptureInfo.CellRangeX1 = atoi(Buffer);

	m_OutputCellRangeX2.GetWindowTextA(Buffer);
	CaptureInfo.CellRangeX2 = atoi(Buffer);

	m_OutputCellRangeY1.GetWindowTextA(Buffer);
	CaptureInfo.CellRangeY1 = atoi(Buffer);

	m_OutputCellRangeY2.GetWindowTextA(Buffer);
	CaptureInfo.CellRangeY2 = atoi(Buffer);

	CaptureInfo.CellOutputOffsetX = 57;
	CaptureInfo.CellOutputOffsetY = 51;

	m_OutputPathText.GetWindowTextA(Buffer);
	CaptureInfo.OutputPath = Buffer;

	return CaptureInfo;
}


void CSkyCaptureDlg::SplitCKImage(Bitmap& RawBitmap, CSkyCKCaptureInfo& CaptureInfo, const char* pSubPath)
{
	CString  OutputFile;
	CStringW OutputFileW;
	int      PixelX;
	int      PixelY = CaptureInfo.CellOffsetY;
	int      CellX;
	int		 DiffX = 0;
	int		 DiffY = 0;
	int      CellY = CaptureInfo.CellY + CaptureInfo.CellCountY/2;
	CLSID	 pngClsid;
	CLSID	 jpgClsid;
	CLSID	 SaveClsid;
	Status	 Result;

	GetEncoderClsid(L"image/png",  &pngClsid);
	GetEncoderClsid(L"image/jpeg", &jpgClsid);

	if (m_SaveasPNG)
		GetEncoderClsid(L"image/png",  &SaveClsid);
	else
		GetEncoderClsid(L"image/jpeg", &SaveClsid);

	for (int Y = 0; Y < CaptureInfo.CellCountY; ++Y)
	{
		PixelX  = CaptureInfo.CellOffsetX;
		CellX = CaptureInfo.CellX - CaptureInfo.CellCountX/2;		
		DiffY = 0;

#if SKYCAP_CELLSIZE_DIFFTYPE == 1
		if (CellY == CaptureInfo.CellY) DiffY = -1;
#elif SKYCAP_CELLSIZE_DIFFTYPE == 2
		DiffY = 0;
#endif
		
		for (int X = 0; X < CaptureInfo.CellCountX; ++X)
		{
			DiffX = 0;

#if SKYCAP_CELLSIZE_DIFFTYPE == 1
			if (CellX == CaptureInfo.CellX) DiffX = +1;
#elif SKYCAP_CELLSIZE_DIFFTYPE == 2
			if (CellX == CaptureInfo.CellX) DiffX = -1;
#endif

			Bitmap* pCellBitmap = RawBitmap.Clone(PixelX, PixelY, CaptureInfo.CellSizeX + DiffX, CaptureInfo.CellSizeY + DiffY, RawBitmap.GetPixelFormat());
			
			if (pCellBitmap)
			{
				Gdiplus::Bitmap* pResizedBitmap = new Gdiplus::Bitmap(m_CaptureCellResize, m_CaptureCellResize, RawBitmap.GetPixelFormat());
			    Gdiplus::Graphics graphics(pResizedBitmap);
				graphics.DrawImage(pCellBitmap, 0, 0, m_CaptureCellResize, m_CaptureCellResize);

				if (m_CaptureCellCount > 1)
				{
					Bitmap* pCellBitmap1 = pResizedBitmap->Clone(0,    0,  256, 256, RawBitmap.GetPixelFormat());
					Bitmap* pCellBitmap2 = pResizedBitmap->Clone(256,  0,  256, 256, RawBitmap.GetPixelFormat());
					Bitmap* pCellBitmap3 = pResizedBitmap->Clone(0,  256,  256, 256, RawBitmap.GetPixelFormat());
					Bitmap* pCellBitmap4 = pResizedBitmap->Clone(256, 256, 256, 256, RawBitmap.GetPixelFormat());
					int BaseX = (CellX + CaptureInfo.CellOutputOffsetX)*2;
					int BaseY = (-CellY + CaptureInfo.CellOutputOffsetY - 1)*2;

					OutputFile.Format("%s\\%s\\%s-%d-%d-17.%s", CaptureInfo.OutputPath, pSubPath, "skyrim", BaseX, BaseY, m_SaveasPNG ? "png" : "jpg");
					OutputFileW = OutputFile;
					Result = pCellBitmap1->Save(OutputFileW, &SaveClsid);

					OutputFile.Format("%s\\%s\\%s-%d-%d-17.%s", CaptureInfo.OutputPath, pSubPath, "skyrim", BaseX+1, BaseY, m_SaveasPNG ? "png" : "jpg");
					OutputFileW = OutputFile;
					Result = pCellBitmap2->Save(OutputFileW, &SaveClsid);

					OutputFile.Format("%s\\%s\\%s-%d-%d-17.%s", CaptureInfo.OutputPath, pSubPath, "skyrim", BaseX, BaseY+1, m_SaveasPNG ? "png" : "jpg");
					OutputFileW = OutputFile;
					Result = pCellBitmap3->Save(OutputFileW, &SaveClsid);

					OutputFile.Format("%s\\%s\\%s-%d-%d-17.%s", CaptureInfo.OutputPath, pSubPath, "skyrim", BaseX+1, BaseY+1, m_SaveasPNG ? "png" : "jpg");
					OutputFileW = OutputFile;
					Result = pCellBitmap4->Save(OutputFileW, &SaveClsid);

					delete pCellBitmap1;
					delete pCellBitmap2;
					delete pCellBitmap3;
					delete pCellBitmap4;
				}
				else 
				{
					OutputFile.Format("%s\\%s\\%s-%d-%d-16.%s", CaptureInfo.OutputPath, pSubPath, "skyrim", CellX + CaptureInfo.CellOutputOffsetX, -CellY + CaptureInfo.CellOutputOffsetY - 1, m_SaveasPNG ? "png" : "jpg");
					OutputFileW = OutputFile;
					Result = pResizedBitmap->Save(OutputFileW, &SaveClsid);
				}

				delete pResizedBitmap;
				delete pCellBitmap;				
			}

			PixelX += CaptureInfo.CellSizeX + DiffX;
			++CellX;
		}

		PixelY += CaptureInfo.CellSizeY + DiffY;
		--CellY;
	}

}


void CSkyCaptureDlg::ProcessCKImage(Bitmap& RawBitmap, Bitmap& NoWaterBmp)
{
	CSkyCKCaptureInfo CaptureInfo = GetCurrentCKData();
	CString OutputFile;
	CStringW OutputFileW;
	CString Buffer;
	CLSID pngClsid;
	Status Result;

	GetEncoderClsid(L"image/png", &pngClsid);
	
	OutputFile.Format("%s\\%s\\%s_%d_%d.png", CaptureInfo.OutputPath, SKYCAP_CKIMAGE_RAWPATH, "full-tamriel", CaptureInfo.CellX, CaptureInfo.CellY);
	OutputFileW = OutputFile;
	Result = RawBitmap.Save(OutputFileW, &pngClsid, NULL);

	OutputFile.Format("%s\\%s\\%s_%d_%d.png", CaptureInfo.OutputPath, SKYCAP_CKIMAGE_RAWPATH, "nw-full-tamriel", CaptureInfo.CellX, CaptureInfo.CellY);
	OutputFileW = OutputFile;
	Result = NoWaterBmp.Save(OutputFileW, &pngClsid, NULL);

	SplitCKImage(RawBitmap,  CaptureInfo, SKYCAP_CKIMAGE_MAPPATH);
	SplitCKImage(NoWaterBmp, CaptureInfo, SKYCAP_CKIMAGE_NOWATERPATH);
}


void CSkyCaptureDlg::OnBnClickedCaptureRenderview()
{
	CString OutputFile;
	DWORD StartTickCount = ::GetTickCount();

	if (m_hRenderView == NULL) 
	{
		PrintLog("Error: Render view window handle is not valid!");
		return;
	}

	if (m_ToggleBorders) SimulateWindowsKeyPress('B', m_hRenderView);
	
	::SetForegroundWindow(m_hRenderView);
	::SetActiveWindow(m_hRenderView);
	::SetFocus(m_hRenderView);

	Sleep(SKYCAP_KEYDELAY);

	SimulateKeyWindowsAltPrintScreen();

	Sleep(1000);

	if (!IsClipboardFormatAvailable(CF_DIB))
	{
		PrintLog("Error: Clipboard image format not available!");
		return;
	}
		
    if ( OpenClipboard() )
    {
        HBITMAP hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
        Bitmap MapBitmap(hBitmap, NULL);
		CloseClipboard();

		SimulateShiftKeyPress('W');
		SimulateKeyWindowsAltPrintScreen();
		SimulateShiftKeyPress('W');

		if (OpenClipboard()) hBitmap = (HBITMAP)GetClipboardData(CF_BITMAP);
        Bitmap NoWaterBitmap(hBitmap, NULL);

		CloseClipboard();

		ProcessCKImage(MapBitmap, NoWaterBitmap);		
    }

	Sleep(SKYCAP_KEYDELAY);

	DWORD DeltaTickCount = ::GetTickCount() - StartTickCount;
	PrintLog("Done capturing CK images (%u ms)...", DeltaTickCount);

	SetForegroundWindow();
	SetActiveWindow();
	SetFocus();

	if (m_ToggleBorders) SimulateWindowsKeyPress('B', m_hRenderView);
}


void CSkyCaptureDlg::SimulateWindowsKeyPress (const BYTE KeyCode, HWND hWnd)
{
	WPARAM wParam;
	LPARAM lParam;

	wParam = KeyCode;
	lParam = 0;
	::SendMessage(m_hRenderView, WM_KEYDOWN, wParam, lParam);

	Sleep(SKYCAP_KEYDELAY);
}


void CSkyCaptureDlg::SimulateShiftKeyPress (const TCHAR KeyChar)
{
	INPUT Inputs[6];
	int i = 0;
	int nKeyScan = VkKeyScan(KeyChar); 
	
	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SHIFT;
	Inputs[i].ki.dwFlags = 0;
	Inputs[i].ki.wScan = MapVirtualKey(VK_SHIFT, 0); 
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	SendInput(1, &Inputs[i], sizeof(INPUT));
	++i;

	Sleep(SKYCAP_KEYDELAY);

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = toupper(KeyChar);
	Inputs[i].ki.dwFlags = 0;
	Inputs[i].ki.wScan = MapVirtualKey(LOBYTE(nKeyScan), 0); ;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	SendInput(1, &Inputs[i], sizeof(INPUT));
	++i;

	Sleep(200);

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = toupper(KeyChar);
	Inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = MapVirtualKey(LOBYTE(nKeyScan), 0); ;;
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	SendInput(1, &Inputs[i], sizeof(INPUT));
	++i;

	Sleep(SKYCAP_KEYDELAY);

	Inputs[i].type = INPUT_KEYBOARD;
	Inputs[i].ki.wVk = VK_SHIFT;
	Inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
	Inputs[i].ki.wScan = MapVirtualKey(VK_SHIFT, 0); 
	Inputs[i].ki.time = 0;
	Inputs[i].ki.dwExtraInfo = 0;
	SendInput(1, &Inputs[i], sizeof(INPUT));
	++i;

	Sleep(SKYCAP_KEYDELAY);
}


void CSkyCaptureDlg::OnBnClickedTestNowater()
{
	if (m_hRenderView == NULL) return;

	if (m_ToggleBorders) SimulateWindowsKeyPress('B', m_hRenderView);
	
	::SetForegroundWindow(m_hRenderView);
	::SetActiveWindow(m_hRenderView);
	::SetFocus(m_hRenderView);

	Sleep(500);

	SimulateShiftKeyPress('W');
	Sleep(1000);
	SimulateShiftKeyPress('W');

	if (m_ToggleBorders) SimulateWindowsKeyPress('B', m_hRenderView); 
}



void CSkyCaptureDlg::OnBnClickedCellstartbutton()
{
	CSkyCKCaptureInfo CaptureInfo = GetCurrentCKData();
	CString Buffer;
	int CellX = CaptureInfo.CellRangeX1 + CaptureInfo.CellCountX/2;
	int CellY = CaptureInfo.CellRangeY1 + CaptureInfo.CellCountY/2;

	Buffer.Format("%d", CellX);
	m_XCellText.SetWindowTextA(Buffer);

	Buffer.Format("%d", CellY);
	m_YCellText.SetWindowTextA(Buffer);

	PrintLog("Restarting at cell (%d, %d)...", CellX, CellY);
	CKMoveToCell(CellX, CellY);
}


void CSkyCaptureDlg::OnBnClickedCellnextbutton()
{
	CSkyCKCaptureInfo CaptureInfo = GetCurrentCKData();
	CString Buffer;
	int NewCellX = CaptureInfo.CellX;
	int NewCellY = CaptureInfo.CellY;

	NewCellY = CaptureInfo.CellY + CaptureInfo.CellCountY;

	if (NewCellY > CaptureInfo.CellRangeY2)
	{
		NewCellX = CaptureInfo.CellX + CaptureInfo.CellCountX;
		NewCellY = CaptureInfo.CellRangeY1 + CaptureInfo.CellCountY/2;

		if (NewCellX > CaptureInfo.CellRangeX2)
		{
			PrintLog("Finished with cell range...");
			return;
		}
	}

	Buffer.Format("%d", NewCellX);
	m_XCellText.SetWindowTextA(Buffer);

	Buffer.Format("%d", NewCellY);
	m_YCellText.SetWindowTextA(Buffer);

	PrintLog("Moving to cell (%d, %d)...", NewCellX, NewCellY);
	CKMoveToCell(NewCellX, NewCellY);
}


BOOL FileExists(LPCTSTR szPath)
{
  DWORD dwAttrib = GetFileAttributes(szPath);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}


void CSkyCaptureDlg::OnBnClickedTestCheckfiles()
{
	CSkyCKCaptureInfo CaptureInfo = GetCurrentCKData();
	CString OutputFile;
	int FileCount = 0;
	int MissingCount = 0;

	PrintLog("Checking for missing map tiles in range (%d, %d)-(%d, %d)...", CaptureInfo.CellRangeX1, CaptureInfo.CellRangeY1, CaptureInfo.CellRangeX2, CaptureInfo.CellRangeY2);
	m_MissingShots.clear();

	for (int X = CaptureInfo.CellRangeX1; X <= CaptureInfo.CellRangeX2; X += CaptureInfo.CellCountX)
	{
		for (int Y = CaptureInfo.CellRangeY1; Y <= CaptureInfo.CellRangeY2; Y += CaptureInfo.CellCountY)
		{
			++FileCount;
			bool isMissing = false;

			OutputFile.Format("%s\\%s\\%s_%d_%d.png", CaptureInfo.OutputPath, SKYCAP_CKIMAGE_RAWPATH, "full-tamriel", X, Y);
			if (!FileExists(OutputFile)) { isMissing = true; PrintLog("%d, %d: Missing regular map tile!", X, Y); ++MissingCount;
			}

			OutputFile.Format("%s\\%s\\%s_%d_%d.png", CaptureInfo.OutputPath, SKYCAP_CKIMAGE_RAWPATH, "nw-full-tamriel", X, Y);
			if (!FileExists(OutputFile)) { isMissing = true; PrintLog("%d, %d: Missing no-water map tile!", X, Y); ++MissingCount; }

			if (isMissing)
			{
				m_MissingShots.push_back({ X, Y });
			}

			continue;

			//OutputFile.Format("%s\\%s\\%s-%d-%d-16.jpg", CaptureInfo.OutputPath, "maps",    "skyrim", X + CaptureInfo.CellOutputOffsetX, Y + CaptureInfo.CellOutputOffsetY - 1);
			OutputFile.Format("%s\\%s\\%s-%d-%d-17.jpg", CaptureInfo.OutputPath, "maps", "bs", X + CaptureInfo.CellOutputOffsetX, Y + CaptureInfo.CellOutputOffsetY - 1);
			if (!FileExists(OutputFile)) { PrintLog("%d, %d: Missing regular map tile!", X, Y); ++MissingCount; }

			//OutputFile.Format("%s\\%s\\%s-%d-%d-16.jpg", CaptureInfo.OutputPath, "nowater", "skyrim", X + CaptureInfo.CellOutputOffsetX, -Y + CaptureInfo.CellOutputOffsetY - 1);
			OutputFile.Format("%s\\%s\\%s-%d-%d-17.jpg", CaptureInfo.OutputPath, "nowater", "bs", X + CaptureInfo.CellOutputOffsetX, -Y + CaptureInfo.CellOutputOffsetY - 1);
			if (!FileExists(OutputFile)) { PrintLog("%d, %d: Missing nowater map tile!", X, Y); ++MissingCount; }
		}
	}

	PrintLog("Found %d missing out of %d total tiles.", MissingCount, FileCount);
}


void CSkyCaptureDlg::OnBnClickedStartautocapture()
{

	if (m_IsAutoCKRunning)
	{
		m_ToggleAutoButton.SetWindowTextA("Start Auto");
		PrintLog("AUTO: Stopping auto CK screenshot capture...");
		m_IsAutoCKRunning = false;
		ResetEvent(m_RunCKEvent);
		SetEvent(m_WaitCKEvent);
	}
	else
	{
		m_ToggleAutoButton.SetWindowTextA("Stop Auto");
		PrintLog("AUTO: Starting auto CK screenshot capture...");
		m_AutoOnlyDoMissing = (m_OnlyMissingCheck.GetCheck() == BST_CHECKED);
		Sleep(SKYCAP_CMDPOSDELAY);
		m_IsAutoCKRunning = true;
		SetEvent(m_RunCKEvent);
	}

}


void CSkyCaptureDlg::OnBnClickedAutoskipwait()
{
	SetEvent(m_WaitCKEvent);
}


bool CSkyCaptureDlg::CheckIfCKCellLoaded (const int CellX, const int CellY)
{
	CString Buffer;
	char    OutputBuffer[256];

	if (m_hListCellWnd == NULL) return false;

	Buffer.Format("%d, %d", CellX, CellY);

	ListView_GetItemText(m_hListCellWnd, 20, 0, OutputBuffer, 200);
	PrintLog("GetText = '%s'", OutputBuffer);

	int Count = ListView_GetItemCount(m_hListCellWnd);
	PrintLog("Count = %d", Count);

	int SelCount = ListView_GetSelectedCount(m_hListCellWnd);
	PrintLog("SelCount = %d", SelCount);

	int SelIndex = ListView_GetNextItem(m_hListCellWnd, -1, LVNI_SELECTED);
	PrintLog("SelIndex = %d", SelIndex);

	if (SelIndex < 0) return false;

	char *pMemory = (char *)VirtualAlloc( NULL, 100000, MEM_RESERVE, PAGE_READWRITE );
	if (pMemory == NULL) return false;
	
	LVCOLUMN ColInfo;

	for (int i = 0; i < 20; ++i)
	{
		ListView_GetItemText(m_hListCellWnd, SelIndex, i, pMemory, 200);
		PrintLog("GetText[%d] = '%s'", i, pMemory);

		if (ListView_GetColumn(m_hListCellWnd, 0, &ColInfo))
		{
			PrintLog("Col[%d] = %d", i, ColInfo.iSubItem);
		}
		else
		{
			PrintLog("Col[%d] doesn't exist", i);
		}
	}

	VirtualFree(pMemory, 0, MEM_RELEASE);
	return false;
}

void CSkyCaptureDlg::OnBnClickedTestChecklist()
{
	CheckIfCKCellLoaded(0, 0);
}


void CSkyCaptureDlg::OnBnClickedTestLoadcheck()
{
	CWnd* pWnd = FindWindow(NULL, "Loading Cell");

	if (pWnd == NULL)
		PrintLog("Loading cell window not found!");
	else 
		PrintLog("Loading cell window found!");
}
