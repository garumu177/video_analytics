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

/*************************       INCLUDES         *************************/
#include <iostream>
#include <sstream>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "rdk_debug.h"
#include "xcv.h"
#include "xcvInterface.h"
#include "iavInterface.h"
#include "RFCCommon.h"
#include "dev_config.h"
#ifdef _ROI_ENABLED_
#include "polling_config.h"
#include <vector>
#include <fileUtils.h>
#endif

#include  "rdk_dynamic_logging_util.h"
#ifdef __cplusplus
extern "C"
{
#endif
#ifndef OSI
#include "sc_tool.h"
#endif
#include "dev_config.h"
#ifdef __cplusplus
}
#endif
#ifdef BREAKPAD
#include "breakpadwrap.h"
#endif
int enable_debug;
#ifdef ENABLE_TEST_HARNESS
#include "THInterface.h"
//#include "RdkCVAManager.h"
//#include "RdkCTestHarness.h"
#endif
#include <rbus.h>

#define MAXSIZE            		 100
#define MINSIZE            		 10
#define RDKC_SUCCESS 		  	 0
#define RDKC_FAILURE 			-1
#define IAV_SRCBUF_3		         2
#define IAV_SRCBUF_4		         3

#ifdef XHB1
#define STN_HERS_CROP_HEIGHT            "480"
#define STN_HERS_CROP_WIDTH             "640"
#endif

#ifdef _OBJ_DETECTION_
#define DEFAULT_DELIVERY_UPSCALE_FACTOR 1.5f
#define DELIVERY_SETTINGS_FILE "/opt/usr_config/delivery_attr.conf"
#endif

#define DEFAULT_DOI_OVERLAP_THRESHOLD 0
#define DOI_SETTINGS_FILE "/opt/usr_config/doi_attr.conf"

/* generate library name according to engine
 * @param : constant string
 * return : buff- string
 */
#if 0
static std::string makeLibraryName(char const* s)
{
    std::stringstream buff;
    buff << "libAnalytics_";
    buff << s;
    buff << ".so";
    return buff.str();
}
#endif

static int doi_enable = 0;
static char doi_url[512] = {};
static int doi_threshold = 0;
#ifdef ENABLE_TEST_HARNESS
#define DEFAULT_DOI_BITMAP_PATH "/opt/doi_bitmap.bmp"
#else
#define DEFAULT_DOI_BITMAP_PATH "/opt/usr_config/doi_bitmap"
#endif

/* function to handle reload signal
 * @param : dummy- int
 */
static volatile sig_atomic_t reload_va_flag = 1;
static void reload_config(int dummy)
{
    reload_va_flag = 1;
    return;
}

/*function to handle terminate signal
 *@param : sig -int
 */
static volatile sig_atomic_t term_flag = 0;
static void self_term(int sig)
{
    term_flag = 1;
    return;
}

static int xvision_check_filelock(char *fname)
{
        int fd = -1;
        pid_t pid = -1;
        int readbytes=0;

        fd = open(fname, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if(fd < 0 && errno == EEXIST)
        {
                fd = open(fname, O_RDONLY, 0644);
                if (fd >= 0)
                {
                        readbytes = read(fd, &pid, sizeof(pid));
                        if( readbytes > 0 ) {
                                kill(pid, SIGTERM);
                                close(fd);
                                sleep(1);
#ifndef OSI
                                if (CheckAppsPidAlive("xvisiond", pid))
                                {
                                        kill(pid, SIGTERM);
                                }
#endif
                                unlink(fname);
                        }
                }
                return -2;
        }
        return fd;
}

static std::vector<float> tokenize(std::string str)
{
    // str : [[0.1-0.2][0.3-0.4][0.5-0.6][0.7-0.8][0.9-0.1][0.11-0.12][0.13-0.14]]
    // with below replace calls str:   0.1 0.2  0.3 0.4  0.5 0.6  0.7 0.8  0.9 0.1  0.11 0.12  0.13 0.14
    std::replace( str.begin(), str.end(), '[', ' ');
    std::replace( str.begin(), str.end(), ']', ' ');
    std::replace( str.begin(), str.end(), '-', ' ');

    std::vector <float> tokens;
    const char* begin = str.c_str();
    char* end;
    double data = strtod( begin, &end );
    errno = 0; //reset
    while ( errno == 0 && end != begin ) {
        tokens.push_back(data);
        begin = end;
        data = strtod( begin, &end ); //each string (here seperated by space) will be converted to double
    }
    return tokens;
}

/** @description: Convert float vector to string
 *  @param[in]: vector<float>
 *  @return: if vector is {0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9,0.1,0.11,0.12,0.13,0.14},
 *      output string is ==> [[0.1-0.2][0.3-0.4][0.5-0.6][0.7-0.8][0.9-0.1][0.11-0.12][0.13-0.14]]
 */
static std::string convertToString(std::vector<float> coords)
{
    std::stringstream ss;
    ss << "[";  //Outer most, enclosing the polygon entity
    for(size_t i = 0; i < coords.size(); ++i)
    {
        // represents x,y eg: vector {0.1,0.2,...} --> [0.1-0.2]
        if(i%2 == 0)    ss << "[";
        if(i%2 == 1)    ss << "-";
        ss << coords[i];
        if(i%2 == 1)    ss << "]";
    }
    ss << "]"; //Outer most, enclosing the polygon entity
    return ss.str();
}

#ifdef _ROI_ENABLED_

/** @description: Get ROI coords from event configuration file
 *  @param[in] void
 *  @return: std::string : ROIcooords
 */
static std::string getROICoords()
{
    int retry = 3;
    events_provision_info_t *eventsCfg = NULL;
    std::string coordinates = "";

    eventsCfg = (events_provision_info_t*) malloc(sizeof(events_provision_info_t));

    if (NULL == eventsCfg) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Error allocating memory.\n", __FILE__, __LINE__);
        return coordinates;
    }

    //Initialize polling config
    if (0 != polling_config_init()) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Error initializing polling config.\n", __FILE__, __LINE__);
        if (eventsCfg) {
            free(eventsCfg);
        }
        return coordinates;
    }

    while (retry) {
        //Read EventConfig file
        if (0 != readEventConfig(eventsCfg)) {
            RDK_LOG(RDK_LOG_WARN,"LOG.RDK.XCV","%s(%d): Error reading EVENTS Config.\n", __FILE__, __LINE__);
        }
        else {
            break;
        }
        //Sleep 1 sec before retrying
        sleep(1);
        retry--;
    }

    coordinates = eventsCfg->roi_coord;

    if (eventsCfg) {
        free(eventsCfg);
        eventsCfg = NULL;
    }

    polling_config_exit();

    return coordinates;
}
#endif

