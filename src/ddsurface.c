#include <windows.h>
#include <stdio.h>
#include "dllmain.h"
#include "dd.h"
#include "hook.h"
#include "ddsurface.h"
#include "mouse.h"
#include "IDirectDrawSurface.h"
#include "winapi_hooks.h"
#include "debug.h"
#include "utils.h"
#include "blt.h"


HRESULT dds_AddAttachedSurface(IDirectDrawSurfaceImpl* This, IDirectDrawSurfaceImpl* lpDDSurface)
{
    if (lpDDSurface)
    {
        IDirectDrawSurface_AddRef(lpDDSurface);

        if (!This->backbuffer)
        {
            lpDDSurface->caps |= DDSCAPS_BACKBUFFER;
            This->backbuffer = lpDDSurface;
        }
    }

    return DD_OK;
}

HRESULT dds_Blt(
    IDirectDrawSurfaceImpl* This,
    LPRECT lpDestRect,
    IDirectDrawSurfaceImpl* lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags,
    LPDDBLTFX lpDDBltFx)
{
    dbg_dump_dds_blt_flags(dwFlags);
    dbg_dump_dds_blt_fx_flags((dwFlags & DDBLT_DDFX) && lpDDBltFx ? lpDDBltFx->dwDDFX : 0);

    if (g_ddraw && 
        g_ddraw->iskkndx &&
        (dwFlags & DDBLT_COLORFILL) &&
        lpDestRect &&
        lpDestRect->right == 640 &&
        lpDestRect->bottom == 480)
    {
        if (This->backbuffer)
        {
            dds_Blt(This->backbuffer, lpDestRect, NULL, NULL, dwFlags, lpDDBltFx);
        }

        lpDestRect = NULL;
    }

    IDirectDrawSurfaceImpl* src_surface = lpDDSrcSurface;

    RECT src_rect = { 0, 0, src_surface ? src_surface->width : 0, src_surface ? src_surface->height : 0 };
    RECT dst_rect = { 0, 0, This->width, This->height };

    if (lpSrcRect && src_surface)
        memcpy(&src_rect, lpSrcRect, sizeof(src_rect));

    if (lpDestRect)
        memcpy(&dst_rect, lpDestRect, sizeof(dst_rect));

    int src_w = src_rect.right - src_rect.left;
    int src_h = src_rect.bottom - src_rect.top;

    int dst_w = dst_rect.right - dst_rect.left;
    int dst_h = dst_rect.bottom - dst_rect.top;

    float scale_w = (src_w > 0 && dst_w > 0) ? (float)src_w / dst_w : 1.0f;
    float scale_h = (src_h > 0 && dst_h > 0) ? (float)src_h / dst_h : 1.0f;

    BOOL is_stretch_blt = src_w != dst_w || src_h != dst_h;

    /* Disable this for now (needs more testing)
    if (This->clipper && !(dwFlags & DDBLT_NO_CLIP) && dst_w > 0 && dst_h > 0)
    {
        DWORD size = 0;

        if (SUCCEEDED(IDirectDrawClipper_GetClipList(This->clipper, &dst_rect, NULL, &size)))
        {
            RGNDATA* list = (RGNDATA*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);

            if (list)
            {
                if (SUCCEEDED(IDirectDrawClipper_GetClipList(This->clipper, &dst_rect, list, &size)))
                {
                    RECT* dst_c_rect = (RECT*)list->Buffer;

                    for (int i = 0; i < list->rdh.nCount; ++i)
                    {
                        RECT src_c_rect = src_rect;

                        if (src_surface)
                        {
                            src_c_rect.left += (LONG)((dst_c_rect[i].left - dst_rect.left) * scale_w);
                            src_c_rect.top += (LONG)((dst_c_rect[i].top - dst_rect.top) * scale_h);
                            src_c_rect.right -= (LONG)((dst_rect.right - dst_c_rect[i].right) * scale_w);
                            src_c_rect.bottom -= (LONG)((dst_rect.bottom - dst_c_rect[i].bottom) * scale_h);
                        }

                        dds_Blt(This, &dst_c_rect[i], src_surface, &src_c_rect, dwFlags | DDBLT_NO_CLIP, lpDDBltFx);
                    }
                }

                HeapFree(GetProcessHeap(), 0, list);

                return DD_OK;
            }
        }
    }
    */

    if (dst_rect.right < 0)
        dst_rect.right = 0;

    if (dst_rect.left < 0)
    {
        src_rect.left += (LONG)(abs(dst_rect.left) * scale_w);
        dst_rect.left = 0;
    }

    if (dst_rect.bottom < 0)
        dst_rect.bottom = 0;

    if (dst_rect.top < 0)
    {
        src_rect.top += (LONG)(abs(dst_rect.top) * scale_h);
        dst_rect.top = 0;
    }

    if (dst_rect.right > This->width)
    {
        src_rect.right -= (LONG)((dst_rect.right - This->width) * scale_w);
        dst_rect.right = This->width;
    }

    if (dst_rect.left > dst_rect.right)
        dst_rect.left = dst_rect.right;

    if (dst_rect.bottom > This->height)
    {
        src_rect.bottom -= (LONG)((dst_rect.bottom - This->height) * scale_h);
        dst_rect.bottom = This->height;
    }

    if (dst_rect.top > dst_rect.bottom)
        dst_rect.top = dst_rect.bottom;

    if (src_surface)
    {
        if (src_rect.right < 0)
            src_rect.right = 0;

        if (src_rect.left < 0)
            src_rect.left = 0;

        if (src_rect.bottom < 0)
            src_rect.bottom = 0;

        if (src_rect.top < 0)
            src_rect.top = 0;

        if (src_rect.right > src_surface->width)
            src_rect.right = src_surface->width;

        if (src_rect.left > src_rect.right)
            src_rect.left = src_rect.right;

        if (src_rect.bottom > src_surface->height)
            src_rect.bottom = src_surface->height;

        if (src_rect.top > src_rect.bottom)
            src_rect.top = src_rect.bottom;
    }

    src_w = src_rect.right - src_rect.left;
    src_h = src_rect.bottom - src_rect.top;

    int src_x = src_rect.left;
    int src_y = src_rect.top;

    dst_w = dst_rect.right - dst_rect.left;
    dst_h = dst_rect.bottom - dst_rect.top;

    int dst_x = dst_rect.left;
    int dst_y = dst_rect.top;

    void* dst_buf = dds_GetBuffer(This);
    void* src_buf = dds_GetBuffer(src_surface);

    if (dst_buf && (dwFlags & DDBLT_COLORFILL) && lpDDBltFx && dst_w > 0 && dst_h > 0)
    {
        blt_colorfill(
            dst_buf,
            dst_x,
            dst_y,
            dst_w,
            dst_h,
            This->pitch,
            lpDDBltFx->dwFillColor,
            This->bpp);
    }

    if (src_surface && src_w > 0 && src_h > 0 && dst_w > 0 && dst_h > 0)
    {
        if (!is_stretch_blt)
        {
            src_w = dst_w = min(src_w, dst_w);
            src_h = dst_h = min(src_h, dst_h);
        }

        BOOL got_fx = (dwFlags & DDBLT_DDFX) && lpDDBltFx;
        BOOL mirror_left_right = got_fx && (lpDDBltFx->dwDDFX & DDBLTFX_MIRRORLEFTRIGHT);
        BOOL mirror_up_down = got_fx && (lpDDBltFx->dwDDFX & DDBLTFX_MIRRORUPDOWN);

        if (This->bpp != src_surface->bpp)
        {
            TRACE_EXT("     NOT_IMPLEMENTED This->bpp=%u, src_surface->bpp=%u\n", This->bpp, src_surface->bpp);

            HDC dst_dc;
            dds_GetDC(This, &dst_dc);

            HDC src_dc;
            dds_GetDC(src_surface, &src_dc);

            if ((dwFlags & DDBLT_KEYSRC) || (dwFlags & DDBLT_KEYSRCOVERRIDE))
            {
                UINT color = 
                    (dwFlags & DDBLT_KEYSRCOVERRIDE) ?
                    lpDDBltFx->ddckSrcColorkey.dwColorSpaceLowValue : src_surface->color_key.dwColorSpaceLowValue;

                if (src_surface->bpp == 32)
                {
                    color = color & 0xFFFFFF;
                }
                else if (src_surface->bpp == 16)
                {
                    unsigned short c = (unsigned short)color;

                    BYTE r = ((c & 0xF800) >> 11) << 3;
                    BYTE g = ((c & 0x07E0) >> 5) << 2;
                    BYTE b = ((c & 0x001F)) << 3;

                    color = RGB(r, g, b);
                }
                else if (src_surface->bpp == 8)
                {
                    RGBQUAD* quad =
                        src_surface->palette ? src_surface->palette->data_rgb :
                        g_ddraw && g_ddraw->primary && g_ddraw->primary->palette ? g_ddraw->primary->palette->data_rgb :
                        NULL;

                    if (quad)
                    {
                        unsigned char i = (unsigned char)color;

                        color = RGB(quad[i].rgbRed, quad[i].rgbGreen, quad[i].rgbBlue);
                    }                   
                }

                GdiTransparentBlt(dst_dc, dst_x, dst_y, dst_w, dst_h, src_dc, src_x, src_y, src_w, src_h, color);
            }
            else
            {
                real_StretchBlt(dst_dc, dst_x, dst_y, dst_w, dst_h, src_dc, src_x, src_y, src_w, src_h, SRCCOPY);
            }

            /*
            StretchBlt(
                dst_dc, 
                lpDestRect->left, 
                lpDestRect->top, 
                lpDestRect->right - lpDestRect->left, 
                lpDestRect->bottom - lpDestRect->top, 
                src_dc, 
                lpSrcRect->left, 
                lpSrcRect->top, 
                lpSrcRect->right - lpSrcRect->left, 
                lpSrcRect->bottom - lpSrcRect->top, 
                SRCCOPY);
                */
        }
        else if (
            (dwFlags & DDBLT_KEYSRC) ||
            (dwFlags & DDBLT_KEYSRCOVERRIDE) ||
            mirror_left_right ||
            mirror_up_down)
        {
            DDCOLORKEY color_key = { 0xFFFFFFFF, 0 };

            if ((dwFlags & DDBLT_KEYSRC) || (dwFlags & DDBLT_KEYSRCOVERRIDE))
            {
                color_key.dwColorSpaceLowValue =
                    (dwFlags & DDBLT_KEYSRCOVERRIDE) ?
                    lpDDBltFx->ddckSrcColorkey.dwColorSpaceLowValue : src_surface->color_key.dwColorSpaceLowValue;

                color_key.dwColorSpaceHighValue =
                    (dwFlags & DDBLT_KEYSRCOVERRIDE) ?
                    lpDDBltFx->ddckSrcColorkey.dwColorSpaceHighValue : src_surface->color_key.dwColorSpaceHighValue;

                if (color_key.dwColorSpaceHighValue < color_key.dwColorSpaceLowValue)
                    color_key.dwColorSpaceHighValue = color_key.dwColorSpaceLowValue;
            }

            if (!is_stretch_blt && !mirror_left_right && !mirror_up_down)
            {
                blt_colorkey(
                    dst_buf,
                    dst_x,
                    dst_y,
                    dst_w,
                    dst_h,
                    This->pitch,
                    src_buf,
                    src_x,
                    src_y,
                    src_surface->pitch,
                    color_key.dwColorSpaceLowValue,
                    color_key.dwColorSpaceHighValue,
                    This->bpp);
            }
            else
            {
                blt_colorkey_mirror_stretch(
                    dst_buf,
                    dst_x,
                    dst_y,
                    dst_w,
                    dst_h,
                    This->pitch,
                    src_buf,
                    src_x,
                    src_y,
                    src_w,
                    src_h,
                    src_surface->pitch,
                    color_key.dwColorSpaceLowValue,
                    color_key.dwColorSpaceHighValue,
                    mirror_up_down,
                    mirror_left_right,
                    This->bpp);
            }
        }
        else if (is_stretch_blt && (src_w != dst_w || src_h != dst_h))
        {
            blt_stretch(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_w,
                src_h,
                src_surface->pitch,
                This->bpp);
        }
        else if (This == src_surface)
        {
            blt_overlap(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_surface->pitch,
                This->bpp);
        }
        else
        {
            blt_clean(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_surface->pitch,
                This->bpp);
        }
    }

    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw && g_ddraw->render.run)
    {
        InterlockedExchange(&g_ddraw->render.surface_updated, TRUE);

        if (!(This->flags & DDSD_BACKBUFFERCOUNT) || This->last_flip_tick + FLIP_REDRAW_TIMEOUT < timeGetTime())
        {
            This->last_blt_tick = timeGetTime();

            ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);
            SwitchToThread();

            if (g_ddraw->ticks_limiter.tick_length > 0)
            {
                g_ddraw->ticks_limiter.use_blt_or_flip = TRUE;
                util_limit_game_ticks();
            }
        }
    }

    return DD_OK;
}

