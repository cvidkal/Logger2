/*
 * Logger2.cpp
 *
 *  Created on: 15 Jun 2012
 *      Author: thomas
 */

#include "Logger2.h"

Logger2::Logger2(int width, int height, int fps, bool tcp)
 : dropping(std::pair<bool, int64_t>(false, -1)),
   lastWritten(-1),
   writeThread(0),
   width(width),
   height(height),
   fps(fps),
   logFile(0),
   numFrames(0),
   logToMemory(false),
{
    encodedImage = 0;

    writing.assignValue(false);

    openNI2Interface = new OpenNI2Interface(width, height, fps);
}

Logger2::~Logger2()
{

    assert(!writing.getValue() && "Please stop writing cleanly first");

    if(encodedImage != 0)
    {
        cvReleaseMat(&encodedImage);
    }

    delete openNI2Interface;
}


void Logger2::encodeJpeg(cv::Vec<unsigned char, 3> * rgb_data)
{
    cv::Mat3b rgb(height, width, rgb_data, width * 3);

    IplImage * img = new IplImage(rgb);

    int jpeg_params[] = {CV_IMWRITE_JPEG_QUALITY, 90, 0};

    if(encodedImage != 0)
    {
        cvReleaseMat(&encodedImage);
    }

    encodedImage = cvEncodeImage(".jpg", img, jpeg_params);

    delete img;
}

void Logger2::logging2Png()
{
    
}


void Logger2::startWritingPng(std::string filePath)
{
    assert(!writeThread && !writing.getValue() && !logFile);
    
    lastTimestamp = -1;
    
    
    this->filename = filePath+"/list.txt";
    
    writing.assignValue(true);
    
    numFrames = 0;
    
    logFile = fopen(filename.c_str(),"w");
    
    writeThread = new std::thread(&Logger2::logging2Png(),this);
}

void Logger2::startWriting(std::string filename)
{
    assert(!writeThread && !writing.getValue() && !logFile);

    lastTimestamp = -1;

    this->filename = filename;

    writing.assignValue(true);

    numFrames = 0;

    if(logToMemory)
    {
        memoryBuffer.clear();
        memoryBuffer.addData((unsigned char *)&numFrames, sizeof(int32_t));
    }
    else
    {
        logFile = fopen(filename.c_str(), "wb+");
        fwrite(&numFrames, sizeof(int32_t), 1, logFile);
    }

    writeThread = new std::thread(&Logger2::loggingThread,
                                                this);
}

void Logger2::stopWriting(QWidget * parent)
{
    assert(writeThread && writing.getValue());

    writing.assignValue(false);

    writeThread->join();

    dropping.assignValue(std::pair<bool, int64_t>(false, -1));

    if(logToMemory)
    {
        memoryBuffer.writeOutAndClear(filename, numFrames, parent);
    }
    else
    {
        fseek(logFile, 0, SEEK_SET);
        fwrite(&numFrames, sizeof(int32_t), 1, logFile);

        fflush(logFile);
        fclose(logFile);
    }

    writeThread = 0;

    logFile = 0;

    numFrames = 0;
}

void Logger2::loggingThread()
{
    while(writing.getValueWait(1000))
    {
        int lastDepth = openNI2Interface->latestDepthIndex.getValue();

        if(lastDepth == -1)
        {
            continue;
        }

        int bufferIndex = lastDepth % OpenNI2Interface::numBuffers;

        if(bufferIndex == lastWritten)
        {
            continue;
        }

        unsigned char * rgbData = 0;
        unsigned char * depthData = 0;
        unsigned long depthSize = depth_compress_buf_size;
        int32_t rgbSize = 0;

            depthSize = width * height * sizeof(short);
            rgbSize = width * height * sizeof(unsigned char) * 3;

            depthData = (unsigned char *)openNI2Interface->frameBuffers[bufferIndex].first.first;
            rgbData = (unsigned char *)openNI2Interface->frameBuffers[bufferIndex].first.second;

        if(logToMemory)
        {
            memoryBuffer.addData((unsigned char *)&openNI2Interface->frameBuffers[bufferIndex].second, sizeof(int64_t));
            memoryBuffer.addData((unsigned char *)&depthSize, sizeof(int32_t));
            memoryBuffer.addData((unsigned char *)&rgbSize, sizeof(int32_t));
            memoryBuffer.addData(depthData, depthSize);
            memoryBuffer.addData(rgbData, rgbSize);
        }
        else
        {
            logData((int64_t *)&openNI2Interface->frameBuffers[bufferIndex].second,
                    (int32_t *)&depthSize,
                    &rgbSize,
                    depthData,
                    rgbData);
        }

        numFrames++;

        lastWritten = bufferIndex;

        if(lastTimestamp != -1)
        {
            if(openNI2Interface->frameBuffers[bufferIndex].second - lastTimestamp > 1000000)
            {
                dropping.assignValue(std::pair<bool, int64_t>(true, openNI2Interface->frameBuffers[bufferIndex].second - lastTimestamp));
            }
        }

        lastTimestamp = openNI2Interface->frameBuffers[bufferIndex].second;
    }
}

void Logger2::logData(int64_t * timestamp,
                      int32_t * depthSize,
                      int32_t * imageSize,
                      unsigned char * depthData,
                      unsigned char * rgbData)
{
    fwrite(timestamp, sizeof(int64_t), 1, logFile);
    fwrite(depthSize, sizeof(int32_t), 1, logFile);
    fwrite(imageSize, sizeof(int32_t), 1, logFile);
    fwrite(depthData, *depthSize, 1, logFile);
    fwrite(rgbData, *imageSize, 1, logFile);
}
