#include "flow/dataflow.h"
#include "utils/llvm_utils.h"

#include <llvm/Support/raw_ostream.h>

#include <list>

llvm::AnalysisKey datautils::DataWorker::Key;


unsigned int datautils::DataWorker::num = 0;

llvm::PreservedAnalyses datautils::DataWorker::run(llvm::Module& M, llvm::ModuleAnalysisManager &MAM){
	return runOnModule(M);
}

llvm::PreservedAnalyses datautils::DataWorker::runOnModule(llvm::Module& M){
	for (auto &gVar : M.getGlobalList()){
		globals.push_back(node(
					llvm::dyn_cast<llvm::Value>(gVar.stripPointerCasts()),
				 	gVar.getName())
				);
	}

	for (auto &F: M){
			for (auto &B: F){
				for (auto &I: B){
					switch (I.getOpcode()){
					case llvm::Instruction::Call:
						{
							llvm::CallInst * callinst = llvm::dyn_cast<llvm::CallInst>(I.stripPointerCasts());
							llvm::Function * func = callinst->getCalledFunction();
              func_calls[I.stripPointerCasts()]= func;
							llvm::errs() << func->getName() << "\n";
							for (auto &arg: func->args()){
								func_args[func].push_back(node(arg.stripPointerCasts(), datautils::getValStaticName(arg.stripPointerCasts())));
								data_flow_edges.push_back(edge(
											node(I.stripPointerCasts(),
											 	datautils::getValStaticName(I.stripPointerCasts())),
										 	node(arg.stripPointerCasts(),
											 	datautils::getValStaticName(arg.stripPointerCasts())))
										);
								// ///TODO:Use iterations over the arguments of the functions
								// for(llvm::Value::use_iterator UI = arg_idx->use_begin(), UE = arg_idx->use_end(); UI != UE; ++UI)
								// {

								//     data_flow_edges.push_back(edge(node(arg_idx, datautils::getvaluestaticname(arg_idx)), node(UI->get(), datautils::getvaluestaticname(UI->get()))));
								// }
							}
						}
						break;
					case llvm::Instruction::Store:
						{
							llvm::StoreInst* storeinst = llvm::dyn_cast<llvm::StoreInst>(I.stripPointerCasts());
							llvm::Value* storeValPtr = storeinst->getPointerOperand();
							llvm::Value* storeval    = storeinst->getValueOperand();
							data_flow_edges.push_back(edge(
										node(I.stripPointerCasts(), datautils::getValStaticName(I.stripPointerCasts())),
									 	node(storeValPtr, datautils::getValStaticName(storeValPtr))));
							data_flow_edges.push_back(edge(
										node(storeval, datautils::getValStaticName(storeval)),
									 	node(I.stripPointerCasts(), datautils::getValStaticName(I.stripPointerCasts()))));
						}
						break;
					case llvm::Instruction::Load:
						{
								llvm::LoadInst* loadinst = llvm::dyn_cast<llvm::LoadInst>(I.stripPointerCasts());
								llvm::Value* loadvalptr = loadinst->getPointerOperand();
								data_flow_edges.push_back(
										edge(
											node(loadvalptr,
											 	datautils::getValStaticName(loadvalptr)),
										 	node(I.stripPointerCasts(),
											 	datautils::getValStaticName(loadvalptr))));
						}break;
					default :
            {
                for(auto &op : I.operands()){
                    if(llvm::dyn_cast<llvm::Instruction>(op->stripPointerCasts()) || 
												llvm::dyn_cast<llvm::Argument>(op->stripPointerCasts()))
												data_flow_edges.push_back(
														edge(
															node(op.get(),
															 	datautils::getValStaticName(op.get())),
														 	node(I.stripPointerCasts(),
															 	datautils::getValStaticName(I.stripPointerCasts()))));
								}
						}break;
				}
				auto next = I.getNextNode();
        func_nodes_ctrl[&F].push_back(
						node(I.stripPointerCasts(),
						 	datautils::getValStaticName(I.stripPointerCasts())));
				if (next != nullptr){
					func_edges_ctrl[&F].push_back(edge(
								node(I.stripPointerCasts(), datautils::getValStaticName(I.stripPointerCasts())),
								node(next->stripPointerCasts(), datautils::getValStaticName(next->stripPointerCasts()))
								));
				}
			}
			auto TI = B.getTerminator();
			for(unsigned int succ_idx  = 0, succ_num = TI->getNumSuccessors(); succ_idx != succ_num; ++succ_idx)
			{
					llvm::BasicBlock * Succ = TI->getSuccessor(succ_idx);
					llvm::Value* succ_inst = Succ->begin()->stripPointerCasts();
					func_edges_ctrl[&F].push_back(
							edge(
								node(
									TI, datautils::getValStaticName(TI)), 
								node(succ_inst, datautils::getValStaticName(succ_inst)))
							);
			}
		}
	}
	std::ofstream outgraphfile("ctrl-data.dot");
  dumpCompleteDiGraph(outgraphfile);
  return llvm::PreservedAnalyses::all();
}

