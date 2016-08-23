/*
 * Logger2.h
 *
 *  Created on: 15 Jun 2012
 *      Author: thomas
 */

/**
 * Format is:
 * int32_t at file beginning for frame count
 *
 * For each frame:
 * int64_t: timestamp
 * int32_t: depthSize
 * int32_t: imageSize
 * depthSize * unsigned char: depth_compress_buf
 * imageSize * unsigned char: encodedImage->data.ptr
 */

#ifndef LOGGER2_H_
#define LOGGER2_H_

#include <zlib.h>

#include <limits>
#include <cassert>
#include <iostream>

#include <opencv2/opencv.hpp>

#ifndef Q_MOC_RUN
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>

#include "OpenNI2/OpenNI2Interface.h"
#include "MemoryBuffer.h"
#include "TcpHandler.h"
#endif

class Logger2
{
    public:
        Logger2(int width, int height, int fps, bool tcp);
        virtual ~Logger2();

        void startWriting(std::string filename);
        void stopWriting(QWidget * parent);

        OpenNI2Interface * getOpenNI2Interface()
        {
            return openNI2Interface;
        }

        MemoryBuffer & getMemoryBuffer()
        {
            return memoryBuffer;
        }

        void setMemoryRecord(bool value)
        {
            assert(!writing.getValue());

            logToMemory = value;
        }


        ThreadMutexObject<std::pair<bool, int64_t> > dropping;

    private:
        OpenNI2Interface * openNI2Interface;
        MemoryBuffer memoryBuffer;

        CvMat * encodedImage;

        int lastWritten;
        std::thread * writeThread;
        ThreadMutexObject<bool> writing;
        std::string filename;
        int64_t lastTimestamp;

        int width;
        int height;
        int fps;


        void encodeJpeg(cv::Vec<unsigned char, 3> * rgb_data);
        void loggingThread();

        FILE * logFile;
        int32_t numFrames;

        bool logToMemory;

        void logData(int64_t * timestamp,
                     int32_t * depthSize,
                     int32_t * imageSize,
                     unsigned char * depthData,
                     unsigned char * rgbData);
};

#endif /* LOGGER2_H_ */
