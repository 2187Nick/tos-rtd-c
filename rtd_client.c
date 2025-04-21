/**
 * rtd_client.c - Real-time Market Data Client
 * 
 * A lightweight COM client for accessing real-time market data from ThinkOrSwim's
 * RTD server.
 */

#include <windows.h>
#include <oleauto.h>
#include <stdio.h>
#include <initguid.h>
#include "rtd_client.h"

/**
 * GUID Definitions
 */
// Define RTDServerLib GUID (used for type library)
DEFINE_GUID(LIBID_RTDServerLib, 
    0xBA792DC8, 0x807E, 0x43E3, 0xB4, 0x84, 0x47, 0x46, 0x5D, 0x82, 0xC4, 0xD1);

// Define IRtdServer interface GUID
DEFINE_GUID(IID_IRtdServer,
    0xEC0E6191, 0xDB51, 0x11D3, 0x8F, 0x3E, 0x00, 0xC0, 0x4F, 0x36, 0x51, 0xB8);

// Define IRTDUpdateEvent interface GUID
DEFINE_GUID(IID_IRTDUpdateEvent,
    0xA43788C1, 0xD91B, 0x11D3, 0x8F, 0x39, 0x00, 0xC0, 0x4F, 0x36, 0x51, 0xB8);

// ---- callback object for IRTDUpdateEvent ----
static volatile LONG g_update_flag = 0;

// Global variables for symbol and topic handling
static WCHAR currentSymbol[32] = L"";  // No default symbol
static WCHAR currentTopic[32] = L"";   // No default topic
static CRITICAL_SECTION symbolLock;
static BOOL shouldReconnect = FALSE;
static BOOL shouldExit = FALSE;        // Flag for application exit
static BOOL shouldPause = FALSE; // Add this line for pause control

// Forward method declarations for our callback object
static HRESULT STDMETHODCALLTYPE CB_QueryInterface(IRTDUpdateEvent*, REFIID, void**);
static ULONG   STDMETHODCALLTYPE CB_AddRef(IRTDUpdateEvent*);
static ULONG   STDMETHODCALLTYPE CB_Release(IRTDUpdateEvent*);
static HRESULT STDMETHODCALLTYPE CB_UpdateNotify(IRTDUpdateEvent*);
static HRESULT STDMETHODCALLTYPE CB_get_HeartbeatInterval(IRTDUpdateEvent*, long*);
static HRESULT STDMETHODCALLTYPE CB_put_HeartbeatInterval(IRTDUpdateEvent*, long);
static HRESULT STDMETHODCALLTYPE CB_Disconnect(IRTDUpdateEvent*);

// IDispatch stubs
static HRESULT STDMETHODCALLTYPE CB_GetTypeInfoCount(IRTDUpdateEvent*, UINT*);
static HRESULT STDMETHODCALLTYPE CB_GetTypeInfo(IRTDUpdateEvent*, UINT, LCID, ITypeInfo**);
static HRESULT STDMETHODCALLTYPE CB_GetIDsOfNames(IRTDUpdateEvent*, REFIID, LPOLESTR*, UINT, LCID, DISPID*);
static HRESULT STDMETHODCALLTYPE CB_Invoke(IRTDUpdateEvent*, DISPID, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*);

/**
 * Define the vtable for our IRTDUpdateEvent implementation
 * This maps function pointers to our implementation methods
 */
static IRTDUpdateEventVtbl cb_vtbl = {
    // IUnknown
    CB_QueryInterface,
    CB_AddRef,
    CB_Release,

    // IDispatch
    CB_GetTypeInfoCount,
    CB_GetTypeInfo,
    CB_GetIDsOfNames,
    CB_Invoke,

    // IRTDUpdateEvent
    CB_UpdateNotify,
    CB_get_HeartbeatInterval,
    CB_put_HeartbeatInterval,
    CB_Disconnect
};

/**
 * Creates a COM callback object
 */
MyCallback* CreateCallback(void)
{
    MyCallback *cb = (MyCallback*)CoTaskMemAlloc(sizeof *cb);
    cb->lpVtbl   = &cb_vtbl;
    cb->refCount = 1;
    return cb;
}

/**
 * QueryInterface implementation for our COM object
 */
