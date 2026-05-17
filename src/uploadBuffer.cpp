#include <cstring>

#include "uploadBuffer.h"

namespace Hydrogen
{
	UploadBuffer::~UploadBuffer()
	{
		if (m_pMapped && GetResource())
		{
			GetResource()->Unmap(0, nullptr);
			m_pMapped = nullptr;
		}
	}

	void UploadBuffer::Write(const void* pData, uint64 sizeInBytes, uint64 offsetInBytes)
	{
		std::memcpy(m_pMapped + offsetInBytes, pData, sizeInBytes);
	}
}
