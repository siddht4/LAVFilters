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

#pragma once

#include <d3d11.h>

#include "MediaSampleSideData.h"
#include "ID3DVideoMemoryConfiguration.h"

class CD3D11MediaSample : public CMediaSampleSideData, public IMediaSampleD3D11
{
  friend class CD3D11SurfaceAllocator;

public:
  CD3D11MediaSample(CD3D11SurfaceAllocator *pAllocator, AVFrame *pFrame, HRESULT *phr);
  virtual ~CD3D11MediaSample();

  // IUnknown
  STDMETHODIMP          QueryInterface(REFIID riid, __deref_out void **ppv);
  STDMETHODIMP_(ULONG)  AddRef();
  STDMETHODIMP_(ULONG)  Release();

  // IMediaSample
  STDMETHODIMP GetPointer(BYTE **ppBuffer) { return E_NOTIMPL; }

  // IMediaSampleD3D11
  STDMETHODIMP GetD3D11Texture(int nView, ID3D11Texture2D **ppTexture, UINT *pArraySlice);

private:
  AVFrame *m_pFrame = nullptr;
};


class CD3D11SurfaceAllocator : public CBaseAllocator
{
public:
  CD3D11SurfaceAllocator(HRESULT* phr);
  virtual ~CD3D11SurfaceAllocator();

  // CBaseAllocator overrides
  STDMETHODIMP GetBuffer(IMediaSample **ppBuffer, REFERENCE_TIME * pStartTime, REFERENCE_TIME * pEndTime, DWORD dwFlags);
  STDMETHODIMP ReleaseBuffer(IMediaSample *pBuffer);
  STDMETHODIMP SetNotify(IMemAllocatorNotifyCallbackTemp *pNotify) { return E_NOTIMPL; }
  STDMETHODIMP GetFreeCount(LONG *plBuffersFree) { return E_NOTIMPL; }

  // LAV interface
  STDMETHODIMP GetBufferForFrame(AVFrame *pFrame, IMediaSample **ppBuffer);
  STDMETHODIMP SetFramesContext(AVBufferRef *pFramesCtx);

protected:
  virtual void Free(void);
  virtual HRESULT Alloc(void);

private:
  AVBufferRef *m_pFramesCtx = nullptr;
};
