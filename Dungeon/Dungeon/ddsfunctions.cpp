//
//	DDS file format module
//
//	Cube map is not supported now
//
#include "stdafx.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <D3D11.h>
#include <D2D1.h>
#include <ddraw.h>
#include <wincodec.h>
#include <stdint.h>
/**
*	DirectDraw.h の DDSURFACEDESC2 は、64bit コンパイラに対応していないので、
*   互換性のある構造体を用意
*/
struct DDS_HEADER
{
    DWORD        dwSize;
	DWORD        dwFlags;
	DWORD        dwHeight;
	DWORD        dwWidth;
	DWORD        pitchOrLinearSize;
	DWORD        dwDepth;		// only if DDS_HEADER_FLAGS_VOLUME is set in flags
	DWORD        dwMipMapCount;
	DWORD        dwAlphaBitDepth;  
	DWORD        reserved[10];	//  lpSurface と colorKey はファイルとは関係なさそうなので再現しない
	DDPIXELFORMAT ddpfPixelFormat;
	DWORD        caps;
	DWORD        caps2;
	DWORD        caps3;
	DWORD        caps4;
	DWORD        reserved2;	//	do not access this member(just padding)
};
//
//	macros
//
#undef SAFE_RELEASE
#undef SAFE_DELETE
#undef SAFE_DELETE_ARRAY
#define SAFE_RELEASE(o) if (o){ (o)->Release(); o = NULL; }
#define SAFE_DELETE(o)  if (o){ delete (o); o = NULL; }
#define SAFE_DELETE_ARRAY(o) if (o){ delete [] (o); o = NULL; }

//
//  prototypes.
//
DXGI_FORMAT	GetCompressedPixelFormatFromDdsHeader(DDS_HEADER *pddsc);

HRESULT CreateBitmapFromDdsLinearImage(ID2D1RenderTarget *pRenderTarget,DDS_HEADER *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID2D1Bitmap **ppBitmap);
HRESULT CreateBitmapFromDdsRgbImage(ID2D1RenderTarget *pRenderTarget, DDS_HEADER *pddsc, VOID *pBuffer, ID2D1Bitmap **ppBitmap);

HRESULT CreateTextureFromDdsRgbImage(ID3D11DeviceContext *pContext, DDS_HEADER *pddsc, VOID *pBuffer, DWORD dataSize, ID3D11Texture2D **ppTexture);
HRESULT CreateTextureFromDdsLinearImage(ID3D11DeviceContext *pContext, DDS_HEADER *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwDataSize, ID3D11Texture2D **ppTexture);

static HRESULT ConvertAndCopyPixelsFromDdsRgbImage(DDS_HEADER *pddsc, BYTE *pDest, DWORD destW, DWORD destH, DWORD destPitch, BYTE *pData, DWORD srcW, DWORD srcH, DWORD srcPitch);

/**
*	@brief function CreateTextureFromDdsFile
*   @param :
*   @param [in]  pContext  : Device's context
*	@param [in]  pFilename : pathname of the image file
*	@param [in]  ppTexture : Load result
*   @param [out] pSrcWidth : width of the source image  ( != texture width )
*   @param [out] pSrcHight : height of the source image ( != texture height )
*
*   @note  mipmap images are supported but cubemaps are not supported.
*/
HRESULT CreateTextureFromDdsFile(ID3D11DeviceContext *pContext, const TCHAR *pFilename, ID3D11Texture2D **ppTexture, DWORD *pSrcWidth, DWORD *pSrcHeight){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD	*pBuffer = NULL;
	ID3D11Texture2D *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	DDS_HEADER	*pddsc = NULL;
	DWORD		dwLinearSize = 0L;
	DXGI_FORMAT	texFormat = DXGI_FORMAT_UNKNOWN;
	DWORD datasize;
	struct _stat st;

	if (0 == _tstat(pFilename, &st)) {
		datasize = st.st_size;
		err = _tfopen_s(&fp, pFilename, _T("rb"));
	}

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),4,1,fp);
	if (*(DWORD*)header != 0x20534444L)	//  check FOURCC "DDS "
		goto ERROR_EXIT;
	datasize -= 4;

#ifdef _AMD64_
	idlength = sizeof(DDSURFACEDESC2);
	idlength = sizeof(DDS_HEADER);
