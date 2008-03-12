#include <windows.h>
#include <gl/gl.h>

#include <wx/dcclient.h>
#include <wx/string.h>

#include <shared/com/basecom.h>

#include "layout.h"
#include "gamewindow.h"

#include "debug.h"


#include "basecomserver.h"


// typedef bool (* OnRecieve_t)(class *lpClassPointer,unsigned long dwData,unsigned long cbData,void *lpData);


//
// CBCServerInternal defintion
//

class CBCServerInternal
{
public:
	CBCServerInternal();
	~CBCServerInternal();

	bool HlaeBcSrvStart(CHlaeBcServer *pBase);
	bool HlaeBcSrvStop();

	LRESULT DispatchToClientProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // outdated, not supported anymore
	LRESULT DispatchStruct(DWORD dwDataCode,DWORD cbDataSize,PVOID lpDataPtr);

private:
	CHlaeBcServer *_pBase; // coordinator class
	HWND _hwClient;
	static LRESULT CALLBACK _HlaeBcSrvWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HINSTANCE	_cl_hInstance;		// filled by RegisterClassA, used for relaying window messages to the H-L windowproc
	WNDPROC		_cl_lpfnWndProc;	// .

	BOOL _MyOnRecieve(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS);
	BOOL _ReturnMessage(HWND hWnd,HWND hwTarget,ULONG dwData,DWORD cbData,PVOID lpData);

	// wrappers:
	BOOL _Wrapper_OnCreateWindow(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS);
	BOOL _Wrapper_UpdateWindow(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS);
};


//
// winapi related globals
//

HWND g_hwHlaeBcSrvWindow = NULL;
BOOL (CBCServerInternal::*g_OnRecieve)(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS) = NULL;
CBCServerInternal *g_pClass = NULL;
	
// USE INTERLOCED ACCES ONLY:
LONG g_lInstancesActive = 0; // this variable is accessed interlocked by HlaeBcSrvStart

//
// Internal class for the internal global
//
// Purpose is to wrap between WinAPI into an abstract view that is suitable for using in a wxWidgets targeted class
// It also does some basic low level handling and processing of the messages and events.
//

CBCServerInternal::CBCServerInternal()
{
	_cl_hInstance = NULL;
	_cl_lpfnWndProc = NULL;
}

CBCServerInternal::~CBCServerInternal()
{
	if (!HlaeBcSrvStop()) throw "Error: Could not stop BaseCom.";
}

bool CBCServerInternal::HlaeBcSrvStart(CHlaeBcServer *pBase)
{
	if (InterlockedIncrement(&g_lInstancesActive)>1)
	{
		 // if already running quit
		InterlockedDecrement(&g_lInstancesActive);
		return false;
	}

	HINSTANCE hInstance = (HINSTANCE)GetCurrentProcessId();

	static bool bRegistered=false;
	static WNDCLASS wc;

	if (!bRegistered)
	{
 
		// Register the main window class. 
		wc.style = NULL; 
		wc.lpfnWndProc = (WNDPROC) _HlaeBcSrvWndProc; 
		wc.cbClsExtra = 0; 
		wc.cbWndExtra = 0; 
		wc.hInstance = hInstance; 
		wc.hIcon = NULL; 
		wc.hCursor = NULL; 
		wc.hbrBackground = NULL; 
		wc.lpszMenuName =  NULL; 
		wc.lpszClassName = HLAE_BASECOM_CLASSNAME;

		if (!RegisterClass(&wc))
		{
			InterlockedDecrement(&g_lInstancesActive);
			return false;
		}

		bRegistered = true;
	 }

	g_OnRecieve = &CBCServerInternal::_MyOnRecieve;
	g_pClass = this;

	_pBase = pBase; // connect to coordinator class

	if (!(g_hwHlaeBcSrvWindow = CreateWindow(wc.lpszClassName,HLAE_BASECOM_SERVER_ID,WS_DISABLED|WS_POPUP,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,hInstance,NULL)))
	{
		g_OnRecieve = NULL;
		g_pClass = NULL;
		_pBase = NULL;
		InterlockedDecrement(&g_lInstancesActive);
		return false;
	}

	return true;
}

bool CBCServerInternal::HlaeBcSrvStop()
{
	if (g_lInstancesActive==0) // according to MSDN 32 bit reads (or writes) are guaranteed to be atomic
		return true;

	if (!DestroyWindow(g_hwHlaeBcSrvWindow)) return false;

	g_hwHlaeBcSrvWindow = NULL;
	_pBase = NULL;
	InterlockedDecrement(&g_lInstancesActive);
	return true;
}

LRESULT CBCServerInternal::DispatchToClientProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return FALSE;
	/*if (!_hwClient) return FALSE;

	static HLAE_BASECOM_CallWndProc_s mycws;

	mycws.hwnd = hwnd;
	mycws.uMsg = uMsg;
	mycws.wParam = wParam;
	mycws.lParam = lParam;

	return DispatchStruct(HLAE_BASECOM_MSGCL_CallWndProc_s,sizeof(HLAE_BASECOM_CallWndProc_s),&mycws);*/
}

