/*
* Copyright 2020 Comcast Cable Communications Management, LLC
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* SPDX-License-Identifier: Apache-2.0
*/

#include "RdkCVAImageTools.h"

using namespace std;
//using namespace cv;

unsigned char* ImageTools::yuvSnapshotData = NULL;

#ifndef _HAS_XSTREAM_
RdkCPluginFactory* ImageTools::temp_factory;
RdkCVideoCapturer* ImageTools::recorder;
#endif

static std::vector<cv::Point> tokenize_to_roi(int width, int height, std::string str)
{
    std::replace( str.begin(), str.end(), '[', ' ');
    std::replace( str.begin(), str.end(), ']', ' ');
    std::replace( str.begin(), str.end(), '-', ' ');

    std::vector <cv::Point> tokens;
    const char* begin = str.c_str();
    char* end;
    double data = strtod( begin, &end );
    errno = 0; //reset

    while ( errno == 0 && end != begin ) {
	int x_coord = width * data;
	begin = end;
	data = strtod( begin, &end ); //each string (here seperated by space) will be converted to double
	int y_coord = height * data;
	begin = end;
	data = strtod( begin, &end ); //each string (here seperated by space) will be converted to double
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): [%d,%d].\n",__FUNCTION__,__LINE__, x_coord, y_coord);
	tokens.push_back(cv::Point(x_coord, y_coord));
    }

    return tokens;
}

/* Constructor */
ImageTools::ImageTools():frame(NULL)
			, buf_id(0)
{
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Constructor.\n",__FUNCTION__,__LINE__);
#ifdef _HAS_XSTREAM_
	consumer = NULL;
	frameInfo = NULL;
#ifndef _DIRECT_FRAME_READ_
	frameHandler.curl_handle = NULL;
	frameHandler.sockfd = -1;
#endif
#endif

	frame = (Frame*) malloc(sizeof(Frame));
	if( NULL == frame ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Failed to allocate memory to frame.\n",__FUNCTION__,__LINE__);
	}
}

/* Destructor */
ImageTools::~ImageTools()
{
	Close();
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Destructor.\n",__FUNCTION__,__LINE__);
}

/**
 * @brief This function is used to initialize the buffer
 * @param void
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::Init(int bufferId)
{
#ifdef _HAS_XSTREAM_
	consumer =  new XStreamerConsumer;
	if( NULL == consumer ) {
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): initialization of XStreamerConsumer failed!\n", __FILE__, __LINE__);
		return RDKC_FAILURE;
	}
#endif
	buf_id = bufferId;
#ifdef _HAS_XSTREAM_
	//initialize
	int ret = RDKC_FAILURE;
#ifdef _DIRECT_FRAME_READ_
	/*
	* Even if GDMA_COPY_ENABLE flag is defined in soc, memory copy is preferred for snapshooter.
	Thus we pass true as second argument to bypass gdma copy.
	* When GDMA_COPY_ENABLE is not defined in soc, second argument has no significance.
	* However, certain platform may only have GDMA copy support(eg: XHB1) in their soc layer,
	in such case the second parameter has no significance.
	*/
	ret = consumer->RAWInit((u16)buf_id, true);

	if ( ret < 0 )
#else
	frameHandler = consumer->RAWInit(buf_id, FORMAT_YUV, 1);

	if ( frameHandler.sockfd < 0 )
#endif //_DIRECT_FRAME_READ_
	{
		RDK_LOG( RDK_LOG_ERROR, "LOG.RDK.THUMBNAILUPLOAD",
			"%s(%d) :TestApp:: Invalid Sockfd\n", __FILE__,
			__LINE__ );
		return RDKC_FAILURE;

	}

	//Get the raw frame container from the consumer
	frameInfo = consumer->GetRAWFrameContainer();