HRESULT dds_BltFast(
    IDirectDrawSurfaceImpl* This,
    DWORD dwX,
    DWORD dwY,
    IDirectDrawSurfaceImpl* lpDDSrcSurface,
    LPRECT lpSrcRect,
    DWORD dwFlags)
{
    dbg_dump_dds_blt_fast_flags(dwFlags);

    IDirectDrawSurfaceImpl* src_surface = lpDDSrcSurface;

    RECT src_rect = { 0, 0, src_surface ? src_surface->width : 0, src_surface ? src_surface->height : 0 };

    if (lpSrcRect && src_surface)
        memcpy(&src_rect, lpSrcRect, sizeof(src_rect));

    int dst_x = dwX;
    int dst_y = dwY;

    if (dst_x < 0)
    {
        src_rect.left += abs(dst_x);
        dst_x = 0;
    }

    if (dst_y < 0)
    {
        src_rect.top += abs(dst_y);
        dst_y = 0;
    }

    if (src_surface)
    {
        if (src_rect.right < 0)
            src_rect.right = 0;

        if (src_rect.left < 0)
            src_rect.left = 0;

        if (src_rect.bottom < 0)
            src_rect.bottom = 0;

        if (src_rect.top < 0)
            src_rect.top = 0;

        if (src_rect.right > src_surface->width)
            src_rect.right = src_surface->width;

        if (src_rect.left > src_rect.right)
            src_rect.left = src_rect.right;

        if (src_rect.bottom > src_surface->height)
            src_rect.bottom = src_surface->height;

        if (src_rect.top > src_rect.bottom)
            src_rect.top = src_rect.bottom;
    }

    int src_x = src_rect.left;
    int src_y = src_rect.top;

    RECT dst_rect = { dst_x, dst_y, (src_rect.right - src_rect.left) + dst_x, (src_rect.bottom - src_rect.top) + dst_y };

    if (dst_rect.right < 0)
        dst_rect.right = 0;

    if (dst_rect.left < 0)
        dst_rect.left = 0;

    if (dst_rect.bottom < 0)
        dst_rect.bottom = 0;

    if (dst_rect.top < 0)
        dst_rect.top = 0;

    if (dst_rect.right > This->width)
        dst_rect.right = This->width;

    if (dst_rect.left > dst_rect.right)
        dst_rect.left = dst_rect.right;

    if (dst_rect.bottom > This->height)
        dst_rect.bottom = This->height;

    if (dst_rect.top > dst_rect.bottom)
        dst_rect.top = dst_rect.bottom;

    dst_x = dst_rect.left;
    dst_y = dst_rect.top;

    int dst_w = dst_rect.right - dst_rect.left;
    int dst_h = dst_rect.bottom - dst_rect.top;

    void* dst_buf = dds_GetBuffer(This);
    void* src_buf = dds_GetBuffer(src_surface);

    if (src_surface && dst_w > 0 && dst_h > 0)
    {
        if (This->bpp != src_surface->bpp)
        {
            TRACE_EXT("     NOT_IMPLEMENTED This->bpp=%u, src_surface->bpp=%u\n", This->bpp, src_surface->bpp);

            HDC dst_dc;
            dds_GetDC(This, &dst_dc);

            HDC src_dc;
            dds_GetDC(src_surface, &src_dc);

            if (dwFlags & DDBLTFAST_SRCCOLORKEY)
            {
                UINT color = src_surface->color_key.dwColorSpaceLowValue;

                if (src_surface->bpp == 32)
                {
                    color = color & 0xFFFFFF;
                }
                else if (src_surface->bpp == 16)
                {
                    unsigned short c = (unsigned short)color;

                    BYTE r = ((c & 0xF800) >> 11) << 3;
                    BYTE g = ((c & 0x07E0) >> 5) << 2;
                    BYTE b = ((c & 0x001F)) << 3;

                    color = RGB(r, g, b);
                }
                else if (src_surface->bpp == 8)
                {
                    RGBQUAD* quad =
                        src_surface->palette ? src_surface->palette->data_rgb :
                        g_ddraw && g_ddraw->primary && g_ddraw->primary->palette ? g_ddraw->primary->palette->data_rgb :
                        NULL;

                    if (quad)
                    {
                        unsigned char i = (unsigned char)color;

                        color = RGB(quad[i].rgbRed, quad[i].rgbGreen, quad[i].rgbBlue);
                    }
                }

                GdiTransparentBlt(dst_dc, dst_x, dst_y, dst_w, dst_h, src_dc, src_x, src_y, dst_w, dst_h, color);
            }
            else
            {
                BitBlt(dst_dc, dst_x, dst_y, dst_w, dst_h, src_dc, src_x, src_y, SRCCOPY);
            }

            /*
            BitBlt(
                dst_dc, 
                dwX, 
                dwY, 
                lpSrcRect->right - lpSrcRect->left, 
                lpSrcRect->bottom - lpSrcRect->top, 
                src_dc, 
                lpSrcRect->left, 
                lpSrcRect->top, 
                SRCCOPY);
                */
        }
        else if (dwFlags & DDBLTFAST_SRCCOLORKEY)
        {
            blt_colorkey(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_surface->pitch,
                src_surface->color_key.dwColorSpaceLowValue,
                src_surface->color_key.dwColorSpaceHighValue,
                This->bpp);
        }
        else if (This == src_surface)
        {
            blt_overlap(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_surface->pitch,
                This->bpp);
        }
        else
        {
            blt_clean(
                dst_buf,
                dst_x,
                dst_y,
                dst_w,
                dst_h,
                This->pitch,
                src_buf,
                src_x,
                src_y,
                src_surface->pitch,
                This->bpp);
        }
    }

    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw && g_ddraw->render.run)
    {
        InterlockedExchange(&g_ddraw->render.surface_updated, TRUE);

        DWORD time = timeGetTime();

        if (!(This->flags & DDSD_BACKBUFFERCOUNT) ||
            (This->last_flip_tick + FLIP_REDRAW_TIMEOUT < time && This->last_blt_tick + FLIP_REDRAW_TIMEOUT < time))
        {
            ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);

            if (g_ddraw->limit_bltfast && g_ddraw->ticks_limiter.tick_length > 0)
            {
                g_ddraw->ticks_limiter.use_blt_or_flip = TRUE;
                util_limit_game_ticks();
            }
        }
    }

    return DD_OK;
}

