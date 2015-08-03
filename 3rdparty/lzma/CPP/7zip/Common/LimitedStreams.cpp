// LimitedStreams.cpp

#include "StdAfx.h"

#include "LimitedStreams.h"
#include "../../Common/Defs.h"

STDMETHODIMP CLimitedSequentialInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 realProcessedSize = 0;
  UInt32 sizeToRead = (UInt32)MyMin((_size - _pos), (UInt64)size);
  HRESULT result = S_OK;
  if (sizeToRead > 0)
  {
    result = _stream->Read(data, sizeToRead, &realProcessedSize);
    _pos += realProcessedSize;
    if (realProcessedSize == 0)
      _wasFinished = true;
  }
  if (processedSize)
    *processedSize = realProcessedSize;
  return result;
}

STDMETHODIMP CLimitedInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
  {
    // 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
    return S_OK;
    // return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
  }
  UInt64 rem = _size - _virtPos;
  if (rem < size)
    size = (UInt32)rem;
  UInt64 newPos = _startOffset + _virtPos;
  if (newPos != _physPos)
  {
    _physPos = newPos;
    RINOK(SeekToPhys());
  }
  HRESULT res = _stream->Read(data, size, &size);
  if (processedSize)
    *processedSize = size;
  _physPos += size;
  _virtPos += size;
  return res;
}

STDMETHODIMP CLimitedInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

HRESULT CreateLimitedInStream(IInStream *inStream, UInt64 pos, UInt64 size, ISequentialInStream **resStream)
{
  *resStream = 0;
  CLimitedInStream *streamSpec = new CLimitedInStream;
  CMyComPtr<ISequentialInStream> streamTemp = streamSpec;
  streamSpec->SetStream(inStream);
  RINOK(streamSpec->InitAndSeek(pos, size));
  streamSpec->SeekToStart();
  *resStream = streamTemp.Detach();
  return S_OK;
}

STDMETHODIMP CClusterInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Size)
    return S_OK;

  if (_curRem == 0)
  {
    UInt32 blockSize = (UInt32)1 << BlockSizeLog;
    UInt32 virtBlock = (UInt32)(_virtPos >> BlockSizeLog);
    UInt32 offsetInBlock = (UInt32)_virtPos & (blockSize - 1);
    UInt32 phyBlock = Vector[virtBlock];
    UInt64 newPos = StartOffset + ((UInt64)phyBlock << BlockSizeLog) + offsetInBlock;
    if (newPos != _physPos)
    {
      _physPos = newPos;
      RINOK(SeekToPhys());
    }
    _curRem = blockSize - offsetInBlock;
    for (int i = 1; i < 64 && (virtBlock + i) < (UInt32)Vector.Size() && phyBlock + i == Vector[virtBlock + i]; i++)
      _curRem += (UInt32)1 << BlockSizeLog;
    UInt64 rem = Size - _virtPos;
    if (_curRem > rem)
      _curRem = (UInt32)rem;
  }
  if (size > _curRem)
    size = _curRem;
  HRESULT res = Stream->Read(data, size, &size);
  if (processedSize)
    *processedSize = size;
  _physPos += size;
  _virtPos += size;
  _curRem -= size;
  return res;
}
 
STDMETHODIMP CClusterInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += Size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  if (_virtPos != (UInt64)offset)
    _curRem = 0;
  _virtPos = offset;
  if (newPosition)
    *newPosition = offset;
  return S_OK;
}


STDMETHODIMP CExtentsStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= Extents.Back().Virt)
    return S_OK;
  if (size == 0)
    return S_OK;
  
  unsigned left = 0, right = Extents.Size() - 1;
  for (;;)
  {
    unsigned mid = (left + right) / 2;
    if (mid == left)
      break;
    if (_virtPos < Extents[mid].Virt)
      right = mid;
    else
      left = mid;
  }
  
  const CSeekExtent &extent = Extents[left];
  UInt64 phyPos = extent.Phy + (_virtPos - extent.Virt);
  if (_needStartSeek || _phyPos != phyPos)
  {
    _needStartSeek = false;
    _phyPos = phyPos;
    RINOK(SeekToPhys());
  }
  
  UInt64 rem = Extents[left + 1].Virt - _virtPos;
  if (size > rem)
    size = (UInt32)rem;
  
  HRESULT res = Stream->Read(data, size, &size);
  _phyPos += size;
  _virtPos += size;
  if (processedSize)
    *processedSize = size;
  return res;
}

