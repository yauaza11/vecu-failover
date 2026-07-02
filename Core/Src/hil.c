
/*
 * Academic License - for use in teaching, academic research, and meeting
 * course requirements at degree granting institutions only.  Not for
 * government, commercial, or other organizational use.
 *
 * File: hil.c
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

#include "hil.h"

/* Block states (default storage) */
DW_hil_T hil_DW;

/* External inputs (root inport signals with default storage) */
ExtU_hil_T hil_U;

/* External outputs (root outports fed by signals with default storage) */
ExtY_hil_T hil_Y;

/* Model step function */
void hil_step(void)
{
  /* Outport: '<Root>/PpEngineSpeed_Val' incorporates:
   *  DiscreteTransferFcn: '<Root>/Discrete Transfer Fcn'
   */
  hil_Y.PpEngineSpeed_Val = 0.01961 * hil_DW.DiscreteTransferFcn_states;

  /* Update for DiscreteTransferFcn: '<Root>/Discrete Transfer Fcn' incorporates:
   *  Gain: '<Root>/Gain'
   *  Inport: '<Root>/RpPedalSensor_Val'
   */
  hil_DW.DiscreteTransferFcn_states = 40.0 * hil_U.RpPedalSensor_Val - -0.9804 *
    hil_DW.DiscreteTransferFcn_states;
}

/* Model initialize function */
void hil_initialize(void)
{
  /* (no initialization code required) */
}

/* Model terminate function */
void hil_terminate(void)
{
  /* (no terminate code required) */
}

/*
 * File trailer for generated code.
 *
 * [EOF]
 */