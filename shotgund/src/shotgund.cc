#include "shotgund.h"

static void null_signal_handler(int){};

int _cmdDebugLevel = -1;
char * _cmdUser = NULL;
int _cmdForeground = 0;
int _cmdPort = 0;

const Shotgund * __server = NULL;

void print_version()
{
#ifdef CRIPPLED
    printf("shotgund "PACKAGE_VERSION"-c - an asynchronous, parallel, domain-swapping shim DNS daemon\n");
#else
    printf("shotgund "PACKAGE_VERSION" - an asynchronous, parallel, domain-swapping shim DNS daemon\n");
#endif
}

void print_usage()
{
    print_version();
    printf("Usage: shotgund [OPTION]...\n\n");

    printf("Mandatory arguments to long options are mandatory for short options too.\n");

    printf("Startup:\n");
    printf("-v,  --version           display the version of shotgund and exit.\n");
    printf("-h,  --help              print this help.\n");
    printf("-f,  --foreground        do not daemonize; run in foreground, also send debug to stdout.\n");
    printf("-d,  --debug=            debug level, 1-4. (overrides config file)\n");
    printf("-c,  --config-file=      configuration file to use (default: /etc/shotgund.conf)\n");
    printf("-p,  --port=             UDP port to bind for use. (overrides config file)\n");
    printf("-u,  --user=             user to run daemon (default: named) (overrides config file)\n");
}

const char * Shotgund::requiredConfigOptions[] = {
    "pid_file",
    "debug",
    "debug_file",
    "listen",
    NULL
};

// ----------

int Shotgund::flag_restart = 0;
int Shotgund::flag_shutdown = 0;

void Shotgund::termination_signal_handler(int s)
{
    flag_shutdown = 1;
}

void Shotgund::restart_signal_handler(int s)
{
    flag_restart = 1;
}


Shotgund::Shotgund(const char * _configFileName):
    log(NULL), config(NULL), pidfile(NULL),
    configFileName(_configFileName),
    queue(NULL), threadManager(NULL)
{
    __server = this;

    for(int i=0; i<8; i++){
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    log = new Log(2); //log level 2
    config = new ShotgundConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);
}