STDMETHODIMP CExtentsStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += Extents.Back().Virt; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}


STDMETHODIMP CLimitedSequentialOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  HRESULT result = S_OK;
  if (processedSize)
    *processedSize = 0;
  if (size > _size)
  {
    if (_size == 0)
    {
      _overflow = true;
      if (!_overflowIsAllowed)
        return E_FAIL;
      if (processedSize)
        *processedSize = size;
      return S_OK;
    }
    size = (UInt32)_size;
  }
  if (_stream)
    result = _stream->Write(data, size, &size);
  _size -= size;
  if (processedSize)
    *processedSize = size;
  return result;
}


STDMETHODIMP CTailInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 cur;
  HRESULT res = Stream->Read(data, size, &cur);
  if (processedSize)
    *processedSize = cur;
  _virtPos += cur;
  return res;
}
  
STDMETHODIMP CTailInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END:
    {
      UInt64 pos = 0;
      RINOK(Stream->Seek(offset, STREAM_SEEK_END, &pos));
      if (pos < Offset)
        return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
      _virtPos = pos - Offset;
      if (newPosition)
        *newPosition = _virtPos;
      return S_OK;
    }
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = offset;
  if (newPosition)
    *newPosition = _virtPos;
  return Stream->Seek(Offset + _virtPos, STREAM_SEEK_SET, NULL);
}

STDMETHODIMP CLimitedCachedInStream::Read(void *data, UInt32 size, UInt32 *processedSize)
{
  if (processedSize)
    *processedSize = 0;
  if (_virtPos >= _size)
  {
    // 9.31: Fixed. Windows doesn't return error in ReadFile and IStream->Read in that case.
    return S_OK;
    // return (_virtPos == _size) ? S_OK: E_FAIL; // ERROR_HANDLE_EOF
  }
  UInt64 rem = _size - _virtPos;
  if (rem < size)
    size = (UInt32)rem;

  UInt64 newPos = _startOffset + _virtPos;
  UInt64 offsetInCache = newPos - _cachePhyPos;
  HRESULT res = S_OK;
  if (newPos >= _cachePhyPos &&
      offsetInCache <= _cacheSize &&
      size <= _cacheSize - (size_t)offsetInCache)
  {
    if (size != 0)
      memcpy(data, _cache + (size_t)offsetInCache, size);
  }
  else
  {
    if (newPos != _physPos)
    {
      _physPos = newPos;
      RINOK(SeekToPhys());
    }
    res = _stream->Read(data, size, &size);
    _physPos += size;
  }
  if (processedSize)
    *processedSize = size;
  _virtPos += size;
  return res;
}

STDMETHODIMP CLimitedCachedInStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _size; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = offset;
  if (newPosition)
    *newPosition = _virtPos;
  return S_OK;
}

STDMETHODIMP CTailOutStream::Write(const void *data, UInt32 size, UInt32 *processedSize)
{
  UInt32 cur;
  HRESULT res = Stream->Write(data, size, &cur);
  if (processedSize)
    *processedSize = cur;
  _virtPos += cur;
  if (_virtSize < _virtPos)
    _virtSize = _virtPos;
  return res;
}
  
STDMETHODIMP CTailOutStream::Seek(Int64 offset, UInt32 seekOrigin, UInt64 *newPosition)
{
  switch (seekOrigin)
  {
    case STREAM_SEEK_SET: break;
    case STREAM_SEEK_CUR: offset += _virtPos; break;
    case STREAM_SEEK_END: offset += _virtSize; break;
    default: return STG_E_INVALIDFUNCTION;
  }
  if (offset < 0)
    return HRESULT_WIN32_ERROR_NEGATIVE_SEEK;
  _virtPos = offset;
  if (newPosition)
    *newPosition = _virtPos;
  return Stream->Seek(Offset + _virtPos, STREAM_SEEK_SET, NULL);
}

STDMETHODIMP CTailOutStream::SetSize(UInt64 newSize)
{
  _virtSize = newSize;
  return Stream->SetSize(Offset + newSize);
}