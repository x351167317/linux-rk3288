/******************************************************************************
 *
 * Copyright 2016, Fuzhou Rockchip Electronics Co.Ltd . All rights reserved.
 * No part of this work may be reproduced, modified, distributed, transmitted,
 * transcribed, or translated into any language or computer format, in any form
 * or by any means without written permission of:
 * Fuzhou Rockchip Electronics Co.Ltd .
 * 
 *
 *****************************************************************************/
#ifndef __AF_H__
#define __AF_H__

/**
 * @file af.h
 *
 * @brief
 *
 *****************************************************************************/
/**
 * @page module_name_page Module Name
 * Describe here what this module does.
 *
 * For a detailed list of functions and implementation detail refer to:
 * - @ref module_name
 *
 * @defgroup AFM Auto Focus Module
 * @{
 *
 */
#include <ebase/types.h>
#include <common/return_codes.h>
#include <common/cam_types.h>
#include <oslayer/oslayer.h>
#include <common/list.h>

/*
*              AF LIB VERSION NOTE
*
*Describe:
* 1)you need descirbe what you do in this version;
* 2)you nedd add keyword so that you can as possible as quickly to find the differnce.
*
*
*
*v0.1.0x00
* 1)init af.c,AfSearchFullRange is ok.
* 2)add af search table(AfPosTbl),and support 8 or 16 units.
* 3)add af reset search direction(search_dir),it can adaptive to choose infinity or marco position according to which is closer position.
* 4)tide the code,to eliminate the warning in the process of compilation.
*v0.1.0x01
* 1)correct the region of touch af.
* 2)if fStart\AfOneShot\AfStop is fail,it need't AfRelease, so rm AfRelease.
*v0.2.0x00
* 1)add touch af trigger that cause by differnce area.keyword: measureWdw.
* 2)separate Af state update and arithmetic run process from AfProcessFrame.keyword: AfUpdateState.
* 3)tide the struct for af.
* 4)complete xcam_get_results function.keyword: FinalLensPosReached.
* 5)fix the case when the window af is null. keyword: mapHalWinToIsp.
* 6)remove bssa and rename af_ctrl. keyword: bssa.
*v0.2.0x01
* 1)add af_module.h,last version is lost.
*/

#define CONFIG_AF_LIB_VERSION KERNEL_VERSION(0, 2, 1)

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct AfContext_s* AfHandle_t;     /**< handle to AF context */

#define AF_TYPE_CONTRAST (1 << 0)
#define AF_TYPE_LASER (1 << 1)
#define AF_TYPE_PDAF (1 << 2)

/*******************************************************************************
 *
 * @struct  AfSearchStrategy_t
 *
 * @brief   AF search strategy
 *
 *******************************************************************************/
typedef enum AfSearchStrategy_e {
  AFM_FSS_INVALID         = 0,
  AFM_FSS_FULLRANGE       = 1,    /**< scan the full focus range to find the point of best focus */
  AFM_FSS_HILLCLIMBING    = 2,    /**< use hillclimbing search */
  AFM_FSS_ADAPTIVE_RANGE  = 3,    /**< similar to full range search, but with multiple subsequent scans
                                         with decreasing range and step size will be performed. */
  AFM_FSS_MAX,
} AfSearchStrategy_t;

/*******************************************************************************
 *
 * @struct  AfVcmMoveStatus_t
 *
 * @brief   AF Vcm move status structure
 *
 *******************************************************************************/
typedef enum AfVcmMoveStatus_e {
  AFM_VCM_INVAL        = 0,
  AFM_VCM_MOVE_START   = 1,
  AFM_VCM_MOVE_RUNNING = 2,
  AFM_VCM_MOVE_END     = 3
} AfVcmMoveStatus_t;

/*******************************************************************************
 *
 * @struct  AfLaserMeas_t
 *
 * @brief   AF Laser measure structure
 *
 *******************************************************************************/
typedef struct AfLaserMeas_s {
  uint32_t  distance;  /* unit: mm */
  uint32_t  rev[16];
} AfLaserMeas_t;

/******************************************************************************
 *
 * @struct  AfMeasuringResult_s
 *
 * @brief   AF measuring result structure
 *
 *****************************************************************************/
