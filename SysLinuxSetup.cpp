// SysLinuxSetup: GUI Front-end for syslinux.exe
// Author: Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
// http://pcman.sayya.org/
// Copyright (C) 2008
// License: GNU GPL 2

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tchar.h>
#include "resource.h"

TCHAR sel_drive[4] = "";
HWND list = NULL;

void SizeToStr( ULARGE_INTEGER& size, LPTSTR size_str )
{
	__int64 val = size.QuadPart;
	if( val == 0 )
		_tcscpy( size_str, "0 Byte" );
	else if( val > (1 << 30) )
		_stprintf( size_str, "%.2f GB", (float)val / (1 << 30) );
	else if( val > (1 << 20) )
		_stprintf( size_str, "%.2f MB", (float)val / (1 << 20) );
	else if( val > (1 << 10) )
		_stprintf( size_str, "%.2f KB", (float)val / (1 << 10) );
	else
		_stprintf( size_str, "%d Bytes", (int)val );
}


BOOL InitList(HWND hwnd)
{
	TCHAR buf[256];
	LPCTSTR drive;
	list = GetDlgItem( hwnd, IDC_LIST );
	ListView_SetExtendedListViewStyle( list, LVS_EX_FULLROWSELECT );

	LVCOLUMN col;
	col.mask = LVCF_TEXT|LVCF_WIDTH;
	col.pszText = _T("Drive");
	col.cx = 200;
	ListView_InsertColumn( list, 0, &col );

	col.pszText = _T("Free Space");
	col.cx = 240;
	ListView_InsertColumn( list, 1, &col );

	TCHAR exe_path[MAX_PATH], *psep;
	TCHAR win_drive[4];

	GetWindowsDirectory( exe_path, MAX_PATH );
	_tcsncpy( win_drive, exe_path, 3 );
	win_drive[3] = 0;

	GetModuleFileName( GetModuleHandle(NULL), exe_path, MAX_PATH );
	if( psep = _tcsstr( exe_path, ":\\" ) )
		psep[2] = 0;
	else
		*exe_path = 0;

	BOOL found = FALSE;
	HIMAGELIST img_list;

	GetLogicalDriveStrings( 256, buf );
	for( drive = buf; *drive; drive += _tcslen( drive ) + 1 )
	{
		UINT type = GetDriveType( drive );
		if( type == DRIVE_REMOVABLE )
		{
			// prevent stupid users from running syslinux on their system partition
			if( 0 == _tcsicmp( drive, "C:\\") || 0 == _tcsicmp( drive, win_drive ) )
				continue;

			found = TRUE;

			ULARGE_INTEGER free, total;
			GetDiskFreeSpaceEx( drive, &free, &total, NULL );
			TCHAR size_str[32];

			SizeToStr( free, size_str );

			SHFILEINFO fi;
			img_list = (HIMAGELIST)SHGetFileInfo( drive, 0, &fi, sizeof(fi), SHGFI_LARGEICON | SHGFI_SYSICONINDEX );

			LV_ITEM item;
			item.mask = LVIF_TEXT|LVIF_IMAGE;
			item.pszText = (LPTSTR)drive;
			item.iImage = fi.iIcon;
			item.iItem = ListView_GetItemCount(list);
			item.iSubItem = 0;
			ListView_InsertItem( list, &item );

			item.mask = LVIF_TEXT;
			item.pszText = size_str;
			item.iSubItem = 1;
			ListView_SetItem( list, &item );

			if( exe_path && (0 == _tcsstr( drive, exe_path )) )
			{
				ListView_SetItemState( list, item.iItem, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
			}
		}
	}

	if( found )
	{
		ListView_SetImageList( list, img_list, LVSIL_SMALL );
		if( ListView_GetNextItem( list, -1, LVNI_SELECTED ) == -1 )
			ListView_SetItemState( list, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED );
	}
	else
	{
		MessageBox( HWND_DESKTOP, _T("No removable disk was found.\n\nPlease insert your USB disk and try again."), NULL, MB_OK|MB_ICONSTOP );
	}

	return found;
}

BOOL CALLBACK DlgProc( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		if( InitList(hwnd) )
			return TRUE;
		else
		{
			EndDialog( hwnd, IDCANCEL );
			return FALSE;
		}
	case WM_COMMAND:
		switch( LOWORD(wp) )
		{
		case IDOK:
			{
				int sel = ListView_GetNextItem( list, -1, LVNI_SELECTED );
				if( sel == -1 )
				{
					MessageBox( hwnd, _T("Please select a drive."), NULL, MB_OK );
					break;
				}
				ListView_GetItemText( list, sel, 0, sel_drive, 4 );
			}
		case IDCANCEL:
			EndDialog( hwnd, LOWORD(wp) );
			return TRUE;
		}
		break;
	}
	return FALSE;
}

int WINAPI WinMain( HINSTANCE hinst, HINSTANCE hprev, LPSTR cmd, int show )
{
	InitCommonControls();
	if( DialogBox( hinst, LPCTSTR(IDD_DIALOG), HWND_DESKTOP, DlgProc ) == IDOK )
	{
		// Remove tailing slash
		sel_drive[2] = 0;

		if( _tcsncmp(cmd, _T("/p"), 2 ) == 0 )	// only print the drive
		{
			_putts( sel_drive );
			return (sel_drive[0] - 'A');
		}
		else
		{
			TCHAR exe_path[MAX_PATH];
			GetModuleFileName( GetModuleHandle(NULL), exe_path, MAX_PATH );
			PathRemoveFileSpec( exe_path );

			if( GetVersion() < 0x80000000) // Windows NT
				_tcscat( exe_path, "\\syslinux.exe" );
			else
				_tcscat( exe_path, "\\syslinux.com" );

			if( ! PathFileExists(exe_path) )
			{
				MessageBox( NULL, _T("Program syslinux is not found"), NULL, MB_OK|MB_ICONSTOP );
				return -1;
			}

			SHELLEXECUTEINFO si;
			si.cbSize = sizeof(si);
			si.lpVerb = _T("open");
			si.lpFile = exe_path;
			si.lpParameters = sel_drive;
			si.lpDirectory = NULL;
			si.nShow = SW_HIDE;
			si.hwnd = NULL;
			si.fMask = SEE_MASK_NOCLOSEPROCESS;

			if( ShellExecuteEx( &si ) )
			{
				WaitForSingleObject( si.hProcess, INFINITE );
				CloseHandle( si.hProcess );
			}
			MessageBox( NULL, _T("The installation is finished"), _T("Finish"), MB_OK | MB_ICONINFORMATION );
		}
	}
	else if( _tcsncmp(cmd, _T("/p"), 2 ) == 0 )	// only print the drive
	{
		return -1;
	}
	return 0;
}
