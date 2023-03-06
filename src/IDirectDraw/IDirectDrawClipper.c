#include "IDirectDrawClipper.h"
#include "ddclipper.h"
#include "debug.h"

HRESULT __stdcall IDirectDrawClipper__QueryInterface(IDirectDrawClipperImpl* This, REFIID riid, LPVOID FAR* ppvObj)
{
    TRACE("NOT_IMPLEMENTED -> %s(This=%p, riid=%08X, ppvObj=%p)\n", __FUNCTION__, This, (unsigned int)riid, ppvObj);
    HRESULT ret = E_NOINTERFACE;
    TRACE("NOT_IMPLEMENTED <- %s\n", __FUNCTION__);
    return ret;
}

ULONG __stdcall IDirectDrawClipper__AddRef(IDirectDrawClipperImpl* This)
{
    TRACE("-> %s(This=%p)\n", __FUNCTION__, This);
    ULONG ret = ++This->ref;
    TRACE("<- %s(This ref=%u)\n", __FUNCTION__, ret);
    return ret;
}

ULONG __stdcall IDirectDrawClipper__Release(IDirectDrawClipperImpl* This)
{
    TRACE("-> %s(This=%p)\n", __FUNCTION__, This);

    ULONG ret = --This->ref;

    if (This->ref == 0)
    {
        TRACE("     Released (%p)\n", This);

        if (This->region)
            DeleteObject(This->region);

        HeapFree(GetProcessHeap(), 0, This);
    }

    TRACE("<- %s(This ref=%u)\n", __FUNCTION__, ret);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__GetClipList(
    IDirectDrawClipperImpl* This,
    LPRECT lpRect,
    LPRGNDATA lpClipList,
    LPDWORD lpdwSiz)
{
    TRACE(
        "NOT_IMPLEMENTED -> %s(This=%p, lpRect=%p, lpClipList=%p, lpdwSiz=%p)\n", 
        __FUNCTION__, 
        This, 
        lpRect, 
        lpClipList, 
        lpdwSiz);

    HRESULT ret = ddc_GetClipList(This, lpRect, lpClipList, lpdwSiz);

    TRACE("NOT_IMPLEMENTED <- %s\n", __FUNCTION__);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__GetHWnd(IDirectDrawClipperImpl* This, HWND FAR* lphWnd)
{
    TRACE("-> %s(This=%p, lphWnd=%p)\n", __FUNCTION__, This, lphWnd);
    HRESULT ret = ddc_GetHWnd(This, lphWnd);
    TRACE("<- %s\n", __FUNCTION__);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__Initialize(IDirectDrawClipperImpl* This, LPDIRECTDRAW lpDD, DWORD dwFlags)
{
    TRACE("-> %s(This=%p)\n", __FUNCTION__, This);
    HRESULT ret = DD_OK;
    TRACE("<- %s\n", __FUNCTION__);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__IsClipListChanged(IDirectDrawClipperImpl* This, BOOL FAR* lpbChanged)
{
    TRACE("-> %s(This=%p, lpbChanged=%p)\n", __FUNCTION__, This, lpbChanged);
    HRESULT ret = ddc_IsClipListChanged(This, lpbChanged);
    TRACE("<- %s\n", __FUNCTION__);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__SetClipList(IDirectDrawClipperImpl* This, LPRGNDATA lpClipList, DWORD dwFlags)
{
    TRACE("NOT_IMPLEMENTED -> %s(This=%p, lpClipList=%p, dwFlags=%08X)\n", __FUNCTION__, This, lpClipList, dwFlags);
    HRESULT ret = ddc_SetClipList(This, lpClipList, dwFlags);
    TRACE("NOT_IMPLEMENTED <- %s\n", __FUNCTION__);
    return ret;
}

HRESULT __stdcall IDirectDrawClipper__SetHWnd(IDirectDrawClipperImpl* This, DWORD dwFlags, HWND hWnd)
{
    TRACE("-> %s(This=%p, dwFlags=%08X, hWnd=%p)\n", __FUNCTION__, This, dwFlags, hWnd);
    HRESULT ret = ddc_SetHWnd(This, dwFlags, hWnd);
    TRACE("<- %s\n", __FUNCTION__);
    return ret;
}

struct IDirectDrawClipperImplVtbl g_ddc_vtbl =
{
    /*** IUnknown methods ***/
    IDirectDrawClipper__QueryInterface,
    IDirectDrawClipper__AddRef,
    IDirectDrawClipper__Release,
    /*** IDirectDrawClipper methods ***/
    IDirectDrawClipper__GetClipList,
    IDirectDrawClipper__GetHWnd,
    IDirectDrawClipper__Initialize,
    IDirectDrawClipper__IsClipListChanged,
    IDirectDrawClipper__SetClipList,
    IDirectDrawClipper__SetHWnd
};
