/*  Copyright (C) 2005 Oleksiy Kryvoshey <oleksiy@voodoo.com.ua> */

#include "probed_commands.h"

ProbedCommands::tCP ProbedCommands::cp[] = {
    //{"HELO",&ProbedCommands::command_HELO},
    {"PACKETS", &ProbedCommands::command_PACKETS},
    {"QUERY", &ProbedCommands::command_QUERY},
    {"VERBOSE", &ProbedCommands::command_VERBOSE},
    {"OPTIONS", &ProbedCommands::command_OPTIONS},
    {"BYE", &ProbedCommands::command_BYE},
    {NULL, NULL}
};

ProbedCommands::ProbedCommands(const Log * _log, ProbedSSLTransport * _tr, ProbedThread * _thread):
    log(_log), tr(_tr)
{
    peerName = _thread->getPeerName();
}

ProbedCommands::~ProbedCommands()
{
}

int ProbedCommands::process(const char * buf)
{
    int i = 0;

    while(cp[i].command){
        if(!strncmp(cp[i].command, buf, strlen(cp[i].command))){
            return (this->*(cp[i].commandFunc))(buf);
        }
        i++;
    }

    return unknownCommand(buf);
}

int ProbedCommands::unknownCommand(const char * buf)
{
    const char * str = "Command unknown\n";
    log->log(1, "[%s] Unknown command: '%s'", peerName, buf);

    if(tr->write(str, strlen(str)) == -1){
        return -1;
    }
    return 0;
}

/*
int ProbedCommands::command_HELO(const char * buf)
{
    return 0;
}
*/

int ProbedCommands::command_PACKETS(const char * buf)
{
    char * p = (char*)buf;

    while(*p){
        if(isdigit(*p)) break;
        p++;
    }

    if(*p){
        int packets = atoi(p);
        int maxpackets = 10;
        char str[128];

        const DOTCONFDocumentNode * node = __server->getConfig()->findNode("max_packets");
        if(node != NULL){
            maxpackets = atoi(node->getValue());
        }
        if(packets < 1 || packets > maxpackets){
            snprintf(str, 128, "Number of packets specified (%d) out of acceptable range (1,%d)\n", packets, maxpackets);
            str[127] = 0;
            return tr->write(str, strlen(str))>0?0:-1;
        }

        log->log(4, "[%s] Setting number of packets to %d", peerName, packets);
        snprintf(str, 128, "PACKETS=%d\n", packets);
        task.packets = packets;
        str[127] = 0;
        return tr->write(str, strlen(str))>0?0:-1;

    } else {
        char str[128];
        snprintf(str, 128, "Syntax error, format: PACKETS=<num>\n");
        str[127] = 0;
        return tr->write(str, strlen(str))>0?0:-1;
    }

    return 0;
}