Shotgund::~Shotgund()
{
    for(int i=0; i<8; i++){
        close(fds[i].fd);
    }

    if(pidfile){
        if(unlink(pidfile) == -1){
            log->log(1, 0,0, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
        free(pidfile);
    }

    delete config;
    delete threadManager;
    delete queue;

#ifdef CRIPPLED
    log->log(1, 0,0, PACKAGE_STRING"-c exited");
#else
    log->log(1, 0,0, PACKAGE_STRING" exited");
#endif

    delete log;
}

int Shotgund::checkPIDFile()
{
    const DOTCONFDocumentNode * node = config->findNode("pid_file");

    const char * pidfile = node->getValue();

    if(access(pidfile, F_OK) == 0){ //file already exist
        char pidbuf[10];
        int fd = open(pidfile, O_RDWR);
        if(fd == -1){
            log->log(1, 0,0, "open(%s) failed: %s", pidfile, strerror(errno));
            return -1;
        }
        int sz = read(fd, pidbuf, 10);
        if(sz == -1){
            log->log(1, 0,0, "read(%s) failed: %s", pidfile, strerror(errno));
            close(fd);
            return -1;
        }
        pid_t pid = 0;
        if(sz > 2){
            pidbuf[sz-1] = 0;
            pid = atoi(pidbuf);
            if(kill(pid, 0) == -1){
                if(errno == ESRCH){
                    log->log(1, 0,0, "unclean shutdown of previous run");
                } else {
                    log->log(1, 0,0, "Failed to ping process with id %d: %s", pid, strerror(errno));
                    close(fd);
                    return -1;
                }
            } else {
                log->log(1, 0,0,  "already running, pid %d", pid);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }

    return 0;
}

int Shotgund::writePIDFile()
{
    const DOTCONFDocumentNode * node = config->findNode("pid_file");
    const char * _pidfile = node->getValue();

    int fd = open(_pidfile, O_RDWR|O_CREAT|O_TRUNC, 00644);

    if(fd == -1){
        log->log(1, 0,0, "failed to open pid_file %s: %s", _pidfile, strerror(errno));
        return -1;
    }

    pid_t pid = getpid();
    char pidbuf[32];

    snprintf(pidbuf, 32, "%ld\n", (long) pid); pidbuf[31] = 0;
    write(fd, pidbuf, strlen(pidbuf));

    close(fd);

    pidfile = strdup(_pidfile);

    return 0;
}

int Shotgund::initsockets()
{
    struct sockaddr_in addr;

    addr.sin_port = htons(53);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    const DOTCONFDocumentNode * node = NULL;
    int i = 0;

    while((node = config->findNode("listen", NULL, node)) != NULL){

        const char * v = node->getValue(1); //port
        int port = 53;
        if(v != NULL){
            port  = atoi(v);
        }
        if(port < 1 || port > 65535){
            log->log(1, 0,0, "file %s, line %d: Invalid 'port' value",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }
        if(_cmdPort != 0) port = _cmdPort;

        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        v = node->getValue();

        if(!v){
            log->log(1,0, 0, "file %s, line %d: Address parameter is empty",
                node->getConfigurationFileName(), node->getConfigurationLineNumber());
            return -1;
        }

        if(strcmp(v, "*")){

            struct hostent * hep = gethostbyname(v);

            if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
                log->log(1,0, 0, "file %s, line %d: Cannot resolve hostname '%s'",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
                return -1;
            }

            if (hep->h_addr_list[1]) {
                log->log(1,0, 0, "file %s, line %d: Host %s has multiple addresses, you must choose one explicitly for use",
                    node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
                return -1;
            }

            addr.sin_addr.s_addr = ((struct in_addr *) (hep->h_addr))->s_addr;
        }

        fds[i].fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(fds[i].fd == -1){
            log->log(1,0,0, "socket() failed: %s", strerror(errno));
            return -1;
        }

        if(fcntl(fds[i].fd, F_SETFL, O_NONBLOCK) == -1){
            log->log(1, 0,0, "setting O_NONBLOCK failed: %s", strerror(errno));
            return -1;
        }

        int arg = 1;
        if(setsockopt(fds[i].fd, SOL_SOCKET, SO_REUSEADDR,(char*)&arg,sizeof(arg)) == -1){
            log->log(1, 0,0, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
            return -1;
        }

        if(bind(fds[i].fd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
            log->log(1,0,0, "bind() failed: %s", strerror(errno));
            return -1;
        }

        log->log(2,0,0, "listening on %s:%d", v, port);
        //if(_cmdPort != 0) break;
        i++;

    }

    return 0;
}

int Shotgund::setUserAndGroupID()
{
    uid_t uid;
    //gid_t gid;

    const char * user = NULL;

    if(_cmdUser == NULL){
        const DOTCONFDocumentNode * node = config->findNode("user");
        if(node == NULL){
            user = "named";
        } else {
            user = node->getValue();
        }
    } else {
        user = _cmdUser;
    }

    if(user != NULL){
        const struct passwd * pwd = getpwnam(user);
        if(pwd != NULL){
            uid = pwd->pw_uid;
        } else {
            log->log(1, 0,0, "No such user: '%s'", user);
            return -1;
        }
    }

    /*
    node = config->findNode("Group");
    const char * group = node->getValue();
    if(group != NULL){
        const struct group * pwd = getgrnam(group);
        if(pwd != NULL){
            gid = pwd->gr_gid;
        } else {
            log->error(0, "No such group: '%s'", group);
            return -1;
        }
    }

    if(setregid(gid, gid) == -1) {
        log->error(0, "Failed to change group id to '%s': %s", group, strerror(errno));
        return -1;
    }
    */

    if(setreuid(uid, uid) == -1) {
        log->log(1, 0,0, "Failed to change user id to '%s': %s", user, strerror(errno));
        return -1;
    }

    return 0;
}

u_int16_t Shotgund::getRandId()
{
    u_int16_t id = 0;

    int fd = open("/dev/urandom", O_RDONLY);
    if(fd != -1){
        read(fd, &id, 2);
        close(fd);
    }

    return id;
}

int Shotgund::initialize()
{
    shotgund_pthread_block_signal(SIGIO);

    if(config->setContent(configFileName) == -1){
        return -1;
    }

    if(config->checkConfig() == -1){
        return -1;
    }

    if(checkPIDFile() == -1){
        return -1;
    }

    const DOTCONFDocumentNode * node = config->findNode("debug_file");

    if(log->initialize(node->getValue()) == -1){
        return -1;
    }

    if(_cmdDebugLevel == -1){
        node = config->findNode("debug");
        log->setLoggingLevel(atoi(node->getValue()));
    } else {
        log->setLoggingLevel(_cmdDebugLevel);
    }

    if(!_cmdForeground){
        if(shotgund_detach(NULL) == -1){
            return -1;
        }
    }

    if(initsockets() == -1){
        return -1;
    }

    if(setUserAndGroupID() == -1){
        return -1;
    }

    if(writePIDFile() == -1){
        return -1;
    }

    threadManager = new ThreadManager(3, 64);

    queue = new ThreadQueue<QueueObj*>(threadManager, HIGHWATERMARK, QUEUE_SIZE);

    if(queue->initialize() == -1){
        log->log(1, 0,0, "failed to initialize thread_queue");
        return -1;
    }

    threadManager->initialize(queue);

    if(threadManager->startThreads() == -1){
        log->log(1, 0,0, "failed to start threads. shutting down");
        return -1;
    }

    shotgund_signal(SIGINT, termination_signal_handler);
    shotgund_signal(SIGTERM, termination_signal_handler);
    shotgund_signal(SIGHUP, restart_signal_handler);

    shotgund_signal(SIGIO, null_signal_handler);
    shotgund_signal(SIGPIPE, SIG_IGN);

#ifdef CRIPPLED
    log->log(1, 0,0, PACKAGE_STRING"-c started");
#else
    log->log(1, 0,0, PACKAGE_STRING" started");
#endif

    return 0;
}

int Shotgund::start()
{
    //char buf[256];
    socklen_t addrlen = sizeof(struct sockaddr_in);
    QueueObj * obj = NULL;

    for(;;){

        int i = poll(fds, 8, 1000);
        if(i == -1){
            if(errno != EINTR){
                log->log( 0, 0,0, "poll() failed: %s", strerror(errno));
                break;
            }
        }

        if(i > 0){
            for(i=0; i < 8; i++){
                if(fds[i].fd == -1)
                    continue;
                if(fds[i].revents & POLLIN){
                    obj = new QueueObj;
                    obj->fd = fds[i].fd;
                    obj->id = getRandId();
                    int responseLen = recvfrom(fds[i].fd, &(obj->msg), sizeof(AsyncDNSMessage), 0, (struct sockaddr*)&(obj->in), &addrlen);
                    if(responseLen == -1){
                        log->log(1,0,0, "recvfrom failed: %s", strerror(errno));
                    }
                    if(inet_ntop(RFC1035_AF_INET, &(obj->in).RFC1035_sin_addr, obj->addr, RFC1035_INET_ADDRSTRLEN) == NULL){
                        obj->addr[0] = 0;
                    }
                    obj->msgLen = responseLen;

                    queue->push(obj);
                    //log->log(4, obj->id,0, "push");
                }
            }
        }

        if(flag_restart){
            flag_restart = 0;
            log->log(1, 0,0, "SIGHUP, rereading configuration...");
            if(restart() == -1){
                break;
            }
        }

        if(flag_shutdown){
            break;
        }
    };

    log->log(1, 0,0, "Shutting down...");
    if(threadManager)
        threadManager->stopThreads();

    return 0;
}

int Shotgund::restart()
{
    threadManager->stopThreads();

    delete queue; queue = NULL;
    delete threadManager; threadManager = NULL;

    delete config; config = NULL;

    config = new ShotgundConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);

    if(config->setContent(configFileName) == -1){
        return -1;
    }

    if(config->checkConfig() == -1){
        return -1;
    }

    Log * newlog = new Log(2); //log level 2
    if(newlog->initialize(config->findNode("debug_file")->getValue()) == -1){
        return -1;
    }
    newlog->setLoggingLevel(atoi(config->findNode("debug")->getValue()));

    delete log;
    log = newlog;

    /*
    if(writePIDFile() == -1){
        return -1;
    }
    */

    /*
    if(initsockets() == -1){
        return -1;
    }
    */

    threadManager = new ThreadManager(3, 64);

    queue = new ThreadQueue<QueueObj*>(threadManager, HIGHWATERMARK, QUEUE_SIZE);

    if(queue->initialize() == -1){
        log->log(1, 0,0, "failed to initialize thread_queue");
        return -1;
    }

    threadManager->initialize(queue);

    if(threadManager->startThreads() == -1){
        log->log(1, 0,0, "failed to start threads. shutting down");
        return -1;
    }

    shotgund_signal(SIGINT, termination_signal_handler);
    shotgund_signal(SIGTERM, termination_signal_handler);
    shotgund_signal(SIGHUP, restart_signal_handler);

    shotgund_signal(SIGIO, null_signal_handler);
    shotgund_signal(SIGPIPE, SIG_IGN);

#ifdef CRIPPLED
    log->log(1, 0,0, PACKAGE_STRING"-c restarted");
#else
    log->log(1, 0,0, PACKAGE_STRING" restarted");
#endif
    return 0;
}

int main(int argc, char *argv[])
{
    int c = -2;
    char * configFile = NULL;

    /* enable core dump */
    struct rlimit rl;
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    tzset();

    struct option long_options[] = {
        {"version", no_argument, NULL, 'v'},
        {"help", no_argument, NULL, 'h'},
        {"foreground", no_argument, NULL, 'f'},
        {"debug", required_argument, NULL, 'd'},
        {"config-file", required_argument, NULL, 'c'},
        {"port", required_argument, NULL, 'p'},
        {"user", required_argument, NULL, 'u'},
    };

    while ((c = getopt_long (argc, argv, "vhfd:c:p:u:", long_options, NULL)) != -1) {
        switch(c) {
            case 'h':
                print_usage();
                return 0;
            case 'v':
                print_version();
                return 0;
            case 'c':
                configFile = strdup(optarg);
                break;
            case 'f':
                _cmdForeground = 1;
                break;
            case 'd':
                _cmdDebugLevel = atoi(optarg);
                break;
            case 'u':
                _cmdUser = strdup(optarg);
            case 'p':
                _cmdPort = atoi(optarg);
        }
    }

    if(c == -2){
        print_usage();
        return 0;
    }

    if(_cmdPort < 0 || _cmdPort > 65535){
        fprintf(stderr, "port value is invalid\n");
        return 0;
    }

    if(_cmdDebugLevel != -1 && (_cmdDebugLevel < 1 || _cmdDebugLevel > 4)){
        fprintf(stderr, "debug value is invalid\n");
        print_usage();
        return 0;
    }

    if(configFile == NULL){
        configFile = strdup("/etc/shotgund.conf");
    }

    Shotgund * shotgund = new Shotgund(configFile);

    int ret_code = 0;

    if(shotgund->initialize() == -1){
        ret_code = 1;
    } else {
        if(shotgund->start() == -1){
            ret_code = 1;
        }
    }

    free(configFile);

    delete shotgund;

    return ret_code;
}