#endif
	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength != 0x7c)
		goto ERROR_EXIT;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);
	datasize -= idlength;

	pddsc = (DDS_HEADER*)header;

	if (pddsc->dwFlags & DDSD_PITCH){
		dwLinearSize = (DWORD)pddsc->pitchOrLinearSize;
		dwLinearSize *= pddsc->dwHeight;
		pBuffer = new DWORD[(datasize+3) >> 2];
		fread((void*)pBuffer, 1, datasize, fp);
		hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,datasize,&pOutput);
		if (SUCCEEDED(hr)){
			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}else if (pddsc->dwFlags & DDSD_LINEARSIZE){
		dwLinearSize = (DWORD)pddsc->pitchOrLinearSize;
		pBuffer = new DWORD[(datasize + 3) >> 2];
		fread((void*)pBuffer, 1, datasize, fp);
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat == DXGI_FORMAT_UNKNOWN){
			hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,datasize,&pOutput);
			if (SUCCEEDED(hr)){
				*pSrcWidth = pddsc->dwWidth;
				*pSrcHeight = pddsc->dwHeight;
				*ppTexture = pOutput;
				pOutput = NULL;
			}
			goto ERROR_EXIT;
		}else{
			hr = CreateTextureFromDdsLinearImage(pContext,pddsc,texFormat,pBuffer,datasize,&pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}else if ((pddsc->dwFlags & DDSD_WIDTH) && (pddsc->dwFlags & DDSD_HEIGHT)){
		//  no pitch no linear size
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat != DXGI_FORMAT_UNKNOWN){

			//	compressed format
			switch(texFormat){
			case	DXGI_FORMAT_BC1_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				dwLinearSize = (dwLinearSize+1) >> 1;
				break;
			case	DXGI_FORMAT_BC2_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			case	DXGI_FORMAT_BC3_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			default:
				goto ERROR_EXIT;
			}
			pBuffer = new DWORD[(datasize+3) >> 2];
			fread((void*)pBuffer,1,datasize, fp);

			hr = CreateTextureFromDdsLinearImage(pContext,pddsc,texFormat,pBuffer,datasize,&pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*pSrcWidth = pddsc->dwWidth;
			*pSrcHeight = pddsc->dwHeight;
			*ppTexture = pOutput;
			pOutput = NULL;

		}else{

			//  uncompressed format

			DWORD pixelSize = pddsc->ddpfPixelFormat.dwRGBBitCount;
			pixelSize = (pixelSize + 7) >> 3;	//	byte size.

			dwLinearSize = pixelSize * pddsc->dwWidth * pddsc->dwHeight;
			pBuffer = new DWORD[(datasize + 3) >> 2];
			fread((void*)pBuffer, 1, datasize, fp);
			hr = CreateTextureFromDdsRgbImage(pContext,pddsc,pBuffer,datasize,&pOutput);
			if (SUCCEEDED(hr)){
				*pSrcWidth = pddsc->dwWidth;
				*pSrcHeight = pddsc->dwHeight;
				*ppTexture = pOutput;
				pOutput = NULL;
			}
		}
		
	}else
		goto ERROR_EXIT;	//  can't calculate the size


ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);
	
	SAFE_RELEASE(pOutput);
	SAFE_DELETE_ARRAY(pBuffer);

	return hr;
}

/**
 *	Create texture from DDS linear image
 *	@param [in] pContext Device's context
 *	@param [in]	pddsc	 pointer to the direct draw surface
 *	@param [in]	fmt      pixel format
 *  @param [in] pBuffer  image buffer
 *  @param [in] dwDataSize image data size
 *  @param [out] ppTexture result texture.
 *  @result  S_OK : succeeded / else failed
 */
