#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

#include <boost/filesystem.hpp>
#include <vector>
#include <iterator>
#include <algorithm>
#include <iostream>

//#include <libmpd-1.0/libmpd/libmpd.h>
//#include <libmpd-1.0/libmpd/debug_printf.h>

//#include <libmpd-1.0/libmpd/libmpdclient.h>

#include <mpd/pair.h>
#include <mpd/song.h>
#include <mpd/client.h>

#include "GPIO.hpp"
#define BASE_PATH "/var/music/"
#define BASE_PATH_DEPTH 2


#ifdef DEBUG
#define PDEBUG(...) printf(__VA_ARGS__)
#define IFDEBUG(...) __VA_ARGS__
#else
#define PDEBUG(...)
#define IFDEBUG(...)
#endif

#define RED "\x1b[31;01m"
#define DARKRED "\x1b[31;06m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32;06m"
#define YELLOW "\x1b[33;06m"


using namespace boost::filesystem;
using namespace std;

bool mainLoopContinue = true;
struct mpd_connection * mpd = NULL;
GPIO GPIO;
std::vector<path> files;



    static void
handle_error(struct mpd_connection *c)
{
#ifdef DEBUG
    if(mpd_connection_get_error(c) == MPD_ERROR_SUCCESS){
        printf(GREEN "no error" RESET "\n");
        return;
    }

    fprintf(stderr, RED "%s" RESET "\n", mpd_connection_get_error_message(c));
    return;
#endif
}

void handle_error(struct mpd_connection *c,const char* txt){
#ifdef DEBUG
    fprintf(stderr,"%s : ",txt);
    handle_error(c);
#endif
}

void signalHandler_kill(int a){
    mainLoopContinue = false;
    printf("received sig %d\nwill stop after a time\n",a);
}

//int getFreePlace(char* fileName[],int previousPos){
//    int fileNumber = previousPos;
//    while (fileName[fileNumber] != NULL){
//        fileNumber ++;
//        if (fileNumber >= MAX_FILES){
//            fileNumber -= MAX_FILES;
//        }
//        if (fileNumber == previousPos){
//            free(fileName[previousPos]);
//            fileName[previousPos] = NULL;
//            return previousPos;
//        }
//    }
//    return fileNumber;
//}

void initDir(path p, std::vector<path> files){
    IFDEBUG(static int profondeur=0;);
    IFDEBUG(profondeur++;);
    std::vector<path> v;
    copy(directory_iterator(p), directory_iterator(), back_inserter(v));
    sort(v.begin(), v.end());
    for (std::vector<path>::const_iterator it(v.begin()), it_end(v.end()); it != it_end; ++it)
    {
        IFDEBUG(for(int i=1;i<profondeur;i++)printf(" |  ");)
            PDEBUG("%s\n",it->string().c_str());
        if (is_directory(*it)){
            initDir(*it,files);
        }
        if (is_regular_file(*it)){
            std::string file("file://");
            file.append(it->string());
            mpd_run_add(mpd,file.c_str());
            handle_error(mpd,"add init_dir");
            if(mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS){
                mpd_connection_clear_error(mpd);
                handle_error(mpd,"previous error cleared");
                sleep(1);
                mpd_run_add(mpd,file.c_str());
                handle_error(mpd,"retrying add init_dir");
                int i=0;
                while(mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS && i++<10){
                    mpd_connection_clear_error(mpd);
                    handle_error(mpd,"previous error cleared");
                    boost::filesystem::path absolute(it->c_str());
                    boost::filesystem::path::const_iterator itr(absolute.begin() );
                    for (int j=0; j<BASE_PATH_DEPTH; j++)
                        itr++;
                    boost::filesystem::path relative;
                    while(itr!=absolute.end()){
                        relative /= *itr;
                        itr++;
                    }
                    mpd_run_update(mpd,relative.c_str());
                    handle_error(mpd,"try reindexing");
                    sleep(1);
                    mpd_run_add(mpd,file.c_str());
                    handle_error(mpd,"    retrying add init_dir");

                }
                if(mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS){
                    mpd_connection_clear_error(mpd);
                    printf("aborting\n");
                    handle_error(mpd,"previous error cleared");
                }else{
                    files.push_back(*it);
                }
                printf("\n");
            }else{
                files.push_back(*it);
            }

        }
    }
    //    while (de != NULL){
    //        printf("while\n");
    //        if (de->d_type == DT_DIR){
    //            printf("opening %s\n",de->d_name);
    //            if (strcmp(de->d_name,".") && strcmp(de->d_name,"..")){
    //                DIR* newdir = opendir(de->d_name);
    //                initDir(newdir, fileName);
    //                closedir(newdir);
    //            }
    //        }
    //        if (de->d_type == DT_REG){
    //            fileNumber = getFreePlace(fileName,fileNumber);
    //            fileName[fileNumber]=strdup(de->d_name);
    //        }
    //        de = readdir(d);
    //    }
    IFDEBUG(profondeur--;)
}

