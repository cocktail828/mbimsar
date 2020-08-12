#include "quectel_observer.h"
#include <assert.h>

MbnSubject::MbnSubject()
{
}
MbnSubject::~MbnSubject()
{
}
void MbnSubject::Attach(MbnObserver *observer)
{
	m_observers.push_back(observer);
}
void MbnSubject::Detach(MbnObserver *oldobserver)
{
	std::vector<MbnObserver *>::iterator iter = m_observers.begin();
	for (; iter != m_observers.end(); ++iter)
	{
		if ((*iter) == oldobserver)
		{
			m_observers.erase(iter);
			break;
		}
	}
}
void MbnSubject::Notify(void *notify)
{
	std::vector<MbnObserver *>::iterator iter = m_observers.begin();
	for (; iter != m_observers.end(); ++iter)
	{
		(*iter)->update(this, notify);
	}
}

// bool quectel_deresigter_sink(IConnectionPoint* cp, DWORD adviseHandle)
// {
// 	HRESULT hr;
// 	hr = cp->Unadvise(adviseHandle);
// 	if(FAILED(hr))
// 	{
// 		return false;
// 	}
// 	return true;
// }