#pragma once

VOID Install(TCHAR* pPath, TCHAR* pName);
VOID UnInstall(TCHAR* pName);
BOOL RunService(TCHAR* pName);
BOOL KillService(TCHAR* pName);
