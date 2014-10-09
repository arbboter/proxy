// HeapDataManager.cpp: implementation of the CHeapDataManager class.
//
//////////////////////////////////////////////////////////////////////

#include "NatCommon.h"
#include "NatHeapDataManager.h"

#include "NatDebug.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNatHeapDataManager::CNatHeapDataManager(int iEachMemorySize, int iHeapCount)
:
m_iEachMemorySize(iEachMemorySize),
m_iHeapCount(iHeapCount)
{
	PUB_InitLock(&m_lock);
	m_ppHeadPoints = new char*[iHeapCount];
	m_pData = new char[iEachMemorySize * iHeapCount];
	char *pTmp = m_pData;
	int i = 0;
	for(i = 0; i < iHeapCount; ++i)
	{		
		m_ppHeadPoints[i] = pTmp;
		pTmp += iEachMemorySize;
	}
	m_iUsed = 0;
}

CNatHeapDataManager::~CNatHeapDataManager()
{	
	delete[] m_pData;	
	delete[] m_ppHeadPoints;
	PUB_DestroyLock(&m_lock);
}

void* CNatHeapDataManager::GetMemory()
{
	CPUB_LockAction	FunctionLock(&m_lock);

	if (m_iUsed >= m_iHeapCount) 
	{
		return NULL;
	}

	return	m_ppHeadPoints[m_iUsed++];
}

void CNatHeapDataManager::ReleaseMemory(void *pData)
{
	CPUB_LockAction	FunctionLock(&m_lock);
	assert(m_iUsed > 0);
	--m_iUsed;
	m_ppHeadPoints[m_iUsed] = static_cast<char*>(pData);
}

int CNatHeapDataManager::GetHeapCount()
{
    return m_iHeapCount;
}

int CNatHeapDataManager::GetEachMemorySize()
{
    return m_iEachMemorySize;
}

////////////////////////////////////////////////////////////////////////////////
// class CHeapDataPointer implements

CNatHeapDataPointer* CNatHeapDataPointer::NewMemory( CNatHeapDataManager *manager )
{
    CNatHeapDataPointer *pointer = NULL;
    void* memory = manager->GetMemory();
    if (NULL != memory)
    {
        pointer = new CNatHeapDataPointer(manager, memory);
        if (NULL == pointer)
        {
            manager->ReleaseMemory(memory);
        }        
    }
    return pointer;    
}


CNatHeapDataPointer::CNatHeapDataPointer( CNatHeapDataManager* manager, void* memory )
:
m_manager(manager),
m_memory(memory),
m_dwShared(1),
m_length(0)
{
}

CNatHeapDataPointer::~CNatHeapDataPointer()
{
    m_manager->ReleaseMemory(m_memory);
}

void* CNatHeapDataPointer::GetMemory() const
{
    return m_memory;
}

int CNatHeapDataPointer::GetLength() const
{
    return m_length;
}

void CNatHeapDataPointer::SetLength(int iLen)
{
    assert(iLen <= GetMaxLength());
    m_length = iLen;
}

int CNatHeapDataPointer::GetMaxLength() const
{
    return m_manager->GetEachMemorySize();
}

unsigned long CNatHeapDataPointer::Add()
{    
    m_dataLock.Lock();
    ++m_dwShared;
    m_dataLock.UnLock();
    return m_dwShared;
}

unsigned long CNatHeapDataPointer::Dec()
{
    unsigned long dwCount = 0;
    m_dataLock.Lock();
    assert(0 < m_dwShared);
    m_dwShared--;
    dwCount = m_dwShared;
    m_dataLock.UnLock();

    if (0 == dwCount) 
    {
        delete this;
        return dwCount;
    }

    return dwCount;
}

