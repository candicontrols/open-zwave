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
	AlarmCmd_Set = 0x06,
	AlarmCmd_SupportedGet = 0x07,
	AlarmCmd_SupportedReport = 0x08
};

enum
{
	AlarmIndex_Type = 0,
	AlarmIndex_Level = 1,
  AlarmIndex_LockState = 2
};


/** 
 * Kwik set Alarm:
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


/*static char const* c_alarmTypes[] =
{
	"Undefined",
	"Smoke Alarm",
	"CO Alarm",
	"CO2 Alarm",
	"Heat Alarm",
	"Water Alarm",
	"Access Control Alarm",
	"Burglar Alarm",
	"Power Management Alarm",
	"System Alarm",
	"Emergency Alarm",
	"Alarm Clock"
};*/

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
	Msg* msg = new Msg( "AlarmCmd_Get", GetNodeId(), REQUEST, FUNC_ID_ZW_SEND_DATA, true, true, FUNC_ID_APPLICATION_COMMAND_HANDLER, GetCommandClassId() );
	msg->SetInstance( this, _instance );
	msg->Append( GetNodeId() );
		msg->Append( 2 );
	msg->Append( GetCommandClassId() );
	msg->Append( AlarmCmd_Get );
	msg->Append( GetDriver()->GetTransmitOptions() );
	GetDriver()->SendMsg( msg, _queue );
	return true;
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
		Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm report: type=%d, level=%d", _data[1], _data[2] );
    uint8 type =  _data[1];
    uint8 aValue = _data[2];

    ValueByte* value;      
    if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Type ) )) )
    {
      value->OnValueRefreshed(type);
      value->Release();
            
      if (type >= Alarm_LockSecuredKeypad && type <= Alarm_LockUnsecuredController) 
      {
        if( ValueString* lvalue = static_cast<ValueString*>( GetValue( _instance, AlarmIndex_LockState)))
        {             
          const char *state = c_lockStates[type - Alarm_LockSecuredKeypad];
     
          Log::Write( LogLevel_Info, GetNodeId(), "Lock state is: %s", state);
          lvalue->OnValueRefreshed(state);
          lvalue->Release();
        }
      }
		}
		
		if( (value = static_cast<ValueByte*>( GetValue( _instance, AlarmIndex_Level ) )) )
		{
			value->OnValueRefreshed( aValue );
			value->Release();
		}
		return true;
	}
	
	
	if (AlarmCmd_SupportedReport == (AlarmCmd)_data[0])
	{
		uint8 version = _data[1] >> 7;
		uint8 num = _data[1] & 0x1f;
		
		Log::Write( LogLevel_Info, GetNodeId(), "Received Alarm Supported Report: version=%d num=%d", version, num);
		
		return true;
	}

	return false;
}

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
		//uint8 alarm = 6;
	 	node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Type, "Alarm Type", "", true, false, 0, 0 );
    node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_Level, "Alarm Level", "", true, false, 0, 0 );
	 	//node->CreateValueByte(ValueID::ValueGenre_User, GetCommandClassId(), _instance, alarm, c_alarmTypes[alarm], "", true, false, 0, 0 );
		//node->CreateValueByte( ValueID::ValueGenre_User, GetCommandClassId(), _instance, , "Alarm Query", "", true, false, 0, 0 );
    node->CreateValueString( ValueID::ValueGenre_User, GetCommandClassId(), _instance, AlarmIndex_LockState, "Lock State", "", true, false, "Unknown", 0 );
	}
}

