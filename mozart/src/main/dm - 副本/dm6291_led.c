#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include "mozart_config.h"
#include "dm6291_led.h"


#include "dm_mozart.h"

#define MAX_BRIGHTNESS 255
#define BREATH_FREQ 64
#define BREATH_STEP 4


extern int get_airkissing();
extern int get_playing();

#if (SUPPORT_LEDTYPE == LEDTYPE_BREATH)
static int g_exitbreath_wifi = 0;
static int g_exitbreath_bt = 0;
static int g_first_set_breath = 1;
static pthread_mutex_t g_breath_mutex;
//static pthread_t g_wifi_breath_pid = 0;
//static pthread_t g_bt_breath_pid = 0;
typedef struct __breath_info
{
    DM6291_LED_NUM lednum;
	int period;
}BREATH_PARAM;


static void set_exitbreath_wifi(int value)
{
	pthread_mutex_lock(&g_breath_mutex);
	g_exitbreath_wifi = value;
	pthread_mutex_unlock(&g_breath_mutex);

}

static void set_exitbreath_bt(int value)
{
	pthread_mutex_lock(&g_breath_mutex);
	g_exitbreath_bt= value;
	pthread_mutex_unlock(&g_breath_mutex);
}

void DM6291_SetLedBrightness(DM6291_LED_NUM lednum,int brightness)
{
    int led_fd = -1;
	int write_size = 0;
    char led_path[64] = {0};
	char brightness_str[4]={0};

	if(lednum == DM6291_LED_WIFI)
	{
        snprintf(led_path, sizeof(led_path),"%s",WIFI_BRIGHTNESS);
	}
	else if(lednum == DM6291_LED_BT)
	{
		snprintf(led_path, sizeof(led_path),"%s",BT_BRIGHTNESS);
	}
	else
	{
        printf("[DM6291 ERROR] unsupport this lednum:num=%d\n", lednum);
		return;
	}
	DM6291_SetLedExitBreath(lednum);
	led_fd = open(led_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
	if (led_fd < 0) 
	{
		printf("[DM6291 ERROR] open %s error: %s\n", led_path,strerror(errno));
		return;
	}
    snprintf(brightness_str, sizeof(brightness_str),"%d",brightness);

	if(DM6291_LED_ON == brightness)//if it is blinking ,must write 0 to finish blink.
	{
	    write(led_fd, "0", 1);
		usleep(1000);
	}
	write_size = write(led_fd, brightness_str, strlen(brightness_str));
	if (write_size < 0) {
		printf("[DM6291 ERROR] write %s to %s failed: %s\n", brightness_str,led_path, strerror(errno));
	}
    close(led_fd);
}

void DM6291_SetLedBlink(DM6291_LED_NUM lednum,int delay_on,int delay_off)
{
    int blinkfd = -1;
	//int write_size = 0;
    char trigger_path[64] = {0};
	char delay_on_path[64] = {0};
	char delay_off_path[64] = {0};
	char delay_str[16]={0};
    
    if(lednum == DM6291_LED_WIFI)
    {
        DM6291_SetLedBrightness(DM6291_LED_BT,DM6291_LED_OFF);
	    snprintf(trigger_path, sizeof(trigger_path),"%s",WIFI_TRIGGER);
		snprintf(delay_on_path, sizeof(delay_on_path),"%s",WIFI_DEYLAY_ON);
		snprintf(delay_off_path, sizeof(delay_off_path),"%s",WIFI_DELAY_OFF);
    }
    else if(lednum == DM6291_LED_BT)
    {
        DM6291_SetLedBrightness(DM6291_LED_WIFI,DM6291_LED_OFF);
	    snprintf(trigger_path, sizeof(trigger_path),"%s",BT_TRIGGER);
		snprintf(delay_on_path, sizeof(delay_on_path),"%s",BT_DEYLAY_ON);
		snprintf(delay_off_path, sizeof(delay_off_path),"%s",BT_DELAY_OFF);
    }
    else
    {
	    printf("[DM6291 ERROR] unsupport this lednum:num=%d\n", lednum);
	    return;
    }
	DM6291_SetLedExitBreath(lednum);
    //write trigger
    blinkfd = open(trigger_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0) 
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", trigger_path,strerror(errno));
	    return;
    }
	write(blinkfd, "timer", 5);
	close(blinkfd);

	//write delay_on
    blinkfd = open(delay_on_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0) 
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", delay_on_path,strerror(errno));
	    return;
    }
	snprintf(delay_str, sizeof(delay_str),"%d",delay_on);
	write(blinkfd, delay_str, strlen(delay_str));
	close(blinkfd);

    //write delay_off
	blinkfd = open(delay_off_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0)
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", delay_off_path,strerror(errno));
	    return;
    }
	memset(delay_str,0x0,sizeof(delay_str));
	snprintf(delay_str, sizeof(delay_str),"%d",delay_off);
	write(blinkfd, delay_str, strlen(delay_str));
	close(blinkfd);
}