HRESULT dds_DeleteAttachedSurface(IDirectDrawSurfaceImpl* This, DWORD dwFlags, IDirectDrawSurfaceImpl* lpDDSurface)
{
    if (lpDDSurface)
    {
        IDirectDrawSurface_Release(lpDDSurface);

        if (lpDDSurface == This->backbuffer)
            This->backbuffer = NULL;
    }

    return DD_OK;
}

HRESULT dds_GetSurfaceDesc(IDirectDrawSurfaceImpl* This, LPDDSURFACEDESC lpDDSurfaceDesc)
{
    if (lpDDSurfaceDesc)
    {
        int size = lpDDSurfaceDesc->dwSize == sizeof(DDSURFACEDESC2) ? sizeof(DDSURFACEDESC2) : sizeof(DDSURFACEDESC);

        memset(lpDDSurfaceDesc, 0, size);

        lpDDSurfaceDesc->dwSize = size;
        lpDDSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_LPSURFACE | DDSD_BACKBUFFERCOUNT;
        lpDDSurfaceDesc->dwWidth = This->width;
        lpDDSurfaceDesc->dwHeight = This->height;
        lpDDSurfaceDesc->lPitch = This->pitch;
        lpDDSurfaceDesc->lpSurface = dds_GetBuffer(This);
        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
        lpDDSurfaceDesc->ddpfPixelFormat.dwFlags = DDPF_RGB;
        lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount = This->bpp;
        lpDDSurfaceDesc->ddsCaps.dwCaps = This->caps;
        lpDDSurfaceDesc->dwBackBufferCount = This->backbuffer_count;

        if ((g_ddraw && !g_ddraw->novidmem) || (This->caps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_BACKBUFFER)))
        {
            lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        }

        if (This->bpp == 8)
        {
            lpDDSurfaceDesc->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
        }
        else if (This->bpp == 16)
        {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xF800;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x07E0;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x001F;
        }
        else if (This->bpp == 32)
        {
            lpDDSurfaceDesc->ddpfPixelFormat.dwRBitMask = 0xFF0000;
            lpDDSurfaceDesc->ddpfPixelFormat.dwGBitMask = 0x00FF00;
            lpDDSurfaceDesc->ddpfPixelFormat.dwBBitMask = 0x0000FF;
        }
    }

    return DD_OK;
}

