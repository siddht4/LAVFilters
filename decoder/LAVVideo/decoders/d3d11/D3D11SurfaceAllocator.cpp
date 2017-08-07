/*
*      Copyright (C) 2017 Hendrik Leppkes
*      http://www.1f0.de
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along
*  with this program; if not, write to the Free Software Foundation, Inc.,
*  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "stdafx.h"
#include "D3D11SurfaceAllocator.h"

extern "C" {
#include "libavutil/hwcontext.h"
}

CD3D11MediaSample::CD3D11MediaSample(CD3D11SurfaceAllocator *pAllocator, AVFrame *pFrame, HRESULT *phr)
  : CMediaSampleSideData(NAME("CD3D11MediaSample"), (CBaseAllocator*)pAllocator, phr, nullptr, 0)
  , m_pFrame(pFrame)
{
  ASSERT(m_pFrame && m_pFrame->format == AV_PIX_FMT_D3D11);
}

CD3D11MediaSample::~CD3D11MediaSample()
{
  av_frame_free(&m_pFrame);
}

//Note: CMediaSample does not derive from CUnknown, so we cannot use the
//		DECLARE_IUNKNOWN macro that is used by most of the filter classes.

STDMETHODIMP CD3D11MediaSample::QueryInterface(REFIID riid, __deref_out void **ppv)
{
  CheckPointer(ppv, E_POINTER);
  ValidateReadWritePtr(ppv, sizeof(PVOID));

  if (riid == __uuidof(IMediaSampleD3D11)) {
    return GetInterface((IMediaSampleD3D11 *) this, ppv);
  }
  else {
    return __super::QueryInterface(riid, ppv);
  }
}

STDMETHODIMP_(ULONG) CD3D11MediaSample::AddRef()
{
  return __super::AddRef();
}

STDMETHODIMP_(ULONG) CD3D11MediaSample::Release()
{
  // Return a temporary variable for thread safety.
  ULONG cRef = __super::Release();
  return cRef;
}

STDMETHODIMP CD3D11MediaSample::GetD3D11Texture(int nView, ID3D11Texture2D **ppTexture, UINT *pArraySlice)
{
  CheckPointer(ppTexture, E_POINTER);
  CheckPointer(pArraySlice, E_POINTER);
  
  // only view 0 is implemented at this time
  if (nView != 0)
    return E_INVALIDARG;

  if (m_pFrame)
  {
    *ppTexture   = (ID3D11Texture2D *)m_pFrame->data[0];
    *pArraySlice = (intptr_t)m_pFrame->data[1];

    return S_OK;
  }

  return E_FAIL;
}

CD3D11SurfaceAllocator::CD3D11SurfaceAllocator(HRESULT* phr)
  : CBaseAllocator(NAME("CD3D11SurfaceAllocator"), nullptr, phr)
{
}

CD3D11SurfaceAllocator::~CD3D11SurfaceAllocator()
{
}

STDMETHODIMP CD3D11SurfaceAllocator::GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime, REFERENCE_TIME * pEndTime, DWORD dwFlags)
{
  CheckPointer(ppBuffer, E_POINTER);
  
  AVFrame *hwFrame = av_frame_alloc();

  for (;;)
  {
    int ret = 0;
    
    // lock access to state variables
    {
      CAutoLock cObjectLock(this);

      if (!m_bCommitted)
      {
        av_frame_free(&hwFrame);
        return VFW_E_NOT_COMMITTED;
      }

      ASSERT(m_pFramesCtx);
      ret = av_hwframe_get_buffer(m_pFramesCtx, hwFrame, 0);
    }
    if (ret >= 0)
    {
      HRESULT hr = S_OK;
      *ppBuffer = dynamic_cast<CMediaSampleSideData*>(new CD3D11MediaSample(this, hwFrame, &hr));
      (*ppBuffer)->AddRef();
      
      if (FAILED(hr))
      {
        delete *ppBuffer;
        return hr;
      }

      // hold one reference on the allocator for every sample
      AddRef();
      return S_OK;
    }

    if (dwFlags & AM_GBF_NOWAIT)
    {
      av_frame_free(&hwFrame);
      return VFW_E_TIMEOUT;
    }

    Sleep(1);
  }

  av_frame_free(&hwFrame);
  return E_FAIL;
}

STDMETHODIMP CD3D11SurfaceAllocator::GetBufferForFrame(AVFrame *pFrame, IMediaSample **ppBuffer)
{
  HRESULT hr = S_OK;

  *ppBuffer = dynamic_cast<CMediaSampleSideData*>(new CD3D11MediaSample(this, pFrame, &hr));
  (*ppBuffer)->AddRef();

  if (FAILED(hr))
  {
    delete *ppBuffer;
    return hr;
  }

  // hold one reference on the allocator for every sample
  AddRef();
  return S_OK;
}

STDMETHODIMP CD3D11SurfaceAllocator::ReleaseBuffer(IMediaSample *pBuffer)
{
  CheckPointer(pBuffer, E_POINTER);
  
  // delete the media sample, its destructor will release the frame within
  delete pBuffer;
  
  // decrement reference on the allocator
  Release();

  return S_OK;
}

HRESULT CD3D11SurfaceAllocator::Alloc(void)
{
  CAutoLock cObjectLock(this);
  
  if (m_pFramesCtx)
    return S_OK;
  else
    return E_FAIL;
}

void CD3D11SurfaceAllocator::Free(void)
{
}

STDMETHODIMP CD3D11SurfaceAllocator::SetFramesContext(AVBufferRef *pFramesCtx)
{
  CAutoLock cObjectLock(this);

  if (m_bCommitted)
    return VFW_E_ALREADY_COMMITTED;

  if (m_pFramesCtx)
    av_buffer_unref(&m_pFramesCtx);

  if (pFramesCtx)
  {
    m_pFramesCtx = av_buffer_ref(pFramesCtx);
    if (m_pFramesCtx == nullptr)
      return E_FAIL;
  }

  return S_OK;
}