LRESULT CBCServerInternal::DispatchStruct(DWORD dwDataCode,DWORD cbDataSize,PVOID lpDataPtr)
{
	if (!_hwClient) return FALSE;

	COPYDATASTRUCT myCopyData;

	myCopyData.dwData=dwDataCode;
	myCopyData.cbData=cbDataSize;
	myCopyData.lpData=lpDataPtr;

	return SendMessageW(
		_hwClient,
		WM_COPYDATA,
		(WPARAM)g_hwHlaeBcSrvWindow, // identify us as sender
		(LPARAM)&myCopyData
	);
}

LRESULT CALLBACK CBCServerInternal::_HlaeBcSrvWndProc(
    HWND hwnd,        // handle to window
    UINT uMsg,        // message identifier
    WPARAM wParam,    // first message parameter
    LPARAM lParam)    // second message parameter
{ 
 
    switch (uMsg) 
    { 
        case WM_CREATE: 
            // Initialize the window.
            return FALSE; 
 
        case WM_PAINT: 
            // Paint the window's client area. 
            return FALSE; 
 
        case WM_SIZE: 
            // Set the size and position of the window. 
            return FALSE; 
 
        case WM_DESTROY: 
            // Clean up window-specific data objects. 
            return FALSE; 

		case WM_COPYDATA:
			if (!(g_OnRecieve && g_pClass)) return FALSE;

			PCOPYDATASTRUCT pMyCDS;
			pMyCDS = (PCOPYDATASTRUCT) lParam;
			
			return (g_pClass->*g_OnRecieve)(hwnd,(HWND)wParam,pMyCDS);
 
        default: 
            return DefWindowProc(hwnd, uMsg, wParam, lParam); 
    } 
    
	return FALSE;
}

BOOL CBCServerInternal::_MyOnRecieve(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS)
// we could add some pointer security checks here, they miss currently, we asume data is consitent.
{
	switch (pMyCDS->dwData)
	{
	case HLAE_BASECOM_QRYSV_HELLO:
		// no checks performed atm
		return FALSE;

	case HLAE_BASECOM_QRYSV_OnCreateWindow:
		return _Wrapper_OnCreateWindow(hWnd,hwSender,pMyCDS);
	case HLAE_BASECOM_MSGSV_UpdateWindow:
		return _Wrapper_UpdateWindow(hWnd,hwSender,pMyCDS);

	default:
		g_debug.SendMessage(wxT("CBCServerInternal::_MyOnRecieve: Recieved unkown message."), hlaeDEBUG_ERROR);
	}
	return FALSE;
}

BOOL CBCServerInternal::_ReturnMessage(HWND hWnd,HWND hwTarget,ULONG dwData,DWORD cbData,PVOID lpData)
{
	if(!hwTarget) return FALSE;
	
	COPYDATASTRUCT myCopyData;
	
	myCopyData.dwData=dwData;
	myCopyData.cbData=cbData;
	myCopyData.lpData=lpData;

	return SendMessageW(
		hwTarget,
		WM_COPYDATA,
		(WPARAM)hWnd, // identify us as sender
		(LPARAM)&myCopyData
	);
}

BOOL CBCServerInternal::_Wrapper_OnCreateWindow(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS)
{
	BOOL bRes;

	HLAE_BASECOM_RET_OnCreateWindow_s *pRet = new HLAE_BASECOM_RET_OnCreateWindow_s;

	HLAE_BASECOM_OnCreateWindow_s * pdata = (HLAE_BASECOM_OnCreateWindow_s *)pMyCDS->lpData;

	pRet->parentWindow = (HWND)( _pBase->_OnCreateWindow( pdata->nWidth,pdata->nHeight ) );

	bRes=_ReturnMessage(hWnd,hwSender,HLAE_BASECOM_RETCL_OnCreateWindow,sizeof(HLAE_BASECOM_RET_OnCreateWindow_s),pRet);

	delete pRet;

	return bRes;
}

BOOL CBCServerInternal:: _Wrapper_UpdateWindow(HWND hWnd,HWND hwSender,PCOPYDATASTRUCT pMyCDS)
{
	BOOL bRes;

	HLAE_BASECOM_UpdateWindows_s * pdata = (HLAE_BASECOM_UpdateWindows_s *)pMyCDS->lpData;

	bRes = _pBase->_UpdateWindow(pdata->nWidth,pdata->nHeight) ? TRUE : FALSE;

	return bRes;
}


//
// the CBCServerInternal global:
//

CBCServerInternal g_BCServerInternal;


///////////////////////////////////////////////////////////////////////////////

//
// CHlaeBcServer
//

CHlaeBcServer::CHlaeBcServer(wxWindow *parent)
{
	_parent = parent;
	_pHlaeGameWindow = NULL;
	_hGLRC = NULL;

	if(!g_BCServerInternal.HlaeBcSrvStart(this))
		g_debug.SendMessage(wxT("ERROR: HlaeBcSrvStart() failed."),hlaeDEBUG_FATALERROR);

}

