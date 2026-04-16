#include "Hook_CreateMove.hpp"

#include <CS2/SDK/SDK.hpp>
#include <CS2/SDK/Interface/IEngineToClient.hpp>
#include <CS2/SDK/Update/CCSGOInput.hpp>

#include <AndromedaClient/CAndromedaClient.hpp>

auto Hook_CreateMove( CCSGOInput* pCCSGOInput , uint32_t split_screen_index , char a3 ) -> bool
{
	auto pCUserCmd = pCCSGOInput->GetUserCmd( nullptr );

	GetAndromedaClient()->OnCreateMove( pCCSGOInput , pCUserCmd );

	return CreateMove_o( pCCSGOInput , split_screen_index , a3 );
}

auto Hook_MessageLite_SerializePartialToArray( google::protobuf::Message* pMsg , void* out_buffer , int size ) -> bool
{
	return MessageLite_SerializePartialToArray_o( pMsg , out_buffer , size );
}
