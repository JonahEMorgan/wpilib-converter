#include <time.h>
#include <windows.h>
#include <stdio.h>
#include <shlobj.h>
#include <stdbool.h>
#include <io.h>

const size_t maxCharacters = 1024;

void wsafecpy(wchar_t* destination,size_t length,wchar_t* source) {
	if(wcscpy_s(destination,length,source) != 0) {
		printf("error: string wasn't copied\n");
		exit(1);
	}
}

void wsafecat(wchar_t* destination,size_t length,wchar_t* source) {
	if(wcscat_s(destination,length,source) != 0) {
		printf("error: string wasn't appended\n");
		exit(1);
	}
}

DWORD WINAPI threadproc(LPVOID lpParameter) {
	Sleep(10);
	HWND close = GetDlgItem(lpParameter,2);
	ShowWindow(close,SW_HIDE);
	HWND select = GetDlgItem(lpParameter,1);
	RECT position;
	if(GetWindowRect(select,&position)) {
		SetWindowPos(select,NULL,0,0,210,position.bottom-position.top,SWP_NOMOVE | SWP_NOZORDER);
	}
	return S_OK;
}

HRESULT queryinterface(IFileDialogEvents* This,const IID* const riid,void** ppvObject) {
	return E_NOTIMPL;
}

ULONG addref(IFileDialogEvents* This) {
	return E_NOTIMPL;
}

ULONG release(IFileDialogEvents* This) {
	return E_NOTIMPL;
}

HRESULT onfileok(IFileDialogEvents* This,IFileDialog* pfd) {
	IShellItem* item;
	if(pfd->lpVtbl->GetResult(pfd,&item) == S_OK){
		wchar_t* path;
		if(item->lpVtbl->GetDisplayName(item,SIGDN_FILESYSPATH,&path) == S_OK) {
			wchar_t* gradle = L"\\gradlew.bat";
			size_t length = wcslen(path)+wcslen(gradle)+1;
			wchar_t* file = malloc(length*sizeof(wchar_t));
			wsafecpy(file,length,path);
			wsafecat(file,length,gradle);
			if(_waccess(file,0) == 0) {
				return S_OK;
			}
		}
	}
	IOleWindow* window;
	if(pfd->lpVtbl->QueryInterface(pfd,&IID_IOleWindow,(void**)&window) == S_OK) {
		HWND handle;
		if(window->lpVtbl->GetWindow(window,&handle) == S_OK) {
			MessageBoxA(handle,"Please choose a folder that contains a WPILib project.",NULL,MB_ICONINFORMATION);
		}
		window->lpVtbl->Release(window);
	}
	return S_FALSE;
}

HRESULT onfolderchanging(IFileDialogEvents* This,IFileDialog* pfd,IShellItem* psiFolder){
	return E_NOTIMPL;
}

HRESULT onfolderchange(IFileDialogEvents* This,IFileDialog* pfd){
	IOleWindow* window;
	if(pfd->lpVtbl->QueryInterface(pfd,&IID_IOleWindow,(void**)&window) == S_OK) {
		HWND handle;
		if(window->lpVtbl->GetWindow(window,&handle) == S_OK) {
			CreateThread(NULL,0,threadproc,handle,0,NULL);
		}
		window->lpVtbl->Release(window);
	}
	return S_OK;
}

HRESULT onselectionchange(IFileDialogEvents* This,IFileDialog* pfd){
	return E_NOTIMPL;
}

HRESULT onshareviolation(IFileDialogEvents* This,IFileDialog* pfd,IShellItem* psi,FDE_SHAREVIOLATION_RESPONSE* pResponse){
	return E_NOTIMPL;
}

HRESULT ontypechange(IFileDialogEvents* This,IFileDialog* pfd){
	return E_NOTIMPL;
}

HRESULT onoverwrite(IFileDialogEvents* This,IFileDialog* pfd,IShellItem* psi,FDE_OVERWRITE_RESPONSE* pResponse){
	return E_NOTIMPL;
}

struct folder {
	wchar_t* name;
	wchar_t* path;
};

struct folder* choosefolder() {
	IFileOpenDialog* dialog;
	if(CoCreateInstance(&CLSID_FileOpenDialog,NULL,CLSCTX_INPROC_SERVER,&IID_IFileOpenDialog,(void**)&dialog) != S_OK) {
		printf("error: couldn't create file dialog\n");
		return NULL;
	}
	if(dialog->lpVtbl->SetOptions(dialog,FOS_PICKFOLDERS) != S_OK) {
		printf("error: couldn't restrict file dialog to folders\n");
		return NULL;
	}
	IFileDialogEventsVtbl vtable = {
		queryinterface,
		addref,
		release,
		onfileok,
		onfolderchanging,
		onfolderchange,
		onselectionchange,
		onshareviolation,
		ontypechange,
		onoverwrite
	};
	IFileDialogEvents events = {
		&vtable
	};
	DWORD cookie;
	if(dialog->lpVtbl->Advise(dialog,&events,&cookie) != S_OK) {
		printf("error: couldn't set file dialog event handler\n");
		return NULL;
	}
	if(dialog->lpVtbl->Show(dialog,NULL) != S_OK) {
		printf("error: couldn't open file dialog\n");
		return NULL;
	}
	if(dialog->lpVtbl->Unadvise(dialog,cookie) != S_OK) {
		printf("error: couldn't stop file dialog event handler\n");
		return NULL;
	}
	IShellItem* item;
	if(dialog->lpVtbl->GetResult(dialog,&item) != S_OK){
		printf("error: couldn't get file dialog result\n");
		return NULL;
	}
	dialog->lpVtbl->Release(dialog);
	struct folder* choice = malloc(sizeof(struct folder));
	if(item->lpVtbl->GetDisplayName(item,SIGDN_FILESYSPATH,&choice->path) != S_OK){
		printf("error: couldn't get name of file dialog result\n");
		return NULL;
	}
	if(item->lpVtbl->GetDisplayName(item,SIGDN_NORMALDISPLAY,&choice->name) != S_OK){
		printf("error: couldn't get name of file dialog result\n");
		return NULL;
	}
	return choice;
}

