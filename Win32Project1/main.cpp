// Win32Project1.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "EtwFilterSample.h"

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名
HWND  g_hwndListBox;
DWORD g_dwPointerSize;

// {7DD42A49-5329-4832-8DFD-43D979153A88}
DEFINE_GUID(LoggerGUID ,
	0xb36daecd, 0x9c9c, 0x43fc, 0x81, 0x26, 0x9a, 0x84, 0xc6, 0x37, 0x21, 0x3b);

DEFINE_GUID( /* {7DD42A49-5329-4832-8DFD-43D979153A88} */
	TcpIpGuid,
	0x7DD42A49,
	0x5329,
	0x4832,
	0x8D, 0xFD, 0x43, 0xD9, 0x79, 0x15, 0x3A, 0x88
);



#define LOGGER_NAME  L"MyLogger"
#define LOGFILE_NAME  L"D:\\hoge.etl"



// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI ThreadProc(LPVOID lpParamater);
void WINAPI EventRecordCallback(PEVENT_RECORD EventRecord);
void ShowPropertyInfo(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, PEVENT_PROPERTY_INFO pPropertyInfo, LPWSTR lpszStructName);
void ShowEventInfo(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo);
void ShowNameString(LPBYTE lp, ULONG uOffset, LPWSTR lpszTitle);
BOOL ConvertSidToName(PSID pSid, LPWSTR lpszName, DWORD dwSizeName);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化しています。
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WIN32PROJECT1, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーションの初期化を実行します:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WIN32PROJECT1));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIN32PROJECT1));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_WIN32PROJECT1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します。
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス処理を格納します。

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的:    メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウの描画
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PEVENT_TRACE_PROPERTIES pProperties = NULL;
	static HANDLE                  hThread = NULL;
	static TRACEHANDLE             hSession = 0;


    switch (message)
    {
	case WM_CREATE: {
		DWORD dwBufferSize;
		ULONG uResult;

		dwBufferSize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(LOGGER_NAME);
		pProperties = (PEVENT_TRACE_PROPERTIES)HeapAlloc(GetProcessHeap(), 0, dwBufferSize);

		ZeroMemory(pProperties, dwBufferSize);
		pProperties->Wnode.BufferSize = dwBufferSize;
		pProperties->Wnode.Guid = LoggerGUID;
		pProperties->Wnode.ClientContext = 1;
		pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
		pProperties->EnableFlags = EVENT_TRACE_FLAG_NETWORK_TCPIP;
		pProperties->FlushTimer = 1;
		pProperties->MaximumFileSize = 5;
		pProperties->LogFileMode = EVENT_TRACE_REAL_TIME_MODE;
		pProperties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

		uResult = StartTrace(&hSession, LOGGER_NAME, pProperties);
		if (uResult != ERROR_SUCCESS) {
			if (uResult == ERROR_ACCESS_DENIED)
				MessageBox(NULL, TEXT("アクセスが拒否されました。"), NULL, MB_ICONWARNING);
			else if (uResult == ERROR_ALREADY_EXISTS)
				MessageBox(NULL, TEXT("セッションは既に存在します。"), NULL, MB_ICONWARNING);
			else
				MessageBox(NULL, TEXT("StartTraceの呼び出しに失敗しました。"), NULL, MB_ICONWARNING);
			return -1;
		}

		EVENT_FILTER_EVENT_ID ids;
		ZeroMemory(&ids, sizeof(EVENT_FILTER_EVENT_ID));
		ids.FilterIn = true;
		ids.Count = 1;
		ids.Events[0] = 27;


		EVENT_FILTER_DESCRIPTOR filter[1];
		ZeroMemory(&filter, sizeof(EVENT_FILTER_DESCRIPTOR));
		filter[0].Type = EVENT_FILTER_TYPE_EVENT_ID;
		filter[0].Size = sizeof(EVENT_FILTER_EVENT_ID);
		filter[0].Ptr = (ULONGLONG) &ids;


		ENABLE_TRACE_PARAMETERS params;
		ZeroMemory(&params, sizeof(ENABLE_TRACE_PARAMETERS));
		params.Version = ENABLE_TRACE_PARAMETERS_VERSION_2;
		params.EnableFilterDesc = filter;
		params.FilterDescCount = 1;

		uResult = EnableTraceEx2(hSession, &TcpIpGuid, EVENT_CONTROL_CODE_ENABLE_PROVIDER, 4, 0x8000000000000030, 0, 0, &params);

		if (uResult != ERROR_SUCCESS)
		{
			switch (uResult) {
			case ERROR_INVALID_PARAMETER: {
				MessageBox(NULL, TEXT("ERROR_INVALID_PARAMETER"), NULL, MB_ICONWARNING);
				break;
			}
			case ERROR_INVALID_FUNCTION: {
				MessageBox(NULL, TEXT("ERROR_INVALID_FUNCTION"), NULL, MB_ICONWARNING);
				break;
			}
			case ERROR_NO_SYSTEM_RESOURCES: {
				MessageBox(NULL, TEXT("ERROR_NO_SYSTEM_RESOURCES"), NULL, MB_ICONWARNING);
				break;
			}
			case ERROR_ACCESS_DENIED: {
				MessageBox(NULL, TEXT("ERROR_ACCESS_DENIED"), NULL, MB_ICONWARNING);
				break;
			}
			}

			return -1;
		}
		
		g_hwndListBox = CreateWindowEx(0, TEXT("LISTBOX"), NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, 0, 0, hWnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, NULL, 0, NULL);
		return 0;
	}
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 選択されたメニューの解析:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: HDC を使用する描画コードをここに追加してください...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		if (hSession != 0) {
			ControlTrace(hSession, KERNEL_LOGGER_NAME, pProperties, EVENT_TRACE_CONTROL_STOP);
			WaitForSingleObject(hThread, 5000);
			CloseHandle(hThread);
			HeapFree(GetProcessHeap(), 0, pProperties);
		}
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

DWORD WINAPI ThreadProc(LPVOID lpParamater)
{
	TRACEHANDLE         hTrace;
	ULONG               uResult;
	EVENT_TRACE_LOGFILE logfile;
	ULONG				uSize = sizeof(EVENT_TRACE_LOGFILE);

	ZeroMemory(&logfile, uSize);
	logfile.LoggerName = LOGGER_NAME;
	logfile.ProcessTraceMode = PROCESS_TRACE_MODE_REAL_TIME | PROCESS_TRACE_MODE_EVENT_RECORD;
	logfile.EventRecordCallback = (PEVENT_RECORD_CALLBACK)EventRecordCallback;

	hTrace = OpenTrace(&logfile);
	if (hTrace == (TRACEHANDLE)INVALID_HANDLE_VALUE)
		return 0;

	uResult = ProcessTrace(&hTrace, 1, NULL, NULL);
	if (uResult != ERROR_SUCCESS) {
		TCHAR szBuf[256];
		wsprintf(szBuf, TEXT("%d"), uResult);
		MessageBox(NULL, szBuf, NULL, MB_ICONWARNING);
		CloseTrace(hTrace);
		return 0;
	}

	CloseTrace(hTrace);

	return 0;
}

void WINAPI EventRecordCallback(PEVENT_RECORD EventRecord)
{
	ULONG             uBufferSize = 0;
	TDHSTATUS         status;
	PTRACE_EVENT_INFO pInfo;

	if (EventRecord->EventHeader.EventDescriptor.Id != 27) {
		return;
	}

	status = TdhGetEventInformation(EventRecord, 0, NULL, 0, &uBufferSize);
	if (status != ERROR_INSUFFICIENT_BUFFER)
		return;

	pInfo = (PTRACE_EVENT_INFO)HeapAlloc(GetProcessHeap(), 0, uBufferSize);
	TdhGetEventInformation(EventRecord, 0, NULL, pInfo, &uBufferSize);

	if ((EventRecord->EventHeader.Flags & EVENT_HEADER_FLAG_32_BIT_HEADER) == EVENT_HEADER_FLAG_32_BIT_HEADER)
		g_dwPointerSize = 4;
	else
		g_dwPointerSize = 8;

	ShowEventInfo(EventRecord, pInfo);

	HeapFree(GetProcessHeap(), 0, pInfo);
}

void ShowEventInfo(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo)
{
	ULONG i;
	TCHAR szBuf[256];

	wsprintf(szBuf, TEXT("Id: %u"), pInfo->EventDescriptor.Id);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Version: %u"), pInfo->EventDescriptor.Version);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Channel: %u"), pInfo->EventDescriptor.Channel);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Level: %u"), pInfo->EventDescriptor.Level);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Opcode: %u"), pInfo->EventDescriptor.Opcode);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Task: %u"), pInfo->EventDescriptor.Task);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	wsprintf(szBuf, TEXT("Keyword: %lu"), pInfo->EventDescriptor.Keyword);
	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

	ShowNameString((LPBYTE)pInfo + pInfo->ProviderNameOffset, pInfo->ProviderNameOffset, L"ProviderName");
	ShowNameString((LPBYTE)pInfo + pInfo->LevelNameOffset, pInfo->LevelNameOffset, L"LevelName");
	ShowNameString((LPBYTE)pInfo + pInfo->ChannelNameOffset, pInfo->ChannelNameOffset, L"ChannelName");
	ShowNameString((LPBYTE)pInfo + pInfo->KeywordsNameOffset, pInfo->KeywordsNameOffset, L"KeywordsName");
	ShowNameString((LPBYTE)pInfo + pInfo->TaskNameOffset, pInfo->TaskNameOffset, L"TaskName");
	ShowNameString((LPBYTE)pInfo + pInfo->OpcodeNameOffset, pInfo->OpcodeNameOffset, L"OpcodeName");
	ShowNameString((LPBYTE)pInfo + pInfo->ActivityIDNameOffset, pInfo->ActivityIDNameOffset, L"ActivityIDName");
	ShowNameString((LPBYTE)pInfo + pInfo->RelatedActivityIDNameOffset, pInfo->RelatedActivityIDNameOffset, L"RelatedActivityIDName");

	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)TEXT("----------Property----------"));
	for (i = 0; i < pInfo->TopLevelPropertyCount; i++)
		ShowPropertyInfo(pEvent, pInfo, &pInfo->EventPropertyInfoArray[i], NULL);

	SendMessage(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)TEXT(""));
}

