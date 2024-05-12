#include "Code_Run_lib.h"

#ifndef REST_Api_Handler_H
#define REST_Api_Handler_H

REST_CompileResp *compile_code(REST_CompileReq *req);
REST_CodeRunResp *run_code(REST_CodeRunReq *req);

#endif