int main() {
	if(CoInitialize(NULL) != S_OK){
		printf("error: couldn't initialize COM\n");
		return 1;
	}
	struct folder* choice = choosefolder();
	CoUninitialize();
	if(choice == NULL) {
		printf("error: folder was not chosen\n");
		return 1;
	}
	wchar_t* slash = L"\\";
	size_t length = wcslen(choice->path)+wcslen(choice->name)+wcslen(slash)+1;
	wchar_t* directory = malloc(length*sizeof(wchar_t));
	wsafecpy(directory,length,choice->path);
	wsafecat(directory,length,slash);
	wsafecat(directory,length,choice->name);
	if(_waccess(directory,2) != 0){
		if(CreateDirectoryW(directory,NULL) == 0) {
			printf("error: directory wasn't created\n");
			return 1;
		}
	}
	wchar_t* sauce = L"\\*.*";
	length = wcslen(choice->path)+wcslen(sauce)+1;
	wchar_t* glob = malloc(length*sizeof(wchar_t));
	wsafecpy(glob,length,choice->path);
	wsafecat(glob,length,sauce);
	WIN32_FIND_DATAW data;
	HWND search = FindFirstFileW(glob,&data);
	while(FindNextFileW(search,&data) != 0) {
		if((wcscmp(data.cFileName,L"..") & wcscmp(data.cFileName,choice->name)) != 0) {
			length = wcslen(choice->path)+wcslen(slash)+wcslen(data.cFileName)+1;
			wchar_t* from = malloc(length*sizeof(wchar_t));
			wsafecpy(from,length,choice->path);
			wsafecat(from,length,slash);
			wsafecat(from,length,data.cFileName);
			length = wcslen(directory)+wcslen(slash)+wcslen(data.cFileName)+1;
			wchar_t* to = malloc(length*sizeof(wchar_t));
			wsafecpy(to,length,directory);
			wsafecat(to,length,slash);
			wsafecat(to,length,data.cFileName);
			if(MoveFileW(from,to) == 0) {
				wprintf(L"error: %s was not copied\n",data.cFileName);
			}
		}
	}
	wchar_t* sublime = L".sublime-project";
	length = wcslen(directory)+wcslen(sublime)+1;
	wchar_t* location = malloc(length*sizeof(wchar_t));
	wsafecpy(location,length,directory);
	wsafecat(location,length,sublime);
	FILE* project;
	if(_wfopen_s(&project,location,L"w") != 0) {
		printf("error: couldn't open project file for writing\n");
		return 1;
	}
	time_t now = time(NULL);
	struct tm clock;
	if(localtime_s(&clock,&now) != 0) {
		printf("error: couldn't convert unix time to local time\n");
		return 1;
	}
	if(fwprintf(project,L"{\n\t\"folders\": [\n\t\t{\n\t\t\t\"path\": \"%s\\\\src\"\n\t\t},\n\t\t{\n\t\t\t\"path\": \"%s\\\\build\"\n\t\t},\n\t\t{\n\t\t\t\"path\": \"%s\",\n\t\t\t\"folder_exclude_patterns\": [\"src\", \"build\"]\n\t\t}\n\t],\n\t\"build_systems\": [\n\t\t{\n\t\t\t\"name\": \"WPILib Build\",\n\t\t\t\"cmd\": [\"gradlew.bat\", \"build\"],\n\t\t\t\"env\": {\n\t\t\t\t\"JAVA_HOME\": \"%%public%%\\\\wpilib\\\\%i\\\\jdk\"\n\t\t\t},\n\t\t\t\"working_dir\": \"$project_path\\\\%s\",\n\t\t\t\"cancel\": {\n\t\t\t\t\"cmd\": [\"gradlew.bat\", \"--stop\"]\n\t\t\t},\n\t\t\t\"variants\": [\n\t\t\t\t{\n\t\t\t\t\t\"name\": \"WPILib Deploy\",\n\t\t\t\t\t\"cmd\": [\"gradlew.bat\", \"deploy\"],\n\t\t\t\t}\n\t\t\t]\n\t\t}\n\t]\n}\n",choice->name,choice->name,choice->name,clock.tm_year+1900,choice->name) < 0) {
		printf("error: couldn't write to project file\n");
	}
	if(fclose(project) == EOF) {
		printf("error: couldn't close project file\n");
	}
	wchar_t* format = L"Conversion complete! Do you want to open %s?";
	length = wcslen(format)+wcslen(location)-1;
	wchar_t* message = malloc(length*sizeof(wchar_t));
	if(swprintf(message,length,format,location) == -1) {
		printf("error: couldn't format success message\n");
		return 1;
	}
	if(MessageBoxW(NULL,message,L"Success",MB_ICONINFORMATION | MB_YESNO) == IDYES) {
		if((INT_PTR)ShellExecuteW(NULL,L"open",location,NULL,NULL,SW_SHOW) <= 32) {
			printf("error: couldn't open project file in editor\n");
			return 1;
		}
	}
	return 0;
}