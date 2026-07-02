
/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: hil.h
 *
 * Code generated for Simulink model 'hil'.
 *
 * Model version                  : 1.6
 * Simulink Coder version         : 26.1 (R2026a) 20-Nov-2025
 * C/C++ source code generated on : Wed Jul  1 16:23:31 2026
 *
 * Target selection: ert.tlc
 * Embedded hardware selection: ARM Compatible->ARM Cortex-M
 * Emulation hardware selection:
 *    Differs from embedded hardware (Custom Processor->MATLAB Host Computer)
 * Code generation objectives: Unspecified
 * Validation result: Not run
 */

#ifndef hil_h_
#define hil_h_
#ifndef hil_COMMON_INCLUDES_
#define hil_COMMON_INCLUDES_
#include "rtwtypes.h"
// #include "rtw_continuous.h"
// #include "rtw_solver.h"
#include "math.h"
#endif                                 /* hil_COMMON_INCLUDES_ */

#include "hil_types.h"

/* Block states (default storage) for system '<Root>' */
typedef struct {
  real_T DiscreteTransferFcn_states;   /* '<Root>/Discrete Transfer Fcn' */
} DW_hil_T;

/* External inputs (root inport signals with default storage) */
typedef struct {
  real_T RpPedalSensor_Val;            /* '<Root>/RpPedalSensor_Val' */
} ExtU_hil_T;

/* External outputs (root outports fed by signals with default storage) */
typedef struct {
  real_T PpEngineSpeed_Val;            /* '<Root>/PpEngineSpeed_Val' */
} ExtY_hil_T;

/* Block states (default storage) */
extern DW_hil_T hil_DW;

/* External inputs (root inport signals with default storage) */
extern ExtU_hil_T hil_U;

/* External outputs (root outports fed by signals with default storage) */
extern ExtY_hil_T hil_Y;

/* Model entry point functions */
extern void hil_initialize(void);
extern void hil_step(void);
extern void hil_terminate(void);

/*-
 * The generated code includes comments that allow you to trace directly
 * back to the appropriate location in the model.  The basic format
 * is <system>/block_name, where system is the system number (uniquely
 * assigned by Simulink) and block_name is the name of the block.
 *
 * Use the MATLAB hilite_system command to trace the generated code back
 * to the model.  For example,
 *
 * hilite_system('<S3>')    - opens system 3
 * hilite_system('<S3>/Kp') - opens and selects block Kp which resides in S3
 *
 * Here is the system hierarchy for this model
 *
 * '<Root>' : 'hil'
 */
#endif                                 /* hil_h_ */

/*
 * File trailer for generated code.
 *
 * [EOF]
 */