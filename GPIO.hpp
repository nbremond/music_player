#ifndef GPIO_HPP
#define GPIO_HPP

#include "wiring.hpp"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>

#define START_PIN 2
#define STOP_PIN 3
#define SOUND_CONTROL_PIN 0

#define THREAD_LOOP_DELAY_MS 10
#define LONG_DELAY_LIMIT_MS 750
#define REAPETE_DELAY_MS 1500

enum Button{
START,
STOP,
};


void void_function(void *);

class GPIO{
    struct gpioInteraction{
        unsigned int timeLastChange;
        bool wasPressed;
        bool wasLongPressed;
        void (*shortFunction)(void*);
        void *shortArg;
        void (*longFunction)(void*);
        void *longArg;
 //       void (*reapeatFunction)(void*);
 //       void *reapeatArg;
    };
    struct gpioInteraction startButton;
    struct gpioInteraction stopButton;
    pthread_t thread;
    bool threadContinue;

    void initGPIOInteraction(gpioInteraction &i){
        i.wasPressed = false;
        i.wasLongPressed = false;
        i.shortFunction = i.longFunction = void_function;
        i.shortArg = i.longArg = NULL;
    }

    public :
    GPIO(){
        _wiringSetup();
        _pinMode(SOUND_CONTROL_PIN, _OUTPUT);
        _pinMode(START_PIN, _INPUT);
        _pinMode(STOP_PIN, _INPUT);
        threadContinue = true;
        initGPIOInteraction(startButton);
        initGPIOInteraction(stopButton);
        int p_cr_return;
        p_cr_return = pthread_create(&thread, NULL, thread_GPIO, (void*) this);
        printf("thread create with code %d\n",p_cr_return);
    }

    ~GPIO(){
        threadContinue = false;
#ifdef DEBUG
        printf("waiting for the thread\n");
#endif
        pthread_join(thread, NULL);
#ifdef DEBUG
        printf("thread killed");
#endif
    }

    void registerShortPushFunction(enum Button b, void (*f)(void*), void* arg){
       gpioInteraction &g = (b==START) ? startButton : stopButton;
       g.shortFunction = f;
       g.shortArg = arg;
    }

    void registerLongPushFunction(enum Button b, void (*f)(void*), void * arg){
       gpioInteraction &g = (b==START) ? startButton : stopButton;
       g.longFunction = f;
       g.longArg = arg;
    }


    void switchSound(bool state){
        _digitalWrite(SOUND_CONTROL_PIN, (state) ? _HIGH : _LOW);
    }

    static void* thread_GPIO(void* G){
        return ((GPIO*) G)->runThread();
    }
    void* runThread(){
        unsigned int time=0;
        struct timespec delay;
        while (threadContinue){
//            printf("loop running since %d\n",time);
            checkInteraction(time, startButton, _digitalRead(START_PIN));
            checkInteraction(time, stopButton, !_digitalRead(STOP_PIN));
            delay.tv_sec=0; 
            delay.tv_nsec=1000000 * THREAD_LOOP_DELAY_MS;
            nanosleep(&delay,NULL);
            time += THREAD_LOOP_DELAY_MS;
        }
        return NULL;
    }

    void checkInteraction(unsigned int time, struct gpioInteraction &g, bool state){
        if (state){
            if (g.wasPressed){
                if (time - g.timeLastChange > LONG_DELAY_LIMIT_MS){
                    g.wasLongPressed = true;
                    g.wasPressed = false;
                    printf("long hold\n");
                    g.timeLastChange = time;
                    g.longFunction(g.longArg);
                }
            }else if(g.wasLongPressed){
                if (time - g.timeLastChange > REAPETE_DELAY_MS){
                    g.timeLastChange = time;
                    printf("reapeat\n");
                    g.longFunction(g.longArg);
                }
            }else{
                g.timeLastChange = time;
                g.wasPressed = true;
            }
        }else{
            if (g.wasPressed){
                printf("short press\n");
                g.wasPressed=false;
                g.shortFunction(g.shortArg);
            }
            g.wasPressed = false;
            g.wasLongPressed = false;
        }




    }
};

#endif