/** @description: Get DOI conf
 *  @param[in] void
 *  @return: true on success, false otherwise
 */
static bool getDOIConf()
{
    int retry = 3;
    doi_config_info_t *doiCfg = NULL;

    // Read DOI config.
    doiCfg = (doi_config_info_t*) malloc(sizeof(doi_config_info_t));

    if (NULL == doiCfg) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Error allocating memory.\n", __FILE__, __LINE__);
        return false;
    }

    if (0 != polling_config_init()) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.VIDEOANALYTICS","%s(%d): Error initializing polling config.\n", __FILE__, __LINE__);
        if (doiCfg) {
            free(doiCfg);
        }
        return false;
    }

    while (retry) {
        if (true != readDOIConfig(doiCfg)) {
            RDK_LOG(RDK_LOG_WARN,"LOG.RDK.VIDEOANALYTICS","%s(%d): Error reading DOI Config.\n", __FILE__, __LINE__);
        } else {
            break;
        }
        //Sleep 1 sec before retrying
        sleep(1);
        retry--;
    }

    // get doi enable flag
    if (strlen(doiCfg->enable) > 0) {
        doi_enable = atoi(doiCfg->enable);
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): getDOIConf doi_enable : %d \n", __FUNCTION__,  __LINE__,doi_enable);
    }

    // get doi threshold
    if (strlen(doiCfg->threshold) > 0) {
        doi_threshold = atoi(doiCfg->threshold);
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): getDOIConf doi_threshold: %d\n", __FUNCTION__,  __LINE__,doi_threshold);
    }

    if (doiCfg) {
        free(doiCfg);
        doiCfg = NULL;
    }

    polling_config_exit();

    return true;
}

static float getDOIOverlapThreshold()
{
    FileUtils doi_settings;
    std::string doiOverlapThresh;
    float doi_overlap_threshold = DEFAULT_DOI_OVERLAP_THRESHOLD;
    struct stat statbuf;
    if((stat(DOI_SETTINGS_FILE, &statbuf) < 0) || (!doi_settings.loadFromFile(DOI_SETTINGS_FILE)))
    {
        RDK_LOG(RDK_LOG_WARN,"LOG.RDK.XCV","%s(%d): Loading DOI settings file failed... Using default value DEFAULT_DOI_OVERLAP_THRESHOLD : %f\n", __FILE__, __LINE__, doi_overlap_threshold);
    } else {
        doi_settings.get("overlap_threshold", doiOverlapThresh);
        if(doiOverlapThresh.compare("") != 0) {
            doi_overlap_threshold = atof(doiOverlapThresh.c_str());
	    // If threshold is invalid, reset to default
	    if(doi_overlap_threshold < 0) {
		doi_overlap_threshold = DEFAULT_DOI_OVERLAP_THRESHOLD;
	    }
        }
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): DOI OVERLAP THRESHOLD: %f\n", __FILE__, __LINE__, doi_overlap_threshold);
    }
    return doi_overlap_threshold;
}

#ifdef _OBJ_DETECTION_
static float get_upscale_factor_for_delivery_blob()
{
    FileUtils delivery_settings;
    std::string upscale_factor;
    float delivery_upscale_factor = DEFAULT_DELIVERY_UPSCALE_FACTOR;
    struct stat statbuf;
    if((stat(DELIVERY_SETTINGS_FILE, &statbuf) < 0) || (!delivery_settings.loadFromFile(DELIVERY_SETTINGS_FILE)))
    {
        RDK_LOG(RDK_LOG_WARN,"LOG.RDK.XCV","%s(%d): Loading delivery settings file failed... Using default value DEFAULT_DELIVERY_UPSCALE_FACTOR\n", __FILE__, __LINE__);
    } else {
        delivery_settings.get("upscale_factor", upscale_factor);
        if(upscale_factor.compare("") != 0) {
            delivery_upscale_factor = atof(upscale_factor.c_str());
        }
        RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): UPSCALE_FACTOR: %f\n", __FILE__, __LINE__, delivery_upscale_factor);
    }
    return delivery_upscale_factor;
}
#endif

#ifdef XHB1
static bool isThumbnailResoHigh()
{
    bool retry = true;
    tn_provision_info_t *tnCfg = NULL;
    bool ret = false;

    tnCfg = (tn_provision_info_t*) malloc(sizeof(tn_provision_info_t));

    if (NULL == tnCfg) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Error allocating memory.\n", __FILE__, __LINE__);
        return false;
    }

    //Initialize polling config
    if (0 != polling_config_init()) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Error initializing polling config.\n", __FILE__, __LINE__);
        if (tnCfg) {
            free(tnCfg);
        }
        return false;
    }

    while (retry) {
        //Read EventConfig file
        if (0 != readTNConfig(tnCfg)) {
            RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Error reading TN Config.\n", __FILE__, __LINE__);
        }
        else {
            break;
        }
        //Sleep 10 sec before retrying
        sleep(10);
    }

    if(!strcmp(tnCfg->height, STN_HERS_CROP_HEIGHT) && !strcmp(tnCfg->width, STN_HERS_CROP_WIDTH)) {
        ret = true;
    }

    if (tnCfg) {
        free(tnCfg);
        tnCfg = NULL;
    }

    polling_config_exit();

    return ret;
}
#endif

/** @description: Checks if the feature is enabled via RFC
 *  @param[in] rfc_feature_fname: RFC feature filename
 *  @param[in] plane1: RFC parameter name
 *  @return: bool
 */
static bool check_enabled_rfc_feature(char*  rfc_feature_fname, char* feature_name)
{
    /* set cvr audio through RFC files */
    char value[MAX_SIZE+1] = {0};

    if((NULL == rfc_feature_fname) ||
       (NULL == feature_name)) {
	return false;
    }

    /* Check if RFC configuration file exists */
    if( RDKC_SUCCESS == IsRFCFileAvailable(rfc_feature_fname)) {
        /* Get the value from RFC file */
        if( RDKC_SUCCESS == GetValueFromRFCFile(rfc_feature_fname, feature_name, value) ) {
        value[MAX_SIZE] = '\0';
	    if( strcmp(value, RDKC_TRUE) == 0) {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): %s is enabled via RFC.\n",__FILE__, __LINE__, feature_name);
		return true;
	    } else {
		RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): %s is disabled via RFC.\n",__FILE__, __LINE__, feature_name);
                return false;
	    }
	}
    	/* If RFC file is not present, disable the featur */
    } else {
	RDK_LOG( RDK_LOG_WARN,"LOG.RDK.XCV","%s(%d): rfc feature file %s is not present.\n",__FILE__, __LINE__, rfc_feature_fname);
	return false;
    }
    return true;
}