HRESULT dds_EnumAttachedSurfaces(
    IDirectDrawSurfaceImpl* This,
    LPVOID lpContext,
    LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback)
{
    static DDSURFACEDESC2 desc;

    memset(&desc, 0, sizeof(desc));

    if (This->backbuffer)
    {
        dds_GetSurfaceDesc(This->backbuffer, (LPDDSURFACEDESC)&desc);
        IDirectDrawSurface_AddRef(This->backbuffer);
        lpEnumSurfacesCallback((LPDIRECTDRAWSURFACE)This->backbuffer, (LPDDSURFACEDESC)&desc, lpContext);
    }

    return DD_OK;
}

HRESULT dds_Flip(IDirectDrawSurfaceImpl* This, IDirectDrawSurfaceImpl* lpDDSurfaceTargetOverride, DWORD dwFlags)
{
    dbg_dump_dds_flip_flags(dwFlags);

    if (This->backbuffer)
    {
        EnterCriticalSection(&g_ddraw->cs);
        IDirectDrawSurfaceImpl* backbuffer = lpDDSurfaceTargetOverride ? lpDDSurfaceTargetOverride : This->backbuffer;

        void* buf = InterlockedExchangePointer(&This->surface, backbuffer->surface);
        HBITMAP bitmap = (HBITMAP)InterlockedExchangePointer(&This->bitmap, backbuffer->bitmap);
        HDC dc = (HDC)InterlockedExchangePointer(&This->hdc, backbuffer->hdc);
        HANDLE map = (HANDLE)InterlockedExchangePointer(&This->mapping, backbuffer->mapping);

        InterlockedExchangePointer(&backbuffer->surface, buf);
        InterlockedExchangePointer(&backbuffer->bitmap, bitmap);
        InterlockedExchangePointer(&backbuffer->hdc, dc);
        InterlockedExchangePointer(&backbuffer->mapping, map);

        if (g_ddraw->flipclear)
        {
            blt_clear(buf, 0, backbuffer->size);
        }

        LeaveCriticalSection(&g_ddraw->cs);

        if (!lpDDSurfaceTargetOverride && This->backbuffer->backbuffer)
        {
            dds_Flip(This->backbuffer, NULL, 0);
        }
    }

    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw && g_ddraw->render.run)
    {
        This->last_flip_tick = timeGetTime();

        InterlockedExchange(&g_ddraw->render.surface_updated, TRUE);
        ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);
        SwitchToThread();

        if ((dwFlags & DDFLIP_WAIT) || g_ddraw->maxgameticks == -2)
        {
            dd_WaitForVerticalBlank(DDWAITVB_BLOCKEND, NULL);
        }

        if (g_ddraw->ticks_limiter.tick_length > 0)
        {
            g_ddraw->ticks_limiter.use_blt_or_flip = TRUE;
            util_limit_game_ticks();
        }
    }

    return DD_OK;
}

