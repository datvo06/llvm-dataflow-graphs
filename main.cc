#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/CommandLine.h>

#include "utils/llvm_utils.h"
#include "flow/dataflow.h"


static void printDotDataFlowGraph(llvm::Module& M){
	// Create MPM and add Wrapper
	llvm::ModulePassManager MPM;
	datautils::DataWorkerWrapper dataWrapper;
	MPM.addPass(dataWrapper);

	// Create analysis manager and register the analysis pass
	llvm::ModuleAnalysisManager MAM;
	MAM.registerPass([&] { return datautils::DataWorker();});

	// Register MAM 
	llvm::PassBuilder PB;
	PB.registerModuleAnalyses(MAM);
	MPM.run(M, MAM);
}


static llvm::cl::OptionCategory DataFlowCategory{"dataflow options"};
static llvm::cl::opt<std::string> InputModule{
		llvm::cl::Positional,
		llvm::cl::desc{"<Module to analyze>"},
		llvm::cl::value_desc{"bitcode filename"},
		llvm::cl::init(""),
		llvm::cl::Required,
		llvm::cl::cat{DataFlowCategory}
};



int main(int argc, char* argv[])
{
	llvm::cl::HideUnrelatedOptions(DataFlowCategory);
	llvm::cl::ParseCommandLineOptions(argc, argv, "Count the number of static function calls in a file \n");
	llvm::llvm_shutdown_obj SDO; // cleans up LLVM objs
	llvm::SMDiagnostic Err;
	llvm::LLVMContext Ctx;

	std::unique_ptr<llvm::Module> M = llvm::parseIRFile(InputModule.getValue(), Err, Ctx);
  if (!M){
		llvm::errs() << "Error reading bitcode file: " << InputModule << "\n";
		Err.print(argv[0], llvm::errs());
		return -1;
	}
	printDotDataFlowGraph(*M);
	return 0;
}