#else
	//initialize plugins
	temp_factory = CreatePluginFactoryInstance();
	if( NULL == temp_factory ) {
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): initialization of temp factory failed!\n", __FILE__, __LINE__);
		return RDKC_FAILURE;
	}

	recorder = ( RdkCVideoCapturer* )temp_factory->CreateVideoCapturer();
	if( NULL == recorder ) {
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): initialization of plugins failed!\n", __FILE__, __LINE__);
		return RDKC_FAILURE;
	}
#endif

	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to get the yuv frame from the camera
 * @param void
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::GetYUVFrame()
{
        int ret = RDKC_FAILURE;
#ifndef _HAS_XSTREAM_
        RDKC_PLUGIN_YUVInfo yuv_info;
#endif
        if( NULL == frame ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): No memory allocated for argument frame.\n",__FILE__,__LINE__);
                return RDKC_FAILURE;
        }

		/*
		* Even if GDMA_COPY_ENABLE flag is defined in soc, memory copy is preferred for snapshooter. Thus we pass bypass_gdmacopy as true.
		* When GDMA_COPY_ENABLE is not defined in soc, bypass_gdmacopy has no significance.
		* Certain platform may only have GDMA copy support(eg: XHB1) in their soc layer,
		in such case bypass_gdmacopy has no significance.
		*/
		bool bypass_gdmacopy = true;

#ifdef _HAS_XSTREAM_
		int count = 0;

		if (NULL == consumer) {
			RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Invalid consumer instance\n", __FILE__, __LINE__);
			return RDKC_FAILURE;
		}

		do {
#ifdef _DIRECT_FRAME_READ_
			ret = consumer->ReadRAWFrame((u16)buf_id, (u16)FORMAT_YUV, frameInfo, bypass_gdmacopy);
#else
			ret = consumer->ReadRAWFrame(buf_id, FORMAT_YUV, frameInfo);
#endif //_DIRECT_FRAME_READ_
			if (ret == XSTREAMER_SUCCESS) {
				ret = RDKC_SUCCESS;
				break;
			}
			count++;
			sleep(10);

		} while(ret == XSTREAMER_FRAME_NOT_READY && count < 10);
#else
		memset(&yuv_info, 0, sizeof(RDKC_PLUGIN_YUVInfo));
		ret = recorder->ReadYUVData(buf_id, &yuv_info, bypass_gdmacopy);
#endif

        if (RDKC_SUCCESS == ret)
        {
#ifdef _HAS_XSTREAM_
                RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Got one frame! width=%d, height=%d, pitch=%d, seq_num=%d, format=%d, flag=%d, dsp_pts=%llu, mono_pts=%llu!\n", __FILE__, __LINE__, frameInfo->width, frameInfo->height, frameInfo->pitch, frameInfo->seq_num, frameInfo->format, frameInfo->flag, frameInfo->dsp_pts, frameInfo->mono_pts);
#else
                RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Got one frame! width=%d, height=%d, pitch=%d, seq_num=%d, format=%d, flag=%d, dsp_pts=%llu, mono_pts=%llu!\n", __FILE__, __LINE__, yuv_info.width, yuv_info.height, yuv_info.pitch, yuv_info.seq_num, yuv_info.format, yuv_info.flag, yuv_info.dsp_pts, yuv_info.mono_pts);
#endif
        }
#ifndef _HAS_XSTREAM_
        else if (1 == ret)
        {
                // No frame data ready, try again
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): No frame data ready.\n",__FILE__,__LINE__);
                return RDKC_FAILURE;
        }
#endif
        else
        {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Read YUV stream error.\n",__FILE__,__LINE__);
                return RDKC_FAILURE;
        }

#ifdef _HAS_XSTREAM_
        frame->height = frameInfo->height;
        frame->width = frameInfo->width;
        frame->y_data = (unsigned char*) frameInfo->y_addr;
        frame->uv_data =  (unsigned char*)frameInfo->uv_addr;
        frame->size = frameInfo->width*frameInfo->height;
