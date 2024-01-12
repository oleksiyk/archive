#include <embedhttp/server.h>
#include <embedhttp/content.h>
#include <embedhttp/log.h>
#include <fnmatch.h>

int process = 1;

void term_handler(int signum)
{
    process = 0;
}

class MyHTTPContent : public HTTPContent
{
public:
    MyHTTPContent(){};
    virtual ~MyHTTPContent(){};
    virtual int processRequest(const HTTPRequest* request, HTTPResponse* response)
    {
        const char * header = "<html><h1>Wildcards</h1><b>This is widcard test page</b><hr>";
        const char * footer = "</html>";

        char body[512];

        sprintf(body, "Request URI <i>%s</i><br>Your browser <i>%s</i><br>Request method <i>%s</i><br>Remote addr <i>%s</i><br>Server protocol <i>%s</i><br>test=%s<br>",
            request->getParameter("REQUEST_URI"),
            request->getParameter("User-Agent"),
            request->getParameter("REQUEST_METHOD"),
            request->getParameter("REMOTE_ADDR"),
            request->getParameter("SERVER_PROTOCOL"),
            request->getParameter("test")); //value of request parameter 'test', if any

        /* set cookie */
        response->setCookie("TESTCOOKIE", "test_cookie");

        response->setContentLength(strlen(header)+strlen(body)+strlen(footer));
        response->setContentType("text/html; charset=koi8-r");

        response->print(header, strlen(header));
        response->print(body, strlen(body));
        response->print(footer, strlen(footer));
    }
};

int main()
{
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);
    
    HTTPAccessLog * accessLog = new HTTPAccessLog(stdout);
    HTTPErrorLog * errorLog = new HTTPErrorLog(stderr);

    HTTPServer * server = new HTTPServer(accessLog, errorLog, 10080, "testing dynamic content");

    MyHTTPContent * content = new MyHTTPContent;

    signal(SIGINT, term_handler);

    if(server->initialize() != -1){
        
        server->getRequestDispatcher()->addContent("/*", content);

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

