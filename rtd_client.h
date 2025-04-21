// rtd_client.h - RTD Client for real-time market data
// COM interface definitions and application declarations

#ifndef __RTD_CLIENT_H__
#define __RTD_CLIENT_H__

#include <windows.h>
#include <oaidl.h>  // For IDispatch, VARIANT, etc.

// Forward declarations
typedef struct IRtdServer IRtdServer;
typedef struct IRTDUpdateEvent IRTDUpdateEvent;
typedef struct IRtdServerVtbl IRtdServerVtbl;
typedef struct IRTDUpdateEventVtbl IRTDUpdateEventVtbl;

// GUID declarations
extern const IID IID_IRtdServer;
extern const IID IID_IRTDUpdateEvent;
extern const IID LIBID_RTDServerLib;

// Callback structure definition (forward declaration of vtable)
typedef struct MyCallback {
    IRTDUpdateEventVtbl *lpVtbl;
    LONG                refCount;
} MyCallback;

// Topic subscription structure
typedef struct {
    SAFEARRAY* pArgs;
    long topicID;
    WCHAR symbol[64];
    WCHAR topic[32];
} TopicSubscription;

// Function declarations
MyCallback* CreateCallback(void);
BOOL ConnectToSymbol(IRtdServer *pSrv, WCHAR *symbol, SAFEARRAY **ppArgs, long *pTopicID);
void FormatVariantValue(VARIANT *value, WCHAR *buffer, size_t bufferSize);

// IRtdServer vtable definition
typedef struct IRtdServerVtbl
{
    // IUnknown methods
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IRtdServer *This,
        REFIID riid,
        void **ppvObject);
    
    ULONG (STDMETHODCALLTYPE *AddRef)(
        IRtdServer *This);
    
    ULONG (STDMETHODCALLTYPE *Release)(
        IRtdServer *This);
    
    // IDispatch methods
    HRESULT (STDMETHODCALLTYPE *GetTypeInfoCount)(
        IRtdServer *This,
        UINT *pctinfo);
    
    HRESULT (STDMETHODCALLTYPE *GetTypeInfo)(
        IRtdServer *This,
        UINT iTInfo,
        LCID lcid,
        ITypeInfo **ppTInfo);
    
    HRESULT (STDMETHODCALLTYPE *GetIDsOfNames)(
        IRtdServer *This,
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId);
    
    HRESULT (STDMETHODCALLTYPE *Invoke)(
        IRtdServer *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);
    
    // IRtdServer specific methods
    HRESULT (STDMETHODCALLTYPE *ServerStart)(
        IRtdServer *This,
        IRTDUpdateEvent *CallbackObject,
        long *pfRes);
    
    HRESULT (STDMETHODCALLTYPE *ConnectData)(
        IRtdServer *This,
        long TopicID,
        SAFEARRAY **Strings,
        VARIANT_BOOL *GetNewValues,
        VARIANT *pvarOut);
    
    HRESULT (STDMETHODCALLTYPE *RefreshData)(
        IRtdServer *This,
        long *TopicCount,
        SAFEARRAY **parrayOut);
    
    HRESULT (STDMETHODCALLTYPE *DisconnectData)(
        IRtdServer *This,
        long TopicID);
    
    HRESULT (STDMETHODCALLTYPE *Heartbeat)(
        IRtdServer *This,
        long *pfRes);
    
    HRESULT (STDMETHODCALLTYPE *ServerTerminate)(
        IRtdServer *This);
} IRtdServerVtbl;

// IRtdServer interface
typedef struct IRtdServer
{
    const IRtdServerVtbl *lpVtbl;
} IRtdServer;

// IRTDUpdateEvent vtable definition
typedef struct IRTDUpdateEventVtbl
{
    // IUnknown methods
    HRESULT (STDMETHODCALLTYPE *QueryInterface)(
        IRTDUpdateEvent *This,
        REFIID riid,
        void **ppvObject);
    
    ULONG (STDMETHODCALLTYPE *AddRef)(
        IRTDUpdateEvent *This);
    
    ULONG (STDMETHODCALLTYPE *Release)(
        IRTDUpdateEvent *This);
    
    // IDispatch methods
    HRESULT (STDMETHODCALLTYPE *GetTypeInfoCount)(
        IRTDUpdateEvent *This,
        UINT *pctinfo);
    
    HRESULT (STDMETHODCALLTYPE *GetTypeInfo)(
        IRTDUpdateEvent *This,
        UINT iTInfo,
        LCID lcid,
        ITypeInfo **ppTInfo);
    
    HRESULT (STDMETHODCALLTYPE *GetIDsOfNames)(
        IRTDUpdateEvent *This,
        REFIID riid,
        LPOLESTR *rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID *rgDispId);
    
    HRESULT (STDMETHODCALLTYPE *Invoke)(
        IRTDUpdateEvent *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS *pDispParams,
        VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo,
        UINT *puArgErr);
    
    // IRTDUpdateEvent specific methods
    HRESULT (STDMETHODCALLTYPE *UpdateNotify)(
        IRTDUpdateEvent *This);
    
    HRESULT (STDMETHODCALLTYPE *get_HeartbeatInterval)(
        IRTDUpdateEvent *This,
        long *plRetVal);
    
    HRESULT (STDMETHODCALLTYPE *put_HeartbeatInterval)(
        IRTDUpdateEvent *This,
        long plRetVal);
    
    HRESULT (STDMETHODCALLTYPE *Disconnect)(
        IRTDUpdateEvent *This);
} IRTDUpdateEventVtbl;

// IRTDUpdateEvent interface
typedef struct IRTDUpdateEvent
{
    const IRTDUpdateEventVtbl *lpVtbl;
} IRTDUpdateEvent;

#endif /* __RTD_CLIENT_H__ */