std::string datautils::getValStaticName(llvm::Value* val)
{
    std::string ret_val = "val";
    if(val->getName().empty()) {ret_val += std::to_string(DataWorker::num);DataWorker::num++;}
    else ret_val = val->getName().str();

    if(llvm::isa<llvm::Instruction>(val))ret_val += ":"+llvmutils::LLVMInstructionAsString(llvm::dyn_cast<llvm::Instruction>(val));

    return ret_val;
}

bool datautils::DataWorker::dumpCompleteDiGraph(std::ofstream& Out){/*{{{*/
    Out << indent << "digraph \"control_and_data_flow\"{\n";
    indent = "\t";
    Out << indent << "compound=true;\n";
    Out << indent << "nodesep=1.0;\n";
    Out << indent << "rankdir=TB;\n";
    Out << indent << "subgraph cluster_globals{\n";
    Out << indent << "label=globalvaldefinitions;\n";
    Out << indent << "color=green;\n";
    if(dumpGlobals(Out))return true;
    Out << indent << "}\n";
    for(auto func_node_pair : func_nodes_ctrl)
    {
        indent = "\t";
        if(dumpFunctionArguments(Out, *func_node_pair.first)) return true;
        Out << indent << "subgraph cluster_" << func_node_pair.first->getName().str() << "{\n";
        indent = "\t\t";
        Out << indent << "label=\""<< func_node_pair.first->getName().str() << "\";\n";
        Out << indent << "color=blue;\n";
        if(dumpNodes(Out, *func_node_pair.first)) return true;
        if(dumpControlflowEdges(Out, *func_node_pair.first)) return true;
        indent = "\t";
        Out << indent << "}\n\n";
    }
    dumpDataflowEdges(Out);
    dumpFunctionCalls(Out);
    Out << indent << "label=control_and_data_flow_graph;\n";
    indent = "";
    Out << indent << "}\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpNodes(std::ofstream& Out, llvm::Function &F)/*{{{*/
{
    for(auto node_l: func_nodes_ctrl[&F])
        Out << indent << "\tNode" << node_l.first << "[shape=record, label=\"" << node_l.second << "\"];\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpControlflowEdges(std::ofstream& Out, llvm::Function &F)/*{{{*/
{
    for(auto edge_l : func_edges_ctrl[&F])
        Out << indent << "\tNode" << edge_l.first.first << " -> Node" << edge_l.second.first << ";\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpGlobals(std::ofstream& Out)/*{{{*/
{
    for(auto globalval : globals)
        Out << indent << "\tNode" << globalval.first << "[shape=record, label=\"" << globalval.second << "\"];\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpDataflowEdges(std::ofstream& Out)/*{{{*/
{
    for(auto edge_l : data_flow_edges)
        Out << indent << "\tNode" << edge_l.first.first << " -> Node" << edge_l.second.first << "[color=red];\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpFunctionCalls(std::ofstream& Out)/*{{{*/
{
    for(auto call_l : func_calls)
        Out << indent << "\tNode" << &*(call_l.second->front().begin()) << " -> Node"<< call_l.first <<"[ltail = cluster_"<< remove_special_chars(call_l.second->getName().str())<<", color=red, label=return];\n";
    return false;
}/*}}}*/

bool datautils::DataWorker::dumpFunctionArguments(std::ofstream& Out, llvm::Function &F)/*{{{*/
{
    ///TODO:Argument in graphviz struct style. i.e., as collection of nodes
    for(auto arg_l : func_args[&F])
        Out << indent << "\tNode" << arg_l.first<<"[label="<<arg_l.second<<", shape=doublecircle, style=filled, color=blue , fillcolor=red];\n";
    return false;
}/*}}}*/

std::string datautils::DataWorker::remove_special_chars(std::string in_str)
{
    std::string ret_val = in_str;
    size_t pos;
    while((pos = ret_val.find('.')) != std::string::npos)
    {
        ret_val[pos] = '_';
    }

    return ret_val;
}


llvm::PreservedAnalyses datautils::DataWorkerWrapper::run(llvm::Module&M, llvm::ModuleAnalysisManager& MAM){
	MAM.getResult<DataWorker>(M);
	return llvm::PreservedAnalyses::all();
}
