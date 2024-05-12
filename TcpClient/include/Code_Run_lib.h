#ifndef CODE_RUN_LIB_H
#define CODE_RUN_LIB_H

typedef struct REST_CompileReq
{
    char *language;
    char *code;
} REST_CompileReq;

// refId for compilled languages
// code and language for runtime languages
typedef struct REST_CodeRunReq
{
    char *refId;
    char *language;
    char *code;
} REST_CodeRunReq;

typedef struct REST_CompileResp
{
    char *refId;
} REST_CompileResp;

typedef struct REST_CodeRunResp
{
    char *output;
    char *error;
} REST_CodeRunResp;


int codeRunLib_RunDemo();
void InitContainersThreadpool();
REST_CompileResp* CompileCCode(REST_CompileReq* req);
REST_CompileResp* CompileRustCode(REST_CompileReq* req);
REST_CodeRunResp* RunCCode(REST_CodeRunReq *req);
REST_CodeRunResp* RunRustCode(REST_CodeRunReq *req);
REST_CodeRunResp* RunPythonCode(REST_CodeRunReq *req);
#endif // CODE_RUN_LIB_H