HRESULT dds_GetAttachedSurface(IDirectDrawSurfaceImpl* This, LPDDSCAPS lpDdsCaps, IDirectDrawSurfaceImpl** lpDDsurface)
{
    if (lpDdsCaps->dwCaps & DDSCAPS_BACKBUFFER)
    {
        if (This->backbuffer)
        {
            IDirectDrawSurface_AddRef(This->backbuffer);
            *lpDDsurface = This->backbuffer;
        }
        else
        {
            IDirectDrawSurface_AddRef(This);
            *lpDDsurface = This;
        }

        return DD_OK;
    }

    return DDERR_NOTFOUND;
}

HRESULT dds_GetCaps(IDirectDrawSurfaceImpl* This, LPDDSCAPS lpDDSCaps)
{
    if (!lpDDSCaps)
        return DDERR_INVALIDPARAMS;

    lpDDSCaps->dwCaps = This->caps;

    return DD_OK;
}

HRESULT dds_GetClipper(IDirectDrawSurfaceImpl* This, IDirectDrawClipperImpl** lpClipper)
{
    if (!lpClipper)
        return DDERR_INVALIDPARAMS;

    *lpClipper = This->clipper;

    if (This->clipper)
    {
        IDirectDrawClipper_AddRef(This->clipper);
        return DD_OK;
    }
    else
    {
        return DDERR_NOCLIPPERATTACHED;
    }
}

HRESULT dds_GetColorKey(IDirectDrawSurfaceImpl* This, DWORD dwFlags, LPDDCOLORKEY lpColorKey)
{
    if (dwFlags != DDCKEY_SRCBLT)
    {
        TRACE_EXT("     NOT_IMPLEMENTED dwFlags=%08X\n", dwFlags);
    }

    if (lpColorKey)
    {
        lpColorKey->dwColorSpaceHighValue = This->color_key.dwColorSpaceHighValue;
        lpColorKey->dwColorSpaceLowValue = This->color_key.dwColorSpaceLowValue;
    }

    return DD_OK;
}

HRESULT dds_GetDC(IDirectDrawSurfaceImpl* This, HDC FAR* lpHDC)
{
    if (!This)
    {
        if (lpHDC)
            *lpHDC = NULL;

        return DDERR_INVALIDPARAMS;
    }

    RGBQUAD* data =
        This->palette ? This->palette->data_rgb :
        g_ddraw && g_ddraw->primary && g_ddraw->primary->palette ? g_ddraw->primary->palette->data_rgb :
        NULL;

    HDC dc = This->hdc;

    if (This->backbuffer || (This->caps & DDSCAPS_BACKBUFFER))
        dc = (HDC)InterlockedExchangeAdd((LONG*)&This->hdc, 0);

    if (This->bpp == 8 && data)
        SetDIBColorTable(dc, 0, 256, data);

    if (lpHDC)
        *lpHDC = dc;

    return DD_OK;
}

HRESULT dds_GetPalette(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl** lplpDDPalette)
{
    if (!lplpDDPalette)
        return DDERR_INVALIDPARAMS;

    *lplpDDPalette = This->palette;

    if (This->palette)
    {
        IDirectDrawPalette_AddRef(This->palette);
        return DD_OK;
    }
    else
    {
        return DDERR_NOPALETTEATTACHED;
    }
}

HRESULT dds_GetPixelFormat(IDirectDrawSurfaceImpl* This, LPDDPIXELFORMAT ddpfPixelFormat)
{
    if (ddpfPixelFormat)
    {
        memset(ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT));

        ddpfPixelFormat->dwSize = sizeof(DDPIXELFORMAT);
        ddpfPixelFormat->dwFlags = DDPF_RGB;
        ddpfPixelFormat->dwRGBBitCount = This->bpp;

        if (This->bpp == 8)
        {
            ddpfPixelFormat->dwFlags |= DDPF_PALETTEINDEXED8;
        }
        else if (This->bpp == 16)
        {
            ddpfPixelFormat->dwRBitMask = 0xF800;
            ddpfPixelFormat->dwGBitMask = 0x07E0;
            ddpfPixelFormat->dwBBitMask = 0x001F;
        }
        else if (This->bpp == 32)
        {
            ddpfPixelFormat->dwRBitMask = 0xFF0000;
            ddpfPixelFormat->dwGBitMask = 0x00FF00;
            ddpfPixelFormat->dwBBitMask = 0x0000FF;
        }

        return DD_OK;
    }

    return DDERR_INVALIDPARAMS;
}

HRESULT dds_Lock(
    IDirectDrawSurfaceImpl* This,
    LPRECT lpDestRect,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    DWORD dwFlags,
    HANDLE hEvent)
{
    if (g_ddraw && g_ddraw->lock_surfaces)
        EnterCriticalSection(&This->cs);

    dbg_dump_dds_lock_flags(dwFlags);

    if (g_ddraw && g_ddraw->fixnotresponding)
    {
        MSG msg; /* workaround for "Not Responding" window problem */
        PeekMessage(&msg, g_ddraw->hwnd, 0, 0, PM_NOREMOVE);
    }

    HRESULT ret = dds_GetSurfaceDesc(This, lpDDSurfaceDesc);

    if (lpDestRect && lpDDSurfaceDesc)
    {
        if (lpDestRect->left < 0 ||
            lpDestRect->top < 0 ||
            lpDestRect->left > lpDestRect->right ||
            lpDestRect->top > lpDestRect->bottom ||
            lpDestRect->right > This->width ||
            lpDestRect->bottom > This->height)
        {
            return DDERR_INVALIDPARAMS;
        }

        lpDDSurfaceDesc->lpSurface =
            (char*)dds_GetBuffer(This) + (lpDestRect->left * This->bytes_pp) + (lpDestRect->top * This->pitch);
    }

    return ret;
}