void play(void*){
    struct mpd_status * status = mpd_run_status(mpd);
    handle_error(mpd,"get status");
    enum mpd_state state = mpd_status_get_state(status);
    if (state == MPD_STATE_PLAY){
        mpd_run_next(mpd);
        handle_error(mpd,"next");
    }else if (state == MPD_STATE_PAUSE){
        GPIO.switchSound(true);
        mpd_run_pause(mpd,false);
        handle_error(mpd,"unpaused");
    }else{
        GPIO.switchSound(true);
        //   mpd_pair song_file;
        //   song_file.name="file";
        //   song_file.value="/var/music/balto.mp3";
        //        struct mpd_song* song_ = mpd_song_begin(&song_file);
        // mpd_command_list_begin(mpd,true);
        //mpd_run_add(mpd,"file:///var/music/test/balto.mp3");
        //handle_error(mpd,"add");
        mpd_run_play(mpd);
        // mpd_command_list_end(mpd);
        handle_error(mpd,"play");
        printf("play\n");
    }
}

void stop(void*){
    GPIO.switchSound(false);
    mpd_run_pause(mpd,true);
    handle_error(mpd,"paused");
}

void switch_source(void*){
    GPIO.switchSound(false);
    mpd_run_stop(mpd);
    handle_error(mpd,"stop");
    files.clear();
    mpd_run_clear(mpd);
    handle_error(mpd,"clear_playlist");
    if(system("umount " BASE_PATH)){
        //echec
        printf("umount failed\n");
        if(system("mount /dev/sda1 " BASE_PATH)){
            printf("mount failed\n");
        }else{
            printf("mount succeed\n");
        }
    }else{
        //succes
        printf("umount succeed\n");
    }
    mpd_run_update(mpd,NULL);
    handle_error(mpd,"database_update");
    path p(BASE_PATH);
    initDir(p,files);
    GPIO.switchSound(true);
    sleep(1);
    GPIO.switchSound(false);
}


int main(){
    GPIO.switchSound(false);
    mpd = mpd_connection_new(NULL,0,0);
    handle_error(mpd,"new connection");
    sleep(1);
    while (mpd_connection_get_error(mpd) != MPD_ERROR_SUCCESS){
        printf(RED "error while connecting" RESET "trying again\n");
        sleep(10);
        mpd = mpd_connection_new(NULL,0,30000);
        handle_error(mpd,"new connection");
    }
    mpd_run_clear(mpd);
    handle_error(mpd,"clear_playlist");

    //    mpd_command_list_begin(mpd,false);
    //handle_error(mpd,"begin list");
    //mpd_send_idle_mask(mpd, MPD_IDLE_DATABASE);
    //handle_error(mpd,"enter idlemode");
    //mpd_send_update(mpd,NULL);
    //handle_error(mpd,"database_update");
    //mpd_response_finish(mpd);
    //handle_error(mpd,"database_update finish");
    //mpd_command_list_end(mpd);
    //handle_error(mpd,"end list");
    //mpd_response_finish(mpd);
    //handle_error(mpd,"list response finish");

    switch_source(NULL);

    //mpd_run_update(mpd,NULL);
    //handle_error(mpd,"database_update");
    //mpd_run_idle(mpd);
    //handle_error(mpd,"database_update finish");

    mpd_run_repeat(mpd,true);
    handle_error(mpd,"set_repeat");
    mpd_run_single(mpd,false);
    handle_error(mpd,"set single");

    signal(SIGINT, signalHandler_kill);

    GPIO.registerShortPushFunction(START, play, NULL);
    GPIO.registerShortPushFunction(STOP, stop, NULL);
    GPIO.registerLongPushFunction(STOP, switch_source, NULL);

    while(mainLoopContinue){
        mpd_connection_set_timeout(mpd, 1<<31);
        //GPIO.switchSound(false);
        sleep(1);
        //GPIO.switchSound(true);
        sleep(1);
    }
    printf("ending\n");
    mpd_connection_free(mpd);
    return 0;
}
