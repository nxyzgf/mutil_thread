#include "stdafx.h"
#include "sfm.h"


using namespace std;

sfm::sfm()
{
	filemonitor = thread(bind(&sfm::file_monitor,this));
	featuresextractionmatching = thread(bind(&sfm::features_extraction_matching, this));
}

sfm::~sfm()
{
	filemonitor.join();
	featuresextractionmatching.join();
}

char* WideCharToMultiByte(LPCTSTR widestr)
{
	int num = WideCharToMultiByte(CP_OEMCP, NULL, widestr, -1, NULL, 0, NULL, FALSE);
	char *pchar = new char[num];
	WideCharToMultiByte(CP_OEMCP, NULL, widestr, -1, pchar, num, NULL, FALSE);
	return pchar;
}

bool IsDirectory(const LPTSTR & strPath)
{
	DWORD dwAttrib = GetFileAttributes(strPath);
	return static_cast<bool>((dwAttrib != 0xffffffff
		&& (dwAttrib & FILE_ATTRIBUTE_DIRECTORY)));
}

void sfm::file_monitor()
{
	std::cout << "file_monitor_thread:begin" << std::endl;
	HANDLE hDir;
	BYTE*  pBuffer = (LPBYTE)new CHAR[4096];
	DWORD  dwBufferSize;
	LPTSTR lpPath = _T("指定文件夹路径");
	WCHAR  szFileName[MAX_PATH];
	char*  szFilePath;
	PFILE_NOTIFY_INFORMATION pNotify = (PFILE_NOTIFY_INFORMATION)pBuffer;

	hDir = CreateFile(lpPath, FILE_LIST_DIRECTORY,
		FILE_SHARE_READ |
		FILE_SHARE_WRITE |
		FILE_SHARE_DELETE, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS |
		FILE_FLAG_OVERLAPPED, NULL);
	if (hDir == INVALID_HANDLE_VALUE)
	{
		printf("INVALID_HANDLE_VALUE");
	}
	string image_names;
	while (true)
	{
		if (ReadDirectoryChangesW(hDir,
			pBuffer,
			4096,
			TRUE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&dwBufferSize,
			NULL,
			NULL))
		{
			memset(szFileName, 0, MAX_PATH);
			memcpy(szFileName, lpPath, _tcslen(lpPath) * sizeof(WCHAR));
			memcpy(szFileName + _tcslen(lpPath), pNotify->FileName, pNotify->FileNameLength);
			szFilePath = WideCharToMultiByte(szFileName);

			switch (pNotify->Action)
			{
			case FILE_ACTION_ADDED:
				if (IsDirectory(szFileName))
				{
					printf("Directory %s added.\n", szFilePath);
				}
				else
				{
					printf("File %s added.\n", szFilePath);
					image_names_vector.emplace_back(szFilePath);
					image_names = szFilePath;
					std::unique_lock<std::mutex> lockTmp(m_Mutex);
					pushDataToQueue(image_names);
					m_ConVar.notify_all(); 
					lockTmp.unlock();
				}
				break;
			default:
				break;
			}
		}
	}	
}

bool sfm::features_extraction_matching()
{
		std::cout << "features_extraction_matching_thread:begin" << std::endl;
		SiftGPU* sift = new SiftGPU(1);
		double T = 20;
		char numStr[5] = "700";
		char*ftNum = NULL;
		BOOL bHardMatchingImage = FALSE;
		if (ftNum) strcpy(numStr, ftNum);
		if (bHardMatchingImage)
		{
			sprintf(numStr, "2000");
			T = 30;
		}
		const char * argv[] = { "-fo", "-1","-loweo", "-tc", numStr,"-cuda", "0" };
		int argc = sizeof(argv) / sizeof(char*);
		sift->ParseParam(argc, argv);
		if (sift->CreateContextGL() != SiftGPU::SIFTGPU_FULL_SUPPORTED) return FALSE;
		candidateimg_names.resize(500);
		ID.resize(500);
		vector<float> descriptors(1);
		vector<SiftGPU::SiftKeypoint> keys(1);
		SiftMatchGPU* matcher = new SiftMatchGPU(num[m]);
		matcher->VerifyContextGL();
		matcher->SetLanguage(3);
		while (true)
		{
		std::unique_lock<std::mutex> lockTmp_1(m_Mutex);
		while (isDataQueueEmpty())
			m_ConVar.wait(lockTmp_1);
		lockTmp_1.unlock();

		string image_names1;
		popDataFromQueue(image_names1);
		cv::Mat image = cv::imread(image_names1);
		int width = image.cols;
		int height = image.rows;
		sift->RunSIFT(width, height, image.data, GL_RGB, GL_UNSIGNED_BYTE);
		num[m] = sift->GetFeatureNum();
		keys.resize(num[m]);
		descriptors.resize(128 * num[m]);
		sift->GetFeatureVector(&keys[0], &descriptors[0]);
		descriptors_for_all.push_back(descriptors);

		cout << num[m] << endl;
		m++;
	}
}

bool sfm::isDataQueueEmpty()
{
	std::lock_guard<std::mutex> lockTmp(m_Mutex4Queue);
	return m_DataQueue.empty();
}

bool sfm::isDataQueueNotEmpty()
{
	std::lock_guard<std::mutex> lockTmp(m_Mutex4Queue);
	return !m_DataQueue.empty();
}

void sfm::pushDataToQueue(string &Data)
{
	std::lock_guard<std::mutex> lockTmp(m_Mutex4Queue);
	m_DataQueue.push(Data);
}

bool sfm::popDataFromQueue(string &Data)
{
	std::lock_guard<std::mutex> lockTmp(m_Mutex4Queue);
	if (m_DataQueue.size())
	{
		Data = m_DataQueue.front();
		m_DataQueue.pop();
		return true;
	}
	else
		return false;
}