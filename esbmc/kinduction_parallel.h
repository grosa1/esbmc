/*
 * K_induction_parallel.h
 *
 *  Created on: 27/06/2013
 *      Author: ceteli04
 */

#ifndef K_INDUCTION_PARALLEL_H_
#define K_INDUCTION_PARALLEL_H_

#include <signal.h>
#include <sys/wait.h>

#include <goto-programs/goto_functions.h>

#include "bmc.h"

const unsigned int MAX_STEPS=50;

enum STEP { BASE_CASE, FORWARD_CONDITION, INDUCTIVE_STEP };

struct resultt
{
  STEP step;
  short result;
  unsigned int k;
};

class base_caset
{
  public:
    base_caset(bmct &bmc,
        goto_functionst &goto_functions);

    resultt startSolving();

  private:
    unsigned int _k;
    bmct &_bmc;
    goto_functionst &_goto_functions;
};

class forward_conditiont
{
  public:
    forward_conditiont(bmct &bmc,
        goto_functionst &goto_functions);

    resultt startSolving();

  private:
    unsigned int _k;
    bmct &_bmc;
    goto_functionst &_goto_functions;
};

class inductive_stept
{
  public:
    inductive_stept(bmct &bmc,
        goto_functionst &goto_functions);

    resultt startSolving();

  private:
    unsigned int _k;
    bmct &_bmc;
    goto_functionst &_goto_functions;
};

#endif /* K_INDUCTION_PARALLEL_H_ */