typedef struct AfMeasuringResult_s {
  uint32_t    SharpnessA;         /**< sharpness of window A */
  uint32_t    SharpnessB;         /**< sharpness of window B */
  uint32_t    SharpnessC;         /**< sharpness of window C */

  uint32_t    LuminanceA;         /**< luminance of window A */
  uint32_t    LuminanceB;         /**< luminance of window B */
  uint32_t    LuminanceC;         /**< luminance of window C */

  AfVcmMoveStatus_t MoveStatus;
} AfMeasuringResult_t;

/*******************************************************************************
 *
 * @struct  AfMeas_t
 *
 * @brief   A structure to represent Laser measure parameter
 *
 *******************************************************************************/
typedef struct AfMeas_s {
  AfLaserMeas_t       laser;
  AfMeasuringResult_t cameric;

  uint32_t            meas_type;
} AfMeas_t;

typedef enum AfState_e {
  AF_STATE_INVALID        = 0,
  AF_STATE_INITIALIZED    = 1,
  AF_STATE_STOPPED        = 2,                /**< stopped */
  AF_STATE_RUNNING        = 3,                /**< searching for best lense position */
  AF_STATE_TRACKING       = 4,                /**< tracking */
  AF_STATE_LOCKED         = 5,                /**< */
  AF_STATE_DNFIRMWARE     = 6,
  AF_STATE_MAX
} AfState_t;

/******************************************************************************
 *
 * @struct  AfOutputResult_s
 *
 * @brief   A structure to represent a complete set of output values.
 *
 *****************************************************************************/
typedef struct AfOutputResult_s {
  int32_t 		   NextLensePos;
  uint32_t		   Window_Num;
  struct Cam_Win   WindowA;
  struct Cam_Win   WindowB;
  struct Cam_Win   WindowC;
  AfState_t        AfState;
  bool             FinalLensPosReached;
} AfOutputResult_t;

/******************************************************************************
 *
 * @struct  AfType_t
 *
 * @brief   AF type  structure
 *
 *****************************************************************************/
typedef struct AfType_s {
  unsigned int contrast_af:1;
  unsigned int laser_af:1;
  unsigned int pdaf:1;
} AfType_t;

/*****************************************************************************
 *
 * @struct  AfConfig_t
 *
 * @brief   AF contrast af configuration structure
 *
 *****************************************************************************/
typedef struct AfContrastAfCfg_s {
  unsigned int rev[16];
} AfContrastAfCfg_t;

/*****************************************************************************
 *
 * @struct  AfConfig_t
 *
 * @brief   AF laser af configuration structure
 *
 *****************************************************************************/
typedef struct AfLaserAfCfg_s {
  float vcmDot[7];
  float distanceDot[7];
} AfLaserAfCfg_t;

/*****************************************************************************
 *
 * @struct  AfConfig_t
 *
 * @brief   AF pdaf configuration structure
 *
 *****************************************************************************/
typedef struct AfPdafCfg_s {
  unsigned int rev[16];
} AfPdafCfg_t;


/*****************************************************************************
 *
 * @struct  AfInstanceConfig_t
 *
 * @brief   AF instance configuration structure
 *
 *****************************************************************************/
typedef struct AfInstanceConfig_s {
  AfHandle_t              hAf;            /**< handle returned by AfInit() */
} AfInstanceConfig_t;

/*****************************************************************************
 *
 * @struct  AfConfig_t
 *
 * @brief   AF configuration structure
 *
 *****************************************************************************/
typedef struct AfConfig_s {
  AfType_t           AfType;
  AfContrastAfCfg_t  ContrastAf;
  AfLaserAfCfg_t     LaserAf;
  AfPdafCfg_t        Pdaf;
  
  uint32_t           Window_Num;
  struct Cam_Win     WindowA;
  struct Cam_Win     WindowB;
  struct Cam_Win     WindowC;

  int32_t 	         LensePos;
  AfSearchStrategy_t Afss;           /**< focus search strategy */
} AfConfig_t;

typedef enum AfEvtId_e {
  AFM_AUTOFOCUS_MOVE        = 0,  /* <Notify on autofocus start and stop. This is useful in continuous > */
  AFM_AUTOFOCUS_FINISHED    = 1,
} AfEvtId_t;

