#ifndef SHILI_H_
#define SHILI_H_
#pragma once
#include "stdafx.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
#include <queue>
#include <condition_variable>
#include <Windows.h>
#include <functional>
#include <SiftGPU.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/features2d/features2d.hpp"
#include <opencv2/opencv.hpp> 
#include <opencv2/xfeatures2d.hpp>
#include "GL/glew.h"
#include <vector>

using namespace std;

class sfm
{
public:
	sfm();
	~sfm();
	int m = 0;
	int num[500] = { 0 };
	vector<vector<float>> descriptors_for_all;
	vector<vector<SiftGPU::SiftKeypoint>> keys_for_all;
	
private:
	void file_monitor();
	bool features_extraction_matching();
	bool isDataQueueEmpty();
	bool isDataQueueNotEmpty();

	void pushDataToQueue(string &Data);
	bool popDataFromQueue(string &Data);

	std::mutex m_Mutex;
	std::mutex m_Mutex4Queue;

	std::queue<string> m_DataQueue;
	std::condition_variable m_ConVar;

	thread filemonitor;
	thread featuresextractionmatching;
};

#endif