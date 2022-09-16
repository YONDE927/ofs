#pragma once

#define MSG_LEN 32
#define PATH_LEN 256

namespace ftp{
    enum requestType{
        echoback,
        lock,
        getattr,
        readdir,
        read,
        write,
        create,
    };

    struct echobackDatagram{
        char msg[MSG_LEN];
    };

    enum lockType{
        rlock,
        wlock,
        unrlock,
        unwlock,
    };

    struct baseReq{
        requestType reqtype;
        char path[PATH_LEN];
    };

    struct lockReq: public baseReq{
        lockType type;
    };

    struct lockRes{
        int errno_;
    };

    struct getattrReq: public baseReq{
    };

    struct getattrRes{
        int errno_;
    };

    struct readdirReq: public baseReq{
    };

    struct readdirRes{
        int errno_;
        int ndirent;
    };

    struct writeReq: public baseReq{
        int offset;
        int size;
    };

    struct writeRes{
        int errno_;
    };

    struct readReq: public baseReq{
        int offset;
        int size;
    };

    struct readRes{
        int errno_;
        int size;
    };

    struct createReq: public baseReq{
    };

    struct createRes{
        int errno_;
    };
}