CHlaeBcServer::~CHlaeBcServer()
{
	if(_pHlaeGameWindow) delete _pHlaeGameWindow;
	g_BCServerInternal.HlaeBcSrvStop();
}

bool CHlaeBcServer::PassEventPreParsed(unsigned int umsg,unsigned int wParam,unsigned int lParam)
{
	return TRUE == g_BCServerInternal.DispatchToClientProc((HWND)(_pHlaeGameWindow->GetHWND()),(UINT)umsg,(WPARAM)wParam,(LPARAM)lParam);
}

bool CHlaeBcServer::PassEventPreParsed(WXHWND hwnd,unsigned int umsg,unsigned int wParam,unsigned int lParam)
{
	return TRUE == g_BCServerInternal.DispatchToClientProc((HWND)hwnd,(UINT)umsg,(WPARAM)wParam,(LPARAM)lParam);
}

bool CHlaeBcServer::Pass_WndRectUpdate(int iLeft, int iTop, int iWidthVisible, int iHeightVisible, int iWidthTotal, int iHeightTotal, int iLeftGlobal, int iTopGlobal)
{
	HLAE_BASECOM_WndRectUpdate_s mys;

	mys.iLeft = iLeft;
	mys.iTop = iTop;
	mys.iWidthVisible = iWidthVisible;
	mys.iHeightVisible = iHeightVisible;
	mys.iWidthTotal = iWidthTotal;
	mys.iHeightTotal = iHeightTotal;
	mys.iLeftGlobal = iLeftGlobal;
	mys.iTopGlobal = iTopGlobal;

	return TRUE==g_BCServerInternal.DispatchStruct(
		HLAE_BASECOM_MSGCL_WndRectUpdate,
		sizeof(mys),
		&mys
	);
}
bool CHlaeBcServer::Pass_MouseEvent(unsigned int uMsg, unsigned int wParam, unsigned short iX,unsigned short iY)
{
	HLAE_BASECOM_MSGCL_MouseEvent_s mys;

	mys.uMsg = uMsg;
	mys.wParam = wParam;
	mys.iX = iX;
	mys.iY = iY;

	// WARNING: HACK HACK! bad hack, we can't decide if the dispatch failed or if it wasn't processed!
	return FALSE==g_BCServerInternal.DispatchStruct(
		HLAE_BASECOM_MSGCL_MouseEvent,
		sizeof(mys),
		&mys
	);
}
bool CHlaeBcServer::Pass_KeyBoardEvent(unsigned int uMsg, unsigned int uKeyCode, unsigned int uKeyFlags)
{
	HLAE_BASECOM_MSGCL_KeyBoardEvent_s mys;

	mys.uMsg = uMsg;
	mys.uKeyCode = uKeyCode;
	mys.uKeyFlags = uKeyFlags;

	// WARNING: HACK HACK! bad hack, we can't decide if the dispatch failed or if it wasn't processed!
	return FALSE==g_BCServerInternal.DispatchStruct(
		HLAE_BASECOM_MSGCL_KeyBoardEvent,
		sizeof(mys),
		&mys
	);
}

WXHWND CHlaeBcServer::_OnCreateWindow(int nWidth, int nHeight)
{
	g_debug.SendMessage(wxT("Client connected."), hlaeDEBUG_VERBOSE_LEVEL3);

	if (!_pHlaeGameWindow)
	{
		// window not present, create it first:
		wxString mycaption("Game Window",wxConvUTF8);

		_pHlaeGameWindow =new CHlaeGameWindow(this,_parent,wxID_ANY,wxDefaultPosition,wxSize(200,150),wxHSCROLL | wxVSCROLL,mycaption);

		// adjust size:
		_UpdateWindow(nWidth, nHeight);
		
		g_layoutmanager.AddPane(_pHlaeGameWindow, wxAuiPaneInfo().CentrePane().Caption(mycaption));
	} else {
		// adjust size and prepare for drawing:
		_UpdateWindow(nWidth, nHeight);
	}

	WXHWND hwRet=_pHlaeGameWindow->GetHWND();

	wxString mystr;
	mystr.Printf(wxT("_OnCreateWindow WXHWND: 0x%08x"),hwRet); 
	g_debug.SendMessage(mystr, hlaeDEBUG_DEBUG);

	return hwRet;
}

bool CHlaeBcServer::_UpdateWindow(int nWidth, int nHeight)
{
	g_debug.SendMessage(wxT("Client sent window size update."), hlaeDEBUG_VERBOSE_LEVEL3);
	if (!_pHlaeGameWindow) return false;
	
	_pHlaeGameWindow->SetVirtualSize(nWidth,nHeight);
	//_pHlaeGameWindow->SetScrollRate(10,10);
	_pHlaeGameWindow->SetScrollbars(1,1,nWidth,nHeight);
	_pHlaeGameWindow->SetMaxSize(wxSize(nWidth,nHeight));

	// prepare for drawing:
	wxClientDC dc(_pHlaeGameWindow);
	_pHlaeGameWindow->DoPrepareDC(dc);

	return true;
}