int ProbedCommands::command_QUERY(const char * buf)
{
    char str[128];

    task.addr.sin_addr.s_addr = INADDR_ANY;
    task.addr.sin_family = AF_INET;

//     task.tr = tr;
    task.sslPeerName = peerName;
    task.rtts.clear();

    char * p = strchr(buf, '=');

    if(!p){
        p = strchr(buf, ' ');
    }

    if(p){
        ++p;
        p = probed_strchop(p);

        struct hostent * hep = gethostbyname(p);

        if ((!hep) || (hep->h_addrtype != AF_INET || !hep->h_addr_list[0])) {
            snprintf(str, 128, "Cannot resolve hostname '%s'", p);
            log->log(1, "[%s] %s", peerName, str);
            strncat(str, "\n", 127); str[127] = 0;
            return tr->write(str, strlen(str))>0?0:-1;
        }

        task.addr.sin_addr.s_addr = ((struct in_addr *) (hep->h_addr))->s_addr;

        char name[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &task.addr.sin_addr, name, INET_ADDRSTRLEN);
        name[INET_ADDRSTRLEN-1] = 0;

        snprintf(str, 128, "Starting probe on %s", name);
        log->log(1, "[%s] %s", peerName, str);
        strncat(str, "\n", 127); str[127] = 0;

        //pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
        //pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        pthread_mutex_lock(&task.lock);

        tr->write(str, strlen(str));

        __server->getPingQueue()->push(&task);

        pthread_cond_wait(&task.cond, &task.lock);

        pthread_mutex_unlock(&task.lock);

        float packet_loss = (float(task.packets) - float(task.packets_recvd)) / float(task.packets);
        packet_loss = packet_loss * 100.0;

        int loss = 0;
        int latency = 0;
        int jitter = 0;

        const ProbedConfig * config = __server->getConfig();

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

        snprintf(str, 128, "Packets sent=%d, packets received=%d, packet_loss=%.2f%%",
            task.packets, task.packets_recvd, packet_loss);
        log->log(1, "[%s] %s", peerName, str);
        strncat(str, "\n", 127); str[127] = 0;

        tr->write(str, strlen(str));

        if(task.packets_recvd == 0){

            snprintf(str, 128, "%s: MOS=-1", name);
            log->log(1, "[%s] %s", peerName, str);
            strncat(str, "\n", 127); str[127] = 0;

            __server->getHistory()->log("%s probed[%d]: QUERY=%s (%s), RESULT=-1", peerName, getpid(), p, name);

            tr->write(str, strlen(str));

        } else {

            snprintf(str, 128, "rtt_average=%.3fms, rtt_min=%.3fms, rtt_max=%.3fms, jitter=%.3fms",
                task.rtt_average,
                task.rtt_min, task.rtt_max,
                task.jitter);
            log->log(1, "[%s] %s", peerName, str);
            strncat(str, "\n", 127); str[127] = 0;

            tr->write(str, strlen(str));

            float mos = (float)(jitter + latency + loss) / 3.0;

            snprintf(str, 128, "%s: LATENCY=%d, JITTER=%d, LOSS=%d, MOS=%.2f", name, latency, jitter, loss, mos);
            log->log(1, "[%s] %s", peerName, str);
            strncat(str, "\n", 127); str[127] = 0;

            __server->getHistory()->log("%s probed[%d]: QUERY=%s (%s), RESULT=%.2f", peerName, getpid(), p, name, mos);

            tr->write(str, strlen(str));
        }

        snprintf(str, 128, "Finished probe on %s", name);
        log->log(1, "[%s] %s", peerName, str);
        strncat(str, "\n", 127); str[127] = 0;

        return tr->write(str, strlen(str))>0?0:-1;
    }

    if(!p){
        snprintf(str, 128, "Syntax error\n");
        str[127] = 0;
        return tr->write(str, strlen(str))>0?0:-1;
    }

    return 0;
}

int ProbedCommands::command_VERBOSE(const char * buf)
{
    char str[128];
    char * p = strchr(buf, '=');

    if(!p){
        p = strchr(buf, ' ');
    }

    if(p){
        ++p;
        p = probed_strchop(p);
        if(!strncmp(p, "ON", 2)){
            task.verbose = true;
        } else if(!strncmp(p, "OFF", 3)) {
            task.verbose = false;
        }
        log->log(4, "[%s] Verbose is set to %s", peerName, task.verbose?"on":"off");
    }

    snprintf(str, 128, "VERBOSE is %s\n", task.verbose?"ON":"OFF");
    str[127] = 0;
    return tr->write(str, strlen(str))>0?0:-1;
}

int ProbedCommands::command_OPTIONS(const char * buf)
{
    char str[128];
    snprintf(str, 128, "Command not implemented\n");
    str[127] = 0;
    return tr->write(str, strlen(str))>0?0:-1;
}

int ProbedCommands::command_BYE(const char * buf)
{
    const char * str = "Bye!\n";
    tr->write(str, strlen(str));

    tr->shutdown();
    return -1;
}