HRESULT CreateTextureFromDdsLinearImage(ID3D11DeviceContext *pContext, DDS_HEADER *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwDataSize, ID3D11Texture2D **ppTexture) {
	HRESULT  hr = E_FAIL;
	D3D11_TEXTURE2D_DESC     td;
	ID3D11Device     *pDevice = NULL;
	ID3D11Texture2D  *pOutput = NULL;

	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
	td.Width = pddsc->dwWidth;
	td.Height = pddsc->dwHeight;
	td.MipLevels = (pddsc->dwFlags&DDSD_MIPMAPCOUNT) ? pddsc->dwMipMapCount : 1;
	td.ArraySize = 1;
	td.Format = fmt;
	td.SampleDesc.Count = 1;	//	MULTI SAMPLE COUNT
	td.SampleDesc.Quality = 0;	//	MULtI SAMPLE QUALITY
	td.Usage = D3D11_USAGE_DEFAULT;	//  Make it writeable
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = 0;// D3D11_CPU_ACCESS_WRITE;
	td.MiscFlags = 0;
	pContext->GetDevice(&pDevice);
	if (td.MipLevels == 0)
		td.MipLevels = 1;

	//  make room on stack with alloca not on heap.
	D3D11_SUBRESOURCE_DATA *pInitialData 
		= (D3D11_SUBRESOURCE_DATA*)alloca(td.MipLevels*sizeof(D3D11_SUBRESOURCE_DATA));
	uint8_t	*ptr;
	DWORD   w = td.Width, h = td.Height;
	DWORD   pitchsize, framesize, blocksize, restsize;
	DWORD	miplevel;
	ZeroMemory((VOID*)pInitialData, td.MipLevels * sizeof(D3D11_SUBRESOURCE_DATA));
	switch (fmt) {
	case	DXGI_FORMAT_BC1_UNORM:
		blocksize = 8;	break;
	case	DXGI_FORMAT_BC2_UNORM:
		blocksize = 16;	break;
	case	DXGI_FORMAT_BC3_UNORM:
		blocksize = 16;	break;
	}
	ptr = (uint8_t*)pBuffer;
	restsize = dwDataSize;
	for (miplevel = 0; miplevel < td.MipLevels; ++miplevel) {
		w = max(1, w);
		pitchsize = ((w + 3) >> 2) * blocksize;
		framesize = pitchsize * ((h + 3) >> 2);
		if (restsize < framesize)
			break;
		pInitialData[miplevel].pSysMem = ptr;
		pInitialData[miplevel].SysMemPitch = pitchsize;
		ptr += framesize;
		restsize -= framesize;
		w >>= 1;
		h >>= 1;
	}
	hr = E_FAIL;
	if (miplevel >= td.MipLevels) {
		hr = pDevice->CreateTexture2D(&td, pInitialData, &pOutput);
		if (SUCCEEDED(hr)) {
			*ppTexture = pOutput;
			pOutput = NULL;
		}
	}
	SAFE_RELEASE(pOutput);
	SAFE_RELEASE(pDevice);
	return hr;
}

/**
* @brief macro to convert and copy pixels 
* @note no lambda no inline functions no objects for better performance but macro
* @param [in] _convert  pixel converting functional macro
* @param [in] _psrc     the pointer which points source image
* @param [in] _sw       width of the source image
* @param [in] _sh       height of the source image
* @param [in] _sp       row pitch of the source image
* @param [ref] _pdest   the pointer which points destination image
* @param [in] _dw       width of the destinaton buffer
* @param [in] _dh       height of the destination buffer
* @param [in] _dp       pitch of the destination buffer
* @return               none
*/
//	pixel convert code macro no lambda nor objects for better performance
#define	_copyconvert(_convert,_psrc,_sw,_sh,_sp,_pdest,_dw,_dh,_dp)	{\
	int __x, __y;\
	BYTE *__pByte, *__pSrc;\
	int _r, _g, _b, _a;\
	USHORT _usPixel;\
	(void)_r;\
	(void)_g;\
	(void)_b;\
	(void)_a;\
	(void)_usPixel;\
	for (__y = 0; __y < (int)_sh; ++__y) {\
		__pByte = _pdest + (__y * _dp);\
		__pSrc = (BYTE*)_psrc + (__y * _sp);\
		for (__x = 0; __x < (int)_sw; ++__x) {\
			_convert\
			__pByte += sizeof(DWORD);\
		}\
		while (__x < (int)_dw) {\
			*(DWORD*)__pByte = 0L;\
			__pByte += sizeof(DWORD);\
			++__x;\
		}\
	}\
	while (__y < (int)_dh) {\
		__pByte = _pdest + (__y * _dp);\
		for (__x = 0; __x < (int)_dw; ++__x) {\
			*(DWORD*)(__pByte + sizeof(DWORD)*__x) = 0L;\
		}\
		++__y;\
	}\
}

