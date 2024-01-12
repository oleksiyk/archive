/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed.h"

static void null_signal_handler(int){};

int _cmdDebugLevel = -1;
char * _cmdUser = NULL;
char * _cmdHistory = NULL;
char * _cmdLog = NULL;
char * _cmdQuery = NULL;
int _cmdForeground = 0;
int _cmdPort = 0;
int _cmdTCP = 0;

const Probed * __server = NULL;

void print_version()
{
    printf("probed "PACKAGE_VERSION" - A simple QOS probe daemon\n");
}

void print_usage()
{
    print_version();
    printf("Usage: probed [OPTIONS] [-q|--query remote-system]\n\n");

    printf("Mandatory arguments to long options are mandatory for short options too.\n");

    printf("Options:\n");
    printf("-v, --version\t\tdisplay the version of probed and exit.\n");
    printf("-h, --history-file\tGather probe queries and results in history-file.\n");
    printf("-l, --logfile\t\tLog all commands, sources, and results to logfile(default: "DEFAULT_LOGFILE")\n");
    printf("-f, --foreground\tdo not daemonize; run in foreground, also send debug to stdout.\n");
    printf("-d, --debug-level=\tdebug level, 1-9. (overrides config file)\n");
    printf("-c, --config-file=\tconfiguration file to use (default: "DEFAULT_CONFIGFILE")\n");
    printf("-p, --port=\t\tTCP port to bind for use. (overrides config file)\n");
    printf("-u, --user=\t\tuser to run daemon (default: "DEFAULT_USER") (overrides config file)\n");
    printf("-e, --tcp\t\tUse the tcp echo port rather than icmp\n");
    printf("-q, --query=\t\tQuery remote system\n");
}

const char * Probed::requiredConfigOptions[] = {
    "debug_level",
    "listen",
    "pid_file",
    "history_file",
    NULL
};

// ----------

int Probed::flag_restart = 0;
int Probed::flag_shutdown = 0;

void Probed::termination_signal_handler(int s)
{
    flag_shutdown = 1;
}

void Probed::restart_signal_handler(int s)
{
    flag_restart = 1;
}


Probed::Probed(const char * _configFileName):
    log(NULL), history(NULL),
    config(NULL), ssl(NULL),
    pidfile(NULL),
    configFileName(_configFileName),
    threadManager(NULL),
    pingThread(NULL),
    pingQueue(NULL)
{
    __server = this;

    sockfd = -1;
    pingfd = -1;

    log = new Log(2); //log level 2
    history = new ProbedHistory();
    config = new ProbedConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);

    pingQueue = new ThreadQueue<ProbedPingTask*>;
}