typedef struct AfMoveEvt_s {
  bool_t start;
} AfMoveEvt_t;

typedef struct AfFinshEvt_s {
  bool_t focus;
} AfFinshEvt_t;

typedef struct AfEvt_s {
  AfEvtId_t            evnt_id;
  union {
    AfMoveEvt_t           mveEvt;
    AfFinshEvt_t          fshEvt;
  } info;
  void*                   pEvntCtx;
} AfEvt_t;

typedef struct AfEvtQue_s {
  List                   list;
  osQueue                queue;
} AfEvtQue_t;

/*****************************************************************************/
/**
 *          AfInit()
 *
 * @brief   This function initializes the Auto Focus Module.
 *
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_INVALID_PARM
 * @retval  RET_OUTOFMEM
 *
 *****************************************************************************/
RESULT AfInit
(
    AfInstanceConfig_t* pInstConfig
);



/*****************************************************************************/
/**
 *          AfRelease()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfRelease
(
    AfHandle_t handle
);



/*****************************************************************************/
/**
 *          AfConfigure()
 *
 * @brief   This function configures the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 * @param   pConfig
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_INVALID_PARM
 * @retval  RET_WRONG_STATE
 * @retval  RET_NOTSUPP         sensor doesn't support AF
 *
 *****************************************************************************/
RESULT AfConfigure
(
    AfHandle_t  handle,
    AfConfig_t* pConfig
);



/*****************************************************************************/
/**
 *          AfReConfigure()
 *
 * @brief   This function re-configures the Auto Focus Module
 *          after e.g. resolution change
 *
 * @param   handle  Handle to AFM
 *
 * @return  Returns the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 * @retval  RET_WRONG_HANDLE
 * @retval  RET_WRONG_STATE
 *
 *****************************************************************************/
RESULT AfReConfigure
(
	AfHandle_t  handle,
	AfConfig_t* pConfig
);



/*****************************************************************************/
/**
 * @brief   This function returns BOOL_TRUE if the AF is settled.
 *
 * @param   handle      AF instance handle
 * @param   pSettled    pointer to settled value
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfSettled
(
    AfHandle_t   handle,
    bool_t*      pSettled
);



/*****************************************************************************/
/**
 *          AfStart()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfStart
(
    AfHandle_t                handle,
    const AfSearchStrategy_t  fss
);



/*****************************************************************************/
/**
 *          AfOneShot()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfOneShot
(
    AfHandle_t                handle,
    const AfSearchStrategy_t  fss
);


/*****************************************************************************/
/**
 *          AfStop()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfStop
(
    AfHandle_t handle
);



/*****************************************************************************/
/**
 *          AfStop()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfStatus
(
    AfHandle_t           handle,
    bool_t*              pRunning,
    AfSearchStrategy_t*  pFss,
    float*               sharpness
);

/******************************************************************************
 * AfMeasureCbRestart()
 *****************************************************************************/
RESULT AfMeasureCbRestart
(
    AfHandle_t                  handle
);

/*****************************************************************************/
/**
 *          AfProcessFrame()
 *
 * @brief   The function releases/frees the Auto Focus Module.
 *
 * @param   handle  Handle to AFM
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfProcessFrame
(
    AfHandle_t handle,
    AfMeas_t*  pMeasResults
);

/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle  AF instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfGetResult
(
    AfHandle_t                  handle,
    AfOutputResult_t*           pOutputResults
);


/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle  AF instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfTryLock
(
    AfHandle_t handle
);



/*****************************************************************************/
/**
 * @brief   The function
 *
 * @param   handle  AF instance handle
 *
 * @return  Return the result of the function call.
 * @retval  RET_SUCCESS
 * @retval  RET_FAILURE
 *
 *****************************************************************************/
RESULT AfUnLock
(
    AfHandle_t handle
);


/******************************************************************************
 * AfRegisterEvtQue
 *****************************************************************************/
RESULT AfRegisterEvtQue
(
    AfHandle_t                  handle,
    AfEvtQue_t*                 evtQue
);

/******************************************************************************
 * AfReset()
 *****************************************************************************/
RESULT AfReset
(
    AfHandle_t                handle
);

#ifdef __cplusplus
}
#endif

/* @} AFM */


#endif /* __AF_H__*/
