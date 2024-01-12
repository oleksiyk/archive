#include <embedhttp/server.h>
#include <embedhttp/content.h>
#include <embedhttp/log.h>
#include <fnmatch.h>

int process = 1;

void term_handler(int signum)
{
    process = 0;
}

int main()
{
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    
    HTTPAccessLog * accessLog = new HTTPAccessLog(stdout);
    HTTPErrorLog * errorLog = new HTTPErrorLog(stderr);

    HTTPServer * server = new HTTPServer(accessLog, errorLog, 10080, "testing file content");

    HTTPFileContent * docs = new HTTPFileContent(errorLog);

    signal(SIGINT, term_handler);

    docs->setContentRoot("/usr/doc");
        
    if(server->initialize() != -1){
        
        server->getRequestDispatcher()->addContent("/*", docs);

        while(process) {
            if(server->processRequest(1) == -1){
                //break;
            }
        }
    }

    delete server;
    delete docs;

    delete accessLog;
    delete errorLog;

    puts("Exit");

    return 0;
}