#define	_convertA8R8G8B8	{*(DWORD*)__pByte = *(DWORD*)__pSrc; __pSrc += sizeof(DWORD);}
#define	_convertA8B8G8R8	{\
	_r = 0xff & (int)*__pSrc++;\
	_g = 0xff & (int)*__pSrc++;\
	_b = 0xff & (int)*__pSrc++;\
	_a = 0xff & (int)*__pSrc++;\
	*(DWORD*)__pByte = (_a << 24) + (_r << 16) + (_g << 8) + _b;\
}
#define	_convertR8G8B8	{\
	_b = 0xff & (int)*__pSrc++;\
	_g = 0xff & (int)*__pSrc++;\
	_r = 0xff & (int)*__pSrc++;\
	_a = 255;\
	*(DWORD*)__pByte = (_a << 24) + (_r << 16) + (_g << 8) + _b;\
}
#define	_convertA1R5G5B5	{\
	_usPixel = *(USHORT*)__pSrc;\
	_r = (((_usPixel >> 10) & 0x1f) * 255) / 31;\
	_g = (((_usPixel >> 5) & 0x1f) * 255) / 31;\
	_b = ((_usPixel & 0x1f) * 255) / 31;\
	_a = ((_usPixel >> 15) & 1) * 255;\
	*(DWORD*)__pByte = (_a << 24) + (_r << 16) + (_g << 8) + _b;\
	__pSrc += sizeof(USHORT);\
}
#define	_convertR5G6B5	{\
	_usPixel = *(USHORT*)__pSrc;\
	_r = (((_usPixel >> 11) & 0x1f) * 255) / 31;\
	_g = (((_usPixel >> 5) & 0x3f) * 255) / 63;\
	_b = ((_usPixel & 0x1f) * 255) / 31;\
	_a = 255;\
	*(DWORD*)__pByte = (_a << 24) + (_r << 16) + (_g << 8) + _b;\
	__pSrc += sizeof(USHORT);\
}

/**
* @brief convert and copy pixels from dds rgb image
* @param [in]  pddsc   DDS_HEADER of the source image
* @param [ref] pDest   destination buffer for the conversion
* @param [in]  destW   width of the destination buffer
* @param [in]  destH   height of the destination buffer
* @param [in]  destPitch pitch of the destination buffer
* @param [in]  pData   pointer to the source image
* @param [in]  srcW    width of the source image
* @param [in]  srcH    height of the source image
* @param [in]  srcPitch pitch of the source image
* @return  S_OK : succeeded / else if failed
*/
static HRESULT ConvertAndCopyPixelsFromDdsRgbImage(DDS_HEADER *pddsc, BYTE *pDest, DWORD destW, DWORD destH, DWORD destPitch, BYTE *pData, DWORD srcW, DWORD srcH, DWORD srcPitch) {

	INT bitCount = (INT)pddsc->ddpfPixelFormat.dwRGBBitCount;

	switch (bitCount) {
	case	32:
		//  A8R8G8B8  Just copy pixels
		if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)) {
			//	A8R8G8B8
			_copyconvert(_convertA8R8G8B8,pData,srcW,srcH,srcPitch, pDest,destW,destH,destPitch);
			return	S_OK;
		}
		else if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x000000ffL)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x00ff0000L)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)) {
			//	A8B8G8R8 -> A8R8G8B8
			_copyconvert(_convertA8B8G8R8, pData, srcW, srcH, srcPitch, pDest, destW, destH, destPitch);
			return	S_OK;
		}
		break;
	case	24:
		if ((pddsc->ddpfPixelFormat.dwRBitMask == 0xff0000L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x00ff00L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x0000ffL)
			) {
			//	R8G8B8 -> A8R8G8B8
			_copyconvert(_convertR8G8B8, pData, srcW, srcH, srcPitch, pDest, destW, destH, destPitch);
			return	S_OK;
		}
		break;
	case	16:
		if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x7c00L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x03e0L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
			&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0x8000L)
			) {
			//	A1R5G5B5 -> A8R8G8B8
			_copyconvert(_convertA1R5G5B5, pData, srcW, srcH, srcPitch, pDest, destW, destH, destPitch);
			return	S_OK;
		}
		else if ((pddsc->ddpfPixelFormat.dwRBitMask == 0xf800L)
			&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x07e0L)
			&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
			) {
			//	R5G6B5 -> A8R8G8B8
			_copyconvert(_convertR5G6B5, pData, srcW, srcH, srcPitch, pDest, destW, destH, destPitch);
			return	S_OK;
		}
	}
	return	E_FAIL;
}

