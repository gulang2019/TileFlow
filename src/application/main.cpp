#include <iostream>
#include <csignal>
#include <cstring>

#include "application/model.hpp"
#include "compound-config/compound-config.hpp"
#include "util/args.hpp"

#include "tileflow/problem/problem.hpp"
#include "tileflow/mapping/mapping.hpp"
#include "tileflow/loop-analysis/nest-analysis.hpp"
#include "tileflow/model/topology.hpp"

extern bool gTerminateEval;

//--------------------------------------------//
//                    MAIN                    //
//--------------------------------------------//

int main(int argc, char* argv[])
{
  assert(argc >= 2);

  std::vector<std::string> input_files;
  std::string output_dir = ".";
  bool success = ParseArgs(argc, argv, input_files, output_dir);
  if (!success)
  {
    std::cerr << "ERROR: error parsing command line." << std::endl;
    exit(1);
  }

  auto config = new config::CompoundConfig(input_files);

  auto root = config->getRoot();

  if (root.exists("macro")) 
    TileFlow::macros = root.lookup("macro");
  
  if (root.exists("verbose"))
    root.lookupValue("verbose", TileFlow::verbose_level);
  
  auto problem = root.lookup("problem");
  problem::TileFlow::Workloads workloads;

  config::CompoundConfigNode arch;

  if (root.exists("arch"))
  {
    arch = root.lookup("arch");
  }
  else if (root.exists("architecture"))
  {
    arch = root.lookup("architecture");
  }
  
  bool is_sparse_topology = root.exists("sparse_optimizations");

  model::Engine::Specs arch_specs_ = model::Engine::ParseSpecs(arch, is_sparse_topology);

  if (root.exists("ERT"))
  {
    std::cout << "Found Accelergy ERT (energy reference table), replacing internal energy model." << std::endl;
    auto ert = root.lookup("ERT");
    arch_specs_.topology.ParseAccelergyERT(ert);
    if (root.exists("ART")){ // Nellie: well, if the users have the version of Accelergy that generates ART
      auto art = root.lookup("ART");
      arch_specs_.topology.ParseAccelergyART(art);  
    }
  }

  std::cout << "Begin ParseWorkload..." << std::endl;
  problem::TileFlow::ParseWorkloads(problem, workloads);
  problem::Workload::SetCurrShape(&workloads.get_shape());

  if (TileFlow::verbose_level)
    workloads.Print();

  auto mapping = mapping::TileFlow::ParseAndConstruct(root.lookup("mapping"), arch_specs_, workloads);
  
  if (TileFlow::verbose_level)
    mapping.Print();


  model::TileFlow::Topology topology_;

  std::cout << "Begin Spec..." << std::endl; 
  topology_.Spec(arch_specs_.topology);

  analysis::TileFlow::NestAnalysis analysis(workloads, mapping, arch_specs_, topology_);
  analysis.analyze();
  
  if (TileFlow::verbose_level)
    analysis.Print();
  
  analysis.Report();

  if (root.exists("output"))  {
    std::string filename;
    root.lookupValue("output", filename);
    analysis.Export(filename);
  }
  
  return 0;
}
