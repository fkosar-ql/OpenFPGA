/***********************************************
 * This file includes functions to generate
 * SPICE subcircuits for multiplexers.
 * including both fundamental submodules
 * such as a branch in a multiplexer
 * and the full multiplexer
 **********************************************/
#include <algorithm>
#include <map>
#include <string>

/* Headers from vtrutil library */
#include "vtr_assert.h"
#include "vtr_log.h"

/* Headers from readarch library */
#include "physical_types.h"

/* Headers from readarcopenfpga library */
#include "circuit_types.h"

/* Headers from openfpgautil library */
#include "openfpga_digest.h"

/* Headers from openfpgashell library */
#include "circuit_library_utils.h"
#include "command_exit_codes.h"
#include "decoder_library_utils.h"
#include "module_manager.h"
#include "mux_graph.h"
#include "mux_utils.h"
#include "openfpga_naming.h"
#include "spice_constants.h"
#include "spice_mux.h"
#include "spice_subckt_writer.h"
#include "spice_writer_utils.h"

/* begin namespace openfpga */
namespace openfpga {

/***********************************************
 * Generate SPICE modeling for an branch circuit
 * for a multiplexer with the given size
 **********************************************/
static void generate_spice_mux_branch_subckt(
  const ModuleManager& module_manager, const CircuitLibrary& circuit_lib,
  std::fstream& fp, const CircuitModelId& mux_model, const MuxGraph& mux_graph,
  std::map<std::string, bool>& branch_mux_module_is_outputted) {
  std::string module_name = generate_mux_branch_subckt_name(
    circuit_lib, mux_model, mux_graph.num_inputs(), mux_graph.num_memory_bits(),
    SPICE_MUX_BASIS_POSTFIX);

  /* Skip outputting if the module has already been outputted */
  auto result = branch_mux_module_is_outputted.find(module_name);
  if ((result != branch_mux_module_is_outputted.end()) &&
      (true == result->second)) {
    return;
  }

  /* Multiplexers built with different technology is in different organization
   */
  switch (circuit_lib.design_tech_type(mux_model)) {
    case CIRCUIT_MODEL_DESIGN_CMOS: {
      /* Skip module writing if the branch subckt is a standard cell! */
      if (true == circuit_lib.valid_model_id(circuit_lib.model(module_name))) {
        /* This model must be a MUX2 gate */
        VTR_ASSERT(CIRCUIT_MODEL_GATE ==
                   circuit_lib.model_type(circuit_lib.model(module_name)));
        VTR_ASSERT(CIRCUIT_MODEL_GATE_MUX2 ==
                   circuit_lib.gate_type(circuit_lib.model(module_name)));
        break;
      }
      /* Structural verilog can be easily generated by module writer */
      ModuleId mux_module = module_manager.find_module(module_name);
      VTR_ASSERT(true == module_manager.valid_module_id(mux_module));
      write_spice_subckt_to_file(fp, module_manager, mux_module);
      /* Add an empty line as a splitter */
      fp << std::endl;
      break;
    }
    case CIRCUIT_MODEL_DESIGN_RRAM:
      /* TODO: RRAM-based Multiplexer SPICE module generation */
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "RRAM multiplexer '%s' is not supported yet\n",
                     circuit_lib.model_name(mux_model).c_str());
      exit(1);
      break;
    default:
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "Invalid design technology of multiplexer '%s'\n",
                     circuit_lib.model_name(mux_model).c_str());
      exit(1);
  }

  /* Record that this branch module has been outputted */
  branch_mux_module_is_outputted[module_name] = true;
}

/***********************************************
 * Generate SPICE modeling for a multiplexer
 * with the given graph-level description
 **********************************************/
static void generate_spice_mux_subckt(const ModuleManager& module_manager,
                                      const CircuitLibrary& circuit_lib,
                                      std::fstream& fp,
                                      const CircuitModelId& mux_model,
                                      const MuxGraph& mux_graph) {
  std::string module_name =
    generate_mux_subckt_name(circuit_lib, mux_model,
                             find_mux_num_datapath_inputs(
                               circuit_lib, mux_model, mux_graph.num_inputs()),
                             std::string(""));

  /* Multiplexers built with different technology is in different organization
   */
  switch (circuit_lib.design_tech_type(mux_model)) {
    case CIRCUIT_MODEL_DESIGN_CMOS: {
      /* Use Verilog writer to print the module to file */
      ModuleId mux_module = module_manager.find_module(module_name);
      VTR_ASSERT(true == module_manager.valid_module_id(mux_module));
      write_spice_subckt_to_file(fp, module_manager, mux_module);
      /* Add an empty line as a splitter */
      fp << std::endl;
      break;
    }
    case CIRCUIT_MODEL_DESIGN_RRAM:
      /* TODO: RRAM-based Multiplexer SPICE module generation */
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "RRAM multiplexer '%s' is not supported yet\n",
                     circuit_lib.model_name(mux_model).c_str());
      exit(1);
      break;
    default:
      VTR_LOGF_ERROR(__FILE__, __LINE__,
                     "Invalid design technology of multiplexer '%s'\n",
                     circuit_lib.model_name(mux_model).c_str());
      exit(1);
  }
}

/***********************************************
 * Generate primitive SPICE subcircuits for all the unique
 * multiplexers in the FPGA device
 **********************************************/
