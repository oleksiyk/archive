/*  Copyright (C) 2006 Oleksiy Kryvoshey <oleksiy@kharkiv.com.ua> */

#include "dac.h"

static void null_signal_handler(int){};

int _cmdDebugLevel = -1;
char * _cmdUser = NULL;
char * _cmdHistory = NULL;
char * _cmdLog = NULL;
char * _cmdQuery = NULL;
int _cmdForeground = 0;
int _cmdPort = 0;
int _cmdTCP = 0;

const Dac * __server = NULL;

void print_version()
{
    printf("dac "PACKAGE_VERSION" - Nominet .UK Domain Availability Checker\n");
}

void print_usage()
{
    print_version();
    printf("Usage: dac [OPTIONS]\n\n");

    printf("Mandatory arguments to long options are mandatory for short options too.\n");

    printf("Options:\n");
    printf("-v, --version\t\tdisplay the version of dac and exit.\n");
    printf("-s, --server\tQuery nominet server (overrides config file).\n");
    printf("-p, --port=\t\tTCP port to connect to for use. (overrides config file)\n");
    printf("-l, --logfile\t\tLog all commands, sources, and results to logfile(default: "DEFAULT_LOGFILE")\n");
    printf("-f, --foreground\tdo not daemonize; run in foreground, also send debug to stdout.\n");
    printf("-d, --debug-level=\tdebug level, 1-9. (overrides config file)\n");
    printf("-c, --config-file=\tconfiguration file to use (default: "DEFAULT_CONFIGFILE")\n");
    printf("-u, --user=\t\tuser to run daemon (default: "DEFAULT_USER") (overrides config file)\n");
}

const char * Dac::requiredConfigOptions[] = {
    "debug_level",
    "connect",
    "pid_file",
    NULL
};

// ----------

int Dac::flag_restart = 0;
int Dac::flag_shutdown = 0;

void Dac::termination_signal_handler(int s)
{
    flag_shutdown = 1;
}

void Dac::restart_signal_handler(int s)
{
    flag_restart = 1;
}


Dac::Dac(const char * _configFileName):
    log(NULL),
    config(NULL),
    pidfile(NULL),
    configFileName(_configFileName),
    readThread(NULL)
{
    __server = this;

    sockfd = -1;

    log = new Log(2); //log level 2
    config = new DacConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);

}

Dac::~Dac()
{
    close(sockfd);

    delete readThread;

    if(pidfile){
        if(unlink(pidfile) == -1){
            log->log(1, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
        free(pidfile);
    }

    delete config;

    log->log(1, PACKAGE_STRING" exited");

    delete log;
}

int Dac::checkPIDFile()
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

int Dac::writePIDFile()
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

int Dac::initsocket()
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    addr.sin_port = htons(2043);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    const DOTCONFDocumentNode * node = config->findNode("connect");

    const char * v = node->getValue(1); //port
    int port = 2043;
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

    v = node->getValue(); //host

    if(!v){
        log->log(1, "file %s, line %d: Address parameter is empty",
            node->getConfigurationFileName(), node->getConfigurationLineNumber());
        return -1;
    }

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

    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        log->log(1, "socket failed: %s", strerror(errno));
        return -1;
    }

    if(connect(sockfd, (struct sockaddr*)&addr, addrlen) == -1){
        log->log(1, "connection to %s:%d failed: %s", v, port, strerror(errno));
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

    log->log(1, "connected to %s:%d", v, port);

    return 0;
}

int Dac::setUserAndGroupID()
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

int Dac::initialize()
{
    dac_pthread_block_signal(SIGIO);

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

    if(!_cmdForeground){
        if(dac_detach(NULL) == -1){
            return -1;
        }
    }

    if(setUserAndGroupID() == -1){
        return -1;
    }

    if(writePIDFile() == -1){
        return -1;
    }


    dac_signal(SIGINT, termination_signal_handler);
    dac_signal(SIGTERM, termination_signal_handler);
    dac_signal(SIGHUP, restart_signal_handler);

    dac_signal(SIGIO, null_signal_handler);
    dac_signal(SIGPIPE, SIG_IGN);

    log->log(1, PACKAGE_STRING" started");

    return 0;
}

int Dac::doqueries()
{
    struct pollfd fdset;
    char buf[256];

    snprintf(buf, 256, "guides.co.uk\r\n");
    buf[256] = 0;

    fdset.fd = sockfd;
    fdset.events = POLLOUT;

    int i = poll(&fdset, 1, 1000);

    if(i == -1){
        log->log(1, "poll failed: %s", strerror(errno));
        return -1;
    }

    if(i > 0){
        if(fdset.revents & POLLOUT){
            int k = write(fdset.fd, buf, strlen(buf));
            if(k == -1) {
                log->log(1, "write failed: %s", strerror(errno));
                return -1;
            }
        } else if(fdset.revents & POLLERR){
            int sock_err = -1; socklen_t sock_err_len = sizeof(sock_err);
            if(getsockopt(fdset.fd, SOL_SOCKET, SO_ERROR, &sock_err, &sock_err_len) < 0){
                log->log(1, "Connection failed: getsockopt failed: %s", strerror(errno));
            } else if(sock_err != 0){
                log->log(1, "Connection failed: %s", strerror(sock_err));
            }
            return -1;
        } else if(fdset.revents & POLLHUP){
            log->log(1, "POLLHUP");
            return -1;
        }
    }

    if(i == 0){
        log->log(1, "poll(POLLOUT) timeout");
    }

    return 0;
}

int Dac::start()
{
    if(initsocket() == -1){
        return -1;
    }

    readThread = new DacThread(sockfd);

    readThread->start();

    for(;;){

        dac_sleep(1,0);

        if(doqueries() == -1){
            break;
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

    readThread->stop();

    return 0;
}

int Dac::restart()
{
    delete config; config = NULL;

    config = new DacConfig(log, DOTCONFDocument::CASESENSETIVE);
    config->setRequiredOptionNames(requiredConfigOptions);

    if(config->setContent(configFileName) == -1){
        return -1;
    }

    if(config->checkConfig() == -1){
        return -1;
    }

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

    dac_signal(SIGINT, termination_signal_handler);
    dac_signal(SIGTERM, termination_signal_handler);
    dac_signal(SIGHUP, restart_signal_handler);

    dac_signal(SIGIO, null_signal_handler);
    dac_signal(SIGPIPE, SIG_IGN);

    log->log(1, PACKAGE_STRING" restarted");
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
        {"foreground", no_argument, NULL, 'f'},
        {"debug", required_argument, NULL, 'd'},
        {"config-file", required_argument, NULL, 'c'},
        {"logfile", required_argument, NULL, 'l'},
        {"port", required_argument, NULL, 'p'},
        {"user", required_argument, NULL, 'u'},
        {"server", required_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    while ((c = getopt_long (argc, argv, "vfd:c:l:p:u:s", long_options, NULL)) != -1) {
        switch(c) {
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
            case 's':
                _cmdQuery = strdup(optarg);
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

    Dac * dac = new Dac(configFile);

    int ret_code = 0;

    if(dac->initialize() == -1){
        ret_code = 1;
    } else {
        if(dac->start() == -1){
            ret_code = 1;
        }
    }

    free(configFile);

    delete dac;

    return ret_code;
}
