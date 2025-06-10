#include "node.h"
#include "codegen.h"
#include "parser.hpp"

using namespace std;

/* Compile the AST into a module */
void CodeGenContext::generateCode(NBlock& root)
{
	std::cout << "Generating code...\n";
	
	/* Create the top level interpreter function to call as entry */
	vector<Type*> argTypes;
	FunctionType *ftype = FunctionType::get(Type::getVoidTy(MyContext), ArrayRef(argTypes), false);
	mainFunction = Function::Create(ftype, GlobalValue::InternalLinkage, "main", module);
	BasicBlock *bblock = BasicBlock::Create(MyContext, "entry", mainFunction, 0);
	
	/* Push a new variable/block context */
	pushBlock(bblock);
	root.codeGen(*this); /* emit bytecode for the toplevel block */
	ReturnInst::Create(MyContext, bblock);
	popBlock();
	
	/* Print the bytecode in a human-readable format 
	   to see if our program compiled properly
	 */
	std::cout << "Code is generated.\n";
	// module->dump();

	legacy::PassManager pm;
	// TODO:
	pm.add(createPrintModulePass(outs()));
	pm.run(*module);
}

/* Executes the AST by running the main function */
GenericValue CodeGenContext::runCode() {
	std::cout << "Running code...\n";
	ExecutionEngine *ee = EngineBuilder( unique_ptr<Module>(module) ).create();
	ee->finalizeObject();
	vector<GenericValue> noargs;
	GenericValue v = ee->runFunction(mainFunction, noargs);
	std::cout << "Code was run.\n";
	return v;
}

/* Returns an LLVM type based on the identifier */
static Type *typeOf(const NIdentifier& type) 
{
	if (type.name.compare("int") == 0) {
		return Type::getInt64Ty(MyContext);
	}
	else if (type.name.compare("double") == 0) {
		return Type::getDoubleTy(MyContext);
	}
	return Type::getVoidTy(MyContext);
}

/* -- Code Generation -- */

Value* NInteger::codeGen(CodeGenContext& context)
{
	std::cout << "Creating integer: " << value << endl;
	return ConstantInt::get(Type::getInt64Ty(MyContext), value, true);
}

Value* NDouble::codeGen(CodeGenContext& context)
{
	std::cout << "Creating double: " << value << endl;
	return ConstantFP::get(Type::getDoubleTy(MyContext), value);
}

Value* NIdentifier::codeGen(CodeGenContext& context)
{
	std::cout << "Creating identifier reference: " << name << endl;
	if (context.locals().find(name) == context.locals().end()) {
		std::cerr << "undeclared variable " << name << endl;
		return NULL;
	}
	Value* var = context.locals()[name]; // Retrieve stored value

    // Attempt to cast the variable to an AllocaInst if it's allocated
    if (AllocaInst* alloc = dyn_cast<AllocaInst>(var)) {
        Type* varType = alloc->getAllocatedType(); // Get the allocated type

        if (varType->isIntegerTy(64)) {
            return new LoadInst(Type::getInt64Ty(MyContext), alloc, name, false, context.currentBlock());
        } 
        else if (varType->isDoubleTy()) {
            return new LoadInst(Type::getDoubleTy(MyContext), alloc, name, false, context.currentBlock());
        } 
        else {
            std::cerr << "Unsupported variable type for: " << name << std::endl;
            return nullptr;
        }


	}
	// return nullptr;  
	// return new LoadInst(context.locals()[name]->getType(),context.locals()[name], name, false, context.currentBlock());
}

Value* NMethodCall::codeGen(CodeGenContext& context)
{
	Function *function = context.module->getFunction(id.name.c_str());
	if (function == NULL) {
		std::cerr << "no such function " << id.name << endl;
	}
	std::vector<Value*> args;
	ExpressionList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		args.push_back((**it).codeGen(context));
	}
	CallInst *call = CallInst::Create(function, ArrayRef(args), "", context.currentBlock());
	std::cout << "Creating method call: " << id.name << endl;
	return call;
}