/** @description: main function
 *  @para argv - 6 for IV engine,5 for XCV Engine
 */
int main(int argc, char* argv[])
{
    pid_t pid = 0;
    std::string libName = "libAnalytics_Comcast.so";
    int event_type = 0;
    int od_mode = 0;
    int time_now = 0;
    unsigned long long timeStamp, timeStamp2;
    unsigned long long framePTS = 0;
    char* configParam = NULL;
    int algIndex = 0;
    int frametype = 0;
    char VA_Engine[MINSIZE+1] = {0};
//    int buf_id = 0;
    int ret = 0;
    int prev_DN_mode = 0, curr_DN_mode = 0;
    int objCount = 0;
#ifndef _HAS_XSTREAM_
    PLUGIN_DayNightStatus day_night_status;
#endif
    int va_send_id = -1;
    char value[MAXSIZE+1] = {0};
    char curr_time[MAXSIZE+1] = {0};
    bool od_frame_upload_enabled = false;
    bool is_smart_thumbnail_enabled = false;
    bool rfc_smart_thumbnail_enabled = false;
    char usr_value[8] = {0};
    struct timespec frame_cap_tstamp;
    eRdkCUpScaleResolution_t resolution = UPSCALE_RESOLUTION_DEFAULT;
    SmarttnMetadata *sm = NULL;
    bool rbusEnabled = false;
    struct timespec processing_start_t, processing_end_t;
    int sleep_time =0;
    // timestamp of metadata generation
    //struct timespec metadata_gen_tstamp;
#ifdef _ROI_ENABLED_
    std::string coords;
#endif

    memset(&frame_cap_tstamp, 0, sizeof(struct timespec));
    //memset(&metadata_gen_tstamp, 0, sizeof(struct timespec));

#ifdef ENABLE_TEST_HARNESS
    int fps = 100, fileNum = 0, frameNum = 0;
    THInterface *th  = new THInterface();
    //th  = new THInterface();
    xcvInterface::set_thinterface(th);
    int32_t clipSize = CLIP_DURATION;
    int fileState = 0;
#ifdef _OBJ_DETECTION_
//  unsigned long long lastClipEndTime = 0;
    int lastClipEndFrame = 0;
    char clipname[MAXSIZE];
	
#endif
#endif

    // Init signal handler
    (void) signal(SIGTERM, self_term);
    (void) signal(SIGUSR1, reload_config);
/* Registering callback function for Breakpadwrap Function */
#ifdef BREAKPAD
    sleep(1);
    BreakPadWrapExceptionHandler eh;
    eh = newBreakPadWrapExceptionHandler();
#endif

    t2_init("videoAnalytics");
    char exe[MAXSIZE+1] = {0};
    char *exe_name = NULL;
    strncpy(exe,argv[0],MAXSIZE);
    exe[MAXSIZE] = '\0';
    // last occurence of '/'
    char *idx = strrchr(exe, '/');
    if(idx) {
	exe_name = &exe[idx-exe+1];
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d): exe name %s\n", __FILE__, __LINE__, exe_name);
    } else {
	exe_name = exe;
	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d): : exe name\n", __FILE__, __LINE__, exe_name);
    }

    /* logger initilization */
    rdk_logger_init("/etc/debug.ini");
    RdkDynamicLogging dynLog;
    dynLog.initialize(exe_name);

    if(config_init() < 0) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) : Configuration manager not initialized successfully\n",__FILE__, __LINE__);
        return XCV_FAILURE;
    }
    if( RDKC_SUCCESS != RFCConfigInit() )
    {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): RFC config Init fails\n", __FILE__, __LINE__);
        //goto err_exit;
	return XCV_FAILURE;
    }
    //check if OD_FRAME_UPLOAD is enabled(true/false)
    od_frame_upload_enabled = check_enabled_rfc_feature(RFCFILE, OD_FRAMES_UPLOAD);

    rfc_smart_thumbnail_enabled = true;

    if(od_frame_upload_enabled) {
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): OD_FRAME upload is enabled. od_frame_upload_enabled %d!!!\n", __FILE__, __LINE__, od_frame_upload_enabled);
    } else {
	RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): OD_FRAME upload is not enabled. od_frame_upload_enabled %d!!!\n", __FILE__, __LINE__, od_frame_upload_enabled);
    }

    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): smart thumbnail is enabled. rfc_smart_thumbnail_enabled %d!!!\n", __FILE__, __LINE__, rfc_smart_thumbnail_enabled);

    strncpy(VA_Engine, "RDKCVA",MINSIZE);
    VA_Engine[MINSIZE] = '\0';
    frametype = YUV;
#if 0
    if( NULL != argv[1]) {
	if(5 == atoi(argv[1])) {
	    algIndex = RDKCVA;
	    strcpy(VA_Engine, "RDKCVA");
	    libName = "libAnalytics_Comcast.so";
	    frametype = YUV;
	}
	else {
	    RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) Invalid VA Engine from command line argument. VAEngine sould be 5 \n ", __FILE__, __LINE__);
	    return XCV_FAILURE;
	}
    }
    else {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) Invalid VA Engine from command line argument. Null argument passed\n ", __FILE__, __LINE__);
	return XCV_FAILURE;
    }
#endif
    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV"," VA_Engine : %s, frame_type : YUV\n", VA_Engine);

#ifdef RTMSG
    xcvInterface::rtMessageInit();
