#include "Code_Run_lib.h"

#ifndef REST_Api_Handler_H
#define REST_Api_Handler_H

CodeRunnerResponse *compile_code(CodeRunnerCompileRequest *req);
CodeRunnerResponse *run_code(CodeRunnerRunRequest *req);

#endif