/**
*	@brief function CreateTextureFromDdsRgbImage
*   @param
*   @param [in]  pContext   : Device's context
*	@param [in]	 pddsc      : direct draw surface desc
*	@param [in]	 pBuffer    : image data
*   @param [in]	 dwDataSize : size of the image data
*   @param [out] ppTexture  : Load result
*	Note:
*		Create texture and put image with uncompressed format.
*		converting pixel formats to DXGI_FORMAT_B8G8R8A8_UNORM
*/
HRESULT CreateTextureFromDdsRgbImage(ID3D11DeviceContext *pContext, DDS_HEADER *pddsc, VOID *pBuffer, DWORD dataSize, ID3D11Texture2D **ppTexture){
	D3D11_TEXTURE2D_DESC	td;
	DWORD texWidth = 4;
	DWORD texHeight = 4;
	HRESULT hr = E_FAIL;
	ID3D11Device *pDevice = NULL;
	ID3D11Texture2D *pOutput = NULL;

	if (0 != (pddsc->ddpfPixelFormat.dwFlags & DDPF_RGB)){
		while (texWidth < pddsc->dwWidth)
			texWidth <<= 1;

		while (texHeight < pddsc->dwHeight)
			texHeight <<= 1;

		ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
		td.Width = texWidth;
		td.Height = texHeight;
		td.MipLevels = (pddsc->dwFlags&DDSD_MIPMAPCOUNT) ? pddsc->dwMipMapCount : 1;
		td.ArraySize = 1;
		td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		td.SampleDesc.Count = 1;	//	MULTI SAMPLE COUNT
		td.SampleDesc.Quality = 0;	//	MULtI SAMPLE QUALITY
		td.Usage = D3D11_USAGE_DEFAULT;	//  Make it writeable
		td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		td.CPUAccessFlags = 0;	//	 D3D11_CPU_ACCESS_WRITE;
		td.MiscFlags = 0;
		pContext->GetDevice(&pDevice);

		//	make room for initial data
		td.MipLevels = max(1, td.MipLevels);
		D3D11_SUBRESOURCE_DATA *pInitialData
			= (D3D11_SUBRESOURCE_DATA*)alloca(td.MipLevels * sizeof(D3D11_SUBRESOURCE_DATA));

		DWORD srcPitch;
		DWORD srcWidth = pddsc->dwWidth;
		DWORD srcHeight = pddsc->dwHeight;
		DWORD srcFrameSize, srcRestSize;
		DWORD destPitch;
		BYTE  *pDest;
		DWORD miplevel;
		INT bitCount = (INT)pddsc->ddpfPixelFormat.dwRGBBitCount;

		ZeroMemory(pInitialData, td.MipLevels * sizeof(D3D11_SUBRESOURCE_DATA));

		if (pddsc->dwFlags & DDSD_PITCH) {
			srcPitch = (DWORD)pddsc->pitchOrLinearSize;
		} else {
			srcPitch = (DWORD)srcWidth * ((bitCount + 7) >> 3);
		}
		srcRestSize = dataSize;
		for (miplevel = 0; miplevel < td.MipLevels; ++miplevel) {
			srcFrameSize = srcPitch * srcHeight;

			if (srcFrameSize > srcRestSize)
				break;
			pDest = new BYTE[texWidth * texHeight * 4];	//	size in B8G8R8A8
			destPitch = texWidth * 4;
			pInitialData[miplevel].pSysMem = pDest;
			pInitialData[miplevel].SysMemPitch = destPitch;
			
			hr = ConvertAndCopyPixelsFromDdsRgbImage(pddsc, pDest, texWidth, texHeight, destPitch, (BYTE*)pBuffer, srcWidth, srcHeight, srcPitch);
			if (FAILED(hr))
				break;
			pBuffer = (uint8_t*)pBuffer + srcFrameSize;
			srcRestSize -= srcFrameSize;
			texWidth >>= 1;
			texHeight >>= 1;
			srcWidth >>= 1;
			srcHeight >>= 1;
			srcPitch = (DWORD)srcWidth * ((bitCount + 7) >> 3);
		}
		if (miplevel >= td.MipLevels) {
			hr = pDevice->CreateTexture2D(&td, pInitialData, &pOutput);
			if (SUCCEEDED(hr)) {
				*ppTexture = pOutput;
				pOutput = NULL;
			}
		}
		//	delete the image data on memory.
		for (miplevel = 0; miplevel < td.MipLevels; ++miplevel) {
			SAFE_DELETE(pInitialData[miplevel].pSysMem);
		}
	}
	SAFE_RELEASE(pOutput);
	SAFE_RELEASE(pDevice);
	return hr;
}

/**
*  @brief Get compressed pixel format from DDS Header
*  @param [in]	pddsc : Direct Draw Surface Desc
*  @return	DXGI_FORMAT
*  @Note if the format is uncompressed format returns DXGI_FORMAT_UNKNOWN
*/
DXGI_FORMAT	GetCompressedPixelFormatFromDdsHeader(DDS_HEADER *pddsc){
	DXGI_FORMAT	format = DXGI_FORMAT_UNKNOWN;
	if (pddsc->ddpfPixelFormat.dwFlags & DDPF_FOURCC){
		switch(pddsc->ddpfPixelFormat.dwFourCC){
		case	FOURCC_DXT1:
			format = DXGI_FORMAT_BC1_UNORM;
			break;
		case	FOURCC_DXT2:
		case	FOURCC_DXT3:
			format = DXGI_FORMAT_BC2_UNORM;
			break;
		case	FOURCC_DXT4:
		case	FOURCC_DXT5:
			format = DXGI_FORMAT_BC3_UNORM;
			break;
		}
	}
	return format;
}

