// -----------------------------------------------------------------
// ID3DVideoMemoryConfiguration interface and data structure definitions
// -----------------------------------------------------------------
#pragma once

// Available surface types
typedef enum _D3DSurfaceType
{
  D3D_SurfaceType_D3D11Texture = 0, //< ID3D11Texture2D object
} D3D_SurfaceType;

// -----------------------------------------------------------------
// Control the surface type between decoder and renderer
// -----------------------------------------------------------------
// A video decoder can notify the renderer about the type of surface
// the decoder is going to deliver. The surface type can only be set
// before the allocator is set, ie. during CompleteConnect or similar.
//
// To switch the surface type (or switch back to system memory buffers)
// the decoder/renderer need to re-connect and select a new allocator.
//
// To facilitate dynamic switching of the adapter used for decoding, the
// renderer should disconnect the decoder and re-connect it. At that
// point the decoder should query GetD3D11AdapterIndex() again and
// create a new decoder on the new device, as appropriate.
interface __declspec(uuid("7C374890-2529-4F16-B652-2E67F5BC3742")) ID3DVideoMemoryConfiguration : public IUnknown
{
  // Set the surface format the decoder is going to send.
  // If the renderer is not ready to accept this format, an error will be returned.
  virtual HRESULT STDMETHODCALLTYPE SetD3DSurfaceType(D3D_SurfaceType dwType) = 0;

  // Get the currently preferred D3D11 adapter index (to be used with IDXGIFactory1::EnumAdapters1)
  virtual UINT STDMETHODCALLTYPE GetD3D11AdapterIndex() = 0;
};

// -----------------------------------------------------------------
// Media Sample to hold a D3D11 texture
// -----------------------------------------------------------------
// D3D11 textures used for decoding are typically array-textures,
// a single ID3D11Texture2D object containing an array of textures
// individually addressable by the ArraySlice index.
//
// The ID3D11Texture2D object returned from this function has to be
// released when it is no longer needed.
//
// The texture lifetime is bound to the media samples lifetime. The
// media sample can only be released when the texture is no longer in
// use, otherwise the texture will be re-used by the decoder.
interface __declspec(uuid("BC8753F5-0AC8-4806-8E5F-A12B2AFE153E")) IMediaSampleD3D11 : public IUnknown
{
  // Get the D3D11 texture for the specified view.
  // 2D images with only one view always use view 0. For 3D, view 0 specifies the base view, view 1 the extension view.
  virtual HRESULT STDMETHODCALLTYPE GetD3D11Texture(int nView, ID3D11Texture2D **ppTexture, UINT *pArraySlice) = 0;
};