#else
        frame->height = yuv_info.height;
        frame->width = yuv_info.width;
        frame->y_data = (unsigned char*) yuv_info.y_addr;
        frame->uv_data =  (unsigned char*)yuv_info.uv_addr;
        frame->size = yuv_info.width*yuv_info.height;
#endif

        return ret;
}
/**
 * @brief This function is used to convert and resize a YUV frame into a JPEG and saves to file system.
 * @param snapshot_filename is filename
 * @param compression_scale is JPEG scale value
 * @param new_width is new width of JPEG image.
 * @param new_height is new height of JPEG image.
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::RdkCVASnapshot_NV12(std::string snapshot_filename, int compression_scale, int new_width, int new_height, std::string roi)
{
	cv::Mat yuvMat;
	int l_width = new_width;
	int l_height = new_height;
	vector<int> compression_params;
	cv::Mat RGBMat;

	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(compression_scale);

	/* Allocate memory as per the frame width and height */
	yuvSnapshotData = (unsigned char *) malloc((frame->size + (frame->size/2)) * sizeof(unsigned char));

	if(NULL == yuvSnapshotData ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Invalid pointer to yuvData\n",__FUNCTION__,__LINE__);
		return RDKC_FAILURE;
	}

	/* Converting an image buffer into Mat */
	memset( yuvSnapshotData, 0, (frame->size + (frame->size/2)) * sizeof(unsigned char));
	memcpy( yuvSnapshotData, frame->y_data,frame->size);
	memcpy( yuvSnapshotData+frame->size,frame->uv_data,frame->size/2 );
	yuvMat = cv::Mat( frame->height + frame->height/2, frame->width, CV_8UC1, yuvSnapshotData );

	/* matrix to store color image */
	RGBMat = cv::Mat( frame->height, frame->width, CV_8UC4);
	/* convert the frame to BGR format */
	cv::cvtColor(yuvMat, RGBMat, CV_YUV2BGR_NV12);

	if( frame->height != l_height || frame->width != l_width ) {
	    /* resize the frame to new width and height */
	    cv::resize(RGBMat, RGBMat,cv::Size(l_width, l_height),0, 0);
	}

	if(!roi.empty()) {
		RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Thumbnail with ROI: %s\n",__FUNCTION__,__LINE__, roi.c_str());
		std::vector<cv::Point> roicoods = std::vector<cv::Point>();
		roicoods = tokenize_to_roi(new_width, new_height, roi.c_str());

		//Draw ROI if it is not empty
		if(!roicoods.empty()) {
			std::vector< std::vector<cv::Point> > polygons;
			polygons.push_back(roicoods);
			cv::drawContours(RGBMat, polygons, 0, cv::Scalar(0, 255, 255));
		} else {
			RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): ROI is empty : Skip drawing ROI\n",__FUNCTION__,__LINE__);
		}
	} else {
		RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Thumbnail without ROI\n",__FUNCTION__,__LINE__);
	}

	cv::imwrite(snapshot_filename, RGBMat, compression_params);

	/* Release memory */
	if( NULL != yuvSnapshotData ) {
		free(yuvSnapshotData);
		yuvSnapshotData = NULL;
	}
	RGBMat.release();
	yuvMat.release();
	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to convert and resize a YUV frame into a JPEG and saves to file system.
 * @param snapshot_filename is filename
 * @param compression_scale is JPEG scale value
 * @param new_width is new width of JPEG image.
 * @param new_height is new height of JPEG image.
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::RdkCVASnapshot_YV12(std::string snapshot_filename, int compression_scale, int new_width, int new_height)
{
	cv::Mat yuvMat;
	int l_width = new_width;
	int l_height = new_height;
	vector<int> compression_params;
	cv::Mat RGBMat;

	compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
	compression_params.push_back(compression_scale);

	/* Allocate memory as per the frame width and height */
	yuvSnapshotData = (unsigned char *) malloc((frame->size + (frame->size/2)) * sizeof(unsigned char));

	if(NULL == yuvSnapshotData ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): Invalid pointer to yuvData\n",__FUNCTION__,__LINE__);
		return RDKC_FAILURE;
	}

	/* Converting an image buffer into Mat */
	memset( yuvSnapshotData, 0, (frame->size + (frame->size/2)) * sizeof(unsigned char));
	memcpy( yuvSnapshotData, frame->y_data,frame->size);
	memcpy( yuvSnapshotData+frame->size,frame->uv_data,frame->size/2 );
	yuvMat = cv::Mat( frame->height + frame->height/2, frame->width, CV_8UC1, yuvSnapshotData );

	/* matrix to store color image */
	RGBMat = cv::Mat( frame->height, frame->width, CV_8UC4);
	/* convert the frame to BGR format */
	cv::cvtColor(yuvMat, RGBMat, CV_YUV2BGR_YV12);

	if( frame->height != l_height || frame->width != l_width ) {
	    /* resize the frame to new width and height */
	    cv::resize(RGBMat, RGBMat,cv::Size(l_width, l_height),0, 0);
	}

	cv::imwrite(snapshot_filename, RGBMat, compression_params);

        /* Release memory */
        if( NULL != yuvSnapshotData ) {
                free(yuvSnapshotData);
                yuvSnapshotData = NULL;
        }

	return RDKC_SUCCESS;
}