HRESULT dds_ReleaseDC(IDirectDrawSurfaceImpl* This, HDC hDC)
{
    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw && g_ddraw->render.run)
    {
        InterlockedExchange(&g_ddraw->render.surface_updated, TRUE);

        DWORD time = timeGetTime();

        if (!(This->flags & DDSD_BACKBUFFERCOUNT) ||
            (This->last_flip_tick + FLIP_REDRAW_TIMEOUT < time && This->last_blt_tick + FLIP_REDRAW_TIMEOUT < time))
        {
            ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);
        }
    }

    return DD_OK;
}

HRESULT dds_SetClipper(IDirectDrawSurfaceImpl* This, IDirectDrawClipperImpl* lpClipper)
{
    if (lpClipper)
    {
        IDirectDrawClipper_AddRef(lpClipper);

        if ((This->caps & DDSCAPS_PRIMARYSURFACE) && lpClipper->hwnd)
        {
            if (lpClipper->region)
                DeleteObject(lpClipper->region);

            RECT rc = { 0, 0, This->width, This->height };

            lpClipper->region = CreateRectRgnIndirect(&rc);
        }
    }

    if (This->clipper)
        IDirectDrawClipper_Release(This->clipper);

    This->clipper = lpClipper;

    return DD_OK;
}

HRESULT dds_SetColorKey(IDirectDrawSurfaceImpl* This, DWORD dwFlags, LPDDCOLORKEY lpColorKey)
{
    if (dwFlags != DDCKEY_SRCBLT || !lpColorKey)
    {
        TRACE_EXT("     NOT_IMPLEMENTED dwFlags=%08X, lpColorKey=%p\n", dwFlags, lpColorKey);
    }

    if (lpColorKey)
    {
        This->color_key.dwColorSpaceHighValue = lpColorKey->dwColorSpaceHighValue;
        This->color_key.dwColorSpaceLowValue = lpColorKey->dwColorSpaceLowValue;
    }

    return DD_OK;
}

HRESULT dds_SetPalette(IDirectDrawSurfaceImpl* This, IDirectDrawPaletteImpl* lpDDPalette)
{
    if (lpDDPalette)
        IDirectDrawPalette_AddRef(lpDDPalette);

    if (This->palette)
        IDirectDrawPalette_Release(This->palette);

    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw)
    {
        EnterCriticalSection(&g_ddraw->cs);
        This->palette = lpDDPalette;
        LeaveCriticalSection(&g_ddraw->cs);

        if (g_ddraw->render.run)
        {
            InterlockedExchange(&g_ddraw->render.palette_updated, TRUE);
            ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);
        }
    }
    else
    {
        This->palette = lpDDPalette;
    }

    return DD_OK;
}

HRESULT dds_Unlock(IDirectDrawSurfaceImpl* This, LPRECT lpRect)
{
    /* Hack for Warcraft II BNE and Diablo */
    HWND hwnd = g_ddraw && g_ddraw->bnet_active ? FindWindowEx(HWND_DESKTOP, NULL, "SDlgDialog", NULL) : NULL;

    if (hwnd && (This->caps & DDSCAPS_PRIMARYSURFACE))
    {
        HDC primary_dc;
        dds_GetDC(This, &primary_dc);

        /* GdiTransparentBlt idea taken from Aqrit's war2 ddraw */

        RGBQUAD quad;
        GetDIBColorTable(primary_dc, 0xFE, 1, &quad);
        COLORREF color = RGB(quad.rgbRed, quad.rgbGreen, quad.rgbBlue);
        BOOL erase = FALSE;

        do
        {
            RECT rc;
            if (fake_GetWindowRect(hwnd, &rc))
            {
                if (rc.bottom - rc.top == 479)
                    erase = TRUE;

                HDC hdc = GetDCEx(hwnd, NULL, DCX_PARENTCLIP | DCX_CACHE);

                GdiTransparentBlt(
                    hdc,
                    0,
                    0,
                    rc.right - rc.left,
                    rc.bottom - rc.top,
                    primary_dc,
                    rc.left,
                    rc.top,
                    rc.right - rc.left,
                    rc.bottom - rc.top,
                    color
                );

                ReleaseDC(hwnd, hdc);
            }

        } while ((hwnd = FindWindowEx(HWND_DESKTOP, hwnd, "SDlgDialog", NULL)));

        if (erase)
        {
            BOOL x = g_ddraw->ticks_limiter.use_blt_or_flip;

            DDBLTFX fx = { .dwFillColor = 0xFE };
            IDirectDrawSurface_Blt(This, NULL, NULL, NULL, DDBLT_COLORFILL, &fx);

            g_ddraw->ticks_limiter.use_blt_or_flip = x;
        }
    }

    /* Hack for Star Trek Armada */
    hwnd = g_ddraw && g_ddraw->armadahack ? FindWindowEx(HWND_DESKTOP, NULL, "#32770", NULL) : NULL;

    if (hwnd && (This->caps & DDSCAPS_PRIMARYSURFACE))
    {
        HDC primary_dc;
        dds_GetDC(This, &primary_dc);

        RECT rc;
        if (fake_GetWindowRect(hwnd, &rc))
        {
            HDC hdc = GetDC(hwnd);

            GdiTransparentBlt(
                hdc,
                0,
                0,
                rc.right - rc.left,
                rc.bottom - rc.top,
                primary_dc,
                rc.left,
                rc.top,
                rc.right - rc.left,
                rc.bottom - rc.top,
                0
            );

            ReleaseDC(hwnd, hdc);
        }

        BOOL x = g_ddraw->ticks_limiter.use_blt_or_flip;

        DDBLTFX fx = { .dwFillColor = 0 };
        IDirectDrawSurface_Blt(This, NULL, NULL, NULL, DDBLT_COLORFILL, &fx);

        g_ddraw->ticks_limiter.use_blt_or_flip = x;
    }


    if ((This->caps & DDSCAPS_PRIMARYSURFACE) && g_ddraw && g_ddraw->render.run)
    {
        InterlockedExchange(&g_ddraw->render.surface_updated, TRUE);

        DWORD time = timeGetTime();

        if (!(This->flags & DDSD_BACKBUFFERCOUNT) ||
            (This->last_flip_tick + FLIP_REDRAW_TIMEOUT < time && This->last_blt_tick + FLIP_REDRAW_TIMEOUT < time))
        {
            ReleaseSemaphore(g_ddraw->render.sem, 1, NULL);

            if (g_ddraw->ticks_limiter.tick_length > 0 && !g_ddraw->ticks_limiter.use_blt_or_flip)
                util_limit_game_ticks();
        }
    }

    if (g_ddraw && g_ddraw->lock_surfaces)
        LeaveCriticalSection(&This->cs);

    return DD_OK;
}

