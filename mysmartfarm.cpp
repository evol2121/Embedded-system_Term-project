#include <stdio.h>

#include <string.h>

#include <errno.h>

#include <wiringPi.h>

#include <wiringPiSPI.h>

#include <mysql/mysql.h>

 

#include <stdlib.h>

#include <stdint.h>

#include <sys/types.h>

#include <unistd.h>

#include <signal.h>

#include <time.h>

#include <math.h>

#include <pthread.h>

 

#define SPI_CHANNEL 0

#define SPI_SPEED 1000000

#define FAN	22 // BCM_GPIO 6

 

#define MAXTIMINGS 85

#define RETRY 5

 

#define DBHOST "localhost"

#define DBUSER "root"

#define DBPASS "root"

#define DBNAME "demofarmdb"

 

#define CS_MCP3208  8        // BCM_GPIO_8

#define RGBLEDPOWER  24

 

int ret_humid, ret_temp;

int adc = 0;

 

static int DHTPIN = 11;

static int dht22_dat[5] = {0,0,0,0,0};

 

MYSQL *connector;

MYSQL_RES *result;

MYSQL_ROW row;

 

int adcChannel  = 0;

int adcValue[8] = {0};

 

int read_dht22_dat_temp();

int get_temperature_sensor();

int read_mcp3208_adc(unsigned char adcChannel);

void sqlinput();

 

int temp, light, humi;

int done =0;

int faning =0;

int ledset =0;

pthread_cond_t empty,fill,fanon,ledon,ledoff;

pthread_mutex_t mutex;

 

 

 

int get_temperature_and_light_sensor()

{

        unsigned char adcChannel_light = 0;

	int received_temp;

	int received_light = 0; 

	int received_humi;

	int _retry = RETRY;

	//DHTPIN = 11;

 

	if (wiringPiSetup() == -1)

		exit(EXIT_FAILURE) ;

	

	if (setuid(getuid()) < 0)

	{

		perror("Dropping privileges failed\n");

		exit(EXIT_FAILURE);

	}

	while ((read_dht22_dat_temp() == 0 || read_mcp3208_adc(adcChannel_light) < 0) && _retry--)

	{

		//delay(100); // wait 1sec to refresh

	}

	received_temp = ret_temp;

        received_light = adc;

	received_humi = ret_humid;

	printf("Temperature = %d, light =%d, humidity = %d\n", received_temp, received_light,received_humi );

	temp = received_temp; light=received_light;  humi = received_humi; 

        done++; 

        //delay(100);

	return 0;

}

 

static uint8_t sizecvt(const int read)

{

  /* digitalRead() and friends from wiringpi are defined as returning a value

  < 256. However, they are returned as int() types. This is a safety function */

 

  if (read > 255 || read < 0)

  {

    printf("Invalid data from wiringPi library\n");

    exit(EXIT_FAILURE);

  }

  return (uint8_t)read;

}

 

int read_dht22_dat_temp()

{

  uint8_t laststate = HIGH;

  uint8_t counter = 0;

  uint8_t j = 0, i;

 

  dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;

 

  // pull pin down for 18 milliseconds

  pinMode(DHTPIN, OUTPUT);

  digitalWrite(DHTPIN, HIGH);

 // delay(10);

  digitalWrite(DHTPIN, LOW);

 // delay(18);

  // then pull it up for 40 microseconds

  digitalWrite(DHTPIN, HIGH);

  delayMicroseconds(40); 

  // prepare to read the pin

  pinMode(DHTPIN, INPUT);

 

  // detect change and read data

  for ( i=0; i< MAXTIMINGS; i++) {

    counter = 0;

    while (sizecvt(digitalRead(DHTPIN)) == laststate) {

      counter++;

      delayMicroseconds(1);

      if (counter == 255) {

        break;

      }

    }

    laststate = sizecvt(digitalRead(DHTPIN));

 

    if (counter == 255) break;

 

    // ignore first 3 transitions

    if ((i >= 4) && (i%2 == 0)) {

      // shove each bit into the storage bytes

      dht22_dat[j/8] <<= 1;

      if (counter > 50 )

        dht22_dat[j/8] |= 1;

      j++;

    }

  }

 

  // check we read 40 bits (8bit x 5 ) + verify checksum in the last byte

  // print it out if data is good

  if ((j >= 40) && 

      (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) {

        float t, h;

		

        h = (float)dht22_dat[0] * 256 + (float)dht22_dat[1];

        h /= 10;

        t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];

        t /= 10.0;

        if ((dht22_dat[2] & 0x80) != 0)  t *= -1;

		

		ret_humid = (int)h;

		ret_temp = (int)t;

               // printf("Humidity = %.2f %% Temperature = %.2f *C \n", h, t );

	       //printf("Humidity = %d Temperature = %d\n", ret_humid, ret_temp);

		

    return ret_temp;

  }

  else

  {

    //printf("Data not good, skip\n");

    return 0;

  }

}

 

