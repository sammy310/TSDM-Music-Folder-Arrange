#include <Windows.h>
#include <ShlObj.h>
#include <list>
#include <string>
#include <wctype.h>
#include <filesystem>
namespace fs = std::experimental::filesystem;
using namespace std;

#define ID_EDIT_FOLDER 1
#define ID_ARRANGEFOLDERBUTTON 11
#define ID_EXITBUTTON 12

HINSTANCE g_hInst;
HWND g_hWnd;
LPCWSTR lpszClass = TEXT("TSDMArrange");
LPCWSTR lpszMenuName = TEXT("TSDM Music Folder Arrange");
int windowWidth = 500;
int windowHeight = 200;

HWND g_hEditFolder;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
BOOL GetOpenFile(WCHAR*);
BOOL GetDirectory(wstring&);
BOOL MusicFolderArrange(wstring&);

void RemoveUnnecessaryFiles(fs::path);
BOOL UpperMP3Folder(fs::path);
void RenameMusicName(fs::path);





int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow) {
	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;

	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDC_ICON);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = lpszMenuName;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(
		lpszClass,
		lpszMenuName,
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		(GetSystemMetrics(SM_CXSCREEN) - windowWidth) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - windowHeight) / 2,
		windowWidth,
		windowHeight,
		NULL,
		(HMENU)NULL,
		hInstance,
		NULL
	);
	ShowWindow(hWnd, nCmdShow);
	g_hWnd = hWnd;

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam) {
	static WCHAR arrangeFolderPath[MAX_PATH] = TEXT("");
	static WCHAR savePath[MAX_PATH] = TEXT("");
	static WCHAR saveFile[260] = TEXT("");

	static wstring MusicFolderPath;


	switch (iMessage) {
	case WM_CREATE:
		g_hEditFolder = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY, windowWidth / 2 - 90, windowHeight / 4, 300, 30, hWnd, (HMENU)ID_EDIT_FOLDER, g_hInst, NULL);
		CreateWindow(TEXT("static"), TEXT("정리할 노래 폴더 :"), WS_CHILD | WS_VISIBLE | SS_CENTER | SS_LEFT, windowWidth / 2 - 250, windowHeight / 4, 150, 30, hWnd, (HMENU)-1, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("정리"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, windowWidth / 2 - 100, windowHeight / 2, 80, 30, hWnd, (HMENU)ID_ARRANGEFOLDERBUTTON, g_hInst, NULL);
		CreateWindow(TEXT("button"), TEXT("종료"), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, windowWidth / 2 + 20, windowHeight / 2, 80, 30, hWnd, (HMENU)ID_EXITBUTTON, g_hInst, NULL);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_EDIT_FOLDER:
			switch (HIWORD(wParam)) {
			case EN_SETFOCUS:
				GetDirectory(MusicFolderPath);
				SetWindowText(g_hEditFolder, MusicFolderPath.c_str());
				break;
			}
			break;
		case ID_ARRANGEFOLDERBUTTON:
			if (MusicFolderArrange(MusicFolderPath) == TRUE)
				MessageBox(hWnd, TEXT("정리 완료"), TEXT("!"), MB_OK);
			break;

		case ID_EXITBUTTON:
			PostQuitMessage(0);
			break;
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, iMessage, wParam, lParam);
}


BOOL GetOpenFile(WCHAR *fileStr) {
	OPENFILENAME OFN;
	memset(&OFN, 0, sizeof(OPENFILENAME));
	OFN.lStructSize = sizeof(OPENFILENAME);
	OFN.hwndOwner = g_hWnd;
	OFN.lpstrFilter = TEXT("폴더\0*\0");
	OFN.lpstrFile = fileStr;
	OFN.nMaxFile = 256;
	OFN.lpstrInitialDir = TEXT("c\\");
	return GetOpenFileName(&OFN);
}


BOOL GetDirectory(wstring& newFolderPath) {
	BROWSEINFO bi;
	LPITEMIDLIST dirList;
	WCHAR nameStr[MAX_PATH];
	WCHAR newPath[MAX_PATH] = TEXT("");

	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = g_hWnd;
	bi.pszDisplayName = nameStr;
	bi.lpszTitle = TEXT("정리할 폴더를 선택하세요");
	bi.ulFlags = BIF_VALIDATE | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
	bi.pidlRoot =

		dirList = SHBrowseForFolder(&bi);
	if (dirList == NULL)return FALSE;

	SHGetPathFromIDList(dirList, newPath);
	newFolderPath.clear();
	newFolderPath.append(newPath);

	LPMALLOC pMalloc = NULL;
	SHGetMalloc(&pMalloc);
	pMalloc->Free(dirList);
	pMalloc->Release();
	return TRUE;
}

