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

    HTTPServer * server = new HTTPServer(accessLog, errorLog, 10080, "testing static content");

    HTTPStaticContent * content = new HTTPStaticContent;

    signal(SIGINT, term_handler);

    content->setContent("<html><h1>Main page</h1><br><hr>this is static content, compiled into application</html>\n");
        
    if(server->initialize() != -1){
        
        server->getRequestDispatcher()->addContent("/", content);
        server->getRequestDispatcher()->addContent("/index.html", content);

        while(process) {
            if(server->processRequest(1) == -1){
                //break;
            }
        }
    }

    delete server;
    delete content;

    delete accessLog;
    delete errorLog;

    puts("Exit");

    return 0;
}

