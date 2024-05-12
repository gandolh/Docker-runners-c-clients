#include "Code_Run_Lib.h"
#include "REST_Api_Handler.h"
#include <stdlib.h>
#include <string.h>

REST_CompileResp *compile_code(REST_CompileReq *req) {
    // Create a response
    REST_CompileResp *resp = malloc(sizeof(REST_CompileResp));

    // For now, just return a static refId. Replace this with your own implementation.
    resp->refId = strdup("1234");

    return resp;
}

REST_CodeRunResp *run_code(REST_CodeRunReq *req) {
    // Create a response
    REST_CodeRunResp *resp = malloc(sizeof(REST_CodeRunResp));

    // For now, just return a static output. Replace this with your own implementation.
    resp->output = strdup("Hello, World!");
   resp->error = NULL;
    return resp;
}