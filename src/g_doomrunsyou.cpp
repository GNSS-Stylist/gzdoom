/*
	g_doomrunsyou.cpp (part of GZDoom-based hack to allow controlling the game using GNSS-data)
	Copyright (C) 2020 Pasi Nuutinmaki (gnssstylist<at>sci<dot>fi)

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
* This is a dirty hack to allow GNSS-data to be used to control the game.
* Uses a pipe to receive movement commands from GNSS-stylus-SW
* ( https://github.com/GNSS-Stylist/GNSS-Stylus/tree/DoomRunsYou )
* that are then "injected" into tick-commands.
* 
* This is Windows-only for now, sorry about that.
*/

#include "g_doomrunsyou.h"
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

enum CommandType
{
	CT_LOCATION_ORIENTATION_COMMAND = 0,
	CT_PING_FROM_CLIENT,
	CT_PING_FROM_SERVER,
};

void G_DoomRunsYou(ticcmd_t* cmd)
{
	static bool initTried = false;
	static bool initOk = false;
	static int ticksWithoutMessages = 0;

	static HANDLE pipeHandle = INVALID_HANDLE_VALUE;
//	LPTSTR pipeName = TEXT("\\\\.\\pipe\\DoomRunsYou");
	const wchar_t* pipeName = L"\\\\.\\pipe\\DoomRunsYou";

	ticksWithoutMessages++;

	if (!initTried || ticksWithoutMessages >= 35 * 10)
	{
		if (pipeHandle != INVALID_HANDLE_VALUE)
		{
			ticksWithoutMessages = 0;
			Printf("No \"Doom runs you\"-messages in 10 s, re-creating pipe...\n");
			CloseHandle(pipeHandle);
		}

		pipeHandle = CreateNamedPipe(
			pipeName,             // pipe name 
			PIPE_ACCESS_DUPLEX,       // read/write access 
			PIPE_TYPE_MESSAGE |       // message type pipe 
			PIPE_READMODE_MESSAGE |   // message-read mode 
			PIPE_NOWAIT,              // non-blocking mode ("Note: Nonblocking mode is supported for compatibility with Microsoft LAN Manager version 2.0, and it should not be used to achieve asynchronous input and output (I/O) with named pipes.")
			PIPE_UNLIMITED_INSTANCES, // max. instances  
			100,                  // output buffer size 
			100,                  // input buffer size 
			0,                        // client time-out 
			NULL);

		if (pipeHandle != INVALID_HANDLE_VALUE)
		{
			initOk = true;
		}
		else
		{
			Printf("Doom runs you: Can not create pipe.\n");
		}

		initTried = true;
	}

	if (initOk)
	{
		int recData[6];
		DWORD bytesRead;

		while (ReadFile(pipeHandle, &recData, sizeof(recData), &bytesRead, NULL))
		{
			if (bytesRead == sizeof(recData))
			{
				DWORD bytesWritten;

				switch (recData[0])
				{
				case CT_PING_FROM_CLIENT:
					// "Ping" it back
					WriteFile(pipeHandle, recData, bytesRead, &bytesWritten, NULL);
					break;

				case CT_LOCATION_ORIENTATION_COMMAND:
					cmd->ucmd.forwardmove += recData[2];
					cmd->ucmd.sidemove += recData[3];
					cmd->ucmd.pitch += recData[4];
					cmd->ucmd.yaw += recData[5];

					// "Ping" it back
					WriteFile(pipeHandle, recData, bytesRead, &bytesWritten, NULL);
					break;

				case CT_PING_FROM_SERVER:
					Printf("Doom runs you: Response to ping request received");
					break;

				default:
					break;
				}

				ticksWithoutMessages = 0;
			}
			else
			{
				break;
			}
		}

		if ((ticksWithoutMessages >= 35 * 5) && (ticksWithoutMessages % 35 == 0))
		{
			// Test the connection by sending pings

			Printf("No \"Doom runs you\"-messages in %d s, testing the connection with a ping...\n", ticksWithoutMessages / 35);

			int pingData[6];

			pingData[0] = CT_PING_FROM_SERVER;

			for (unsigned int i = 1; i < sizeof(pingData) / sizeof(pingData[0]); i++)
			{
				pingData[i] = 0;
			}

			DWORD bytesWritten;
			WriteFile(pipeHandle, pingData, sizeof(pingData), &bytesWritten, NULL);
		}
	}
}