HRESULT dds_GetDDInterface(IDirectDrawSurfaceImpl* This, LPVOID* lplpDD)
{
    if (!lplpDD)
        return DDERR_INVALIDPARAMS;

    *lplpDD = This->ddraw;
    IDirectDraw_AddRef(This->ddraw);

    return DD_OK;
}

HRESULT dds_SetSurfaceDesc(IDirectDrawSurfaceImpl* This, LPDDSURFACEDESC2 lpDDSD, DWORD dwFlags)
{
    dbg_dump_dds_flags(lpDDSD->dwFlags);
    dbg_dump_dds_caps(lpDDSD->ddsCaps.dwCaps);

    DWORD req_flags = DDSD_LPSURFACE | DDSD_PITCH | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;

    if ((lpDDSD->dwFlags & req_flags) != req_flags)
        return DDERR_UNSUPPORTED;


    if (This->bitmap)
    {
        DeleteObject(This->bitmap);
        This->bitmap = NULL;
    }
    else if (This->surface && !This->custom_buf)
    {
        HeapFree(GetProcessHeap(), 0, This->surface);
        This->surface = NULL;
    }

    if (This->hdc)
    {
        DeleteDC(This->hdc);
        This->hdc = NULL;
    }

    if (This->bmi)
    {
        HeapFree(GetProcessHeap(), 0, This->bmi);
        This->bmi = NULL;
    }

    if (This->mapping)
    {
        CloseHandle(This->mapping);
        This->mapping = NULL;
    }


    switch (lpDDSD->ddpfPixelFormat.dwRGBBitCount)
    {
    case 8:
        This->bpp = 8;
        break;
    case 15:
        TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSD->ddpfPixelFormat.dwRGBBitCount);
    case 16:
        This->bpp = 16;
        break;
    case 24:
        TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSD->ddpfPixelFormat.dwRGBBitCount);
    case 32:
        This->bpp = 32;
        break;
    default:
        This->bpp = 8;
        TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSD->ddpfPixelFormat.dwRGBBitCount);
        break;
    }

    This->width = lpDDSD->dwWidth;
    This->height = lpDDSD->dwHeight;
    This->surface = lpDDSD->lpSurface;
    This->pitch = lpDDSD->lPitch;
    This->bytes_pp = This->bpp / 8;
    This->size = This->pitch * This->height;
    This->custom_buf = TRUE;

    return DD_OK;
}

void* dds_GetBuffer(IDirectDrawSurfaceImpl* This)
{
    if (!This)
        return NULL;

    if (This->backbuffer || (This->caps & DDSCAPS_BACKBUFFER))
        return (void*)InterlockedExchangeAdd((LONG*)&This->surface, 0);

    return This->surface;
}