int read_mcp3208_adc(unsigned char adcChannel) 

{

	unsigned char buff[3];

	

	buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);

	buff[1] = ((adcChannel & 0x07) << 6);

	buff[2] = 0x00;

	

	digitalWrite(CS_MCP3208, 0);

	wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);

	

	buff[1] = 0x0f & buff[1];

	adc = (buff[1] << 8 ) | buff[2];

	

	digitalWrite(CS_MCP3208, 1);

	

	return adc;

}

 

void sqlinput()

{

  

    char query[1024];

    sprintf(query,"insert into thl values (now(),%d,%d,%d)", temp,humi,light);

 

    if(mysql_query(connector, query))

    {

      fprintf(stderr, "%s\n", mysql_error(connector));

      printf("Write DB error\n");

    }

    printf("write DB sucess\n");

    delay(0); //1sec delaydkla

    done = 0;

}

 

void *producer(void *arg){

                

                while(1){

		pthread_mutex_lock(&mutex);

		while(done == 10000)

                {

                  pthread_cond_signal(&fill); 

		  pthread_cond_wait(&empty, &mutex);

                }

 		get_temperature_and_light_sensor();

                if(temp >= 28)

                { 

                  faning = 1;

                  pthread_cond_signal(&fanon);

                }

		else faning = 0;

 

                if(light > 140)

                { 

                  ledset = 1;

                  pthread_cond_signal(&ledon);

                }

                else {

                  ledset = 0; 

                  pthread_cond_signal(&ledoff);

                }

 		pthread_mutex_unlock(&mutex);

                delay(1);

                }

            

}

 

void *consumer(void *arg) {

               

                while(1){

		pthread_mutex_lock(&mutex);

		while(done < 10000)

                {

		  pthread_cond_wait(&fill,&mutex);

                }

		sqlinput();

                pthread_cond_signal(&empty);

		pthread_mutex_unlock(&mutex);

		delay(10000);

                }

}

 

void *fan(void *arg)

{

                while(1){

                pthread_mutex_lock(&mutex);  

		while(faning == 0)

                {

		 pthread_cond_wait(&fanon,&mutex);

                }

		printf("fanon\n");

		//digitalWrite(FAN,1);

	        pthread_mutex_unlock(&mutex);

		//while(faning == 1){

		digitalWrite(FAN,1);

		delay(5000);

	        digitalWrite(FAN,0);

	        //}	

		}

}

 

void *led(void *arg)

{

                while(1){

                pthread_mutex_lock(&mutex);  

		while(ledset == 0)

                {

                 digitalWrite(RGBLEDPOWER, 0);

		 pthread_cond_wait(&ledon,&mutex);

                }

                while(ledset == 1)

                {

                 digitalWrite(RGBLEDPOWER, 1);

                 printf("ledon\n");

		 pthread_cond_wait(&ledoff,&mutex);

                }

                pthread_mutex_unlock(&mutex);

                }

}

 

void sig_handler(int signo)

{

    printf("process stop\n");

	digitalWrite (FAN, 0) ; // Off	

	digitalWrite(RGBLEDPOWER, 0);

	exit(0);

} 

 

int main()

{

  signal(SIGINT, sig_handler);

  int status;

  connector = mysql_init(NULL);

  if (!mysql_real_connect(connector, DBHOST, DBUSER, DBPASS, DBNAME, 3306, NULL, 0))

  {

    fprintf(stderr, "%s\n", mysql_error(connector));

    return 0;

  }

 

  printf("MySQL(demodb) opened.\n");

  if(wiringPiSetup() == -1)

  {    

    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror(errno));

    return 1 ;

  }

 

  if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)

  {

    fprintf (stdout, "wiringPiSPISetup Failed: %s\n", strerror(errno));

    return 1 ;

  }

  pinMode (FAN, OUTPUT);

  pinMode(RGBLEDPOWER, OUTPUT);

        pthread_t p1, p2, p3, p4;

        pthread_create(&p1, NULL, producer, NULL );

	pthread_create(&p2, NULL, consumer, NULL );

        pthread_create(&p3, NULL, fan, NULL );

        pthread_create(&p4, NULL, led, NULL );

	pthread_join(p1,(void **) &status);

        pthread_join(p2,(void **) &status);

        pthread_join(p3,(void **) &status);

        pthread_join(p4,(void **) &status);

 

        mysql_close(connector);

	return 0;

}