#endif

    void* lib = dlopen(libName.c_str(), RTLD_NOW);
    if (!lib) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Failed to open %s Reason : %s \n ", __FILE__, __LINE__, libName.c_str(), dlerror());
        exit(1);
    }

    CreateEngine_t* create = (CreateEngine_t*) dlsym(lib, "CreateEngine");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) Cannot create engine  %s \n ", __FILE__, __LINE__, dlsym_error);
        return XCV_FAILURE;
    }

    xcvAnalyticsEngine* engine = create();
    algIndex = engine->GetAlgIndex();

    if(XCV_SUCCESS != engine->reset(engine,frametype)) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Engine Not Created Successfully \n", __FILE__, __LINE__);
	return XCV_FAILURE;
    }

    engine->objects = (iObject *)malloc(sizeof(iObject) * OD_MAX_NUM);
    if(NULL == engine->objects) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV"," %s(%d) Malloc error while creating objects!\n", __FILE__, __LINE__);
	return XCV_FAILURE;
    }

    engine->events = (iEvent *)malloc(sizeof(iEvent) * MAX_EVENTS_NUM);
    if(NULL == engine->events) {
	RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Malloc error while creating events!\n", __FILE__, __LINE__);
	return XCV_FAILURE;
    }

#if 0
    // Init resource for share result to streaming module
    if( RDKC_SUCCESS != RFCConfigInit() ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV"," RFC config Init fails\n");
	goto err_exit;
    }
#endif
#ifdef XCAM2 
    /* use 4rth souce buffer for video-analytics */
    if(0 != (ret = iavInterfaceAPI::rdkc_source_buffer_init(IAV_SRCBUF_4, &engine->width, &engine->height))) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","init iav driver error, ret=%d\n", ret);
        goto err_exit;
    }
#elif XHB1
    /* use 4rth souce buffer for video-analytics */
    if(0 != (ret = iavInterfaceAPI::rdkc_source_buffer_init(IAV_SRCBUF_4, &engine->width, &engine->height))) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","init iav driver error, ret=%d\n", ret);
        goto err_exit;
    }
#elif XHC3
    /* use 2nd souce buffer for video-analytics */
    if(0 != (ret = iavInterfaceAPI::rdkc_source_buffer_init(IAV_SRCBUF_2, &engine->width, &engine->height))) {
        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","init iav driver error, ret=%d\n", ret);
        goto err_exit;
    }
#else
    /* use 3rd souce buffer for video-analytics */
    if(0 != (ret = iavInterfaceAPI::rdkc_source_buffer_init(IAV_SRCBUF_3, &engine->width, &engine->height))) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","init iav driver error, ret=%d\n", ret);
	goto err_exit;
    }
#endif
    /* Read and set the command line arguement, the upscale resolution. */
    if(2 == argc) {
	resolution = (eRdkCUpScaleResolution_t)atoi(argv[1]);
    }

#ifdef XHB1
    /* Commending the check for resolution of thumbnail to hardcode 400x300 as default for XHB1.*/ 
//    if(isThumbnailResoHigh() == false) {
        RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Setting upscale resolution to 640x480\n");
        resolution = UPSCALE_RESOLUTION_640_480;
  //  } else {
  //      RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Setting upscale resolution to %s\n", ((resolution == 1) ? "1280x720" : "1280x960"));
  //  }
#endif
    engine->SetUpscaleResolution(resolution);

    if(XCV_SUCCESS != engine->Init()) {
       RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Engine Init Failed\n", __FILE__, __LINE__);
       goto err_exit;
    }

#ifdef ENABLE_TEST_HARNESS
    th->THInit(engine->width,engine->height);
    th->THGetClipSize(&clipSize);
    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d) TestHarness: Clip Size : %d\n", __FILE__, __LINE__, clipSize);

#endif

#ifdef _HAS_XSTREAM_
    prev_DN_mode = iavInterfaceAPI::read_DN_mode();
#else
    prev_DN_mode = iavInterfaceAPI::read_DN_mode(&day_night_status);
#endif

    /* Check for failure */
    if( RDKC_FAILURE == prev_DN_mode ) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Unable to ready Day-Night status. Setting it as default day mode.\n",__FUNCTION__, __LINE__);
	prev_DN_mode = DEFAULT_DN_MODE;
    }

    va_send_id = iavInterfaceAPI::VA_send_init();
    if (XCV_FAILURE == va_send_id) {
	RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","rdkc_va_send_init error, va_send_id=%d\n", va_send_id);
	goto err_exit;
    }

    while (!term_flag) {
	//Check if smart thumbnail is enabled.
        is_smart_thumbnail_enabled = xcvInterface::get_smart_TN_status() || rfc_smart_thumbnail_enabled;
	if(is_smart_thumbnail_enabled) {
		RDK_LOG(RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d) smart thumbnail is enabled.\n", __FILE__, __LINE__);
                // Notify smartthumbnail if ROI has changed
#ifndef ENABLE_TEST_HARNESS
#ifdef _ROI_ENABLED_
                if(xcvInterface::get_ROI_status()) {
                    // Tokenize the coordinate list to integer vector
                    //TODO: sajna ---- roicoords to be read from xcvInterface
                    std::vector<float> tokenized_ROIcoords = (tokenize(xcvInterface::roiCoords));
                    for (int j =0; j < tokenized_ROIcoords.size(); j++){
                        RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d): Tokenized Coordinates: %f\n", __FILE__, __LINE__, tokenized_ROIcoords[j]);
                    }

                    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d): Setting ROI, Coordinates are: %s\n", __FILE__, __LINE__, xcvInterface::roiCoords);
                    if( XCV_SUCCESS == engine->SetROI(tokenized_ROIcoords)) {
#ifdef _OBJ_DETECTION_
                        // Notify smartthumbnail on ROI change
                        xcvInterface::notifySmartThumbnail(engine->GetROI());
                        RDK_LOG( RDK_LOG_DEBUG, "LOG.RDK.XCV", "%s(%d): Setting ROI is success, notifying smartthumbnail\n", __FILE__, __LINE__);
#endif
                    }
                    xcvInterface::set_ROI_status(false);
                }