Value* NBinaryOps
::codeGen(CodeGenContext& context)
{

	/* Emit IR for a binary expression involving arithmetic and comparison operators
	*  For Arithmetic operators, it first creates a variable of type Instruction::BinaryOps which holds
	   the type of arithmetic instruction. We then use a switch-case to determine which binary instruction to create based on the opcode(+, -, *, /)
	   Afterwards, we return a Value* value. 
	   The IR for the left hand and right hand expressions are emitted recursively by calling their codegen functions

	   For Comparison, we are only having comparisons involving integer values. We therefore use predicates that represent Integer Comaprison instructions like CmpInst::ICmp_NE for not equal

	   We then insert the instructions into our current block in the return
	*/
	std::cout << "Creating binary operation " << op << endl;
	Instruction::BinaryOps instr;
	CmpInst::Predicate instr2; //store the predicate e.g the enum value of llvm representing a particular comparison
	Value * left = lhs.codeGen(context);
	Value * right = rhs.codeGen(context);

	if (!left || !right){
		std::cerr<<"Error: Null operand in binary operation "<<std::endl;
		return NULL;
	}
	Type* leftType = left->getType();
	Type* rightType = left->getType();
	if (leftType != rightType){
		std::cerr<<"Type Mismatch "<<std::endl;
	}
	bool isFloat = leftType->isDoubleTy();

	switch (op) {
		/* To do Arithmetic operators*/
		case PLUS: 		instr = isFloat ? Instruction::FAdd : Instruction::Add; goto math;
		case MINUS: 	instr = isFloat ? Instruction::FSub : Instruction::Sub; goto math;
		case MUL: 		instr = isFloat ? Instruction::FMul : Instruction::Mul; goto math;
		case DIV: 		instr = isFloat ? Instruction::FDiv : Instruction::SDiv; goto math;
		
				
		/* TODO comparison */
		case EQ:		instr2 = CmpInst::ICMP_EQ; goto cmp;	
		case LT:		instr2 = CmpInst::ICMP_SLT; goto cmp;
		case GT:		instr2 = CmpInst::ICMP_SGT; goto cmp;
		case GE:		instr2 = CmpInst::ICMP_SGE; goto cmp;
		case LE:		instr2 = CmpInst::ICMP_SLE; goto cmp;
		case NE:		instr2 = CmpInst::ICMP_NE; goto cmp;
	}

	//The opcode doesn't match any case and in that instance return a NULL
	std::cout<<"Invalid mathematical operator"<<std::endl;
	return NULL;
math:
	return BinaryOperator::Create(instr, lhs.codeGen(context), 
		rhs.codeGen(context), "", context.currentBlock());

cmp:
	return CmpInst::Create(Instruction::ICmp,instr2,lhs.codeGen(context),rhs.codeGen(context),"",context.currentBlock());
}

Value* NAssignment::codeGen(CodeGenContext& context)
{
	std::cout << "Creating assignment for " << lhs.name << endl;
	if (context.locals().find(lhs.name) == context.locals().end()) {
		std::cerr << "undeclared variable " << lhs.name << endl;
		return NULL;
	}
	return new StoreInst(rhs.codeGen(context), context.locals()[lhs.name], false, context.currentBlock());
}

Value* NBlock::codeGen(CodeGenContext& context)
{
	StatementList::const_iterator it;
	Value *last = NULL;
	for (it = statements.begin(); it != statements.end(); it++) {
		std::cout << "Generating code for " << typeid(**it).name() << endl;
		last = (**it).codeGen(context);
	}
	std::cout << "Creating block" << endl;
	return last;
}

Value* NExpressionStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating code for " << typeid(expression).name() << endl;
	return expression.codeGen(context);
}

Value* NReturnStatement::codeGen(CodeGenContext& context)
{
	std::cout << "Generating return code for " << typeid(expression).name() << endl;
	Value *returnValue = expression.codeGen(context);
	context.setCurrentReturnValue(returnValue);
	return returnValue;
}

Value* NVariableDeclaration::codeGen(CodeGenContext& context)
{
	std::cout << "Creating variable declaration " << type.name << " " << id.name << endl;
	AllocaInst *alloc = new AllocaInst(typeOf(type),4, id.name.c_str(), context.currentBlock());
	context.locals()[id.name] = alloc;
	if (assignmentExpr != NULL) {
		NAssignment assn(id, *assignmentExpr);
		assn.codeGen(context);
	}
	return alloc;
}

Value* NExternDeclaration::codeGen(CodeGenContext& context)
{
    vector<Type*> argTypes;
    VariableList::const_iterator it;
    for (it = arguments.begin(); it != arguments.end(); it++) {
        argTypes.push_back(typeOf((**it).type));
    }
    FunctionType *ftype = FunctionType::get(typeOf(type), ArrayRef(argTypes), false);
    Function *function = Function::Create(ftype, GlobalValue::ExternalLinkage, id.name.c_str(), context.module);
    return function;
}

Value* NFunctionDeclaration::codeGen(CodeGenContext& context)
{
	vector<Type*> argTypes;
	VariableList::const_iterator it;
	for (it = arguments.begin(); it != arguments.end(); it++) {
		argTypes.push_back(typeOf((**it).type));
	}
	FunctionType *ftype = FunctionType::get(typeOf(type), ArrayRef(argTypes), false);
	Function *function = Function::Create(ftype, GlobalValue::InternalLinkage, id.name.c_str(), context.module);
	BasicBlock *bblock = BasicBlock::Create(MyContext, "entry", function, 0);

	context.pushBlock(bblock);

	Function::arg_iterator argsValues = function->arg_begin();
    Value* argumentValue;

	for (it = arguments.begin(); it != arguments.end(); it++) {
		(**it).codeGen(context);
		
		argumentValue = &*argsValues++;
		argumentValue->setName((*it)->id.name.c_str());
		StoreInst *inst = new StoreInst(argumentValue, context.locals()[(*it)->id.name], false, bblock);
	}
	
	block.codeGen(context);
	ReturnInst::Create(MyContext, context.getCurrentReturnValue(), bblock);

	context.popBlock();
	std::cout << "Creating function: " << id.name << endl;
	return function;
}
