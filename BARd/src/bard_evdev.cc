#include "bard_evdev.h"

BARdEvdev::BARdEvdev(const Log * _log, int _fd):
    log(_log), fd(_fd)
{
}

BARdEvdev::~BARdEvdev()
{
}

int BARdEvdev::logInfo() const
{
    unsigned short id[4];
    int version;
    char name[256];

    if (ioctl(fd, EVIOCGID, id) == -1) {
        log->error(0,"ioctl(EVIOCGID): %s", strerror(errno));
        return -1;
    }

    /*
    if(ioctl(fd, EVIOCGPHYS(sizeof(name)), name) == -1){
        log->error(0,"ioctl(EVIOCGPHYS): %s", strerror(errno));
        return -1;
    }
    */

    log->info(0, "Device: vendor 0x%04hx product 0x%04hx version 0x%04hx",
        id[ID_VENDOR], id[ID_PRODUCT], id[ID_VERSION]);

    /*
    if (ioctl(fd, EVIOCGBUS, id) == -1) {
        log->error(0,"ioctl(EVIOCGBUS): %s", strerror(errno));
        return -1;
    }
    */

    if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) == -1){
        log->error(0,"ioctl(EVIOCGNAME): %s", strerror(errno));
        return -1;
    }

    log->info(0, "\tname: %s", name);

    if(ioctl(fd, EVIOCGVERSION, &version) == -1) {
        log->error(0,"ioctl(EVIOCGVERSION): %s", strerror(errno));
        return -1;
    }

    log->info(0, "\tdriver version is %d.%d.%d",
        version >> 16, (version >> 8) & 0xff, version & 0xff);

    return 0;
}

int BARdEvdev::readData(char * buf, size_t szbuf)
{
    int  i = 0;
    size_t k = 0;

    u_int8_t uppercase = 0;

    while ((i = read(fd, events, sizeof(struct input_event) * 64)) > 0 ) {

        if (i < (int) sizeof(struct input_event)) {
            log->error(0, "evdev short read: %s", strerror(errno));
            return -1;
        }

        for (int z = 0; z < (int) (i/sizeof(struct input_event)); z++){

            if(k >= (szbuf + 1) ){
                break;
            }

            if(events[z].value == 1){
                switch ( events[z].code ){
                    case KEY_1 : buf[k++] = '1'; break;
                    case KEY_2 : buf[k++] = '2'; break;
                    case KEY_3 : buf[k++] = '3'; break;
                    case KEY_4 : buf[k++] = '4'; break;
                    case KEY_5 : buf[k++] = '5'; break;
                    case KEY_6 : buf[k++] = '6'; break;
                    case KEY_7 : buf[k++] = '7'; break;
                    case KEY_8 : buf[k++] = '8'; break;
                    case KEY_9 : buf[k++] = '9'; break;
                    case KEY_0 : buf[k++] = '0'; break;
                    case KEY_MINUS : buf[k++] = uppercase?'_':'-'; break;
                    case KEY_EQUAL : buf[k++] = uppercase?'+':'='; break;
                    case KEY_Q : buf[k++] = uppercase?'Q':'q'; break;
                    case KEY_W : buf[k++] = uppercase?'W':'w'; break;
                    case KEY_E : buf[k++] = uppercase?'E':'e'; break;
                    case KEY_R : buf[k++] = uppercase?'R':'r'; break;
                    case KEY_T : buf[k++] = uppercase?'T':'t'; break;
                    case KEY_Y : buf[k++] = uppercase?'Y':'y'; break;
                    case KEY_U : buf[k++] = uppercase?'U':'u'; break;
                    case KEY_I : buf[k++] = uppercase?'I':'i'; break;
                    case KEY_O : buf[k++] = uppercase?'O':'o'; break;
                    case KEY_P : buf[k++] = uppercase?'P':'p'; break;
                    case KEY_LEFTBRACE : buf[k++] = uppercase?'{':'['; break;
                    case KEY_RIGHTBRACE : buf[k++] = uppercase?'}':']'; break;
                    case KEY_A : buf[k++] = uppercase?'A':'a'; break;
                    case KEY_S : buf[k++] = uppercase?'S':'s'; break;
                    case KEY_D : buf[k++] = uppercase?'D':'d'; break;
                    case KEY_F : buf[k++] = uppercase?'F':'f'; break;
                    case KEY_G : buf[k++] = uppercase?'G':'g'; break;
                    case KEY_H : buf[k++] = uppercase?'H':'h'; break;
                    case KEY_J : buf[k++] = uppercase?'J':'j'; break;
                    case KEY_K : buf[k++] = uppercase?'K':'k'; break;
                    case KEY_L : buf[k++] = uppercase?'L':'l'; break;
                    case KEY_SEMICOLON : buf[k++] = uppercase?':':';'; break;
                    case KEY_APOSTROPHE : buf[k++] = uppercase?'"':'\''; break;
                    case KEY_GRAVE : buf[k++] = uppercase?'~':'`'; break;
                    case KEY_BACKSLASH : buf[k++] = uppercase?'|':'\\'; break;
                    case KEY_Z : buf[k++] = uppercase?'Z':'z'; break;
                    case KEY_X : buf[k++] = uppercase?'X':'x'; break;
                    case KEY_C : buf[k++] = uppercase?'C':'c'; break;
                    case KEY_V : buf[k++] = uppercase?'V':'v'; break;
                    case KEY_B : buf[k++] = uppercase?'B':'b'; break;
                    case KEY_N : buf[k++] = uppercase?'N':'n'; break;
                    case KEY_M : buf[k++] = uppercase?'M':'m'; break;
                    case KEY_COMMA : buf[k++] = uppercase?'<':','; break;
                    case KEY_DOT : buf[k++] = uppercase?'>':'.'; break;
                    case KEY_SLASH : buf[k++] = uppercase?'?':'/'; break;
                    case KEY_KPASTERISK : buf[k++] = '*'; break;
                    case KEY_SPACE : buf[k++] = ' '; break;
                    case KEY_ENTER : buf[k++] = '\n'; break;
                    case KEY_LEFTSHIFT:
                    case KEY_RIGHTSHIFT:
                        uppercase = 1; break;
                }
            } else {
                switch ( events[z].code ){
                    case KEY_LEFTSHIFT:
                    case KEY_RIGHTSHIFT:
                        uppercase = 0; break;
                }
            }

            //log->info(0, "Event: code %d, value %d", events[z].code, events[z].value);
        }
    }
    //buf[k++] = '\n';
    buf[k] = 0;

    if (i == -1) {
        if(errno != EAGAIN){
            log->error(0, "read failed (short read): %s", strerror(errno));
            return -1;
        }
    }

    return 0;
}