#endif
#endif
	}
	//Reset objectsCount for each frame
	if( access( ENABLE_IVA_RDK_DEBUG_LOG_FILE, F_OK ) != -1 ) {
	    enable_debug = 1;
	}
	else {
	    enable_debug = 0;
	}
	objCount = 0;  //reset objectsCount for each frame
	if(reload_va_flag) {
	    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","Iva reload or first_loop\n");
#ifdef _HAS_XSTREAM_
            curr_DN_mode = iavInterfaceAPI::read_DN_mode();
#else
            curr_DN_mode = iavInterfaceAPI::read_DN_mode(&day_night_status);
#endif
	    /* Check for failure */
	    if( RDKC_FAILURE == curr_DN_mode ) {
		RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d): Unable to ready Day-Night status. Setting it as previous mode.\n",__FUNCTION__, __LINE__);
		curr_DN_mode = prev_DN_mode;
	    }

	    if( prev_DN_mode != curr_DN_mode ) {
		    RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d) Day/Night mode switching, %s mode -> %s mode!!!\n", __FILE__, __LINE__, (prev_DN_mode == 0 ? "DAY" : "NIGHT"), (curr_DN_mode == 0 ? "DAY" : "NIGHT") );
		    engine ->SetDayNightMode(curr_DN_mode);
		    engine->SetSensitivity(curr_DN_mode);
		    prev_DN_mode = curr_DN_mode;
	    }
	    else {
		    engine ->SetDayNightMode(curr_DN_mode);
		    if(XCV_SUCCESS != engine->InitOnce()) {
		        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Engine Initialization Failed\n", __FILE__, __LINE__);
		    }
		    else {
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d) Engine Initialized successfully\n", __FILE__, __LINE__);
		    }
		    engine->SetDOIOverlapThreshold(getDOIOverlapThreshold());

	    }

	    reload_va_flag  = 0;

#ifdef _OBJ_DETECTION_
	    engine -> SetDeliveryUpscaleFactor(get_upscale_factor_for_delivery_blob());
#endif

#ifndef ENABLE_TEST_HARNESS
#ifdef _ROI_ENABLED_
            //Read from event.conf and set ROI
            coords = getROICoords();
            strcpy(xcvInterface::roiCoords, coords.c_str());
            engine -> SetROI(tokenize(coords));
#endif
#endif

	    /*FILE *fp = fopen(SYSTEM_CONF, "rt");
	    if (NULL != fp) {
		PRO_GetInt(SEC_MOTION, MOTION_MODE, &od_mode, fp);
		fclose(fp);
	    }
	    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","od_mode = %d\n",od_mode);*/

	    // read md mode from config files
	    if (RDKC_SUCCESS != rdkc_get_user_setting(MD_MODE, usr_value)) {
		configParam = (char*)rdkc_envGet(MD_MODE);
	    } else {
		configParam = usr_value;
	    }

	    if(strcmp(configParam, "1") == 0) {
		od_mode = 1;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.XCV", "%s(%d): enabling md mode. od_mode[%s]\n", __FILE__, __LINE__, configParam);
	    } else {
		od_mode = 1;
		RDK_LOG( RDK_LOG_INFO, "LOG.RDK.XCV", "%s(%d): enabling  md mode forcefully. od_mode[%s]\n", __FILE__, __LINE__, configParam);
	    }
            /* Check DOI configuration and set DOI accordingly */
            if (0 == access(DOI_CONFIG_FILE, F_OK)) {
                if(true == getDOIConf()) {
                    RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): getDOIConf success!!\n", __FUNCTION__, __LINE__);
                    if( 1 == doi_enable ) {
                        if( true == engine->applyDOIthreshold(true, DEFAULT_DOI_BITMAP_PATH, doi_threshold) ) {
                            RDK_LOG( RDK_LOG_INFO,"LOG.RDK.VIDEOANALYTICS","%s(%d): applyDOIthreshold success!!\n", __FUNCTION__, __LINE__);
                        }
                    }
#ifdef _OBJ_DETECTION_
		    xcvInterface::notifySmartThumbnail(doi_enable, DEFAULT_DOI_BITMAP_PATH, doi_threshold);
#endif
                }
            }
	}

        if(xcvInterface::get_DOI_status()) {
            RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d) DOI changed: %d, %s, %d\n", __FILE__, __LINE__, xcvInterface::doienabled, xcvInterface::doibitmapPath, xcvInterface::doiBitmapThreshold);
            engine->applyDOIthreshold(xcvInterface::doienabled, xcvInterface::doibitmapPath, xcvInterface::doiBitmapThreshold);
#ifdef _OBJ_DETECTION_
	    xcvInterface::notifySmartThumbnail(xcvInterface::doienabled, xcvInterface::doibitmapPath, xcvInterface::doiBitmapThreshold);
#endif
            xcvInterface::set_DOI_status(false);
        }

#ifdef ENABLE_TEST_HARNESS
    th->reset_TH_object_event();
    if(th->THGetFileFeedEnabledParam() == true) {
        framePTS ++;
        engine->framePTS ++;
        while(VA_FAILURE == (fileState = th->THGetFrame(&engine->plane0, &engine->plane1, fps))) {
	    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.XCV", "%s(%d): GetFrame failed... triggering retry\n", __FILE__, __LINE__);
            sleep(2);
        }
        if(RDKC_FILE_START == fileState) {
            fileNum++;
            frameNum = 0;
        }
#ifdef _OBJ_DETECTION_
//    if(lastClipEndTime == framePTS) {

    /* At first OR After CLIP_GEN_END send CLIP_GEN_START to smartthumbnail */
    if(lastClipEndFrame == frameNum) {
        struct timespec start_t;
        struct tm* tv;

        memset(clipname, '\0', sizeof(clipname));
        clock_gettime(CLOCK_REALTIME, &start_t);
        tv = gmtime(&start_t.tv_sec);
        snprintf(clipname, sizeof(clipname), "%04d%02d%02d%02d%02d%02d", (tv->tm_year+1900), tv->tm_mon+1, tv->tm_mday, tv->tm_hour, tv->tm_min, tv->tm_sec);
        int rc = xcvInterface::notifySmartThumbnail_ClipStatus(th_CLIP_GEN_START, clipname);
        if(rc) {
            RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError failed to notifySmartThubnail with upload CLIP_GEN_START. Clipname : %s\n",__FILE__,__LINE__, clipname);
        } else {
            RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","%s(%d):\tNotifySmartThubnail with upload CLIP_GEN_START. Clipname : %s\n",__FILE__,__LINE__, clipname);
        }
    }
#endif
	frameNum ++;
        if(th->THIsROICoordinatesChanged()) {
            RDK_LOG( RDK_LOG_INFO, "LOG.RDK.XCV", "%s(%d): ROI changed \n", __FILE__, __LINE__);
            engine -> SetROI(tokenize(th->THGetROICoordinates()));
#ifdef _OBJ_DETECTION_
            xcvInterface::notifySmartThumbnail(engine->GetROI());
#endif
        }
	if(th->THIsDOIChanged()) {
	    RDK_LOG( RDK_LOG_INFO, "LOG.RDK.XCV", "%s(%d): DOI changed \n", __FILE__, __LINE__);
	    int threshold = 0;
	    bool enabled = th->THGetDOIConfig(threshold);
	    engine->applyDOIthreshold(enabled, DEFAULT_DOI_BITMAP_PATH, threshold);
#ifdef _OBJ_DETECTION_
            xcvInterface::notifySmartThumbnail(enabled, DEFAULT_DOI_BITMAP_PATH, threshold);
#endif
	}
    }
    else {
#endif

        // Frame processing start time
        clock_gettime(CLOCK_REALTIME, &processing_start_t);

	if( YUV ==  frametype ) {
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV"," %s(%d)  I m reading YUV frame\n",  __FILE__, __LINE__);
		int status = iavInterfaceAPI::rdkc_get_yuv_frame(&engine->framePTS, &engine->plane0, &engine->plane1);
		if( status == XCV_OTHER) {
			RDK_LOG(RDK_LOG_INFO,"LOG.RDK.XCV"," %s(%d) YUV frame not ready, retrying \n",  __FILE__, __LINE__);
			sleep(10);
			continue;
		}
	}

#ifdef ENABLE_TEST_HARNESS
	th->convert_to_TH_planes(&engine->plane0, &engine->plane1);
    }