static void *DM6291_SetLedBreathing(void *arg)
{
    int led_fd = -1;
	int write_size = 0;
    char led_path[64] = {0};
	char brightness_str[4]={0};
	int sleeptime = 0;
	int cur_brightness = 0;
	int lighting = 1;
	BREATH_PARAM *breath_info = (BREATH_PARAM *)arg;
	DM6291_LED_NUM lednum=DM6291_LED_COUNT;
	int period=0;
	//printf("tttttttttttt lednum=%d period=%d\n",breath_info->lednum,breath_info->period);

    lednum = breath_info->lednum;
	period = breath_info->period;

	if(lednum == DM6291_LED_WIFI)
	{
        snprintf(led_path, sizeof(led_path),"%s",WIFI_BRIGHTNESS);
	}
	else if(lednum == DM6291_LED_BT)
	{
		snprintf(led_path, sizeof(led_path),"%s",BT_BRIGHTNESS);
	}
	else
	{
        printf("[DM6291 ERROR] unsupport this lednum Breath:num=%d\n", lednum);
		return NULL;
	}
	
	led_fd = open(led_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
	if (led_fd < 0) 
	{
		printf("[DM6291 ERROR] open %s error: %s\n", led_path,strerror(errno));
		return NULL;
	}
    //printf("DM6291_SetLedBreathing start! g_exitbreath_bt=%d\n",g_exitbreath_bt);
	//printf("tttttttttttt lednum=%d period=%d\n",lednum,period);
	while(((lednum == DM6291_LED_WIFI)&&(0 == g_exitbreath_wifi)) ||
		  ((lednum == DM6291_LED_BT)&&(0 == g_exitbreath_bt)))
	{
	    memset(brightness_str,0x0,4);//printf("DM6291_SetLedBreathing!!!\n");
        snprintf(brightness_str, sizeof(brightness_str),"%d",cur_brightness);//printf("111111111\n");
		write_size = write(led_fd, brightness_str, strlen(brightness_str));//printf("2222222 write_size=%d\n",write_size);
	    if (write_size < 0) {
		    printf("[DM6291 ERROR] write %s to %s failed: %s\n", brightness_str,led_path, strerror(errno));
	    }//printf("33333\n");
		if(lighting)//lightting
		{//printf("4444444444\n");
            cur_brightness += BREATH_STEP;
			if(cur_brightness > MAX_BRIGHTNESS)
			{
                cur_brightness = MAX_BRIGHTNESS;
				lighting = 0;
			}
		}
		else//darking
		{//printf("555555555\n");
            cur_brightness -= BREATH_STEP;
			if(cur_brightness < 0)
			{
                cur_brightness = 0;
				lighting = 1;
			}
		}//printf("6666666666\n");
		sleeptime = (500000*period)/(BREATH_FREQ);//1000000us*period/(BREATH_FREQ*2);
		//printf("sleeptime=%d\n", sleeptime);
		usleep(sleeptime);
	}
	//printf("exitbreathing! g_exitbreath_wifi=%d,g_exitbreath_bt=%d\n",g_exitbreath_wifi,g_exitbreath_bt);
    close(led_fd);
	led_fd = 0;
	return NULL;
}


int DM6291_SetLedExitBreath(DM6291_LED_NUM lednum)
{
    int ret = 0;

    if(DM6291_LED_WIFI == lednum)
	{
	    set_exitbreath_wifi(1);
	}
    else if(DM6291_LED_BT== lednum)
	{
	    set_exitbreath_bt(1);
	}
	else
		ret = 1;
	//printf("DM6291_SetLedExitBreath!lednum=%d g_exitbreath_wifi=%d,g_exitbreath_bt=%d\n",lednum,g_exitbreath_wifi,g_exitbreath_bt);
	return ret;
}


int DM6291_SetLedBreath(DM6291_LED_NUM lednum,int period)
{
	int breathing = -1;
	static BREATH_PARAM breath_info;//must static
	pthread_t breath_pid = 0;

    if(g_first_set_breath)
		pthread_mutex_init(&g_breath_mutex,NULL);//initial lock
    else if(0 == DM6291_SetLedExitBreath(lednum))//exit pre breathing
	{
	    sleep(1);
	}
	else
	{
		printf(">unsupport led %d breath!>\n",lednum);
		return -1;
	}
	breath_info.lednum = lednum;
	breath_info.period = period;

    //initial
    DM6291_SetLedBrightness(lednum,DM6291_LED_OFF);

    if(DM6291_LED_WIFI == lednum)//init globe flag
	{   
	    set_exitbreath_wifi(0);//printf("sssssssssssssss\n");
	}
    else if(DM6291_LED_BT== lednum)
	{
	    set_exitbreath_bt(0);//printf("gggggggggggggg\n");
	}
	
	//printf("ttttggggt lednum=%d period=%d\n",breath_info.lednum, breath_info.period);
	breathing = pthread_create(&breath_pid, NULL, DM6291_SetLedBreathing, &breath_info);
	if(breathing != 0) {
		printf ("Create DM6291_SetLedBreathing failed: %s!\n", strerror(errno));
	}
	pthread_detach(breath_pid);
	return breathing;
}


#else

void DM6291_SetLedBrightness(DM6291_LED_NUM lednum,int brightness)
{
    int led_fd = -1;
	int write_size = 0;
    char led_path[64] = {0};
	char brightness_str[4]={0};

	if(lednum == DM6291_LED_WIFI)
	{
        snprintf(led_path, sizeof(led_path),"%s",WIFI_BRIGHTNESS);
	}
	else if(lednum == DM6291_LED_BT)
	{
		snprintf(led_path, sizeof(led_path),"%s",BT_BRIGHTNESS);
	}
	else if(lednum == DM6291_LED_PLAY)
	{
		snprintf(led_path, sizeof(led_path),"%s",PLAY_BRIGHTNESS);
	}
	else
	{
        printf("[DM6291 ERROR] unsupport this lednum:num=%d\n", lednum);
		return;
	}
	
	led_fd = open(led_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
	if (led_fd < 0) 
	{
		printf("[DM6291 ERROR] open %s error: %s\n", led_path,strerror(errno));
		return;
	}
    snprintf(brightness_str, sizeof(brightness_str),"%d",brightness);

	if(DM6291_LED_ON == brightness)//if it is blinking ,must write 0 to finish blink.
	{
	    write(led_fd, "0", 1);
		usleep(1000);
	}
	write_size = write(led_fd, brightness_str, strlen(brightness_str));
	if (write_size < 0) {
		printf("[DM6291 ERROR] write %s to %s failed: %s\n", brightness_str,led_path, strerror(errno));
	}
    close(led_fd);
}

void DM6291_SetLedBlink(DM6291_LED_NUM lednum,int delay_on,int delay_off)
{
    int blinkfd = -1;
	//int write_size = 0;
    char trigger_path[64] = {0};
	char delay_on_path[64] = {0};
	char delay_off_path[64] = {0};
	char delay_str[16]={0};

    if(lednum == DM6291_LED_WIFI)
    {
	    snprintf(trigger_path, sizeof(trigger_path),"%s",WIFI_TRIGGER);
		snprintf(delay_on_path, sizeof(delay_on_path),"%s",WIFI_DEYLAY_ON);
		snprintf(delay_off_path, sizeof(delay_off_path),"%s",WIFI_DELAY_OFF);
    }
    else if(lednum == DM6291_LED_BT)
    {
	    snprintf(trigger_path, sizeof(trigger_path),"%s",BT_TRIGGER);
		snprintf(delay_on_path, sizeof(delay_on_path),"%s",BT_DEYLAY_ON);
		snprintf(delay_off_path, sizeof(delay_off_path),"%s",BT_DELAY_OFF);
    }
    else if(lednum == DM6291_LED_PLAY)
    {
	    snprintf(trigger_path, sizeof(trigger_path),"%s",PLAY_TRIGGER);
		snprintf(delay_on_path, sizeof(delay_on_path),"%s",PLAY_DEYLAY_ON);
		snprintf(delay_off_path, sizeof(delay_off_path),"%s",PLAY_DELAY_OFF);
    }
    else
    {
	    printf("[DM6291 ERROR] unsupport this lednum:num=%d\n", lednum);
	    return;
    }
    //write trigger
    blinkfd = open(trigger_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0) 
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", trigger_path,strerror(errno));
	    return;
    }
	write(blinkfd, "timer", 5);
	close(blinkfd);

	//write delay_on
    blinkfd = open(delay_on_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0) 
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", delay_on_path,strerror(errno));
	    return;
    }
	snprintf(delay_str, sizeof(delay_str),"%d",delay_on);
	write(blinkfd, delay_str, strlen(delay_str));
	close(blinkfd);

    //write delay_off
	blinkfd = open(delay_off_path, O_WRONLY);// O_RDONLY  O_WRONLY O_RDWR
    if (blinkfd < 0)
    {
	    printf("[DM6291 ERROR] open %s error: %s\n", delay_off_path,strerror(errno));
	    return;
    }
	memset(delay_str,0x0,sizeof(delay_str));
	snprintf(delay_str, sizeof(delay_str),"%d",delay_off);
	write(blinkfd, delay_str, strlen(delay_str));
	close(blinkfd);
}
#endif
int update_led_status()
{
#if (SUPPORT_LEDTYPE == LEDTYPE_BREATH)
	if (get_airkissing()) {
		printf("led airkiss status\n");
		//DM6291_SetLedBrightness(DM6291_LED_BT,DM6291_LED_OFF);//Mutex
		DM6291_SetLedBlink(DM6291_LED_WIFI,200,200);
		DM6291_SetLedBrightness(DM6291_LED_BT,DM6291_LED_OFF);
		return 0;
	} 
	DM6291_SetLedBrightness(DM6291_LED_WIFI,DM6291_LED_OFF);
	if (get_playing()) {
		printf("led playing status\n");
		DM6291_SetLedBrightness(DM6291_LED_BT,DM6291_LED_ON);
	} else {
		printf("led idle status\n");
		DM6291_SetLedBreath(DM6291_LED_BT,2);
	}
	return 0;
#else
	if (get_airkissing())
		DM6291_SetLedBlink(DM6291_LED_WIFI,200,200);
	else
		DM6291_SetLedBrightness(DM6291_LED_WIFI,DM6291_LED_ON);
	if (get_playing())
		DM6291_SetLedBlink(DM6291_LED_PLAY,1000,1000);
	else 
		DM6291_SetLedBrightness(DM6291_LED_PLAY,DM6291_LED_ON);
	return 0;
#endif
}