/**
*	function CreateD2D1BitmapFromDdsFile
*   @param :
*   @param  [in]pRenderTarget : Direct2D's Render Target
*	@param  [in]pFilename     : pathname of the image file
*	@param  [out]ppTexture    : Load result
*   @param  [out]pSrcWidth    : width of the source image  ( != texture width )
*   @param  [out]pSrcHight    : height of the source image ( != texture height )
*/
HRESULT CreateD2D1BitmapFromDdsFile(ID2D1RenderTarget *pRenderTarget,const TCHAR *pFilename, ID2D1Bitmap **ppBitmap){
	FILE *fp = NULL;
	errno_t err;
	unsigned char header[256];
	DWORD	*pBuffer = NULL;
	ID2D1Bitmap *pOutput = NULL;
	HRESULT	hr = E_FAIL;
	int idlength;
	DDS_HEADER	*pddsc = NULL;
	DWORD		dwLinearSize = 0L;
	DXGI_FORMAT	texFormat = DXGI_FORMAT_UNKNOWN;

	err = _tfopen_s(&fp,pFilename,_T("rb"));

	if (fp== NULL)
		goto ERROR_EXIT;

	//  read the header;
	fread_s(header,sizeof(header),4,1,fp);
	if (*(DWORD*)header != 0x20534444L)	//  check FOURCC "DDS "
		goto ERROR_EXIT;

	fread_s(header,sizeof(header),1,1,fp);
	idlength = ((int)header[0]) & 255;
	if (idlength != 0x7c)
		goto ERROR_EXIT;
	fread_s(header+1,sizeof(header)-1,idlength-1,1,fp);

	pddsc = (DDS_HEADER*)header;

	if (pddsc->dwFlags & DDSD_PITCH){
		dwLinearSize = (DWORD)pddsc->pitchOrLinearSize;
		dwLinearSize *= pddsc->dwHeight;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
		if (SUCCEEDED(hr)){
			*ppBitmap = pOutput;
			pOutput = NULL;
		}
	}else if (pddsc->dwFlags & DDSD_LINEARSIZE){
		dwLinearSize = (DWORD)pddsc->pitchOrLinearSize;
		pBuffer = new DWORD[(dwLinearSize+3) >> 2];
		fread((void*)pBuffer,1,dwLinearSize, fp);
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat == DXGI_FORMAT_UNKNOWN){
			hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*ppBitmap = pOutput;
				pOutput = NULL;
			}
			goto ERROR_EXIT;
		}else{
			hr= CreateBitmapFromDdsLinearImage(pRenderTarget,pddsc, texFormat,pBuffer, dwLinearSize, &pOutput);
			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*ppBitmap = pOutput;
			pOutput = NULL;
		}
	}else if ((pddsc->dwFlags & DDSD_WIDTH) && (pddsc->dwFlags & DDSD_HEIGHT)){
		//  no pitch no linear size
		texFormat = GetCompressedPixelFormatFromDdsHeader(pddsc);
		if (texFormat != DXGI_FORMAT_UNKNOWN){
			//	compressed format
			switch(texFormat){
			case	DXGI_FORMAT_BC1_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				dwLinearSize = (dwLinearSize+1) >> 1;
				break;
			case	DXGI_FORMAT_BC2_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			case	DXGI_FORMAT_BC3_UNORM:
				dwLinearSize = pddsc->dwWidth * pddsc->dwHeight;
				break;
			default:
				goto ERROR_EXIT;
			}
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);
			hr= CreateBitmapFromDdsLinearImage(pRenderTarget,pddsc, texFormat,pBuffer, dwLinearSize, &pOutput);

			SAFE_DELETE_ARRAY(pBuffer);
			if (FAILED(hr))
				goto ERROR_EXIT;

			*ppBitmap = pOutput;
			pOutput = NULL;

		}else{

			//  uncompressed format

			DWORD pixelSize = pddsc->ddpfPixelFormat.dwRGBBitCount;
			pixelSize = (pixelSize + 7) >> 3;	//	byte size.

			dwLinearSize = pixelSize * pddsc->dwWidth * pddsc->dwHeight;
			pBuffer = new DWORD[(dwLinearSize+3) >> 2];
			fread((void*)pBuffer,1,dwLinearSize, fp);
			hr = CreateBitmapFromDdsRgbImage(pRenderTarget,pddsc,pBuffer,&pOutput);
			if (SUCCEEDED(hr)){
				*ppBitmap = pOutput;
				pOutput = NULL;
			}
		}
		
	}else
		goto ERROR_EXIT;	//  can't calculate the size