#endif
	// initialize dateTime with current date and time
	//get_iDateTime(&dateTime);
	// initialize timeStamp with current frame timestamp in ms
	// Below "timeStamp" and "timeStamp2" are needed to control the frame rate to analytics engine. Commenting the code now to make it similar to stable2/release.
#ifdef OSI
        timeStamp = getCurrentTime(NULL);
#else
	timeStamp = sc_linear_time(NULL);
#endif
	//RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","current timestamp: %dms\n", timeStamp);
	//Fetch frame
	//curr_time = 0;
	//if (RDKC_SUCCESS != xcvInterface::get_current_time(&curr_time)) {
	//    goto err_exit;
	//}

	//Reset VAI results structure
	xcvInterface::reset_vai_results();

#ifdef RTMSG
        memset(&frame_cap_tstamp, 0, sizeof(struct timespec));
        memset(curr_time, 0, sizeof(curr_time));
        //clock_gettime(CLOCK_REALTIME, &smt_tstamp);
        if (RDKC_SUCCESS != xcvInterface::get_current_time(&frame_cap_tstamp)) {
            goto err_exit;
        }

        strncpy(curr_time, std::to_string(frame_cap_tstamp.tv_sec).c_str(),MAXSIZE);
        curr_time[MAXSIZE] = '\0';
        RDK_LOG(RDK_LOG_DEBUG, "LOG.RDK.XCV","(%s)%d Current time stamp: %s\n", __FILE__, __LINE__, curr_time);

        if(is_smart_thumbnail_enabled) {
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
	    if(th->THGetFileFeedEnabledParam() == true) {
		if(engine->framePTS > 10) {
			std::string cmd = "ls /tmp/THFrame_* | while read file; do   a=${file#/tmp/THFrame_}; [ \"${a%.bmp}\" -lt ";
                        cmd += std::to_string(( engine->framePTS - 10 ));
			cmd += " ] && rm -v \"$file\"; done;";
			system(cmd.c_str());
		}
		//Write frame to file
		std::string curr_frame_fname = "/tmp/THFrame_" + std::to_string(engine->framePTS) + ".bmp";
		th->THWriteFrame(curr_frame_fname);
	    }
                xcvInterface::notifySmartThumbnail(pid, engine->framePTS, fileNum, frameNum, fps);
#else
                // Send notification to smart thumbnail process to capture 720*1280 yuv data
                xcvInterface::notifySmartThumbnail(pid, engine->framePTS);
#endif
        }
#endif
	engine->ProcessFrame();

	float val = 0.0;
	engine->GetMotionLevel(&val);

	(xcvInterface::get_vai_structure())->motion_level = val;
	if (od_mode == 0) {
	    usleep(SLEEPTIMER);
	    continue;
	}

	// Event type initialize
	(xcvInterface::get_vai_structure())->event_type = 0;

	/* update timestamp */
	(xcvInterface::get_vai_structure())->timestamp = engine->framePTS;
	RDK_LOG( RDK_LOG_DEBUG1, "LOG.RDK.XCV"," %s(%d):  engine->framePTS : %llu\n", __FILE__, __LINE__ ,engine->framePTS);

	// Get processing results using Standard or Advanced API calls
	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): query objects...\n",__FILE__, __LINE__);

	engine->GetObjectsCount();
	if (engine->objectsCount > 0) {
	    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): %d objects detected!\n", __FILE__, __LINE__, engine->objectsCount);
	    if(engine->objectsCount > OD_MAX_NUM) {
		engine->objectsCount = OD_MAX_NUM;
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV"," %s(%d): %d objects ignored!!!\n", __FILE__, __LINE__, engine->objectsCount - OD_MAX_NUM);
	    }

	    engine->GetObjects();
	    xcvInterface::convert_iav_results(engine->framePTS, (xcvInterface::get_vai_structure()), engine->objects, engine->objectsCount);

	    iObject* obj_ptr = engine->objects;

	    if ((engine->objects)->m_e_class == eOC_Human) {
		#ifdef OSI
        	time_now = getCurrentTime(NULL);
		#else
		time_now = sc_linear_time(NULL);
		#endif
		event_type = EVENT_TYPE_PEOPLE;
		(xcvInterface::get_vai_structure())->event_type |= 1<<EVENT_TYPE_PEOPLE;
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): People detected (xcvInterface::get_vai_structure())->event_type %d...\n",__FILE__, __LINE__, (xcvInterface::get_vai_structure())->event_type );
	    }
	    else {
		#ifdef OSI
                time_now = getCurrentTime(NULL);
		#else
		time_now = sc_linear_time(NULL);
		#endif
		event_type = EVENT_TYPE_MOTION;
		(xcvInterface::get_vai_structure())->event_type |= 1<<EVENT_TYPE_MOTION;
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Motion detected (xcvInterface::get_vai_structure())->event_type %d...\n",__FILE__, __LINE__, (xcvInterface::get_vai_structure())->event_type );
	    }


#ifdef ENABLE_TEST_HARNESS
            th->convert_to_TH_object(obj_ptr, engine->objectsCount);
