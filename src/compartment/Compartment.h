#ifndef ENISI_MSM_COMPARTMENT_H
#define ENISI_MSM_COMPARTMENT_H

#include "agent/ENISIAgent.h"
#include "agent/AgentPackage.h"
#include "grid/Iterator.h"

namespace ENISI {

class DiffuserLayer;
class Cytokine;
class SharedValueLayer;
class GroupInterface;
class DiffuserImpl;

template <class T, class Package, class PackageExchange> class ICompartmentLayer;

class Compartment
{
private:
  static Compartment* INSTANCES[];

  typedef ICompartmentLayer< Agent, AgentPackage, AgentPackageExchange > SharedLayer;

public:
  typedef boost::filter_iterator<repast::IsLocalAgent< Agent >, repast::SharedContext< Agent >::const_iterator> LocalIterator;

  static const char* Names[];
  enum Type{lumen, epithilium, lamina_propria, gastric_lymph_node, INVALID = -1};

  struct sProperties
  {
    double spaceX;
    double spaceY;
    double gridSize;
    double gridX;
    double gridY;
    Type borderLowCompartment;
    Borders::Type borderLowType;
    Type borderHighCompartment;
    Borders::Type borderHighType;
  };

  static Compartment* instance(const Type & type);

  Compartment(const Type & type);

  virtual ~Compartment();


  const repast::GridDimensions & spaceDimensions() const;
  const repast::GridDimensions & gridDimensions() const;
  const repast::GridDimensions & localSpaceDimensions() const;
  const repast::GridDimensions & localGridDimensions() const;
  const Borders * spaceBorders() const;
  const Borders * gridBorders() const;
  const Compartment * getAdjacentCompartment(const Borders::Coodinate &coordinate, const Borders::Side & side) const;
  Iterator begin();

  double gridToSpace(const Borders::Coodinate &coordinate, const int & grid) const;
  std::vector< double > gridToSpace(const std::vector< int > & grid) const;

  int spaceToGrid(const Borders::Coodinate &coordinate, const double & space) const;
  std::vector<int> spaceToGrid(const std::vector<double> & space) const;

  void getLocation(const repast::AgentId & id, std::vector<double> & Location) const;
  bool moveTo(const repast::AgentId &id, repast::Point< double > &pt);
  bool moveTo(const repast::AgentId &id, std::vector< double > &newLocation);
  bool moveRandom(const repast::AgentId &id, const double & maxSpeed);
  bool addAgent(Agent * agent, const std::vector< double > & pt);
  bool addAgentToRandomLocation(Agent * agent);
  void removeAgent (Agent * pAgent);

  void getNeighbors(const repast::Point< int > &pt, unsigned int range, std::vector< Agent * > &out);
  void getNeighbors(const repast::Point< int > &pt, unsigned int range, const int & types, std::vector< Agent * > &out);
  void getAgents(const repast::Point< int > &pt, std::vector< Agent * > &out);
  void getAgents(const repast::Point< int > &pt, const int & types, std::vector< Agent * > &out);
  void getAgents(const repast::Point< int > &pt, const int & xOffset, const int & yOffset, std::vector< Agent * > &out);
  void getAgents(const repast::Point< int > &pt, const int & xOffset, const int & yOffset, const int & types, std::vector< Agent * > &out);

  size_t addCytokine(const std::string & name);
  const std::vector< Cytokine * > & getCytokines() const;
  const Cytokine * getCytokine(const std::string & name) const;

  double & cytokineValue(const std::string & name, const repast::Point< int > & pt);
  double & cytokineValue(const std::string & name, const repast::Point< int > & pt, const int & xOffset, const int & yOffset);
  std::vector< double > & cytokineValues(const repast::Point< int > & pt);

  void initializeDiffuserData();
  SharedValueLayer * getDiffuserData();

  void synchronizeCells();
  void synchronizeDiffuser();

  const Type & getType() const;

  size_t localCount(const double & concentration);

  std::set< size_t > getRanks(const std::vector< int > & location,
                              const Borders::Coodinate & coordinate,
                              const Borders::Side & side) const;
  void bisect(const Borders::Coodinate & coordinate,
              const std::vector< int > & low,
              const std::vector< int > & high,
              std::set< size_t > & ranks) const;
  size_t getRank(const std::vector< int > & location) const;
  size_t getRank(const std::vector< int > & location, const int & xOffset, const int & yOffset) const;

  virtual void write(const std::string & separator);

  void getBorderCellsToPush(std::set<repast::AgentId>& agentsToTest,
                            std::map< int, std::set< repast::AgentId > > & agentsToPush);

  void getBorderValuesToPush(std::set<repast::AgentId>& agentsToTest,
                             std::map< int, std::set< repast::AgentId > > & agentsToPush);

  LocalIterator localBegin();
  LocalIterator localEnd();

  void addGroup(GroupInterface * pGroup);
  void act();
  void intervene();
  void diffuse();

  std::string getName() const;

private:
  void determineProcessDimensions();
  void getBorderCellsToPush(const Borders::Coodinate &coordinate,
                            const Borders::Side & side,
                             std::map< int, std::set< repast::AgentId > > & agentsToPush);

  void getBorderValuesToPush(const Borders::Coodinate &coordinate,
                             const Borders::Side & side,
                             std::map< int, std::set< repast::AgentId > > & agentsToPush);

  Compartment * transform(std::vector< double > & pt) const;
  Compartment * transform(std::vector< int > & pt) const;
  Compartment * mapToOtherCompartment(std::vector< double > & pt,
                                      const std::vector< Borders::BoundState > & BoundState) const;

  Type mType;

  sProperties mProperties;
  std::vector<int> mProcessDimensions;
  repast::GridDimensions mSpaceDimensions;
  repast::GridDimensions mGridDimensions;
  SharedLayer * mpLayer;
  Borders * mpSpaceBorders;
  Borders * mpGridBorders;

  std::vector< std::vector< Type > > mAdjacentCompartments;
  repast::DoubleUniformGenerator mUniform;

  std::map< std::string, size_t > mCytokineMap;
  std::vector< Cytokine * > mCytokines;

  SharedValueLayer * mpDiffuserValues;
  std::vector< GroupInterface * > mGroups;
  DiffuserImpl * mpDiffuser;

  bool mNoLocalAgents;

}; /* end Compartment */

}
#endif
