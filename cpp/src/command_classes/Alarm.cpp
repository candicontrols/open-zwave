//-----------------------------------------------------------------------------
//
//	Alarm.cpp
//
//	Implementation of the Z-Wave COMMAND_CLASS_ALARM
//
//	Copyright (c) 2010 Mal Lansell <openzwave@lansell.org>
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------

#include "command_classes/CommandClasses.h"
#include "command_classes/Alarm.h"
#include "Defs.h"
#include "Msg.h"
#include "Node.h"
#include "Driver.h"
#include "platform/Log.h"

#include "value_classes/ValueByte.h"
#include "ValueBool.h"
#include "ValueString.h"

using namespace OpenZWave;

enum AlarmCmd
{
	AlarmCmd_Get	= 0x04,
	AlarmCmd_Report = 0x05,
	// Version 2
	AlarmCmd_Set = 0x06,
	AlarmCmd_SupportedGet = 0x07,
	AlarmCmd_SupportedReport = 0x08
};


/**
 * Kwikset handling
 */

#if 0
enum
{
	AlarmIndex_Type = 0,
	AlarmIndex_Level = 1,
  AlarmIndex_LockState = 2
};


/** 
 * Kwikset Alarm:
 * 
 * Type     Level
 * 0x15 (21)      1   Manual Unlock
 * 0x16 (22)      1   Manual Lock
 * 0x17 (23)      1   Remote Lock/Unlock
 * 0x11 (17)      1   User 1 Lock/Unlock
 * 0x11 (17)      2   User 1 Lock/Unlock
 * 
 * 
 * From spec:
 
 
Alarm Type | Alarm Level 
 “xxx”         "zzz"  

  021           001									Lock Secured using Keyed cylinder or inside thumb-turn            
  022           001									Lock Un-Secured using Keyed cylinder or inside thumb-turn
  026           001									Lock Auto Secured – Bolt Jammed (Not fully extended)
  027           001									Lock Auto Secured – Successful (Fully extended)
  017           001									Lock Secured at Keypad – Bolt Jammed (Not fully extended)
  018           000 or User-ID#*		Lock Secured at Keypad – Successful (Fully extended)
  019           User-ID#*						Lock Un-Secured by User (User-ID) at Keypad
  023           001									Lock Secured by Controller – Bolt Jammed (Not fully extended)
  024           001									Lock Secured by Controller – Successful (Fully extended)
  025           001									Lock Un-Secured by Controller – Successful (Fully retracted)
  112           User-ID#*						New User Code (User-ID#) added to the lock
  032           001									All User Codes deleted from lock
  161           001									Failed User Code attempt at Keypad
  162           User-ID#*						Attempted access by user (User-ID#) outside of scheduled
  167           001									Low battery level
  168           001									Critical battery level
  169           001									Battery level too low to operate lock

 */


enum AccessControl 
{
  AccessControl_ManualLockOperation   = 0x01, 
  AccessControl_ManualUnlockOperation = 0x02, 
  AccessControl_RFLockOperation       = 0x03,
  AccessControl_RFUnlockOperation     = 0x04,
  AccessControl_KeypadLockOperation   = 0x05,
  AccessControl_KeypadUnlockOperation = 0x06
};
  

enum SecuredState
{
  Alarm_LockSecuredKeypad             = 17,
  Alarm_LockSecuredManual             = 21,
  Alarm_LockUnsecuredManual           = 22,
  Alarm_LockSecuredControllerJammed   = 23,
  Alarm_LockSecuredController         = 24,
  Alarm_LockUnsecuredController       = 25
};


// Offset of 17 
static char const* c_lockStates[] =
{
  "Secured at Keypad - Jammed",
  "Secured at Keypad - Success",
  "Unsecured at Keypad",
  "Unknown",
  "Secured Manually",
  "Unsecured Manually",
  "Secured by Controller - Jammed",
  "Secured by Controller",
  "Unsecured by Controller"
};


#endif



enum
{
	AlarmIndex_Type = 0,
	AlarmIndex_Level,
	AlarmIndex_SourceNodeId
};

enum
{
	Alarm_General = 0,
	Alarm_Smoke,
	Alarm_CarbonMonoxide,
	Alarm_CarbonDioxide,
	Alarm_Heat,
	Alarm_Flood,
	Alarm_Access_Control,
	Alarm_Burglar,
	Alarm_Power_Management,
	Alarm_System,
	Alarm_Emergency,
	Alarm_Clock,
	Alarm_Appliance,
	Alarm_HomeHealth,
	Alarm_Count
};

static char const* c_alarmTypeName[] =
{
		"General",
		"Smoke",
		"Carbon Monoxide",
		"Carbon Dioxide",
		"Heat",
		"Flood",
		"Access Control",
		"Burglar",
		"Power Management",
		"System",
		"Emergency",
		"Clock",
		"Appliance",
		"HomeHealth"
};

//-----------------------------------------------------------------------------
// <WakeUp::WakeUp>
// Constructor
//-----------------------------------------------------------------------------
Alarm::Alarm
(
		uint32 const _homeId,
		uint8 const _nodeId
):
CommandClass( _homeId, _nodeId )
{
	SetStaticRequest( StaticRequest_Values );
}


