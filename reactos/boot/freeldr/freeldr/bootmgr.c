/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

VOID RunLoader(VOID)
{
	CHAR	SettingValue[80];
	ULONG_PTR	SectionId;
	ULONG		OperatingSystemCount;
	PCSTR	*OperatingSystemSectionNames;
	PCSTR	*OperatingSystemDisplayNames;
	PCSTR SectionName;
	ULONG		DefaultOperatingSystem;
	LONG		TimeOut;
	ULONG		SelectedOperatingSystem;

	// FIXME: if possible, only detect and register ARC devices...
	if (!MachHwDetect())
	{
		UiMessageBoxCritical("Error when detecting hardware");
		return;
	}

	if (!IniFileInitialize())
	{
		UiMessageBoxCritical("Error initializing .ini file");
		return;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		UiMessageBoxCritical("Section [FreeLoader] not found in freeldr.ini.");
		return;
	}
	TimeOut = GetTimeOut();

	if (!UiInitialize(TimeOut))
	{
		UiMessageBoxCritical("Unable to initialize UI.");
		return;
	}


	if (!InitOperatingSystemList(&OperatingSystemSectionNames, &OperatingSystemDisplayNames, &OperatingSystemCount))
	{
		UiMessageBox("Press ENTER to reboot.");
		goto reboot;
	}

	if (OperatingSystemCount == 0)
	{
		UiMessageBox("There were no operating systems listed in freeldr.ini.\nPress ENTER to reboot.");
		goto reboot;
	}

	DefaultOperatingSystem = GetDefaultOperatingSystem(OperatingSystemSectionNames, OperatingSystemCount);

	//
	// Find all the message box settings and run them
	//
	UiShowMessageBoxesInSection("FreeLoader");

	for (;;)
	{

		// Redraw the backdrop
		UiDrawBackdrop();

		// Show the operating system list menu
		if (!UiDisplayMenu(OperatingSystemDisplayNames, OperatingSystemCount, DefaultOperatingSystem, TimeOut, &SelectedOperatingSystem, FALSE, MainBootMenuKeyPressFilter))
		{
			UiMessageBox("Press ENTER to reboot.");
			goto reboot;
		}

		TimeOut = -1;

		// Try to open the operating system section in the .ini file
		SettingValue[0] = ANSI_NULL;
		SectionName = OperatingSystemSectionNames[SelectedOperatingSystem];
		if (IniOpenSection(SectionName, &SectionId))
		{
			// Try to read the boot type
			IniReadSettingByName(SectionId, "BootType", SettingValue, sizeof(SettingValue));
		}

		if (SettingValue[0] == ANSI_NULL && SectionName[0] != ANSI_NULL)
		{
			// Try to infere boot type value
#ifdef __i386__
			CHAR LastChar;
			LastChar = SectionName[strlen(SectionName) - 1];
			if (LastChar == '\\' ||
			    (strstr(SectionName, ")partition(") != NULL &&
			     strstr(SectionName, ")partition(0)") == NULL))
			{
				strcpy(SettingValue, "Partition");
			}
			else if (LastChar == ')' || LastChar == ':')
			{
				strcpy(SettingValue, "Drive");
			}
			else if (TRUE)
			{
				strcpy(SettingValue, "BootSector");
			}
			else
#endif
			{
				strcpy(SettingValue, "Windows2003");
			}
		}

		// Install the drive mapper according to this sections drive mappings
#ifdef __i386__
		DriveMapMapDrivesInSection(SectionName);
#endif
		if (_stricmp(SettingValue, "ReactOS") == 0)
		{
			LoadAndBootReactOS(SectionName);
		}
#ifdef FREELDR_REACTOS_SETUP
		else if (_stricmp(SettingValue, "ReactOSSetup") == 0)
		{
			// In future we could pass the selected OS details through this
			// to have different install methods, etc.
			LoadReactOSSetup();
		}
#if defined(__i386__) || defined(__x86_64__)
		else if (_stricmp(SettingValue, "ReactOSSetup2") == 0)
		{
			// WinLdr-style boot
			LoadReactOSSetup2();
		}
#endif
#endif
#ifdef __i386__
		else if (_stricmp(SettingValue, "WindowsNT40") == 0)
		{
			LoadAndBootWindows(SectionName, _WIN32_WINNT_NT4);
		}
		else if (_stricmp(SettingValue, "Windows2003") == 0)
		{
			LoadAndBootWindows(SectionName, _WIN32_WINNT_WS03);
		}
		else if (_stricmp(SettingValue, "Linux") == 0)
		{
			LoadAndBootLinux(SectionName, OperatingSystemDisplayNames[SelectedOperatingSystem]);
		}
		else if (_stricmp(SettingValue, "BootSector") == 0)
		{
			LoadAndBootBootSector(SectionName);
		}
		else if (_stricmp(SettingValue, "Partition") == 0)
		{
			LoadAndBootPartition(SectionName);
		}
		else if (_stricmp(SettingValue, "Drive") == 0)
		{
			LoadAndBootDrive(SectionName);
		}
#endif
	}


reboot:
	UiUnInitialize("Rebooting...");
	return;
}

ULONG	 GetDefaultOperatingSystem(PCSTR OperatingSystemList[], ULONG	 OperatingSystemCount)
{
	CHAR	DefaultOSText[80];
	PCSTR	DefaultOSName;
	ULONG_PTR	SectionId;
	ULONG	DefaultOS = 0;
	ULONG	Idx;

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return 0;
	}

	DefaultOSName = CmdLineGetDefaultOS();
	if (NULL == DefaultOSName)
	{
		if (IniReadSettingByName(SectionId, "DefaultOS", DefaultOSText, sizeof(DefaultOSText)))
		{
			DefaultOSName = DefaultOSText;
		}
	}

	if (NULL != DefaultOSName)
	{
		for (Idx=0; Idx<OperatingSystemCount; Idx++)
		{
			if (_stricmp(DefaultOSName, OperatingSystemList[Idx]) == 0)
			{
				DefaultOS = Idx;
				break;
			}
		}
	}

	return DefaultOS;
}

LONG GetTimeOut(VOID)
{
	CHAR	TimeOutText[20];
	LONG		TimeOut;
	ULONG_PTR	SectionId;

	TimeOut = CmdLineGetTimeOut();
	if (0 <= TimeOut)
	{
		return TimeOut;
	}

	if (!IniOpenSection("FreeLoader", &SectionId))
	{
		return -1;
	}

	if (IniReadSettingByName(SectionId, "TimeOut", TimeOutText, sizeof(TimeOutText)))
	{
		TimeOut = atoi(TimeOutText);
	}
	else
	{
		TimeOut = -1;
	}

	return TimeOut;
}

BOOLEAN MainBootMenuKeyPressFilter(ULONG KeyPress)
{
	if (KeyPress == KEY_F8)
	{
		DoOptionsMenu();

		return TRUE;
	}

	// We didn't handle the key
	return FALSE;
}
