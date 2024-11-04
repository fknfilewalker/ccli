module;
#include <ccli/ccli.h>
export module ccli;

export namespace ccli {
	using ccli::parseArgs;
	using ccli::loadConfig;
	using ccli::writeConfig;
	using ccli::executeCallbacks;
	using ccli::forEachVar;

	using ccli::ConfigCache;
	using ccli::IterationDecision;
	using ccli::VarBase;
	using ccli::Var;

	using ccli::Flag;
	using ccli::Storage;
	using ccli::MinLimit;
	using ccli::MaxLimit;

	using ccli::CCLIError;
	using ccli::DuplicatedVarNameError;
	using ccli::FileError;
	using ccli::UnknownVarNameError;
	using ccli::MissingValueError;
	using ccli::ConversionError;
}