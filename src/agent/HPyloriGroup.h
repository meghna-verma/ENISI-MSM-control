#ifndef ENISI_MSM_AGENT_HPYLORIGROUP_H
#define ENISI_MSM_AGENT_HPYLORIGROUP_H

#include "agent/AgentStates.h"
#include "repast_hpc/Point.h"
#include "agent/GroupInterface.h"

namespace ENISI
{

class HPyloriGroup: public GroupInterface
{
public:
  HPyloriGroup(Compartment * pCompartment, const double & concentrations);
  virtual std::string classname() const {return "HPyloriGroup";}

protected:
  virtual void act(const repast::Point<int> &);
  virtual void move();
  virtual void intervene(const repast::Point<int> &);
  virtual void write(const repast::Point<int> & pt);

private:
  double p_HPepitoLP;
  double p_HPyloricap;
  double p_HPylorirep;
  double p_HPdeathduetoTcells;
  double p_HPyloriDeath;
};

} // namespace ENISI

#endif