#endif
	    while(engine->objectsCount-->0) {
		xcvInterface::print_object(obj_ptr++);
	    }
	}

	RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): query events...\n",__FILE__, __LINE__);

	engine->GetEventsCount();

	if (engine->eventsCount > 0) {
	    RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): %d Events detected!\n",__FILE__, __LINE__,engine->eventsCount);
	    engine->GetEvents();

	    if (engine->events->m_e_type == eSceneChange) {
			#ifdef OSI
                	time_now = getCurrentTime(NULL);
			#else
			time_now = sc_linear_time(NULL);
			#endif
#ifndef _HAS_XSTREAM_
			iavInterfaceAPI::read_DN_mode(&day_night_status);
			RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Tamper detected! day_night_status.count_time = %d, time_now=%d\n",__FILE__, __LINE__, day_night_status.count_time,time_now);

			if (compare_timestamp(time_now, day_night_status.count_time) <= 3) {
				RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","Tamper ignored due to Day/Night switch!\n");
			}
			else {
				event_type = EVENT_TYPE_TAMPER;
				(xcvInterface::get_vai_structure())->event_type |= 1<<EVENT_TYPE_TAMPER;
			}
#else
			RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Tamper detected ", __FILE__, __LINE__);
			event_type = EVENT_TYPE_TAMPER;
			(xcvInterface::get_vai_structure())->event_type |= 1<<EVENT_TYPE_TAMPER;
#endif
		}
	    else if(engine->events->m_e_type == eLightsOn) {
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) eLightsOn\n", __FILE__, __LINE__);
	    }
	    else if(engine->events->m_e_type == eLightsOff) {
		RDK_LOG(RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d) eLightsOff\n", __FILE__, __LINE__);
	    }
	    iEvent *evt_ptr = engine->events;

#ifdef ENABLE_TEST_HARNESS
            th->convert_to_TH_event(evt_ptr, engine->eventsCount);
#endif
	    while(engine->eventsCount-->0) {
		xcvInterface::print_event(evt_ptr++);
	    }
	}

#ifdef ENABLE_TEST_HARNESS
        th->THSetROIMotion(engine -> IsMotionInsideROI());
	th->THSetDOIMotion(engine -> IsMotionInsideDOI());
        RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","send required data to test-harness...\n");
        if( RDKC_FAILURE == th->THProcessFrame() ) {
            RDK_LOG( RDK_LOG_INFO,"LOG.RDK.XCV","Test-harness failed to process data. Continue with next frame.\n");
        }
#endif

	val = 0.0;
	engine->GetRawMotion(engine->framePTS,val);
	(xcvInterface::get_vai_structure())->motion_level_raw = val;
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","motion_level_raw : %f\n",val);

#ifdef RTMSG
	uint16_t levent_type = (xcvInterface::get_vai_structure())->event_type;
	// Suppress the Motion event outside ROI for CVR
	if(!engine->IsMotionInsideROI()) {
		levent_type = (levent_type) & (~(1 << EVENT_TYPE_MOTION));
	}

	if(od_frame_upload_enabled) {
		engine -> motionScore = 0.0;
		engine  -> boundingBoxXOrd = 0;
		engine -> boundingBoxYOrd = 0;
		engine -> boundingBoxHeight = 0;
		engine -> boundingBoxWidth = 0;

		engine -> GetEngineVersion();
		engine -> GetMotionScore();
		engine -> GetObjectBBoxCoords();
		engine -> GetBlobsBBoxCoords();

		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): vaEngineVersion: %s\n",__FILE__, __LINE__, engine -> vaEngineVersion);
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): motionScore: %f\n",__FILE__, __LINE__, engine  -> motionScore);
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxXOrd:%d\n",__FILE__, __LINE__ , engine -> boundingBoxXOrd);
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxYOrd:%d\n",__FILE__, __LINE__, engine  -> boundingBoxYOrd);
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxHeight:%d\n",__FILE__, __LINE__, engine -> boundingBoxHeight);
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxWidth:%d\n",__FILE__, __LINE__, engine -> boundingBoxWidth);
        RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): blobBoundingBoxCoords:\n",__FILE__, __LINE__);
        for (size_t i = 0; i < sizeof(engine -> blobBoundingBoxCoords)/sizeof(engine -> blobBoundingBoxCoords[0]); ++i){
            RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):  -- blobBoundingBoxCoords %d: [%d]\n",__FILE__, __LINE__, i, engine -> blobBoundingBoxCoords[i]);
        }
		RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Current timestamp:%d\n",__FILE__, __LINE__, curr_time);

		xcvInterface::notifyCVR(engine -> vaEngineVersion, (xcvInterface::get_vai_structure())->timestamp, levent_type, (xcvInterface::get_vai_structure())->motion_level_raw, engine ->motionScore , engine  -> boundingBoxXOrd , engine -> boundingBoxYOrd, engine -> boundingBoxHeight, engine -> boundingBoxWidth, curr_time);

	} else {
		xcvInterface::notifyCVR((xcvInterface::get_vai_structure())->timestamp, levent_type, (xcvInterface::get_vai_structure())->motion_level_raw, curr_time);
	}

	 if(is_smart_thumbnail_enabled) {
                engine -> motionScore = 0.0;
                engine  -> boundingBoxXOrd = 0;
                engine -> boundingBoxYOrd = 0;
                engine -> boundingBoxHeight = 0;
                engine -> boundingBoxWidth = 0;

                engine -> GetEngineVersion();
                engine -> GetMotionScore();
                engine -> GetObjectBBoxCoords();
                engine -> GetBlobsBBoxCoords();
#ifdef _OBJ_DETECTION_
                engine -> GetDetectionObjectBBoxCoords();
#endif

                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): vaEngineVersion: %s\n",__FILE__, __LINE__, engine -> vaEngineVersion);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): motionScore: %f\n",__FILE__, __LINE__, engine  -> motionScore);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxXOrd:%d\n",__FILE__, __LINE__ , engine -> boundingBoxXOrd);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxYOrd:%d\n",__FILE__, __LINE__, engine  -> boundingBoxYOrd);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxHeight:%d\n",__FILE__, __LINE__, engine -> boundingBoxHeight);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): boundingBoxWidth:%d\n",__FILE__, __LINE__, engine -> boundingBoxWidth);
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): blobBoundingBoxCoords:\n",__FILE__, __LINE__);
                for (size_t i = 0; i < sizeof(engine -> blobBoundingBoxCoords)/sizeof(engine -> blobBoundingBoxCoords[0]); ++i){
                    RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d):  -- blobBoundingBoxCoords %d: [%d]\n",__FILE__, __LINE__, i, engine -> blobBoundingBoxCoords[i]);
                }
                RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","%s(%d): Current timestamp:%d\n",__FILE__, __LINE__, curr_time);

                /*memset(&metadata_gen_tstamp, 0, sizeof(struct timespec));
                if (RDKC_SUCCESS != xcvInterface::get_current_time(&metadata_gen_tstamp)) {
                        goto err_exit;
                }*/
                sm = SmarttnMetadata::from_xcvInterface(engine, (xcvInterface::get_vai_structure()), curr_time);
                if(!sm) {
                    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError creating SmarttnMetadata.\n",__FILE__,__LINE__);
                } else {
                    int motionFlags = 0;

		    /** MOTION FLAGS 
                     *  Motion flags is an integer containing flags describing the motion.
                     *  Each bit corresponds to some flags.
                     *
                     *   [ 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ]
                     *                                                             | | | |
                     *                                                             | | | Is motion inside DOI
                     *                                                             | | If DOI is set
                     *                                                             | Is Motion inside ROI
                     *                                                             If ROI is set
                     *
                     * All other bit are unused. */
      
                    /* Set ROI flags */
		    motionFlags |= engine -> IsROISet();
                    motionFlags = motionFlags << 1;
		    if(engine -> IsMotionInsideROI()) {
                        motionFlags |= 0x01;
                    }
                    /* Set DOI flags */
		    motionFlags = (motionFlags << 1) | engine -> IsDOISet();
                    motionFlags = motionFlags << 1;
                    if(engine -> IsMotionInsideDOI()) {
                        motionFlags |= 0x01;
                    }

                    RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","%s(%d):\tMotion Flags: %d \n",__FILE__,__LINE__, motionFlags);
                    int rc = xcvInterface::notifySmartThumbnail(engine -> vaEngineVersion, sm, motionFlags);
                    delete sm;
                    sm = NULL;
                    if(rc) {
                        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError failed to notifySmartThubnail.\n",__FILE__,__LINE__);
                    }
                }