//-----------------------------------------------------------------------------
// <Alarm::RequestState>
// Request current state from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestState
(
		uint32 const _requestFlags,
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( ( _requestFlags & RequestFlag_Static ) && HasStaticRequest( StaticRequest_Values ) )
	{
		if( GetVersion() > 1 )
		{
			// Request the supported alarm types
			Msg* msg = new Msg( "AlarmCmd_SupportedGet", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_SupportedGet );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
	}

	if( _requestFlags & RequestFlag_Dynamic )
	{
		return RequestValue( _requestFlags, 0, _instance, _queue );
	}

	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::RequestValue>
// Request current value from the device
//-----------------------------------------------------------------------------
bool Alarm::RequestValue
(
		uint32 const _requestFlags,
		uint8 const _dummy1,	// = 0 (not used)
		uint8 const _instance,
		Driver::MsgQueue const _queue
)
{
	if( IsGetSupported() )
	{
		if( GetVersion() == 1 )
		{
			Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
			msg->SetInstance( this, _instance );
			msg->Append( GetNodeId() );
			msg->Append( 2 );
			msg->Append( GetCommandClassId() );
			msg->Append( AlarmCmd_Get );
			msg->Append( GetDriver()->GetTransmitOptions() );
			GetDriver()->SendMsg( msg, _queue );
			return true;
		}
		else
		{
			bool res = false;
			for( uint8 i = 0; i < Alarm_Count; i++ )
			{
				if( Value* value = GetValue( _instance, i + 3 ) ) {
					value->Release();
					Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
					msg->SetInstance( this, _instance );
					msg->Append( GetNodeId() );
					msg->Append( GetVersion() == 2 ? 4 : 5);
					msg->Append( GetCommandClassId() );
					msg->Append( AlarmCmd_Get );
					msg->Append( 0x00); // ? proprietary alarm ?
					msg->Append( i );
					if( GetVersion() > 2 )
						msg->Append(0x01); //get first event of type.
					msg->Append( GetDriver()->GetTransmitOptions() );
					GetDriver()->SendMsg( msg, _queue );
					res = true;
				}
			}
			return res;
		}
	} else {
		Log::Write(  LogLevel_Info, GetNodeId(), "AlarmCmd_Get Not Supported on this node");
	}
	return false;
}

//-----------------------------------------------------------------------------
// <Alarm::HandleMsg>
// Handle a message from the Z-Wave network
//-----------------------------------------------------------------------------
bool Alarm::HandleMsg
(
		uint8 const* _data,
		uint32 const _length,
		uint32 const _instance	// = 1
)
{
	if (AlarmCmd_Report == (AlarmCmd)_data[0])
	{
		// We have received a report from the Z-Wave device
		if( GetVersion() == 1 )
		{
			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d", _data[1], _data[2] );
		}
		else
		{
			string alarm_type =  ( _data[5] < Alarm_Count ) ? c_alarmTypeName[_data[5]] : "Unknown type";

			Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d, sensorSrcID=%d, type:%s event:%d, status=%d",
							_data[1], _data[2], _data[3], alarm_type.c_str(), _data[6], _data[4] );
		}

		ValueByte* value;
		if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) )) )
		{
			value->OnValueRefreshed( _data[1] );
			value->Release();
		}
		if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) )) )
		{
			value->OnValueRefreshed( _data[2] );
			value->Release();
		}

		// With Version=2, the data has more detailed information about the alarm
		if(( GetVersion() > 1 ) && ( _length >= 7  ))
		{
			if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_SourceNodeId ) )) )
			{
				value->OnValueRefreshed( _data[3] );
				value->Release();
			}

			if( (value = static_cast<ValueByte*>( GetValue( _instance, _data[5]+3 ) )) )
			{
				value->OnValueRefreshed( _data[6] );
				value->Release();
			}
		}

		return true;
	}

	if( AlarmCmd_SupportedReport == (AlarmCmd)_data[0] )
	{
		if( Node* node = GetNodeUnsafe() )
		{
			// We have received the supported alarm types from the Z-Wave device
			Log::Write( LogLevel_Info, GetNodeId(), "Received supported alarm types" );

			node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_SourceNodeId, "SourceNodeId", "", true, false, 0, 0 );
			Log::Write( LogLevel_Info, GetNodeId(), "    Added alarm SourceNodeId" );

			// Parse the data for the supported alarm types
			uint8 numBytes = _data[1];
			for( uint32 i=0; i<numBytes; ++i )
			{
				for( int32 bit=0; bit<8; ++bit )
				{
					if( ( _data[i+2] & (1<<bit) ) != 0 )
					{
						int32 index = (int32)(i<<3) + bit;
						if( index < Alarm_Count )
						{
							node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, index+3, c_alarmTypeName[index], "", true, false, 0, 0 );
							Log::Write( LogLevel_Info, GetNodeId(), "    Added alarm type: %s", c_alarmTypeName[index] );
						} else {
							Log::Write( LogLevel_Info, GetNodeId(), "    Unknown alarm type: %d", index );
						}
					}
				}
			}
		}

		ClearStaticRequest( StaticRequest_Values );
		return true;
	}

	return false;
}



#if 0
//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------

// Kwikset handling

void Alarm::CreateVars
(
	uint8 const _instance
)
{
	if( Node* node = GetNodeUnsafe() )
	{
		//uint8 alarm = 6;
	 	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
    node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
	 	//node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, alarm, c_alarmTypes[alarm], "", true, false, 0, 0 );
		//node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, , "Alarm Query", "", true, false, 0, 0 );
    node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_LockState, "Lock State", "", true, false, "Unknown", 0 );
	}
}

#endif

//-----------------------------------------------------------------------------
// <Alarm::CreateVars>
// Create the values managed by this command class
//-----------------------------------------------------------------------------
void Alarm::CreateVars
(
                uint8 const _instance
)
{
        if( Node* node = GetNodeUnsafe() )
        {
                node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
                node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
        }
}




