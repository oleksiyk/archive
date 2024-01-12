#include "barcode.h"

static void null_signal_handler(int){};

void print_usage(const char * argv0)
{
    printf("usage: %s [-h|--help] [-v|--version] [-c|--config]\n", argv0);
}

// ----------

int BARd::flag_restart = 0;
int BARd::flag_shutdown = 0;

void BARd::termination_signal_handler(int s)
{
    flag_shutdown = 1;
}

void BARd::restart_signal_handler(int s)
{
    flag_restart = 1;
}


BARd::BARd(const char * _configFileName):
    log(NULL), config(NULL),
    output_fp(NULL), configFileName(_configFileName)
{
    for(int i=0; i<32; i++){
        devices[i] = 0;
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    log = new Log(2); //log level 2
    config = new BARdConfig(log);
}

BARd::~BARd()
{    
    if(output_fp){
        if(fclose(output_fp) == EOF){
            log->error(0, "fclose(%s) failed: %s",
                config->getBarcodeOutputFile(), strerror(errno));
        }
    }

    for(int i=0; i<32; i++){
        delete devices[i];
        close(fds[i].fd);
    }

    const char * pidfile = config->getBarcodePIDFile();
    
    if(pidfile){
        if(unlink(pidfile) == -1){
            log->error(0, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
    }

    delete config;
    
    log->info(0, PACKAGE_STRING" exited");
    
    delete log;
}

int BARd::checkPIDFile()
{
    const char * pidfile = config->getBarcodePIDFile();
    
    if(access(pidfile, F_OK) == 0){ //file already exist
        char pidbuf[10];
        int fd = open(pidfile, O_RDWR);
        if(fd == -1){
            log->error(0, "open(%s) failed: %s", pidfile, strerror(errno));
            return -1;
        }
        int sz = read(fd, pidbuf, 10);
        if(sz == -1){
            log->error(0, "read(%s) failed: %s", pidfile, strerror(errno));
            close(fd);
            return -1;
        }
        pid_t pid = 0;
        if(sz > 2){
            pidbuf[sz-1] = 0;
            pid = atoi(pidbuf);
            if(kill(pid, 0) == -1){
                if(errno == ESRCH){
                    log->warning(0, "unclean shutdown of previous run");
                } else {
                    log->error(0, "Failed to ping process with id %d: %s", pid, strerror(errno));
                    close(fd);
                    return -1;
                }
            } else {
                log->error(0, "already running, pid %d", pid);
                close(fd);
                return -1;
            }
        }
        close(fd);
    }

    return 0;
}

int BARd::writePIDFile()
{
    const char * pidfile = config->getBarcodePIDFile();

    int fd = open(pidfile, O_RDWR|O_CREAT|O_TRUNC, 00644);

    if(fd == -1){
        log->error(0, "Failed to open PIDFile %s: %s", pidfile, strerror(errno));
        return -1;
    }

    pid_t pid = getpid();
    char pidbuf[32];

    snprintf(pidbuf, 32, "%ld\n", (long) pid); pidbuf[31] = 0;
    write(fd, pidbuf, strlen(pidbuf));

    close(fd);

    return 0;
}

/*
int BARd::setUserAndGroupID()
{
    uid_t uid;
    gid_t gid;

    const DOTCONFDocumentNode * node = configuration->findNode("User");

    const char * user = node->getValue();
    if(user != NULL){
        const struct passwd * pwd = getpwnam(user);
        if(pwd != NULL){
            uid = pwd->pw_uid;
        } else {
            log->error(0, "No such user: '%s'", user);
            return -1;
        }
    } else {
        log->error(0, "'User' parameter is empty");
        return -1;
    }

    node = configuration->findNode("Group");
    const char * group = node->getValue();
    if(group != NULL){
        const struct group * pwd = getgrnam(group);
        if(pwd != NULL){
            gid = pwd->gr_gid;
        } else {
            log->error(0, "No such group: '%s'", group);
            return -1;
        }
    } else {
        log->error(0, "'Group' parameter is empty");
        return -1;
    }

    if(setregid(gid, gid) == -1) {
        log->error(0, "Failed to change group id to '%s': %s", group, strerror(errno));
        return -1;
    }
    if(setreuid(uid, uid) == -1) {
        log->error(0, "Failed to change user id to '%s': %s", user, strerror(errno));
        return -1;
    }

    return 0;
}
*/

int BARd::pingMainApp()
{
    const char * pidfile = config->getSignalAppPIDFile();
    
    pid_t pid = 0;

    if(access(pidfile, F_OK) == 0){
        char pidbuf[10];
        int fd = open(pidfile, O_RDWR);
        if(fd == -1){
            log->error(0, "open(%s) failed: %s", pidfile, strerror(errno));
            return -1;
        }
        int sz = read(fd, pidbuf, 10);
        if(sz == -1){
            log->error(0, "read(%s) failed: %s", pidfile, strerror(errno));
            close(fd);
            return -1;
        }
        if(sz > 2){
            pidbuf[sz-1] = 0;
            pid = atoi(pidbuf);
        }
        close(fd);
    }

    if(pid == 0){
        if(config->getSignalAppPath() != NULL){
            log->info(2, "Failed to get main app PID from pidfile %s, searching process table...", pidfile);
        }
    }

    if(pid){
        if(kill(pid, 0) == -1){
            if(errno == ESRCH){
                log->warning(0, "Main application is not running but its PID file is in place...");
            } else {
                log->error(0, "Failed to send signal to main application: kill() failed: %s",
                    strerror(errno));
                return -1;
            }
        }
    } else {
        log->info(2, "Failed to signal main app, beacause its PID is unknwon");
        return -1;
    }

    return 0;
}

pid_t BARd::searchPID()
{    
    DIR *dirfd;
    struct dirent *dir1;
    char path[32];

    size_t l = strlen(config->getSignalAppPath());
    char buf[l + 1];

    pid_t pid = 0;

    dirfd = opendir("/proc");
    if (dirfd == NULL) {
        log->error(2, "opendir(/proc) failed: %s", strerror(errno));
        return pid;
    }

    while ((dir1 = readdir(dirfd)) != NULL) {
        snprintf(path, 32, "/proc/%s/exe", dir1->d_name);
        path[31] = 0;
        int k = readlink(path, buf, l);
        if(k == -1){
            log->error(2, "readlink(%s) failed: %s", path, strerror(errno));
            break;
        }
        buf[k] = 0;
        if(!strcmp(config->getSignalAppPath(), buf)){
            pid = atoi(dir1->d_name);
            break;
        }

    }
    closedir(dirfd);
    return pid;
}

int BARd::openReaders()
{
    char evname[256];
    int found = 0;
    unsigned short id[4];

    for(int i= 0; i<32; i++){
        snprintf(evname, 256, "/dev/input/event%d", i);
        evname[255] = 0;

        fds[i].fd = open(evname, O_RDONLY);
        if(fds[i].fd == -1){
            if(errno != ENODEV)
                log->error(0, "open(%s) failed: %s", evname, strerror(errno));
            continue;
        }

        if (ioctl(fds[i].fd, EVIOCGID, id) == -1) {
            log->error(0,"ioctl(EVIOCGID): %s", strerror(errno));
            continue;
        }
        if( id[ID_VENDOR] == 0x04b4 && id[ID_PRODUCT] == 0x0101){ //manson barcode reader

            if(fcntl(fds[i].fd, F_SETFL, O_NONBLOCK) == -1){
                log->error(0,"setting O_NONBLOCK failed: %s", strerror(errno));
                return -1;
            }

            devices[i] = new BARdEvdev(log, fds[i].fd);
            devices[i++]->logInfo();
            found++;
        } else {
            log->notice(0, "device on %s is not barcode reader", evname);
            close(fds[i].fd);
            fds[i].fd = -1;
        }
    }

    if(found == 0){
        log->error(0, "No barcode readers found");
        return -1;
    } else {
        log->info(0, "%d barcode readers found", found);
    }

    return 0;
}

int BARd::initialize()
{
    if(config->initialize(configFileName) == -1){
        return -1;
    }

    if(config->getBarcodePIDFile() == NULL){
        log->error(0, "BARCODE_PID_FILE is not defined in configuration");
        return -1;
    }

    if(checkPIDFile() == -1){
        return -1;
    }

    if(log->initialize(config->getErrorLogFile()) == -1){
        return -1;
    }

    if(config->getSignalAppPIDFile() == NULL){
        log->error(0, "SIGNAL_APP_PID_FILE is not defined in configuration");
        return -1;
    }    
    if(config->getBarcodeOutputFile() == NULL){
        log->error(0, "BARCODE_OUTPUT_FILE is not defined in configuration");
        return -1;
    }

    output_fp = fopen(config->getBarcodeOutputFile(), "a+");
    if(output_fp == NULL){
        log->error(0, "fopen(%s) failed: %s",
            config->getBarcodeOutputFile(), strerror(errno));
        return -1;
    }
    
    log->info(0, PACKAGE_STRING" starting");
    
    log->setLoggingLevel(config->getErrorLogLevel());

    if(bard_detach(NULL) == -1){
        return -1;
    }
    
    if(writePIDFile() == -1){
        return -1;
    }
    
    bard_signal(SIGINT, termination_signal_handler);
    bard_signal(SIGTERM, termination_signal_handler);
    bard_signal(SIGHUP, restart_signal_handler);
    
    bard_signal(SIGIO, null_signal_handler);    
    bard_signal(SIGPIPE, SIG_IGN);

    if(openReaders() == -1){
        return -1;
    }
    
    log->info(0, PACKAGE_STRING" started");
    
    return 0;
}

int BARd::start()
{
    char buf[256];

    for(;;){
    
        int i = poll(fds, 32, 1000);
        if(i == -1){
            if(errno != EINTR){
                log->error( 0, "poll() failed: %s", strerror(errno));
                break;
            }
        }
        
        if(i > 0){
            for(i=0; i < 32; i++){
                if(fds[i].fd == -1)
                    continue;
                if(fds[i].revents & POLLIN){
                    fds[i].events = POLLIN;
                    if(devices[i]->readData(buf, 256) == -1){
                        continue;
                    }

                    size_t l = strlen(buf);
                    if(l){
                        if(fwrite(buf, l, 1, output_fp) != 1){
                            log->error(0, "fwrite() failed: %s", strerror(errno));
                        }
                    }
                }
            }
            if(fflush(output_fp) == EOF){
                log->error(0, "fflush() failed: %s", strerror(errno));
            }
            if(fsync(fileno(output_fp)) == -1){
                log->error(0, "fsync() failed: %s", strerror(errno));
            }
            if(fclose(output_fp) == EOF){
                log->error(0, "fclose() failed: %s", strerror(errno));
            }

            pingMainApp();

            output_fp = fopen(config->getBarcodeOutputFile(), "a+");
            if(output_fp == NULL){
                log->error(0, "fopen(%s) failed: %s",
                    config->getBarcodeOutputFile(), strerror(errno));
                return -1;
            }
        }
    
        if(flag_restart){
            flag_restart = 0;
            log->info(0, "SIGHUP, rereading configuration...");
            if(restart() == -1){
                break;
            }
        }

        if(flag_shutdown){
            log->info(0, "Shutting down...");
            break;
        }
    
    };

    return 0;
}

int BARd::restart()
{
    if(output_fp){
        if(fclose(output_fp) == EOF){
            log->error(0, "fclose(%s) failed: %s",
                config->getBarcodeOutputFile(), strerror(errno));
        }
    }

    for(int i=0; i<32; i++){
        delete devices[i];
        close(fds[i].fd);
    }

    const char * pidfile = config->getBarcodePIDFile();
    
    if(pidfile){
        if(unlink(pidfile) == -1){
            log->error(0, "unlink(%s) failed %s", pidfile, strerror(errno));
        }
    }

    for(int i=0; i<32; i++){
        devices[i] = 0;
        fds[i].fd = -1;
        fds[i].events = POLLIN;
    }

    if(config->initialize(configFileName) == -1){
        return -1;
    }

    Log * newlog = new Log(2); //log level 2
    if(newlog->initialize(config->getErrorLogFile()) == -1){
        return -1;
    }
    log->setLoggingLevel(config->getErrorLogLevel());

    delete log;
    log = newlog;

    if(config->getBarcodePIDFile() == NULL){
        log->error(0, "BARCODE_PID_FILE is not defined in configuration");
        return -1;
    }
    if(config->getSignalAppPIDFile() == NULL){
        log->error(0, "SIGNAL_APP_PID_FILE is not defined in configuration");
        return -1;
    }
    if(config->getBarcodeOutputFile() == NULL){
        log->error(0, "BARCODE_OUTPUT_FILE is not defined in configuration");
        return -1;
    }

    output_fp = fopen(config->getBarcodeOutputFile(), "a+");
    if(output_fp == NULL){
        log->error(0, "fopen(%s) failed: %s",
            config->getBarcodeOutputFile(), strerror(errno));
        return -1;
    }
    
    if(writePIDFile() == -1){
        return -1;
    }
    
    bard_signal(SIGINT, termination_signal_handler);
    bard_signal(SIGTERM, termination_signal_handler);
    bard_signal(SIGHUP, restart_signal_handler);
    
    bard_signal(SIGIO, null_signal_handler);    
    bard_signal(SIGPIPE, SIG_IGN);

    if(openReaders() == -1){
        return -1;
    }
    
    log->info(0, PACKAGE_STRING" restarted");
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
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {"config", required_argument, NULL, 'c'}
    };
    
    while ((c = getopt_long (argc, argv, "hvc:", long_options, NULL)) != -1) {
        switch(c) {
            case 'h':
                print_usage(argv[0]);
                return 0;               
            case 'v':
                printf(PACKAGE_STRING"\n");
                return 0;
            case 'c':
                configFile = strdup(optarg);
                break;
        }
    }
    
    if(c == -2){
        print_usage(argv[0]);
    }
    
    BARd * bard = new BARd(configFile);
    
    int ret_code = 0;

    if(bard->initialize() == -1){
        ret_code = 1;
    } else {
        if(bard->start() == -1){
            ret_code = 1;
        }
    }
    
    free(configFile);
    
    delete bard;
    
    return ret_code;
}
