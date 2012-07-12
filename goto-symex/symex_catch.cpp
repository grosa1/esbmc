/*******************************************************************\

Module: Symbolic Execution

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "goto_symex.h"

/*******************************************************************\

Function: goto_symext::symex_catch

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_symext::symex_catch()
{
  // there are two variants: 'push' and 'pop'
  const goto_programt::instructiont &instruction= *cur_state->source.pc;

  if(instruction.targets.empty()) // pop
  {
    if(cur_state->call_stack.empty())
      throw "catch-pop on empty call stack";

    if(cur_state->top().catch_map.empty())
      throw "catch-pop on function frame";

    // pop the stack frame
    has_throw_target = true;
    cur_state->call_stack.pop_back();
  }
  else // push
  {
    cur_state->call_stack.push_back(goto_symex_statet::framet(cur_state->source.thread_nr));
    goto_symex_statet::framet &frame=cur_state->call_stack.back();

    // copy targets
    const irept::subt &exception_list=
      instruction.code.find("exception_list").get_sub();

    assert(exception_list.size()==instruction.targets.size());

    unsigned i=0;

    for(goto_programt::targetst::const_iterator
        it=instruction.targets.begin();
        it!=instruction.targets.end();
        it++, i++)
      frame.catch_map[exception_list[i].id()]=*it;
  }
}

/*******************************************************************\

Function: goto_symext::symex_throw

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void goto_symext::symex_throw()
{
  const goto_programt::instructiont &instruction= *cur_state->source.pc;

  // get the list of exceptions thrown
  const irept::subt &exceptions_thrown=
    instruction.code.find("exception_list").get_sub();

  // go through the call stack, beginning with the top

  for(goto_symex_statet::call_stackt::const_reverse_iterator
      s_it=cur_state->call_stack.rbegin();
      s_it!=cur_state->call_stack.rend();
      s_it++)
  {
    const goto_symex_statet::framet &frame=*s_it;

    if(frame.catch_map.empty()) continue;

    for(irept::subt::const_iterator
        e_it=exceptions_thrown.begin();
        e_it!=exceptions_thrown.end();
        e_it++)
    {
      goto_symex_statet::framet::catch_mapt::const_iterator
      c_it=frame.catch_map.find(e_it->id());

      if(c_it!=frame.catch_map.end())
      {
        throw_target = (*c_it).second;
      }
      else
      {
        // An un-caught exception. Behaves like assume(0);
        cur_state->guard.add(false_exprt());
        exprt tmp=cur_state->guard.as_expr();
        target->assumption(cur_state->guard, tmp, cur_state->source);
      }
    }
  }
}
