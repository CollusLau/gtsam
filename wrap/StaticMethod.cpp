/* ----------------------------------------------------------------------------

 * GTSAM Copyright 2010, Georgia Tech Research Corporation, 
 * Atlanta, Georgia 30332-0415
 * All Rights Reserved
 * Authors: Frank Dellaert, et al. (see THANKS for the full author list)

 * See LICENSE for the license information

 * -------------------------------------------------------------------------- */

/**
 * @file StaticMethod.ccp
 * @author Frank Dellaert
 * @author Andrew Melim
 **/

#include <iostream>
#include <fstream>

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "StaticMethod.h"
#include "utilities.h"

using namespace std;
using namespace wrap;


/* ************************************************************************* */
void StaticMethod::addOverload(bool verbose, const std::string& name,
		const ArgumentList& args, const ReturnValue& retVal) {
	this->verbose = verbose;
	this->name = name;
	this->argLists.push_back(args);
	this->returnVals.push_back(retVal);
}

void StaticMethod::proxy_wrapper_fragments(FileWriter& proxyFile, FileWriter& wrapperFile,
																		 const string& cppClassName,
																		 const string& matlabClassName,
																		 const string& wrapperName,
																		 const vector<string>& using_namespaces,
																		 vector<string>& functionNames) const {

  string upperName = name;  upperName[0] = std::toupper(upperName[0], std::locale());

	proxyFile.oss << "    function varargout = " << upperName << "(varargin)\n";

	for(size_t overload = 0; overload < argLists.size(); ++overload) {
		const ArgumentList& args = argLists[overload];
		const ReturnValue& returnVal = returnVals[overload];
		size_t nrArgs = args.size();

		const int id = functionNames.size();

		// Output proxy matlab code

		// check for number of arguments...
		proxyFile.oss << "      " << (overload==0?"":"else") << "if length(varargin) == " << nrArgs;
		if (nrArgs>0) proxyFile.oss << " && ";
		// ...and their types
		bool first = true;
		for(size_t i=0;i<nrArgs;i++) {
			if (!first) proxyFile.oss << " && ";
			proxyFile.oss << "isa(varargin{" << i+1 << "},'" << args[i].matlabClass() << "')";
			first=false;
		}
		proxyFile.oss << "\n";

		// output call to C++ wrapper
		string output;
		if(returnVal.isPair)
			output = "[ varargout{1} varargout{2} ] = ";
		else if(returnVal.category1 == ReturnValue::VOID)
			output = "";
		else
			output = "varargout{1} = ";
		proxyFile.oss << "        " << output << wrapperName << "(" << id << ", varargin{:});\n";

		// Output C++ wrapper code
		
		const string wrapFunctionName = wrapper_fragment(
			wrapperFile, cppClassName, matlabClassName, overload, id, using_namespaces);

		// Add to function list
		functionNames.push_back(wrapFunctionName);

	}
	
  proxyFile.oss << "      else\n";
	proxyFile.oss << "        error('Arguments do not match any overload of function " <<
		matlabClassName << "." << upperName << "');" << endl;

	proxyFile.oss << "      end\n";
	proxyFile.oss << "    end\n";
}

/* ************************************************************************* */
string StaticMethod::wrapper_fragment(FileWriter& file, 
			    const string& cppClassName,
			    const string& matlabClassName,
					int overload,
					int id,
			    const vector<string>& using_namespaces) const {

  // generate code

	const string wrapFunctionName = matlabClassName + "_" + name + "_" + boost::lexical_cast<string>(id);

	const ArgumentList& args = argLists[overload];
	const ReturnValue& returnVal = returnVals[overload];

	// call
	file.oss << "void " << wrapFunctionName << "(int nargout, mxArray *out[], int nargin, const mxArray *in[])\n";
	// start
	file.oss << "{\n";
	generateUsingNamespace(file, using_namespaces);

  if(returnVal.isPair)
  {
      if(returnVal.category1 == ReturnValue::CLASS)
        file.oss << "  typedef boost::shared_ptr<"  << returnVal.qualifiedType1("::")  << "> Shared" <<  returnVal.type1 << ";"<< endl;
      if(returnVal.category2 == ReturnValue::CLASS)
        file.oss << "  typedef boost::shared_ptr<"  << returnVal.qualifiedType2("::")  << "> Shared" <<  returnVal.type2 << ";"<< endl;
  }
  else
      if(returnVal.category1 == ReturnValue::CLASS)
        file.oss << "  typedef boost::shared_ptr<"  << returnVal.qualifiedType1("::")  << "> Shared" <<  returnVal.type1 << ";"<< endl;

  file.oss << "  typedef boost::shared_ptr<"  << cppClassName  << "> Shared;" << endl;

  // check arguments
  // NOTE: for static functions, there is no object passed
  file.oss << "  checkArguments(\"" << matlabClassName << "." << name << "\",nargout,nargin," << args.size() << ");\n";

  // unwrap arguments, see Argument.cpp
  args.matlab_unwrap(file,0); // We start at 0 because there is no self object

  file.oss << "  ";

  // call method with default type
  if (returnVal.type1!="void")
    file.oss << returnVal.return_type(true,ReturnValue::pair) << " result = ";
  file.oss << cppClassName  << "::" << name << "(" << args.names() << ");\n";

  // wrap result
  // example: out[0]=wrap<bool>(result);
  returnVal.wrap_result(file);

  // finish
  file.oss << "}\n";

	return wrapFunctionName;
}

/* ************************************************************************* */