/**
 * @brief This function is used to generate thumbnail image
 * @param snapshot_filename is filename
 * @param compression_scale is JPEG scale value
 * @param new_width is new width of JPEG image.
 * @param new_height is new height of JPEG image.
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::GenerateSnapshot(std::string snapshot_filename, int compression_scale, int new_width, int new_height, std::string roi)
{
        //std::string snapshot_filename;
        //snapshot_filename = SNAPSHOT_FILE;

        if ( RDKC_SUCCESS != GetYUVFrame() ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): GetYUVFrame failed\n",__FILE__, __LINE__);
                return RDKC_FAILURE;
        }

        if( RDKC_SUCCESS != RdkCVASnapshot_NV12(snapshot_filename, compression_scale, new_width, new_height, roi) ) {
                RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD","%s(%d): RdkCVASnapshot_NV12 failed\n",__FILE__, __LINE__);
                return RDKC_FAILURE;
        }

        return RDKC_SUCCESS;
}

/**
 * @brief This function is used to close
 * @param void
 * @return RDKC_SUCCESS on success.
 */
int ImageTools::Close()
{
	int ret = RDKC_SUCCESS;
#ifdef _HAS_XSTREAM_

#ifdef _DIRECT_FRAME_READ_
	if( RDKC_FAILURE != consumer->RAWClose() )
#else
	if( RDKC_FAILURE != consumer->RAWClose(frameHandler.curl_handle) )
#endif //_DIRECT_FRAME_READ_
	//Release the curl handle
	{
		RDK_LOG(RDK_LOG_INFO,"LOG.RDK.THUMBNAILUPLOAD", "%s(%d) RAWClose Successful!!!\n", __FUNCTION__ , __LINE__);
	}
	else {
		RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.THUMBNAILUPLOAD", "%s(%d) RAWClose Failed!!!\n", __FUNCTION__ , __LINE__);
		ret = RDKC_FAILURE;
	}
	frameInfo = NULL;
#ifndef _DIRECT_FRAME_READ_
	frameHandler.curl_handle = NULL;
	frameHandler.sockfd = -1;
#endif

	//Delete the XStreamerConsumer object
	if( NULL != consumer ) {
		delete consumer;
		consumer = NULL;
	}
#else
	if( NULL != recorder ) {
		delete recorder;
		recorder = NULL;
	}
	if( NULL != temp_factory ) {
		delete temp_factory;
		temp_factory = NULL;
	}
#endif

	if( NULL != frame ) {
		free(frame);
		frame = NULL;
	}
	if(NULL != yuvSnapshotData) {
		free(yuvSnapshotData);
		yuvSnapshotData = NULL;
	}
	return ret;
}
