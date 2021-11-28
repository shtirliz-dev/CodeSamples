
#include "Reflection/Value.h"
#include "Reflection/MetaData.h"

RawPtr VDataCastHelper::CastPointer(const VMetaData* pMainMetaData, RawPtr pPointer, const VMetaData* pTargetType)
{
	return reinterpret_cast<RawPtr>(pMainMetaData->CastPointer(pPointer, pTargetType));
}