#if defined(ENABLE_TEST_HARNESS) && defined(_OBJ_DETECTION_)
                // Notify SMTN with CLIP_GEN_END
                if((th->THGetFileFeedEnabledParam() == true) && (((clipSize != 0) && ((frameNum - lastClipEndFrame) >= ( fps * clipSize ))) || (fileState == RDKC_FILE_END))) {
                    int rc = xcvInterface::notifySmartThumbnail_ClipStatus(th_CLIP_GEN_END, clipname);
                    if(rc) {
                        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError failed to notifySmartThubnail with upload CLIP_GEN_END.\n",__FILE__,__LINE__);
                    }
                    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tnotifySmartThubnail with upload CLIP_GEN_END.\n",__FILE__,__LINE__);
                    //Update last clip end time
                    lastClipEndFrame = frameNum;
                    rc = xcvInterface::notifySmartThumbnail_ClipUpload(clipname);
                    if(rc) {
                        RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tError failed to notifySmartThubnail with upload CLIP_UPLOAD.\n",__FILE__,__LINE__);
                    }
                    RDK_LOG( RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d):\tnotifySmartThubnail with upload CLIP_UPLOAD.\n",__FILE__,__LINE__);
		    if (fileState == RDKC_FILE_END) {
                        lastClipEndFrame = 0;
		    }
                    sleep(10);
                }
#endif

        }

#endif
	//Send VAI Results to hydra
        iavInterfaceAPI::VA_send_result(va_send_id, (xcvInterface::get_vai_structure()));

	// Log VAI Results which is sent to hydra
	vai_result_t* va_to_hydra = xcvInterface::get_vai_structure();
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","Event[%d] Motion Level[%f] Raw Motion Level[%f] Number of Detected Objects[%d]  TimeStamp[%llu]\n", va_to_hydra->event_type, va_to_hydra->motion_level, va_to_hydra->motion_level_raw, va_to_hydra->num, va_to_hydra->timestamp);

	// Below "timeStamp2" and above "timeStamp" are needed to control the frame rate to analytics engine. Commenting the code now to make it similar to stable2/release.
	#ifdef OSI
        timeStamp2 = getCurrentTime(NULL);
	#else
        timeStamp2 = sc_linear_time(NULL);
	#endif
	RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","loop cost: %llu ms\n", timeStamp2-timeStamp);
#ifdef ENABLE_TEST_HARNESS
                /* to avoid sleep only when frame is from file */
                if( th->THGetFileFeedEnabledParam() == true ) {
                        goto iva_loop_end;
                }

#endif
	/*if(timeStamp2-timeStamp < SLEEP_TIME_MAX) {
	    usleep((SLEEP_TIME_MAX-(timeStamp2-timeStamp)) * 1000);
	}
	else {
	    usleep(SLEEPTIMER);
	}*/

        // Frame processing start time
        clock_gettime(CLOCK_REALTIME, &processing_end_t);

        sleep_time = (1000000/6) - (((int)(processing_end_t.tv_nsec - processing_start_t.tv_nsec) /1000) + ((int)(processing_end_t.tv_sec - processing_start_t.tv_sec) *1000000));

	RDK_LOG( RDK_LOG_DEBUG,"LOG.RDK.XCV","sleeping %d ms\n", sleep_time);

        if(sleep_time > 0) {
	        usleep(sleep_time);
	} else {
		// sleep for 100ms after processing every frame
		usleep(SLEEPTIMER);
        }

#ifdef ENABLE_TEST_HARNESS
iva_loop_end:
#endif
RDK_LOG( RDK_LOG_DEBUG1,"LOG.RDK.XCV","............................\n");
    }
err_exit:

    engine->Shutdown();

    if(engine->objects) {
	free(engine->objects);
    }
    if(engine->events) {
        free(engine->events);
    }

#ifdef ENABLE_TEST_HARNESS
        th->THRelease();
#endif
    config_release(); // release config manager
#ifdef RTMSG
    xcvInterface::rtMessageClose();
#endif

    iavInterfaceAPI::rdkc_source_buffer_close();

    DestroyEngine_t* destroy = (DestroyEngine_t*) dlsym(lib, "DestroyEngine");
    dlsym_error = dlerror();
    if (dlsym_error) {
        RDK_LOG(RDK_LOG_ERROR,"LOG.RDK.XCV","%s(%d) Cannot destroy engine : %s \n", __FILE__, __LINE__, dlsym_error);
        return XCV_FAILURE;
    }
    destroy(engine);
    dlclose(lib);
    return XCV_SUCCESS;
}