void ShowPropertyInfo(PEVENT_RECORD pEvent, PTRACE_EVENT_INFO pInfo, PEVENT_PROPERTY_INFO pPropertyInfo, LPWSTR lpszStructName)
{
	WCHAR                    szBuf[1024];
	PROPERTY_DATA_DESCRIPTOR propertyData[2];
	LPWSTR                   lpszName;
	LPBYTE                   lpProperty;
	ULONG                    i, uPropertyCount;
	ULONG                    uBufferSize;

	lpszName = (LPWSTR)((LPBYTE)pInfo + pPropertyInfo->NameOffset);

	if (pPropertyInfo->Flags & PropertyStruct) {
		PEVENT_PROPERTY_INFO p;
		for (i = 0; i < pPropertyInfo->structType.NumOfStructMembers; i++) {
			p = &pInfo->EventPropertyInfoArray[i + pPropertyInfo->structType.StructStartIndex];
			ShowPropertyInfo(pEvent, pInfo, p, lpszName);
		}
		return;
	}

	if (lpszStructName == NULL) {
		propertyData[0].ArrayIndex = ULONG_MAX;
		propertyData[0].PropertyName = (ULONGLONG)lpszName;
		uPropertyCount = 1;
	}
	else {
		propertyData[0].ArrayIndex = 0;
		propertyData[0].PropertyName = (ULONGLONG)lpszStructName;
		propertyData[1].ArrayIndex = ULONG_MAX;
		propertyData[1].PropertyName = (ULONGLONG)lpszName;
		uPropertyCount = 2;
	}

	uBufferSize = 0;
	TdhGetPropertySize(pEvent, 0, NULL, uPropertyCount, propertyData, &uBufferSize);
	lpProperty = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, uBufferSize);
	TdhGetProperty(pEvent, 0, NULL, uPropertyCount, propertyData, uBufferSize, lpProperty);

	if (pPropertyInfo->nonStructType.MapNameOffset == 0) {
		WCHAR szContainStructName[256];

		if (lpszStructName != NULL) {
			wsprintfW(szContainStructName, L"%s.%s", lpszStructName, lpszName);
			lpszName = szContainStructName;
		}

		switch (pPropertyInfo->nonStructType.InType) {
		case TDH_INTYPE_UNICODESTRING:
			wsprintfW(szBuf, L"%s : %s", lpszName, (LPWSTR)lpProperty);
			break;
		case TDH_INTYPE_ANSISTRING: {
			LPSTR  lpszA = (LPSTR)lpProperty;
			LPWSTR lpszW;
			DWORD  dwSize;

			dwSize = (lstrlenA(lpszA) + 1) * sizeof(WCHAR);
			lpszW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, dwSize);
			MultiByteToWideChar(CP_ACP, 0, lpszA, -1, lpszW, dwSize);

			wsprintfW(szBuf, L"%s : %s", lpszName, lpszW);
			HeapFree(GetProcessHeap(), 0, lpszW);

			break;
		}
		case TDH_INTYPE_INT16:
			wsprintfW(szBuf, L"%s : %d", lpszName, *(PSHORT)lpProperty);
			break;
		case TDH_INTYPE_UINT16:
			wsprintfW(szBuf, L"%s : %d", lpszName, *(PUSHORT)lpProperty);
			break;
		case TDH_INTYPE_INT32:
			wsprintfW(szBuf, L"%s : %d", lpszName, *(PLONG)lpProperty);
			break;
		case TDH_INTYPE_UINT32:
			wsprintfW(szBuf, L"%s : %d", lpszName, *(PULONG)lpProperty);
			break;
		case TDH_INTYPE_UINT64:
			wsprintfW(szBuf, L"%s : %d", lpszName, *(PULONGLONG)lpProperty);
			break;
		case TDH_INTYPE_GUID: {
			WCHAR szGuid[256];
			StringFromGUID2(*(LPGUID)lpProperty, szGuid, 256);
			wsprintfW(szBuf, L"%s : %s", lpszName, szGuid);
			break;
		}
		case TDH_INTYPE_POINTER:
			if (g_dwPointerSize == 4)
				wsprintfW(szBuf, L"%s : %#x", lpszName, *(PULONG)lpProperty);
			else
				wsprintfW(szBuf, L"%s : %#x", lpszName, *(PULONGLONG)lpProperty);
			break;
		case TDH_INTYPE_SID: {
			WCHAR szAccountName[256];
			PSID  pSid = (PSID)lpProperty;
			ConvertSidToName(pSid, szAccountName, 256);
			wsprintfW(szBuf, L"%s : %s", lpszName, szAccountName);
			break;
		}
		case TDH_INTYPE_WBEMSID: {
			WCHAR szAccountName[256];
			PSID  pSid = (PSID)(lpProperty + g_dwPointerSize * 2);
			ConvertSidToName(pSid, szAccountName, 256);
			wsprintfW(szBuf, L"%s : %s", lpszName, szAccountName);
			break;
		}
		default:
			wsprintfW(szBuf, L"%s : type %d", lpszName, pPropertyInfo->nonStructType.InType);
			break;
		}
	}
	else {
		PEVENT_MAP_INFO pMapInfo;
		LPWSTR          lpszMapName;
		TDHSTATUS       status;
		ULONG           uMapValue = *(PULONG)lpProperty;
		BOOL            bFound = FALSE;

		uBufferSize = 0;
		lpszMapName = (LPWSTR)((LPBYTE)pInfo + pPropertyInfo->nonStructType.MapNameOffset);
		status = TdhGetEventMapInformation(pEvent, lpszMapName, NULL, &uBufferSize);
		if (status != ERROR_INSUFFICIENT_BUFFER) {
			HeapFree(GetProcessHeap(), 0, lpProperty);
			return;
		}
		pMapInfo = (PEVENT_MAP_INFO)HeapAlloc(GetProcessHeap(), 0, uBufferSize);
		TdhGetEventMapInformation(pEvent, lpszMapName, pMapInfo, &uBufferSize);

		if (pMapInfo->Flag & EVENTMAP_INFO_FLAG_MANIFEST_VALUEMAP) {
			LPWSTR lpsz;

			for (i = 0; i < pMapInfo->EntryCount; i++) {
				if (pMapInfo->MapEntryArray[i].Value == uMapValue) {
					lpsz = (LPWSTR)((LPBYTE)pMapInfo + pMapInfo->MapEntryArray[i].OutputOffset);
					wsprintfW(szBuf, L"%s : %s(%d)", lpszName, lpsz, uMapValue);
					bFound = TRUE;
					break;
				}
			}
		}

		if (!bFound)
			wsprintfW(szBuf, L"%s : %d", lpszName, uMapValue);

		HeapFree(GetProcessHeap(), 0, pMapInfo);
	}

	SendMessageW(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);

	HeapFree(GetProcessHeap(), 0, lpProperty);
}

void ShowNameString(LPBYTE lp, ULONG uOffset, LPWSTR lpszTitle)
{
	LPWSTR lpszName = (LPWSTR)lp;
	WCHAR  szBuf[256];

	if (uOffset != 0) {
		wsprintfW(szBuf, L"%s: %s", lpszTitle, lpszName);
		SendMessageW(g_hwndListBox, LB_ADDSTRING, 0, (LPARAM)szBuf);
	}
}

BOOL ConvertSidToName(PSID pSid, LPWSTR lpszName, DWORD dwSizeName)
{
	WCHAR        szDomainName[256];
	DWORD        dwSizeDomain = sizeof(szDomainName) / sizeof(TCHAR);
	SID_NAME_USE sidName;

	return LookupAccountSidW(NULL, pSid, lpszName, &dwSizeName, szDomainName, &dwSizeDomain, &sidName);
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