Probed::~Probed()
{
    close(sockfd);
    close(pingfd);

    if(pidfile){
        if(unlink(pidfile) == -1){
            log->log(1, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
        free(pidfile);
    }

    delete config;
    if(ssl){
        ssl->finish();
    }
    delete ssl;
    delete threadManager;

    delete pingThread;
    delete pingQueue;

    delete history;

    log->log(1, PACKAGE_STRING" exited");

    delete log;
}

int Probed::checkPIDFile()
{
    const DOTCONFDocumentNode * node = config->findNode("pid_file");

    const char * pidfile = node->getValue();

    if(access(pidfile, F_OK) == 0){ //file already exist
        char pidbuf[10];
        int fd = open(pidfile, O_RDWR);
        if(fd == -1){
            log->log(1, "open(%s) failed: %s", pidfile, strerror(errno));
            return -1;
        }
        int sz = read(fd, pidbuf, 10);
        if(sz == -1){
            log->log(1, "read(%s) failed: %s", pidfile, strerror(errno));
            close(fd);
            return -1;
        }
        pid_t pid = 0;
        if(sz > 2){
            pidbuf[sz-1] = 0;
            pid = atoi(pidbuf);
            if(kill(pid, 0) == -1){
                if(errno == ESRCH){
                    log->log(1, "unclean shutdown of previous run");
                } else {
                    log->log(1, "Failed to ping process with id %d: %s", pid, strerror(errno));
                    close(fd);
                    return -1;
                }
            } else {
                log->log(1, "already running, pid %d", pid);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }

    return 0;
}

int Probed::writePIDFile()
{
    const DOTCONFDocumentNode * node = config->findNode("pid_file");
    const char * _pidfile = node->getValue();

    int fd = open(_pidfile, O_RDWR|O_CREAT|O_TRUNC, 00644);

    if(fd == -1){
        log->log(1, "failed to open pid_file %s: %s", _pidfile, strerror(errno));
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

int Probed::initsocket()
{
    struct sockaddr_in addr;

    addr.sin_port = htons(665);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    const DOTCONFDocumentNode * node = config->findNode("listen");

    const char * v = node->getValue(1); //port
    int port = 665;
    if(v != NULL){
        port  = atoi(v);
    }
    if(port < 1 || port > 65535){
        log->log(1, "file %s, line %d: Invalid 'port' value",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }
    if(_cmdPort != 0) port = _cmdPort;

    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    v = node->getValue();

    if(!v){
        log->log(1, "file %s, line %d: Address parameter is empty",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

    if(strcmp(v, "*")){

        struct hostent * hep = gethostbyname(v);

        if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
            log->log(1, "file %s, line %d: Cannot resolve hostname '%s'",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
            return -1;
        }

        if (hep->h_addr_list[1]) {
            log->log(1, "file %s, line %d: Host %s has multiple addresses, you must choose one explicitly for use",
                node->getConfigurationFileName(), node->getConfigurationLineNumber(), v);
            return -1;
        }

        addr.sin_addr.s_addr = ((struct in_addr *) (hep->h_addr))->s_addr;
    }

    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        log->log(1, "socket failed: %s", strerror(errno));
        return -1;
    }

    if(fcntl(sockfd, F_SETFL, O_NONBLOCK|O_ASYNC) == -1){
        log->log(1, "setting O_NONBLOCK|O_ASYNC failed: %s", strerror(errno));
        return -1;
    }
    if(fcntl(sockfd, F_SETOWN, getpid()) == -1){
        log->log(1, "fcntl(F_SETOWN) failed: %s", strerror(errno));
        return -1;
    }

    int arg = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(char*)&arg,sizeof(arg)) == -1){
        log->log(1, "setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        return -1;
    }

    if(bind(sockfd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
        log->log(1, "bind() failed: %s", strerror(errno));
        return -1;
    }
    if(listen(sockfd, 511) == -1){
        log->log(1, "listen failed: %s", strerror(errno));
        return -1;
    }

    log->log(1, "listening on %s:%d", v, port);

    return 0;
}

int Probed::initPingSocket()
{
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(23456);


    pingfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(pingfd == -1){
        log->log(1, "failed to create icmp socket: %s", strerror(errno));
        return -1;
    }

    int size = ICMP_BUFSIZE;
    if(setsockopt(pingfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size))){
        log->log(4, "warning: setting ICMP receive buffer failed: %s", strerror(errno));
    }

    /*
    int opt = ~(ICMP_ECHOREPLY|ICMP_DEST_UNREACH|ICMP_SOURCE_QUENCH|ICMP_REDIRECT|ICMP_TIME_EXCEEDED|ICMP_PARAMETERPROB);
    if(setsockopt(pingfd, SOL_RAW, ICMP_FILTER, opt, sizeof(opt)) == -1){
        log->log(4, "warning: setting ICMP FILTER options failed: %s", strerror(errno));
    }
    */

    /*
    if(bind(pingfd, (sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1){
        log->log(1, "bind() icmp socket failed: %s", strerror(errno));
        return -1;
    }
    */

    return 0;
}

int Probed::setUserAndGroupID()
{
    uid_t uid;
    //gid_t gid;

    const char * user = NULL;

    if(_cmdUser == NULL){
        const DOTCONFDocumentNode * node = config->findNode("user");
        if(node == NULL){
            user = DEFAULT_USER;
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
            log->log(1, "No such user: '%s'", user);
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
        log->log(1, "Failed to change user id to '%s': %s", user, strerror(errno));
        return -1;
    }

    return 0;
}

int Probed::initialize()
{
    probed_pthread_block_signal(SIGIO);

    if(config->setContent(configFileName) == -1){
        return -1;
    }

    if(config->checkConfig() == -1){
        return -1;
    }

    if(checkPIDFile() == -1){
        return -1;
    }

    const DOTCONFDocumentNode * node = NULL;

    node = config->findNode("logfile");

    const char * lf = _cmdLog;
    if(!lf){
        if(!node)
            lf = DEFAULT_LOGFILE;
        else
            lf = node->getValue();
    }

    if(log->initialize(lf) == -1){
        return -1;
    }

    if(_cmdDebugLevel == -1){
        node = config->findNode("debug_level");
        log->setLoggingLevel(atoi(node->getValue()));
    } else {
        log->setLoggingLevel(_cmdDebugLevel);
    }

    const char * v = _cmdHistory;
    if(!v){
        node = config->findNode("history_file");
        v = node->getValue();
    }
    if(history->initialize(v) == -1){
        return -1;
    }

    if(!_cmdForeground){
        if(probed_detach(NULL) == -1){
            return -1;
        }
    }

    if(initsocket() == -1){
        return -1;
    }

    if(!_cmdTCP){
        if(initPingSocket() == -1){
            return -1;
        }
    }

    if(setUserAndGroupID() == -1){
        return -1;
    }

    if(writePIDFile() == -1){
        return -1;
    }

    ssl = new ProbedSSL;
    if(ssl->initialize() == -1){
        return -1;
    }

    threadManager = new ProbedThreadManager();

    pingThread = new ProbedPingThread(pingQueue, pingfd);

    if(pingQueue->initialize() == -1){
        return -1;
    }

    if(pingThread->start() == -1){
        return -1;
    }

    probed_signal(SIGINT, termination_signal_handler);
    probed_signal(SIGTERM, termination_signal_handler);
    probed_signal(SIGHUP, restart_signal_handler);

    probed_signal(SIGIO, null_signal_handler);
    probed_signal(SIGPIPE, SIG_IGN);

    log->log(1, PACKAGE_STRING" started");

    if(_cmdQuery){
        doQuery();
    }

    return 0;
}

int Probed::doQuery()
{
    ProbedPingTask task;

    task.addr.sin_addr.s_addr = INADDR_ANY;
    task.addr.sin_family = AF_INET;

//     task.tr = NULL;
    task.sslPeerName = "-";
    task.rtts.clear();

    struct hostent * hep = gethostbyname(_cmdQuery);

    if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
        log->log(1, "Cannot resolve hostname '%s'", _cmdQuery);
        return -1;
    }

    task.addr.sin_addr.s_addr = ((struct in_addr *) (hep->h_addr))->s_addr;

    char name[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &task.addr.sin_addr, name, INET_ADDRSTRLEN);
    name[INET_ADDRSTRLEN-1] = 0;

    log->log(1, "Starting probe on %s", name);

    pthread_mutex_lock(&task.lock);

    pingQueue->push(&task);

    pthread_cond_wait(&task.cond, &task.lock);

    pthread_mutex_unlock(&task.lock);

    float packet_loss = (float(task.packets) - float(task.packets_recvd)) / float(task.packets);
    packet_loss = packet_loss * 100.0;

    int loss = 0;
    int latency = 0;
    int jitter = 0;

    for(int i=0; i<5; i++){
        if(config->loss_thresholds[i][0] == -1) continue;
        if( packet_loss >= config->loss_thresholds[i][0] && (packet_loss <= config->loss_thresholds[i][1] || config->loss_thresholds[i][1] == 0)){
            loss = (int)config->loss_thresholds[i][2];
            break;
        }
    }

    for(int i=0; i<5; i++){
        if(config->latency_thresholds[i][0] == -1) continue;
        if( task.rtt_average >= config->latency_thresholds[i][0] && (task.rtt_average <= config->latency_thresholds[i][1] || config->latency_thresholds[i][1] == 0)){
            latency = (int)config->latency_thresholds[i][2];
            break;
        }
    }

    for(int i=0; i<5; i++){
        if(config->jitter_thresholds[i][0] == -1) continue;
        if( task.jitter >= config->jitter_thresholds[i][0] && (task.jitter <= config->jitter_thresholds[i][1] || config->jitter_thresholds[i][1] == 0)){
            jitter = (int)config->jitter_thresholds[i][2];
            break;
        }
    }

    log->log(1, "[-] Packets sent=%d, packets received=%d, packet_loss=%.2f%%",
        task.packets, task.packets_recvd, packet_loss);

    log->log(1, "[-] rtt_average=%.3fms, rtt_min=%.3fms, rtt_max=%.3fms, jitter=%.3fms",
        task.rtt_average,
        task.rtt_min, task.rtt_max,
        task.jitter);

    float mos = (float)(jitter + latency + loss) / 3.0;

    log->log(1, "[-] %s: LATENCY=%d, JITTER=%d, LOSS=%d, MOS=%.2f", name, latency, jitter, loss, mos);

    history->log("- probed[%d]: QUERY=%s (%s), RESULT=%.2f", getpid(), _cmdQuery, name, mos);

    log->log(1, "[-] Finished probe on %s", name);

    return 0;
}

int Probed::start()
{
    int clfd = -1;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in claddr;
    struct pollfd fds;

    fds.fd = sockfd;
    fds.events = POLLIN;
    fds.revents = 0;

    std::list<ProbedConfig::ipms>::iterator it = config->ipAllowList.begin();

    for(;;){

        int i = poll(&fds, 1, 1000);
        if(i == -1){
            if(errno != EINTR){
                log->log(0, "poll() failed: %s", strerror(errno));
                break;
            }
        }

        if(i > 0){
            if(fds.revents & POLLIN){
                while( (clfd = accept(sockfd, (sockaddr*)&claddr, &addrlen)) > 0 ){
                    log->log(9, "new tcp connection accepted");
                    bool ok = false;
                    for(it = config->ipAllowList.begin(); it != config->ipAllowList.end(); it++){
                        //log->log(1, "s_addr=%u, addr=%u, mask=%u", claddr.sin_addr.s_addr, (*it).addr, (*it).mask);
                        if((claddr.sin_addr.s_addr & (*it).mask) == ((*it).addr & (*it).mask)){
                            ok = true; break;
                        }
                    }
                    if(ok){
                        threadManager->startThread(clfd);
                    } else {
                        char name[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &claddr.sin_addr, name, INET_ADDRSTRLEN);
                        name[INET_ADDRSTRLEN-1] = 0;
                        log->log(1, "Client (%s) denied by server configuration", name);
                        close(clfd);
                    }
                }
                if( errno != EAGAIN){
                    if(errno == EMFILE || errno == ENFILE ){
                        log->log(0, "RUNNING OUT OF MAXIMUM OPEN FILE DESCRIPTORS. NOT ACCEPTING NEW CONNECTIONS FOR 20 sec");
                        probed_sleep(20, 0);
                    } else {
                        log->log(0, "accept() failed: %s", strerror(errno));
                        break;
                    }
                }
            }
        }

        if(flag_restart){
            flag_restart = 0;
            log->log(1, "SIGHUP, rereading configuration...");
            if(restart() == -1){
                break;
            }
        }

        if(flag_shutdown){
            break;
        }
    }

    log->log(1,  "Shutting down...");
    if(threadManager)
        threadManager->stopThreads();

    pingThread->stop();

    return 0;
}

int Probed::restart()
{
    threadManager->stopThreads();

    pingThread->stop();

    delete threadManager; threadManager = NULL;

    delete config; config = NULL;

    delete pingThread; pingThread = NULL;

    config = new ProbedConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);

    if(config->setContent(configFileName) == -1){
        return -1;
    }

    if(config->checkConfig() == -1){
        return -1;
    }

    delete history;
    history = new ProbedHistory();

    Log * newlog = new Log(2); //log level 2
    const DOTCONFDocumentNode * node = config->findNode("logfile");
    if(!node){
        if(newlog->initialize(DEFAULT_LOGFILE) == -1){
            return -1;
        }
    } else {
        if(newlog->initialize(node->getValue()) == -1){
            return -1;
        }
    }
    newlog->setLoggingLevel(atoi(config->findNode("debug_level")->getValue()));

    delete log;
    log = newlog;

    const char * v = _cmdHistory;
    if(!v){
        node = config->findNode("history_file");
        v = node->getValue();
    }
    if(history->initialize(v)){
        return -1;
    }

    pingThread = new ProbedPingThread(pingQueue, pingfd);

    pingThread->start();

    threadManager = new ProbedThreadManager();

    probed_signal(SIGINT, termination_signal_handler);
    probed_signal(SIGTERM, termination_signal_handler);
    probed_signal(SIGHUP, restart_signal_handler);

    probed_signal(SIGIO, null_signal_handler);
    probed_signal(SIGPIPE, SIG_IGN);

    log->log(1, PACKAGE_STRING" restarted");
    return 0;
}

SSL_CTX * Probed::getSSL_CTX()const
{
    if(ssl){
        return ssl->getSSL_CTX();
    }
    return NULL;
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
        {"history-file", required_argument, NULL, 'h'},
        {"foreground", no_argument, NULL, 'f'},
        {"debug", required_argument, NULL, 'd'},
        {"config-file", required_argument, NULL, 'c'},
        {"logfile", required_argument, NULL, 'l'},
        {"port", required_argument, NULL, 'p'},
        {"user", required_argument, NULL, 'u'},
        {"query", required_argument, NULL, 'q'},
        {"tcp", required_argument, NULL, 'e'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long (argc, argv, "vh:fd:c:l:p:u:q:e", long_options, NULL)) != -1) {
        switch(c) {
            case 'h':
                _cmdHistory = strdup(optarg);
                break;
            case 'l':
                _cmdLog = strdup(optarg);
                break;
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
                break;
            case 'p':
                _cmdPort = atoi(optarg);
                break;
            case 'q':
                _cmdQuery = strdup(optarg);
                break;
            case 'e':
                _cmdTCP = 1;
                break;
            case '?':
            case ':':
                print_usage();
                return 0;
            default:
                print_usage();
                return 0;
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

    if(_cmdDebugLevel != -1 && (_cmdDebugLevel < 1 || _cmdDebugLevel > 9)){
        fprintf(stderr, "debug_level value is invalid\n");
        print_usage();
        return 0;
    }

    if(configFile == NULL){
        configFile = strdup(DEFAULT_CONFIGFILE);
    }

    Probed * probed = new Probed(configFile);

    int ret_code = 0;

    if(probed->initialize() == -1){
        ret_code = 1;
    } else {
        if(probed->start() == -1){
            ret_code = 1;
        }
    }

    free(configFile);

    delete probed;

    return ret_code;
}