ERROR_EXIT:
	if (fp != NULL)
		fclose(fp);

	SAFE_RELEASE(pOutput);
	SAFE_DELETE_ARRAY(pBuffer);
	return hr;
}

/**
*  @brief Create bitmap from DDS linear image
*  @param [in]  pRenderTarget : Direct2D's Render Target
*  @param [in]	pddsc	      : pointer to the direct draw surface
*  @param [in]	fmt           : pixel format
*  @param [in]  pBuffer       : image buffer
*  @param [in]  dwLinearSize  : image data size
*  @param [out] ppBitmap      : result bitmap
*/
HRESULT CreateBitmapFromDdsLinearImage(ID2D1RenderTarget *pRenderTarget,DDS_HEADER *pddsc, DXGI_FORMAT fmt, VOID *pBuffer, DWORD dwLinearSize, ID2D1Bitmap **ppBitmap){
	HRESULT  hr = E_FAIL;
	ID2D1Bitmap *pOutput = NULL;
	D2D1_SIZE_U bitmapSize;
	D2D1_BITMAP_PROPERTIES bitmapProperties;
	BYTE *pDest = NULL;
	
	bitmapSize.width = pddsc->dwWidth;
	bitmapSize.height = pddsc->dwHeight;
	bitmapProperties.dpiX = 96.0f;
	bitmapProperties.dpiY = 96.0f;
	bitmapProperties.pixelFormat.format = fmt;
	bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_STRAIGHT;

	hr = pRenderTarget->CreateBitmap(bitmapSize,pBuffer,0,&bitmapProperties,&pOutput);
	if (SUCCEEDED(hr)){
		*ppBitmap = pOutput;
	}
	SAFE_RELEASE(pOutput);
	return hr;
}


