#include "CPlayer_ItemServices.h"

CEconItemView::CEconItemView( void )
{
	m_iItemDefinitionIndex() = 0;
	m_iEntityQuality() = -1;
	m_iEntityLevel() = 0;
	m_iItemID() = 0;
	m_iItemIDLow() = 0;
	m_iItemIDHigh() = 0xFFFFFFFF;
	m_iInventoryPosition() = 0;
	m_bInitialized() = false;
	m_iAccountID() = 0;
}