static int print_spice_submodule_mux_primitives(
  NetlistManager& netlist_manager, const ModuleManager& module_manager,
  const MuxLibrary& mux_lib, const CircuitLibrary& circuit_lib,
  const std::string& submodule_dir) {
  int status = CMD_EXEC_SUCCESS;

  std::string spice_fname(submodule_dir +
                          std::string(MUX_PRIMITIVES_SPICE_FILE_NAME));

  /* Create the file stream */
  std::fstream fp;
  fp.open(spice_fname, std::fstream::out | std::fstream::trunc);

  check_file_stream(spice_fname.c_str(), fp);

  /* Print out debugging information for if the file is not opened/created
   * properly */
  VTR_LOG("Writing SPICE netlist for Multiplexer primitives '%s' ...",
          spice_fname.c_str());

  print_spice_file_header(fp, "Multiplexer primitives");

  /* Record if the branch module has been outputted
   * since different sizes of routing multiplexers may share the same branch
   * module
   */
  std::map<std::string, bool> branch_mux_module_is_outputted;

  /* Generate basis sub-circuit for unique branches shared by the multiplexers
   */
  for (auto mux : mux_lib.muxes()) {
    const MuxGraph& mux_graph = mux_lib.mux_graph(mux);
    CircuitModelId mux_circuit_model = mux_lib.mux_circuit_model(mux);
    /* Create a mux graph for the branch circuit */
    std::vector<MuxGraph> branch_mux_graphs =
      mux_graph.build_mux_branch_graphs();
    /* Create branch circuits, which are N:1 one-level or 2:1 tree-like MUXes */
    for (auto branch_mux_graph : branch_mux_graphs) {
      generate_spice_mux_branch_subckt(module_manager, circuit_lib, fp,
                                       mux_circuit_model, branch_mux_graph,
                                       branch_mux_module_is_outputted);
    }
  }

  /* Close the file stream */
  fp.close();

  /* Add fname to the netlist name list */
  NetlistId nlist_id = netlist_manager.add_netlist(spice_fname);
  VTR_ASSERT(NetlistId::INVALID() != nlist_id);
  netlist_manager.set_netlist_type(nlist_id, NetlistManager::SUBMODULE_NETLIST);

  VTR_LOG("Done\n");

  return status;
}

/***********************************************
 * Generate top-level SPICE subcircuits for all the unique
 * multiplexers in the FPGA device
 **********************************************/
static int print_spice_submodule_mux_top_subckt(
  NetlistManager& netlist_manager, const ModuleManager& module_manager,
  const MuxLibrary& mux_lib, const CircuitLibrary& circuit_lib,
  const std::string& submodule_dir) {
  int status = CMD_EXEC_SUCCESS;

  std::string spice_fname(submodule_dir + std::string(MUXES_SPICE_FILE_NAME));

  /* Create the file stream */
  std::fstream fp;
  fp.open(spice_fname, std::fstream::out | std::fstream::trunc);

  check_file_stream(spice_fname.c_str(), fp);

  /* Print out debugging information for if the file is not opened/created
   * properly */
  VTR_LOG("Writing SPICE netlist for Multiplexers '%s' ...",
          spice_fname.c_str());

  print_spice_file_header(fp, "Multiplexers");

  /* Generate unique Verilog modules for the multiplexers */
  for (auto mux : mux_lib.muxes()) {
    const MuxGraph& mux_graph = mux_lib.mux_graph(mux);
    CircuitModelId mux_circuit_model = mux_lib.mux_circuit_model(mux);
    /* Create MUX circuits */
    generate_spice_mux_subckt(module_manager, circuit_lib, fp,
                              mux_circuit_model, mux_graph);
  }

  /* Close the file stream */
  fp.close();

  /* Add fname to the netlist name list */
  NetlistId nlist_id = netlist_manager.add_netlist(spice_fname);
  VTR_ASSERT(NetlistId::INVALID() != nlist_id);
  netlist_manager.set_netlist_type(nlist_id, NetlistManager::SUBMODULE_NETLIST);

  VTR_LOG("Done\n");

  return status;
}

/***********************************************
 * Generate SPICE modules for all the unique
 * multiplexers in the FPGA device
 * Output to two SPICE netlists:
 * - A SPICE netlist contains all the primitive
 *   cells for build the routing multiplexers
 * - A SPICE netlist contains all the top-level
 *   module for routing multiplexers
 **********************************************/
int print_spice_submodule_muxes(NetlistManager& netlist_manager,
                                const ModuleManager& module_manager,
                                const MuxLibrary& mux_lib,
                                const CircuitLibrary& circuit_lib,
                                const std::string& submodule_dir) {
  int status = CMD_EXEC_SUCCESS;

  status = print_spice_submodule_mux_primitives(
    netlist_manager, module_manager, mux_lib, circuit_lib, submodule_dir);

  if (CMD_EXEC_FATAL_ERROR == status) {
    return status;
  }

  status = print_spice_submodule_mux_top_subckt(
    netlist_manager, module_manager, mux_lib, circuit_lib, submodule_dir);

  if (CMD_EXEC_FATAL_ERROR == status) {
    return status;
  }

  return status;
}

} /* end namespace openfpga */
