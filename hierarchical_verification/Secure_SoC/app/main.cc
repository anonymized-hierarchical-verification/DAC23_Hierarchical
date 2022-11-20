#include <aes_ila.h>
#include <secure_soc.h>

#include <ilang/vtarget-out/vtarget_gen.h>
#include <ilang/ilang++.h>
#include <ilang/target-json/interface.h>
#include <ilang/util/log.h>
#include <iostream>
#include <simple/lib.h>
using namespace std;

using namespace ilang;

/// the function to generate configuration
VerilogVerificationTargetGenerator::vtg_config_t SetConfiguration();
VerilogVerificationTargetGenerator::vtg_config_t HandleArguments(int argc, char **argv);
void verify_top_ila_vs_rtl(
  Ila& model, 
  VerilogVerificationTargetGenerator::vtg_config_t vtg_cfg,
  const std::vector<std::string> & design_files);
void verify_top_ila_vs_comp(
  Ila& model, 
  VerilogVerificationTargetGenerator::vtg_config_t vtg_cfg,
  const std::vector<std::string> & design_files);

/// Build the model
int main(int argc, char **argv) {
  auto vtg_cfg = SetConfiguration();
  //auto vtg_cfg = HandleArguments(argc, argv);

  // Top level model for 8051
  auto proc_file = "../src/oc8051.json";
  auto proc_ila = IlaSerDesMngr::DesFromFile(proc_file);
  std::cout << "oc8051 loaded" << std::endl;

  std::vector<ExprPtr> tmp_iram_elements;
  ModifyIramInterface(proc_ila, tmp_iram_elements);
  std::cout << "oc8051 iram modified" << std::endl;
  AddIromInputPort(proc_ila);
  std::cout << "oc8051 irom modified" << std::endl;
  AddXramOutPort(proc_ila);
  std::cout << "oc8051 xram modified" << std::endl;
  for (int i = 0; i < 255; i++) {
    Model8051RemapMemInterface(proc_ila, tmp_iram_elements, i);
    // cout << "instr " << i << " has done." << endl; 
  }

  // Top level model for AES
  AES aes_ila;
  std::cout << "aes loaded" << std::endl;

  // Top level model for SoC
  auto secure_soc_ila = GetSoCIla("secure_soc");
  std::cout << "secure_soc loaded" << std::endl;

  std::string verilog_file_name = "oc8051_ila.v";
  std::ofstream fw_verilog(verilog_file_name);
  VerilogGenerator vgen(VerilogGenerator::VlgGenConfig(
      true)); // overwrite default configuration : memory is external
  vgen.ExportIla(proc_ila);
  vgen.DumpToFile(fw_verilog);
  fw_verilog.close();

  verilog_file_name = "aes_ila.v";
  fw_verilog.open(verilog_file_name);
  aes_ila.model.ExportToVerilog(fw_verilog);
  fw_verilog.close();

  std::vector<std::string> design_files_rtl = {
    "soc_8051.v",
    "oc8051_gm_top.v",
    "oc8051_memarbiter.v",
    "oc8051_xiommu.v",
    "oc8051_xram.v",
    "reg2byte.v",
    "reg16byte.v",
    "aes_top.v",
    "aes_table.v",
    "aes_round.v",
    "aes_128.v"
  };
  
  std::vector<std::string> design_files_comp = {
    "secure_comp.v",
    "oc8051_ila.v",
    "aes_ila.v"
  };

  verify_top_ila_vs_rtl(secure_soc_ila, vtg_cfg, design_files_rtl);
  verify_top_ila_vs_comp(secure_soc_ila, vtg_cfg, design_files_comp);

  return 0;
}

void verify_top_ila_vs_rtl(
  Ila& model, 
  VerilogVerificationTargetGenerator::vtg_config_t vtg_cfg,
  const std::vector<std::string> & design_files
) {
  VerilogGeneratorBase::VlgGenConfig vlg_cfg;
  vlg_cfg.pass_node_name = true;
  vtg_cfg.ForceInstCheckReset = true;

  std::string RootPath    = "..";
  std::string VerilogPath = RootPath    + "/verilog/";
  std::string IncludePath = VerilogPath + "include/";
  std::string RefrelPath  = RootPath    + "/refinement/";
  std::string OutputPath  = RootPath    + "/verification/";

  std::vector<std::string> path_to_design_files;
  for(auto && f : design_files)
    path_to_design_files.push_back( VerilogPath + f );
  

  VerilogVerificationTargetGenerator vg(
      {IncludePath},                                         // one include path
      path_to_design_files,                                  // designs
      "soc_8051",                               // top_module_name
      RefrelPath + "ref-rel-var-map.json",                // variable mapping
      RefrelPath + "ref-rel-inst-cond.json",              // conditions of start/ready
      OutputPath,                                            // output path
      model.get(),                                           // model
      VerilogVerificationTargetGenerator::backend_selector::JASPERGOLD, // backend: JASPERGOLD
      vtg_cfg,  // target generator configuration
      vlg_cfg); // verilog generator configuration

  vg.GenerateTargets();
}