HRESULT dd_CreateSurface(
    IDirectDrawImpl* This,
    LPDDSURFACEDESC lpDDSurfaceDesc,
    IDirectDrawSurfaceImpl** lpDDSurface,
    IUnknown FAR* unkOuter)
{
    dbg_dump_dds_flags(lpDDSurfaceDesc->dwFlags);
    dbg_dump_dds_caps(lpDDSurfaceDesc->ddsCaps.dwCaps);

    if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        return DDERR_UNSUPPORTED;

    if ((lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
        g_ddraw->primary &&
        g_ddraw->primary->width == g_ddraw->width &&
        g_ddraw->primary->height == g_ddraw->height &&
        g_ddraw->primary->bpp == g_ddraw->bpp)
    {
        *lpDDSurface = g_ddraw->primary;
        IDirectDrawSurface_AddRef(g_ddraw->primary);

        return DD_OK;
    }

    IDirectDrawSurfaceImpl* dst_surface =
        (IDirectDrawSurfaceImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawSurfaceImpl));

    dst_surface->lpVtbl = &g_dds_vtbl;

    lpDDSurfaceDesc->dwFlags |= DDSD_CAPS;

    InitializeCriticalSection(&dst_surface->cs);

    dst_surface->bpp = g_ddraw->bpp == 0 ? 16 : g_ddraw->bpp;
    dst_surface->flags = lpDDSurfaceDesc->dwFlags;
    dst_surface->caps = lpDDSurfaceDesc->ddsCaps.dwCaps;
    dst_surface->ddraw = This;

    if (dst_surface->flags & DDSD_PIXELFORMAT)
    {
        switch (lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount)
        {
        case 8:
            dst_surface->bpp = 8;
            break;
        case 15:
            TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
        case 16:
            dst_surface->bpp = 16;
            break;
        case 24:
            TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
        case 32:
            dst_surface->bpp = 32;
            break;
        default:
            TRACE("     NOT_IMPLEMENTED bpp=%u\n", lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);
            break;
        }
    }

    if (dst_surface->caps & DDSCAPS_PRIMARYSURFACE)
    {
        dst_surface->width = g_ddraw->width;
        dst_surface->height = g_ddraw->height;
    }
    else
    {
        dst_surface->width = lpDDSurfaceDesc->dwWidth;
        dst_surface->height = lpDDSurfaceDesc->dwHeight;
    }

    if ((dst_surface->flags & DDSD_LPSURFACE) && (dst_surface->flags & DDSD_PITCH))
    {
        dst_surface->surface = lpDDSurfaceDesc->lpSurface;
        dst_surface->pitch = lpDDSurfaceDesc->lPitch;
        dst_surface->bytes_pp = dst_surface->bpp / 8;
        dst_surface->size = dst_surface->pitch * dst_surface->height;
        dst_surface->custom_buf = TRUE;
    }
    else if (dst_surface->width && dst_surface->height)
    {
        dst_surface->bytes_pp = dst_surface->bpp / 8;
        dst_surface->pitch = ((dst_surface->width * dst_surface->bpp + 31) & ~31) >> 3;
        dst_surface->size = dst_surface->pitch * dst_surface->height;

        DWORD aligned_width = dst_surface->pitch / dst_surface->bytes_pp;

        DWORD bmi_size = sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256;
        DWORD bmp_size = dst_surface->pitch * (dst_surface->height + g_ddraw->guard_lines);

        dst_surface->bmi = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bmi_size);
        dst_surface->bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        dst_surface->bmi->bmiHeader.biWidth = aligned_width;
        dst_surface->bmi->bmiHeader.biHeight = -((int)dst_surface->height + g_ddraw->guard_lines);
        dst_surface->bmi->bmiHeader.biPlanes = 1;
        dst_surface->bmi->bmiHeader.biBitCount = dst_surface->bpp;
        dst_surface->bmi->bmiHeader.biCompression = dst_surface->bpp == 8 ? BI_RGB : BI_BITFIELDS;

        WORD clr_bits = (WORD)(dst_surface->bmi->bmiHeader.biPlanes * dst_surface->bmi->bmiHeader.biBitCount);

        if (clr_bits < 24)
        {
            dst_surface->bmi->bmiHeader.biClrUsed = (1 << clr_bits);
        }

        dst_surface->bmi->bmiHeader.biSizeImage =
            ((aligned_width * clr_bits + 31) & ~31) / 8 * dst_surface->height;

        if (dst_surface->bpp == 8)
        {
            for (int i = 0; i < 256; i++)
            {
                dst_surface->bmi->bmiColors[i].rgbRed = i;
                dst_surface->bmi->bmiColors[i].rgbGreen = i;
                dst_surface->bmi->bmiColors[i].rgbBlue = i;
                dst_surface->bmi->bmiColors[i].rgbReserved = 0;
            }
        }
        else if (dst_surface->bpp == 16 && g_ddraw->rgb555)
        {
            ((DWORD*)dst_surface->bmi->bmiColors)[0] = 0x7C00;
            ((DWORD*)dst_surface->bmi->bmiColors)[1] = 0x03E0;
            ((DWORD*)dst_surface->bmi->bmiColors)[2] = 0x001F;
        }
        else if (dst_surface->bpp == 16)
        {
            ((DWORD*)dst_surface->bmi->bmiColors)[0] = 0xF800;
            ((DWORD*)dst_surface->bmi->bmiColors)[1] = 0x07E0;
            ((DWORD*)dst_surface->bmi->bmiColors)[2] = 0x001F;
        }
        else if (dst_surface->bpp == 32)
        {
            ((DWORD*)dst_surface->bmi->bmiColors)[0] = 0xFF0000;
            ((DWORD*)dst_surface->bmi->bmiColors)[1] = 0x00FF00;
            ((DWORD*)dst_surface->bmi->bmiColors)[2] = 0x0000FF;
        }

        dst_surface->hdc = CreateCompatibleDC(g_ddraw->render.hdc);

        dst_surface->mapping =
            CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE | SEC_COMMIT,
                0,
                bmp_size + 256,
                NULL);

        DWORD map_offset = 0;

        if (dst_surface->mapping)
        {
            LPVOID data = MapViewOfFile(dst_surface->mapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
            if (data)
            {
                while (((DWORD)data + map_offset) % 128) map_offset++;
                UnmapViewOfFile(data);
            }

            if (!data || (map_offset % sizeof(DWORD)))
            {
                map_offset = 0;
                CloseHandle(dst_surface->mapping);
                dst_surface->mapping = NULL;
            }
        }

        dst_surface->bitmap =
            CreateDIBSection(
                dst_surface->hdc,
                dst_surface->bmi,
                DIB_RGB_COLORS,
                (void**)&dst_surface->surface,
                dst_surface->mapping,
                map_offset);

        dst_surface->bmi->bmiHeader.biHeight = -((int)dst_surface->height);

        if (!dst_surface->bitmap)
        {
            dst_surface->surface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bmp_size);
        }

        if (dst_surface->caps & DDSCAPS_PRIMARYSURFACE)
        {
            g_ddraw->primary = dst_surface;
            FakePrimarySurface = dst_surface->surface;
        }

        SelectObject(dst_surface->hdc, dst_surface->bitmap);
    }

    if (dst_surface->flags & DDSD_BACKBUFFERCOUNT)
    {
        dst_surface->backbuffer_count = lpDDSurfaceDesc->dwBackBufferCount;

        TRACE("     dwBackBufferCount=%d\n", lpDDSurfaceDesc->dwBackBufferCount);

        DDSURFACEDESC desc;
        memset(&desc, 0, sizeof(desc));

        if (lpDDSurfaceDesc->dwBackBufferCount > 1)
        {
            desc.dwBackBufferCount = lpDDSurfaceDesc->dwBackBufferCount - 1;
            desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
        }

        desc.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER;

        desc.dwWidth = dst_surface->width;
        desc.dwHeight = dst_surface->height;

        dd_CreateSurface(This, &desc, &dst_surface->backbuffer, unkOuter);
    }

    TRACE(
        "     surface = %p (%ux%u@%u)\n",
        dst_surface,
        dst_surface->width,
        dst_surface->height,
        dst_surface->bpp);

    *lpDDSurface = dst_surface;

    dst_surface->ref = 0;
    IDirectDrawSurface_AddRef(dst_surface);

    return DD_OK;
}