BOOL MusicFolderArrange(wstring& newFolderPath) {
	fs::path path = newFolderPath;

	if (fs::exists(path) == false) return FALSE;

	// 메인 폴더의 제목 정리
	for (auto &p : fs::directory_iterator(path)) {
		if (fs::is_directory(p.status())) {
			fs::path folder = p.path();
			wstring folderName = folder.filename().generic_wstring();
			INT folderNameLength = folderName.size();

			if (folderNameLength < 17) continue;
			// 날짜 제거
			wstring date = folderName.substr(0, 8);
			BOOL isDate = TRUE;
			int dateSubSize = 0;
			if (date.at(0) == *TEXT("[") && date.at(7) == *TEXT("]")) {
				for (auto& dStr : date.substr(1, 6)) {
					if (!iswdigit(dStr)) {
						isDate = FALSE;
						break;
					}
				}
				if (folderName.at(8) == *TEXT(" ")) dateSubSize = 9;
				else dateSubSize = 8;
			}
			else {
				isDate = FALSE;
			}

			// 음질 제거
			wstring freq = folderName.substr(folderName.size() - 6);
			BOOL isFreq = TRUE;
			int freqSubSize = 0;
			if (freq.compare(TEXT("[320K]")) == 0) {
				if (folderName.at(folderName.size() - 7) == *TEXT(" ")) freqSubSize = 7;
				else freqSubSize = 6;
			}
			else {
				isFreq = FALSE;
			}

			if (isDate == TRUE || isFreq == TRUE)
			{
				folder.replace_filename(folderName.substr(dateSubSize, folderName.size() - dateSubSize - freqSubSize));
			}

			// 태그 제거
			folderName.clear();
			folderName.append(folder.filename().generic_wstring());
			int index = folderName.find(TEXT("TVアニメ"));
			int tagSize = 5;
			if (index != 0) { index = folderName.find(TEXT("アニメ")); tagSize = 3; }
			if (index != 0) { index = folderName.find(TEXT("劇場版")); tagSize = 3; }
			if (index != 0) { index = folderName.find(TEXT("ゲーム")); tagSize = 3; }
			if (index != 0) { index = folderName.find(TEXT("アプリゲーム")); tagSize = 6; }

			if (index == 0)
				folder.replace_filename(folderName.substr(index + tagSize));

			if (isDate == TRUE || isFreq == TRUE || index == 0) {
				if (fs::exists(folder) == false)
					rename(p.path(), folder);
			}
		}
	}

	RemoveUnnecessaryFiles(path);
	RenameMusicName(path);
	UpperMP3Folder(path);

	return TRUE;
}

void RemoveUnnecessaryFiles(fs::path newPath) {
	for (auto &p : fs::directory_iterator(newPath)) {
		if (fs::is_directory(p.status())) {
			RemoveUnnecessaryFiles(p.path());
		}
		else {
			fs::path file = p.path();
			if (file.filename().generic_wstring().compare(TEXT("desktop")) == 0 && file.extension().generic_wstring().compare(TEXT(".ini")) == 0) {
				remove(file);
			}
			else if (file.extension().generic_wstring().compare(TEXT(".url")) == 0) {
				remove(file);
			}
			/*else if (file.extension().generic_wstring().compare(TEXT(".txt")) == 0) {
				remove(file);
			}*/
		}
	}
}

BOOL UpperMP3Folder(fs::path newPath) {
	if (newPath.filename().compare(TEXT("MP3")) == 0) {
		wstring upperFolderPath = newPath.parent_path().generic_wstring();
		for (auto &p : fs::directory_iterator(newPath)) {
			MoveFile(p.path().generic_wstring().c_str(), (upperFolderPath + TEXT("/") + p.path().filename().generic_wstring()).c_str());
		}
		RemoveDirectory(newPath.generic_wstring().c_str());
		return TRUE;
	}

	for (auto &p : fs::directory_iterator(newPath)) {
		if (fs::is_directory(p.status())) {
			if (UpperMP3Folder(p.path()) == TRUE) break;
		}
	}

	return FALSE;
}

void RenameMusicName(fs::path newPath) {
	for (auto &p : fs::directory_iterator(newPath)) {
		if (fs::is_directory(p.status())) {
			RenameMusicName(p.path());
		}
		else {
			fs::path file = p.path();
			wstring fileName = file.filename().generic_wstring();
			if (file.extension().generic_wstring().compare(TEXT(".mp3")) == 0) {
				wstring newFileName;
				BOOL isNameChange = FALSE;
				int subSize = 0;
				int numSize = 0;
				for (; numSize < 3;) {
					if (iswdigit(fileName.at(numSize))) {
						numSize++;
					}
					else {
						break;
					}
				}

				if (numSize == 0) {
					continue;
				}
				else if (numSize == 1) {
					newFileName.append(TEXT("0"));
					newFileName.append(fileName.substr(0,1));
					isNameChange = TRUE;
				}
				else {
					newFileName.append(fileName.substr(0, numSize));
				}
				subSize = numSize;


				newFileName.append(TEXT(". "));
				for (int count = 0;; count++) {
					auto nextChar = fileName.at(numSize + count);
					if (nextChar == *TEXT(".")) {
						if (count != 0) isNameChange = TRUE;
						subSize++;
					}
					else if (nextChar == *TEXT(" ")) {
						if (count != 1) isNameChange = TRUE;
						subSize++;
					}
					else {
						if (count != 2) isNameChange = TRUE;
						break;
					}
				}

				if (isNameChange == TRUE) {
					file.replace_filename(newFileName + fileName.substr(subSize));
					rename(p.path(), file);
				}
			}
			else {
				if (_stricmp(file.filename().string().substr(0, 5).c_str(), "cover") == 0 && file.extension().generic_wstring().compare(TEXT(".jpg")) == 0) {
					fs::path tempPath = file;
					tempPath.replace_filename(TEXT("aCover") + file.filename().wstring().substr(5));
					rename(p.path(), tempPath);
					file.replace_filename(TEXT("Cover") + file.filename().wstring().substr(5));
					rename(tempPath, file);
				}
			}
		}
	}
}