void verify_top_ila_vs_comp(
  Ila& model, 
  VerilogVerificationTargetGenerator::vtg_config_t vtg_cfg,
  const std::vector<std::string> & design_files) {
  VerilogGeneratorBase::VlgGenConfig vlg_cfg;
  vlg_cfg.pass_node_name = true;
  vtg_cfg.ForceInstCheckReset = true;

  std::string RootPath    = "..";
  std::string VerilogPath = RootPath    + "/verilog/composition/";
  std::string IncludePath = VerilogPath + "include/";
  std::string RefrelPath  = RootPath    + "/refinement/";
  std::string OutputPath  = RootPath    + "/verification/comp";

  std::vector<std::string> path_to_design_files;
  for(auto && f : design_files)
    path_to_design_files.push_back( VerilogPath + f );
  

  VerilogVerificationTargetGenerator vg(
      {IncludePath},                                         // one include path
      path_to_design_files,                                  // designs
      "soc_comp",                               // top_module_name
      RefrelPath + "ref-rel-var-map-comp.json",                // variable mapping
      RefrelPath + "ref-rel-inst-cond-comp.json",              // conditions of start/ready
      OutputPath,                                            // output path
      model.get(),                                           // model
      VerilogVerificationTargetGenerator::backend_selector::JASPERGOLD, // backend: JASPERGOLD
      vtg_cfg,  // target generator configuration
      vlg_cfg); // verilog generator configuration

  vg.GenerateTargets();
}


VerilogVerificationTargetGenerator::vtg_config_t HandleArguments(int argc, char **argv) {
  // the solver, the cosa environment
  // you can use a commandline parser if desired, but since it is not the main focus of
  // this demo, we skip it

  // set ilang option, operators like '<' will refer to unsigned arithmetics
  SetUnsignedComparison(true); 
  
  VerilogVerificationTargetGenerator::vtg_config_t ret;

  for(unsigned p = 1; p<argc; p++) {
    std::string arg = argv[p];
    auto split = arg.find("=");
    auto argName = arg.substr(0,split);
    auto param   = arg.substr(split+1);

    if(argName == "Solver")
      ret.CosaSolver = param;
    else if(argName == "Env")
      ret.CosaPyEnvironment = param;
    else if(argName == "Cosa")
      ret.CosaPath = param;
    // else unknown
    else {
      std::cerr<<"Unknown argument:" << argName << std::endl;
      std::cerr<<"Expecting Solver/Env/Cosa=???" << std::endl;
    }
  }

  ret.CosaGenTraceVcd = true;

  /// other configurations
  ret.PortDeclStyle = VlgVerifTgtGenBase::vtg_config_t::NEW;
  ret.CosaGenJgTesterScript = true;
  //ret.CosaOtherSolverOptions = "--blackbox-array";
  //ret.ForceInstCheckReset = true;

  return ret;
}

VerilogVerificationTargetGenerator::vtg_config_t SetConfiguration() {

  // set ilang option, operators like '<' will refer to unsigned arithmetics
  SetUnsignedComparison(true); 
  
  VerilogVerificationTargetGenerator::vtg_config_t ret;
  ret.CosaSolver = "z3";
  ret.CosaPyEnvironment = "/ibuild/ilang-env/bin/activate";
  ret.CosaPath = "/ibuild/CoSA";
  ret.CosaGenTraceVcd = true;

  /// other configurations
  ret.PortDeclStyle = VlgVerifTgtGenBase::vtg_config_t::NEW;
  ret.CosaGenJgTesterScript = true;
  //ret.CosaOtherSolverOptions = "--blackbox-array";
  //ret.ForceInstCheckReset = true;

  return ret;
}