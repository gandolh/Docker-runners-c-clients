#ifndef CODE_RUN_LIB_H
#define CODE_RUN_LIB_H

typedef struct CodeRunnerResponse
{
    char *status;
    char *filename;
    char *stdout;
    char *stderr;
} CodeRunnerResponse;

typedef struct CodeRunnerCompileRequest
{
    // default type COMPILE for this kind of requests
    char *type;
    char *language;
    char *code;
} CodeRunnerCompileRequest;

typedef struct CodeRunnerRunRequest
{
    // default type RUN for this kind of requests
    char *type;
    char *filename;
    char *code;
    char *language;
} CodeRunnerRunRequest;

int codeRunLib_RunDemo();
void InitContainersThreadpool();
CodeRunnerResponse *CompileCCode(char *const code);
CodeRunnerResponse *CompileRustCode(char *const req);
CodeRunnerResponse *RunCCode(char *const filename);
CodeRunnerResponse *RunRustCode(char *const filename);
CodeRunnerResponse *RunPythonCode(char *const filename);
#endif // CODE_RUN_LIB_H