static HRESULT STDMETHODCALLTYPE CB_QueryInterface(IRTDUpdateEvent *this,
                                           REFIID riid,
                                           void **ppv)
{
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDispatch) ||
        IsEqualIID(riid, &IID_IRTDUpdateEvent))
    {
        *ppv = this;
        this->lpVtbl->AddRef(this);
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

/**
 * AddRef implementation for our COM object
 */
static ULONG STDMETHODCALLTYPE CB_AddRef(IRTDUpdateEvent *this)
{
    MyCallback *cb = (MyCallback*)this;
    return InterlockedIncrement(&cb->refCount);
}

/**
 * Release implementation for our COM object
 */
static ULONG STDMETHODCALLTYPE CB_Release(IRTDUpdateEvent *this)
{
    MyCallback *cb = (MyCallback*)this;
    LONG c = InterlockedDecrement(&cb->refCount);
    if (c == 0) {
        CoTaskMemFree(cb);
    }
    return c;
}

/**
 * Called by the RTD server when data has changed
 */
static HRESULT STDMETHODCALLTYPE CB_UpdateNotify(IRTDUpdateEvent *this)
{
    InterlockedExchange(&g_update_flag, 1);
    return S_OK;
}

/**
 * Returns the heartbeat interval in milliseconds
 */
static HRESULT STDMETHODCALLTYPE CB_get_HeartbeatInterval(IRTDUpdateEvent *this,
                                                  long *plRetVal)
{
    *plRetVal = 100;  // 100ms for more frequent updates
    return S_OK;
}

/**
 * Sets the heartbeat interval
 */
static HRESULT STDMETHODCALLTYPE CB_put_HeartbeatInterval(IRTDUpdateEvent *this,
                                                  long plRetVal)
{
    return S_OK;
}

/**
 * Called when the server disconnects
 */
static HRESULT STDMETHODCALLTYPE CB_Disconnect(IRTDUpdateEvent *this)
{
    return S_OK;
}

/**
 * IDispatch stub implementations
 */
static HRESULT STDMETHODCALLTYPE CB_GetTypeInfoCount(IRTDUpdateEvent *this, UINT *pctinfo)
{
    *pctinfo = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE CB_GetTypeInfo(IRTDUpdateEvent *this, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    *ppTInfo = NULL;
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE CB_GetIDsOfNames(IRTDUpdateEvent *this, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE CB_Invoke(IRTDUpdateEvent *this, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pvarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    return E_NOTIMPL;
}

/**
 * Console handler for Ctrl+C/Ctrl+V
 */
static BOOL WINAPI ConsoleHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        wprintf(L"\nShutting down...\n");
        shouldExit = TRUE;  // Set the global exit flag
        return TRUE;
    }
    return FALSE;
}

/**
 * Input thread to handle symbol changes
 */
static DWORD WINAPI InputThreadProc(LPVOID lpParam) {
    char input[256];
    WCHAR wideInput[256];
    
    while (!shouldExit) {
        // Only print the prompt if the stream is paused (waiting for user input)
        if (shouldPause) {
            printf("Enter new symbol (or 'quit' to exit): ");
            fflush(stdout);
        }
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        
        // Remove newline
        input[strcspn(input, "\r\n")] = 0;
        
        if (strcmp(input, "quit") == 0) {
            wprintf(L"\nShutting down...\n");
            shouldExit = TRUE;  // Set global exit flag
            break;
        }
        
        // If input is empty, pause the stream and show prompt again
        if (input[0] == '\0') {
            shouldPause = TRUE;
            continue;
        } else {
            shouldPause = FALSE;
        }
        
        // Convert to wide string
        MultiByteToWideChar(CP_UTF8, 0, input, -1, wideInput, sizeof(wideInput)/sizeof(WCHAR));
        
        // Update symbol with lock protection
        EnterCriticalSection(&symbolLock);
        wcscpy_s(currentSymbol, 32, wideInput);
        shouldReconnect = TRUE;
        LeaveCriticalSection(&symbolLock);
        // Do not print the prompt here; let the main loop handle the next pause
    }
    
    return 0;
}

/**
 * Connect to a symbol with specified topic
 */
BOOL ConnectToSymbol(IRtdServer *pSrv, WCHAR *symbol, SAFEARRAY **ppArgs, long *pTopicID) {
    // Disconnect existing if needed
    if (*ppArgs) {
        pSrv->lpVtbl->DisconnectData(pSrv, *pTopicID);
        SafeArrayDestroy(*ppArgs);
        *ppArgs = NULL;
    }    // Create variants for the topic and symbol
    VARIANT vType, vSym;
    VariantInit(&vType);
    vType.vt = VT_BSTR;
    vType.bstrVal = SysAllocString(currentTopic);  // Use specified topic

    VariantInit(&vSym);
    vSym.vt = VT_BSTR;
    vSym.bstrVal = SysAllocString(symbol);
    
    // Create SAFEARRAY
    SAFEARRAYBOUND sab;
    sab.cElements = 2;
    sab.lLbound = 0;
    *ppArgs = SafeArrayCreate(VT_VARIANT, 1, &sab);
    
    // Add elements to SAFEARRAY
    SafeArrayPutElement(*ppArgs, (LONG[]){0}, &vType);
    SafeArrayPutElement(*ppArgs, (LONG[]){1}, &vSym);
    
    // Connect to data
    VARIANT initVal; 
    VariantInit(&initVal);
    VARIANT_BOOL getNew = VARIANT_TRUE;
    HRESULT hr = pSrv->lpVtbl->ConnectData(pSrv, *pTopicID, ppArgs, &getNew, &initVal);
    
    // Clean up variants
    VariantClear(&vType);
    VariantClear(&vSym);
    
    if (FAILED(hr)) {
        wprintf(L"Connection failed for symbol %ls: 0x%08X\n", symbol, hr);
        return FALSE;
    }
       
    wprintf(L"Connected to symbol: %ls\n", symbol);
    return TRUE;
}

/**
 * Format variant value as string
 */
void FormatVariantValue(VARIANT *value, WCHAR *buffer, size_t bufferSize) {
    switch (value->vt) {
        case VT_BSTR:
            swprintf(buffer, bufferSize, L"%ls", value->bstrVal);
            break;
        case VT_R8:
            swprintf(buffer, bufferSize, L"%.6f", value->dblVal);
            break;
        case VT_I4:
            swprintf(buffer, bufferSize, L"%ld", value->lVal);
            break;
        default:
            swprintf(buffer, bufferSize, L"<unknown type %d>", value->vt);
    }
}

/**
 * Main application entry point
 */
int main(void)
{
    HRESULT hr;
    CLSID   clsid;
    IRtdServer      *pSrv = NULL;
    IRTDUpdateEvent *pCB  = NULL;
    SAFEARRAY       *pArgs = NULL;
    SAFEARRAY       *pOutArr = NULL;
    long            topicCount = 0;
    long            topicID = 1;
    BOOL            running = TRUE;
    
    // Set up Ctrl+C handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    wprintf(L"\n");
    wprintf(L"+--------------------------------------------------+\n");
    wprintf(L"| RTD Client - Real-time Market Data Viewer        |\n");
    wprintf(L"+--------------------------------------------------+\n\n");

    wprintf(L"\nPress \"Enter\" at any time to change symbols or to exit\n\n");



    // Initialize COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        wprintf(L"COM initialization failed: 0x%08X\n", hr);
        return 1;
    }

    // Initialize thread safety for symbol changes
    InitializeCriticalSection(&symbolLock);
    
    // Prompt for initial symbol
    char symbolInput[64];
    wprintf(L"Enter stock symbol: ");
    if (fgets(symbolInput, sizeof(symbolInput), stdin) == NULL) {
        wprintf(L"Error reading input. Exiting.\n");
        return 1;
    }
    symbolInput[strcspn(symbolInput, "\r\n")] = 0;  // Remove newline
    MultiByteToWideChar(CP_UTF8, 0, symbolInput, -1, currentSymbol, sizeof(currentSymbol)/sizeof(WCHAR));
    
    // Prompt for initial topic
    char topicInput[32];
    wprintf(L"Enter data topic (LAST, BID, ASK, etc): ");
    if (fgets(topicInput, sizeof(topicInput), stdin) == NULL) {
        wprintf(L"Error reading input. Exiting.\n");
        return 1;
    }
    topicInput[strcspn(topicInput, "\r\n")] = 0;  // Remove newline
    MultiByteToWideChar(CP_UTF8, 0, topicInput, -1, currentTopic, sizeof(currentTopic)/sizeof(WCHAR));
    
    // Create and start the input thread for symbol changes
    HANDLE inputThread = CreateThread(NULL, 0, InputThreadProc, NULL, 0, NULL);
    if (!inputThread) {
        wprintf(L"Failed to create input thread: %d\n", GetLastError());
        return 1;
    }
    
    // Get the COM class for the RTD server
    hr = CLSIDFromProgID(L"Tos.RTD", &clsid);
    if (FAILED(hr)) {
        wprintf(L"Failed to get RTD server CLSID: 0x%08X\n", hr);
        wprintf(L"Make sure ThinkOrSwim is running and the RTD server is available\n");
        goto cleanup;
    }

    // Create an instance of the RTD server
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
                        &IID_IRtdServer, (void**)&pSrv);
    if (FAILED(hr)) {
        wprintf(L"Failed to create RTD server instance: 0x%08X\n", hr);
        goto cleanup;
    }

    // Create our callback and start the server
    pCB = (IRTDUpdateEvent*)CreateCallback();
    hr = pSrv->lpVtbl->ServerStart(pSrv, pCB, &(long){1000});
    if (FAILED(hr)) {
        wprintf(L"RTD server failed to start: 0x%08X\n", hr);
        goto cleanup;
    }
    
    wprintf(L"\nRTD server connection established successfully\n");

    // Connect to initial symbol
    if (!ConnectToSymbol(pSrv, currentSymbol, &pArgs, &topicID)) {
        wprintf(L"Initial connection failed\n");
        goto cleanup;
    }
      // Main event loop
    while (running && !shouldExit) {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = FALSE;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Check if we need to reconnect with a different symbol
        EnterCriticalSection(&symbolLock);
        if (shouldReconnect) {
            if (wcslen(currentSymbol) > 0) {
                wprintf(L"\nReconnecting with new symbol: %ls\n", currentSymbol);
                if (ConnectToSymbol(pSrv, currentSymbol, &pArgs, &topicID)) {
                    wprintf(L"Connected to symbol: %ls\n", currentSymbol);
                } else {
                    wprintf(L"Failed to connect to symbol: %ls\n", currentSymbol);
                }
            }
            shouldReconnect = FALSE;
        }
        LeaveCriticalSection(&symbolLock);

        // Pause the stream if shouldPause is set
        if (shouldPause) {
            Sleep(100); // Wait until user enters a new symbol
            continue;
        }

        // Process RTD updates
        if (InterlockedCompareExchange(&g_update_flag, 0, 1)) {
            topicCount = 0;
            hr = pSrv->lpVtbl->RefreshData(pSrv, &topicCount, &pOutArr);
            
            if (SUCCEEDED(hr) && pOutArr && topicCount > 0) {
                // Process the data
                if (pOutArr->cDims == 2) {
                    LONG rowCount = pOutArr->rgsabound[0].cElements;
                    LONG colCount = pOutArr->rgsabound[1].cElements;
                    
                    VARIANT *pData = NULL;
                    hr = SafeArrayAccessData(pOutArr, (void**)&pData);
                    
                    if (SUCCEEDED(hr)) {
                        // Get current timestamp for display
                        SYSTEMTIME st;
                        GetLocalTime(&st);
                        
                        // Process all elements that were updated
                        for (int i = 0; i < min(rowCount, topicCount); i++) {
                            // Get the topic ID from the first column
                            if (pData[i*colCount].vt == VT_I4) {
                                long rcvTopicID = pData[i*colCount].lVal;
                                
                                // If this is our topic ID, print the value
                                if (rcvTopicID == topicID) {
                                    // Get the value from the second column
                                    VARIANT *value = &pData[i*colCount + 1];
                                    WCHAR valueStr[128] = L"";
                                    
                                    // Format the value
                                    FormatVariantValue(value, valueStr, ARRAYSIZE(valueStr));
                                    
                                    // Print update with timestamp
                                    wprintf(L"[%02d:%02d:%02d.%03d] %ls = %ls\n", 
                                            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                                            currentSymbol, valueStr);
                                }
                            }
                        }
                        
                        SafeArrayUnaccessData(pOutArr);
                    }
                }
                
                SafeArrayDestroy(pOutArr);
                pOutArr = NULL;
            }
        }
        
        Sleep(100);  // Small sleep to avoid excessive CPU usage
    }

cleanup:
    wprintf(L"Cleaning up and exiting\n");
    
    // Disconnect data if connected
    if (pArgs) {
        pSrv->lpVtbl->DisconnectData(pSrv, topicID);
        SafeArrayDestroy(pArgs);
        pArgs = NULL;
    }
    
    // Terminate RTD server
    if (pSrv) {
        pSrv->lpVtbl->ServerTerminate(pSrv);
        pSrv->lpVtbl->Release(pSrv);
    }
    
    // Release callback
    if (pCB) pCB->lpVtbl->Release(pCB);
    
    // Clean up thread resources
    DeleteCriticalSection(&symbolLock);
    
    CoUninitialize();
    return 0;
}