/**
*  @brief function CreateBitmapFromDdsRgbImage
*  @param [in]  pRenderTarget : Direct2D's Render Target
*  @param [in]	pddsc         : Direct draw surface desc
*  @param [in]	pBuffer       : Image data
*  @param [out]	ppBitmap      : Load result
*  Note:
*  	Create bitmap and put image with uncompressed format.
*  	converting pixel formats to DXGI_FORMAT_B8G8R8A8_UNORM
*/
HRESULT CreateBitmapFromDdsRgbImage(ID2D1RenderTarget *pRenderTarget, DDS_HEADER *pddsc, VOID *pBuffer, ID2D1Bitmap **ppBitmap){
	HRESULT hr = E_FAIL;
	ID2D1Bitmap *pOutput = NULL;
	D2D1_SIZE_U bitmapSize;
	D2D1_BITMAP_PROPERTIES bitmapProperties;
	BYTE *pDest = NULL;
	INT bitCount;

	int r, g, b, a;
	DWORD srcPitch;
	DWORD srcWidth;
	DWORD srcHeight;
	DWORD destPitch;
	DWORD x, y;

	if (0 != (pddsc->ddpfPixelFormat.dwFlags & DDPF_RGB)) {

		bitmapSize.width = pddsc->dwWidth;
		bitmapSize.height = pddsc->dwHeight;
		bitmapProperties.dpiX = 96.0f;
		bitmapProperties.dpiY = 96.0f;
		bitmapProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		bitmapProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_STRAIGHT;

		bitCount = (INT)pddsc->ddpfPixelFormat.dwRGBBitCount;

		hr = E_FAIL;

		srcWidth = pddsc->dwWidth;
		srcHeight = pddsc->dwHeight;
		if (pddsc->dwFlags & DDSD_PITCH) {
			srcPitch = (DWORD)pddsc->pitchOrLinearSize;
		}
		else {
			//DWORD pixelByte = (bitCount+7)>>3;
			srcPitch = (DWORD)pddsc->dwWidth * ((bitCount + 7) >> 3);
		}
		pDest = (BYTE*)new DWORD[srcWidth*srcHeight];
		BYTE *pByte, *pSrc;
		destPitch = srcWidth * sizeof(DWORD);

		switch (bitCount) {
		case	32:
			//  A8R8G8B8  Just copy pixels
			if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x00ff0000L)
				&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
				&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x000000ffL)
				&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)) {
				//	A8R8G8B8
				hr = pRenderTarget->CreateBitmap(bitmapSize, pBuffer, srcPitch, &bitmapProperties, &pOutput);
			}
			else if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x000000ffL)
				&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x0000ff00L)
				&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x00ff0000L)
				&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0xff000000L)) {
				//	A8B8G8R8 -> A8R8G8B8
				for (y = 0; y < srcHeight; ++y) {
					pByte = pDest + (y * destPitch);
					pSrc = (BYTE*)pBuffer + (y * srcPitch);
					for (x = 0; x < srcWidth; ++x) {
						r = 0xff & (int)*pSrc++;
						g = 0xff & (int)*pSrc++;
						b = 0xff & (int)*pSrc++;
						a = 0xff & (int)*pSrc++;
						*(DWORD*)pByte = (a << 24) + (r << 16) + (g << 8) + b;
						pByte += sizeof(DWORD);
					}
				}
				hr = pRenderTarget->CreateBitmap(bitmapSize, pDest, destPitch, &bitmapProperties, &pOutput);
			}
			break;
		case	24:
			if ((pddsc->ddpfPixelFormat.dwRBitMask == 0xff0000L)
				&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x00ff00L)
				&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x0000ffL)
				) {
				//	R8G8B8 -> A8R8G8B8
				for (y = 0; y < srcHeight; ++y) {
					pByte = pDest + (y * destPitch);
					pSrc = (BYTE*)pBuffer + (y * srcPitch);
					for (x = 0; x < srcWidth; ++x) {
						b = 0xff & (int)*pSrc++;
						g = 0xff & (int)*pSrc++;
						r = 0xff & (int)*pSrc++;
						a = 255;
						*(DWORD*)pByte = (a << 24) + (r << 16) + (g << 8) + b;
						pByte += sizeof(DWORD);
					}
				}
				hr = pRenderTarget->CreateBitmap(bitmapSize, pDest, destPitch, &bitmapProperties, &pOutput);
			}
			break;
		case	16:
			if ((pddsc->ddpfPixelFormat.dwRBitMask == 0x7c00L)
				&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x03e0L)
				&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
				&& (pddsc->ddpfPixelFormat.dwRGBAlphaBitMask == 0x8000L)
				) {
				//	A1R5G5B5 -> A8R8G8B8
				USHORT usPixel;
				for (y = 0; y < srcHeight; ++y) {
					pByte = pDest + (y * destPitch);
					pSrc = (BYTE*)pBuffer + (y * srcPitch);
					for (x = 0; x < srcWidth; ++x) {
						usPixel = *(USHORT*)pSrc;
						r = (((usPixel >> 10) & 0x1f) * 255) / 31;
						g = (((usPixel >> 5) & 0x1f) * 255) / 31;
						b = ((usPixel & 0x1f) * 255) / 31;
						a = ((usPixel >> 15) & 1) * 255;

						*(DWORD*)pByte = (a << 24) + (r << 16) + (g << 8) + b;
						pSrc += sizeof(USHORT);
						pByte += sizeof(DWORD);
					}
				}
				hr = pRenderTarget->CreateBitmap(bitmapSize, pDest, destPitch, &bitmapProperties, &pOutput);
			}
			else if ((pddsc->ddpfPixelFormat.dwRBitMask == 0xf800L)
				&& (pddsc->ddpfPixelFormat.dwGBitMask == 0x07e0L)
				&& (pddsc->ddpfPixelFormat.dwBBitMask == 0x001fL)
				) {
				//	R5G6B5 -> A8R8G8B8
				USHORT usPixel;
				for (y = 0; y < srcHeight; ++y) {
					pByte = pDest + (y * destPitch);
					pSrc = (BYTE*)pBuffer + (y * srcPitch);
					for (x = 0; x < srcWidth; ++x) {
						usPixel = *(USHORT*)pSrc;
						r = (((usPixel >> 11) & 0x1f) * 255) / 31;
						g = (((usPixel >> 5) & 0x3f) * 255) / 63;
						b = ((usPixel & 0x1f) * 255) / 31;
						a = 255;

						*(DWORD*)pByte = (a << 24) + (r << 16) + (g << 8) + b;
						pSrc += sizeof(USHORT);
						pByte += sizeof(DWORD);
					}
				}
				hr = pRenderTarget->CreateBitmap(bitmapSize, pDest, destPitch, &bitmapProperties, &pOutput);
			}
		}
		if (SUCCEEDED(hr)) {
			*ppBitmap = pOutput;
			pOutput = NULL;
		}
	}
	SAFE_DELETE_ARRAY(pDest);
	SAFE_RELEASE(pOutput);
	return hr;
}

